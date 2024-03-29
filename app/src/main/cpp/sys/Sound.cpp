/***************************

		サウンド管理

 ***************************/

#include "Sound.h"
#include "Scene.h"


namespace sys
{

SLObjectItf		SoundPlayer::engineObject;				// エンジンオブジェクト
SLEngineItf		SoundPlayer::engineEngine;				// インタフェース
float			SoundPlayer::master_volume;				// マスター音量

/********************
    エンジン初期化
 ********************/
void	SoundPlayer::create_engine(void)
{
	SLresult	result;

	result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);		// エンジンオブジェクト作成
	assert(SL_RESULT_SUCCESS == result);

	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);				// リアライズ
	assert(SL_RESULT_SUCCESS == result);

	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	assert(SL_RESULT_SUCCESS == result);											// インタフェース取得

	master_volume = 1.0f;				// マスター音量初期化
}

/******************
    エンジン終了
 ******************/
void	SoundPlayer::release_engine(void)
{
	if ( engineObject ) {
		(*engineObject)->Destroy(engineObject);
		engineObject = nullptr;
	}
}


/********************
    コンストラクタ
 ********************/
SoundPlayer::SoundPlayer(void)
{
	state = STOPPED;
	sound_data = nullptr;
	pcm_buffer = nullptr;
}

void	SoundPlayer::create(void)
{
	SLresult	result;

	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr);
	assert(SL_RESULT_SUCCESS == result);									// 出力オブジェクト作成

	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);									// リアライズ

	pcm_buffer		= new char[PCM_BUF_SIZE];
	bqPlayerObject	= nullptr;
	pthread_mutex_init(&mutex, nullptr);

	if ( sound_data ) {					// 再開
		prepare();
		if ( state == PLAYING ) {
			play();
		}
	}
}

/******************
    デストラクタ
 ******************/
SoundPlayer::~SoundPlayer()
{
	destroy();
	if ( sound_data ) {
		delete	sound_data;
	}
}

void	SoundPlayer::destroy(void)
{
	if ( pcm_buffer ) {
		pthread_mutex_lock(&mutex);
		if ( bqPlayerObject ) {
			(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
			(*bqPlayerBufferQueue)->Clear(bqPlayerBufferQueue);
			(*bqPlayerObject)->Destroy(bqPlayerObject);
			bqPlayerObject = nullptr;
		}
		pthread_mutex_unlock(&mutex);
		pthread_mutex_destroy(&mutex);
		if ( sound_data && (sound_data->format == SoundData::OGG) ) {
			sound_data->position = ov_pcm_tell(&ov_file);
			::ov_clear(&ov_file);
		}
		(*outputMixObject)->Destroy(outputMixObject);
		delete[]	pcm_buffer;
		pcm_buffer = nullptr;
	}
}


/********************
    メモリ読み込み
 ********************/
static
size_t	callback_ov_read(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	return	((SoundPlayer*)datasource)->ov_read(ptr, size, nmemb);
}

size_t	SoundPlayer::ov_read(void* ptr, size_t size, size_t nmemb)
{
	if ( ptr == nullptr ) {
		return	0;
	}

	size_t	_count = (size_t)((sound_data->size - sound_data->position)/size);

	if ( _count >= nmemb ) {
		_count = nmemb;
	}
	else if ( _count <= 0 ) {
		return	0;
	}
	size *= _count;
	memcpy(ptr, (char*)sound_data->data + sound_data->position, size);
	sound_data->position += size;

	return	_count;
}

/******************
    メモリシーク
 ******************/
static
int		callback_ov_seek(void* datasource, ogg_int64_t offset, int whence)
{
	return	((SoundPlayer*)datasource)->ov_seek(offset, whence);
}

int		SoundPlayer::ov_seek(ogg_int64_t offset, int whence)
{
	switch( whence ) {
	  case SEEK_CUR :
		sound_data->position += offset;
		break;

	  case SEEK_END :
		sound_data->position = sound_data->size + offset;
		break;

	  case SEEK_SET :
		sound_data->position = offset;
		break;

	  default :
		return	-1;
	}

	if ( sound_data->position > sound_data->size ) {
		sound_data->position = sound_data->size;
		return	-1;
	}
	else if ( sound_data->position < 0 ) {
		sound_data->position = 0;
		return	-1;
	}
	return	0;
}

/********************
    メモリクローズ
 ********************/
static
int		callback_ov_close(void* datasource)
{
	return	((SoundPlayer*)datasource)->ov_close();
}

int		SoundPlayer::ov_close(void)
{
	return	0;
}

/********************
    メモリ位置通達
 ********************/
static
long	callback_ov_tell(void* datasource)
{
	return	((SoundPlayer*)datasource)->ov_tell();
}

long	SoundPlayer::ov_tell(void)
{
	return	sound_data->position;
}

/***************************
    OggVorbisコールバック
 ***************************/
static
ov_callbacks	callbacks =
				{
					callback_ov_read,
					callback_ov_seek,
					callback_ov_close,
					callback_ov_tell,
				};


// WAVデータヘッダ
struct WaveFormat
{
	int8_t		riff[4];
	uint32_t	total_size;
	int8_t		fmt[8];
 	uint32_t	fmt_size;
	uint16_t	format;
	uint16_t	channel;
	uint32_t	rate;
	uint32_t	avgbyte;
	uint16_t	block;
	uint16_t	bit;
	int8_t		data[4];
	uint32_t	data_size;
};

/**********************
    再生コールバック
 **********************/
static
void	bqPlayerCallback(SLAndroidSimpleBufferQueueItf, void* context)
{
	((SoundPlayer*)context)->callback_play();
}

void	SoundPlayer::callback_play(void)
{
	pthread_mutex_lock(&mutex);
	if ( (state == STOPPED) || (bqPlayerObject == nullptr) || (sound_data == nullptr) ) {
		pthread_mutex_unlock(&mutex);
		return;
	}

	SLuint32	_size, _t;

	switch ( sound_data->format ) {							// データフォーマット
	  case SoundData::WAV :			// WAV
		while ( true ) {
			_size = (SLuint32)(sound_data->size - sound_data->position);
			if ( _size > 0 ) {								// 再生中
				if ( _size > PCM_BUF_SIZE ) {
					_size = PCM_BUF_SIZE;
				}
				(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, (char*)sound_data->data + sound_data->position, _size);
				sound_data->position += _size;
				pthread_mutex_unlock(&mutex);
				return;
			}
			else if ( sound_data->loop != 1 ) {				// ループ
				if ( sound_data->loop > 1 ) {
					sound_data->loop--;
				}
				sound_data->position = sizeof(WaveFormat);
			}
			else if ( sound_data->next != nullptr ) {		// 連続再生
				SoundData*	_next = sound_data->next;

				sound_data->next = nullptr;
				delete	sound_data;
				sound_data = _next;
			}
			else {											// 終了
				break;
			}
		}
		break;

	  case SoundData::OGG :			// OGG
		while ( true ) {
			_size = 0;
			do {
				_t = ::ov_read(&ov_file, pcm_buffer + _size, PCM_BUF_SIZE - _size, nullptr);
				_size += _t;
			} while ( (_size < PCM_BUF_SIZE) && (_t > 0) );
			if ( _size > 0 ) {								// 再生中
				(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcm_buffer, _size);
				pthread_mutex_unlock(&mutex);
				return;
			}
			else if ( sound_data->loop != 1 ) {				// ループ
				if ( sound_data->loop > 1 ) {
					sound_data->loop--;
				}
				::ov_pcm_seek(&ov_file, 0);
			}
			else if ( sound_data->next != nullptr ) {		// 連続再生
				SoundData*	_next = sound_data->next;

				sound_data->next = nullptr;
				delete	sound_data;
				sound_data = _next;
				::ov_clear(&ov_file);
				if ( ::ov_open_callbacks(this, &ov_file, nullptr, 0, callbacks) ) {
					LOGE("ov_open_callbacks ERROR!");
					break;
				}
			}
			else {											// 終了
				break;
			}
		}
		break;
	}
	pthread_mutex_unlock(&mutex);
	stop();													// 停止
}


/****************************************
    サウンドデータ作成
		引数	_data = データ
				_size = データサイズ
				_loop = ループカウンタ
		戻り値	サウンドデータ
 ****************************************/
SoundData::SoundData(const void* _data, size_t _size, int _loop)
{
	switch ( _size ) {
	  case FILE_ASSET :				// assetファイル
		file_data = std::move(Scene::load_asset((char*)_data, &size));
		data = file_data.get();
		break;

	  default :						// メモリ
		data = (const uint8_t*)_data;
		size = _size;
		break;
	}
	loop = _loop;
	next = nullptr;

	switch ( *((uint32_t*)data) ) {						// データフォーマット
	  case 0x46464952 :				// WAV
		format   = WAV;
		size     = ((WaveFormat*)data)->data_size + sizeof(WaveFormat);
		position = sizeof(WaveFormat);
		break;

	  case 0x5367674f :				// OGG
		format   = OGG;
		position = 0;
		break;

	  default :
		assert(false);
		break;
	}
}

/****************************************************
    再生
		引数	_data = サウンドデータ
				_size = サウンドデータサイズ
				_loop = ループ回数（0:無限ループ）
				_vol  = 音量
 ****************************************************/
void	SoundPlayer::prepare(const void* _data, uint32_t _size, int _loop, float _vol)
{
	stop();
	pthread_mutex_lock(&mutex);
	sound_data = new SoundData((char*)_data, _size, _loop);				// サウンドデータ作成
	pthread_mutex_unlock(&mutex);
	volume		= _vol;
	fade_volume	= 0.0f;
	state = PREPARED;
	prepare();
}

void	SoundPlayer::prepare(void)
{
	SLDataFormat_PCM	_pcm;
	SLresult			result;

	switch ( sound_data->format ) {					// データフォーマット
	  case SoundData::WAV :			// WAV
		{
			WaveFormat*		_info = (WaveFormat*)sound_data->data;

			_pcm.formatType		= SL_DATAFORMAT_PCM;
			_pcm.numChannels	= (SLuint32)_info->channel;
			_pcm.samplesPerSec	= (SLuint32)_info->rate*1000;
			_pcm.bitsPerSample	= (SLuint32)_info->bit;
			_pcm.containerSize	= (SLuint32)_info->bit;
			_pcm.channelMask	= (_info->channel == 1) ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
			_pcm.endianness		= SL_BYTEORDER_LITTLEENDIAN;
		}
		break;

	  case SoundData::OGG :			// OGG
		{
			ogg_int64_t		_pos = sound_data->position;

			sound_data->position = 0;
			if ( ::ov_open_callbacks(this, &ov_file, nullptr, 0, callbacks) ) {
				LOGE("ov_open_callbacks ERROR!");
				::ov_clear(&ov_file);
				delete	sound_data;
				sound_data = nullptr;
				return;
			}
			::ov_pcm_seek(&ov_file, _pos);

			vorbis_info*	_info = ::ov_info(&ov_file, -1);

			_pcm.formatType		= SL_DATAFORMAT_PCM;
			_pcm.numChannels	= (SLuint32)_info->channels;
			_pcm.samplesPerSec	= (SLuint32)_info->rate*1000;
			_pcm.bitsPerSample	= (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16;
			_pcm.containerSize	= (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16;
			_pcm.channelMask	= (_info->channels == 1) ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
			_pcm.endianness		= SL_BYTEORDER_LITTLEENDIAN;
		}
		break;

	  default :
		assert(false);
		break;
	}

	if ( (bqPlayerObject == nullptr) || memcmp(&format_pcm, &_pcm, sizeof(SLDataFormat_PCM)) ) {
		if ( bqPlayerObject ) {													// PCMデータフォーマット変更
			(*bqPlayerObject)->Destroy(bqPlayerObject);
		}
		memcpy(&format_pcm, &_pcm, sizeof(SLDataFormat_PCM));

		SLDataLocator_AndroidSimpleBufferQueue	loc_bufq   = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
		SLDataSource							audioSrc   = {&loc_bufq, &format_pcm};
		SLDataLocator_OutputMix					loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
		SLDataSink								audioSnk   = {&loc_outmix, nullptr};

		const SLInterfaceID		ids[3] = {SL_IID_PLAY, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
		const SLboolean			req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

		result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 3, ids, req);
		if ( SL_RESULT_SUCCESS != result ) {									// プレイヤーオブジェクト作成
			bqPlayerObject = nullptr;
			return;
		}

		result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);	// リアライズ
		assert(SL_RESULT_SUCCESS == result);

		result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
		assert(SL_RESULT_SUCCESS == result);									// インタフェース取得

		result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
		assert(SL_RESULT_SUCCESS == result);									// 音量インタフェース

		result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
		assert(SL_RESULT_SUCCESS == result);									// バッファキューインタフェース

		result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
		assert(SL_RESULT_SUCCESS == result);									// 再生コールバック設定
	}
	set_volume();																// 音量設定
	callback_play();															// データ読み込み
}

void	SoundPlayer::play(void)
{
	if ( bqPlayerObject && sound_data ) {
		state = PLAYING;
		(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);		// 再生開始
	}
}

void	SoundPlayer::play(const void* _data, uint32_t _size, int _loop, float _vol)
{
	prepare(_data, _size, _loop, _vol);
	play();
}


/****************************************************
    連続再生設定
		引数	_data = サウンドデータ
				_size = サウンドデータサイズ
				_loop = ループ回数（0:無限ループ）
 ****************************************************/
void	SoundPlayer::set_next(const void* _data, uint32_t _size, int _loop)
{
	if ( sound_data ) {
		pthread_mutex_lock(&mutex);
		sound_data->set_next(new SoundData(_data, _size, _loop));
		pthread_mutex_unlock(&mutex);
	}
	else {									// すでに鳴り終わっている
		play(_data, _size, _loop);
	}
}


/*************************************
    停止
		引数	_cnt = フェード時間
 *************************************/
void	SoundPlayer::stop(int _cnt)
{
	pthread_mutex_lock(&mutex);
	if ( (state != STOPPED) && bqPlayerObject ) {		// 再生中
		if ( (_cnt == 0) || (volume == 0.0f) ) {
			(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);			// 停止状態
			(*bqPlayerBufferQueue)->Clear(bqPlayerBufferQueue);
			if ( sound_data ) {
				if ( sound_data->format == SoundData::OGG ) {
					::ov_clear(&ov_file);
				}
				delete	sound_data;
				sound_data = nullptr;
			}
		}
		else {
			fade_volume = volume/_cnt;
			pthread_mutex_unlock(&mutex);
			return;
		}
	}
	state = STOPPED;
	pthread_mutex_unlock(&mutex);
}


/*****************************************************
    音量設定
		引数	_vol = 音量（0.0:最小 ～ 1.0:最大）
 *****************************************************/
void	SoundPlayer::set_volume(float _vol)
{
	volume		= _vol;
	fade_volume	= 0.0f;
	set_volume();
}

void	SoundPlayer::set_volume(void)
{
	if ( bqPlayerObject ) {
		float		_vol = volume*master_volume;
		SLresult	result;

		result = (*bqPlayerVolume)->SetVolumeLevel(bqPlayerVolume, (_vol >= 1.0f) ? 0 : ((_vol < 0.01f) ? -16000
																							: (SLmillibel)(8000.0f*log10f(_vol))));
		assert(SL_RESULT_SUCCESS == result);
	}
}

/*************************************************************
    マスター音量設定
		引数	_vol = マスター音量（0.0:最小 ～ 1.0:最大）
		戻り値	マスター音量が変更されたか
 *************************************************************/
bool	SoundPlayer::set_master_volume(float _vol)
{
	if ( master_volume != _vol ) {
		master_volume = _vol;
		return	true;
	}
	return	false;
}


/**************
    一時停止
 **************/
void	SoundPlayer::pause(void)
{
	if ( state == PLAYING ) {				// 再生中
		if ( bqPlayerObject ) {
			(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);		// 一時停止状態
		}
		state = PAUSED;
	}
}

/**********
    再開
 **********/
void	SoundPlayer::resume(void)
{
	if ( state == PAUSED ) {
		play();
	}
}


/**********
    稼働
 **********/
void	SoundPlayer::update(void)
{
	switch ( state ) {
	  case PLAYING :						// 再生中
		if ( fade_volume > 0.0f ) {
			volume -= fade_volume;
			if ( volume <= 0.0f ) {
				stop();
			}
			else {
				set_volume();
			}
		}
		break;
	}
}



// サウンドコマンド
enum
{
	COMMAND_UPDATE,			// 稼働
	COMMAND_PREPARE,		// 準備
	COMMAND_PLAY,			// 再生・再開
	COMMAND_STOP,			// 停止
	COMMAND_VOLUME,			// 音量設定
	COMMAND_NEXT,			// 連続再生設定
	COMMAND_PAUSE,			// 一時停止
	COMMAND_RESUME,			// 再開
	COMMAND_RELEASE,		// 終了
};

int					Sound::channel_max;				// チャンネル数
SoundPlayer*		Sound::player;					// プレイヤー
pthread_t			Sound::thread;					// スレッド
pthread_mutex_t		Sound::mutex;
pthread_cond_t		Sound::cond;
SoundCommand		Sound::queue[QUEUE_SIZE];		// コマンドキュー
int volatile		Sound::reserve_pos;
int volatile		Sound::run_pos;

/*************************************
    サウンド管理初期化
		引数	_max = チャンネル数
 *************************************/
void	Sound::create(int _max)
{
	channel_max = _max;										// チャンネル数
	if ( channel_max > 0 ) {
		player = new SoundPlayer[channel_max];				// プレイヤー
		reserve_pos	= 0;
		run_pos		= 0;
	}
}

void	Sound::resume_manager(void)
{
	if ( channel_max > 0 ) {
		SoundPlayer::create_engine();						// サウンドエンジン初期化
		for (int i = 0; i < channel_max; i++) {
			player[i].create();
		}
		pthread_mutex_init(&mutex, nullptr);
		pthread_cond_init(&cond, nullptr);
		pthread_create(&thread, nullptr, run, nullptr);		// コマンド実行スレッド
	}
}

/**********
    終了
 **********/
void	Sound::pause_manager(void)
{
	if ( channel_max > 0 ) {
		set_command(-1, COMMAND_RELEASE);					// 終了コマンド
		pthread_join(thread, nullptr);
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
		for (int i = 0; i < channel_max; i++) {
			player[i].destroy();
		}
		SoundPlayer::release_engine();						// サウンドエンジン終了
	}
}

void	Sound::release(void)
{
	if ( channel_max > 0 ) {
		delete[]	player;
		player = nullptr;
	}
}

/*******************************************
    コマンド予約
		引数	_channel = チャンネル番号
				_command = コマンド
				_data    = データ
				_size    = サイズ
				_loop    = ループ回数
				_volume  = 音量
 *******************************************/
void	Sound::set_command(int _channel, int _command, const void* _data, uint32_t _size, int _loop, float _volume)
{
	while ( (run_pos + 1) % QUEUE_SIZE == reserve_pos ) {
		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	}

	queue[run_pos].set(_channel, _command, _data, _size, _loop, _volume);
	run_pos = ++run_pos % QUEUE_SIZE;
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

/**************************
    コマンド実行スレッド
 **************************/
void*	Sound::run(void*)
{
	SoundCommand*	p;

	while ( true ) {
		pthread_mutex_lock(&mutex);
	    pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);

		while ( reserve_pos != run_pos ) {
			p = &queue[reserve_pos];
			reserve_pos = ++reserve_pos % QUEUE_SIZE;

			switch ( p->command ) {
			  case COMMAND_UPDATE :
				for (int i = 0; i < channel_max; i++) {
					player[i].update();
				}
				break;

			  case COMMAND_PREPARE :
				player[p->channel].prepare(p->data, p->size, p->loop, p->volume);
				break;

			  case COMMAND_PLAY :
				if ( p->channel >= 0 ) {
					if ( p->size ) {
						player[p->channel].prepare(p->data, p->size, p->loop, p->volume);
					}
					player[p->channel].play();
				}
				else {
					for (int i = 0; i < channel_max; i++) {			// 全て開始
						player[i].play();
					}
				}
				break;

			  case COMMAND_STOP :
				if ( p->channel >= 0 ) {
					player[p->channel].stop((int)p->size);
				}
				else {
					for (int i = 0; i < channel_max; i++) {			// 全て停止
						player[i].stop();
					}
				}
				break;

			  case COMMAND_VOLUME :
				if ( p->channel >= 0 ) {
					player[p->channel].set_volume(p->volume);
				}
				else if ( SoundPlayer::set_master_volume(p->volume) ) {		// マスター音量設定
					for (int i = 0; i < channel_max; i++) {
						player[i].set_volume();								// 音量再設定
					}
				}
				break;

			  case COMMAND_NEXT :
				player[p->channel].set_next(p->data, p->size, p->loop);
				break;

			  case COMMAND_PAUSE :
				if ( p->channel >= 0 ) {
					player[p->channel].pause();
				}
				else {
					for (int i = 0; i < channel_max; i++) {			// 全て一時停止
						player[i].pause();
					}
				}
				break;

			  case COMMAND_RESUME :
				if ( p->channel >= 0 ) {
					player[p->channel].resume();
				}
				else {
					for (int i = 0; i < channel_max; i++) {			// 全て再開
						player[i].resume();
					}
				}
				break;

			  case COMMAND_RELEASE :
				return	nullptr;
			}
		}
	}
	return	nullptr;
}


/**********
    稼働
 **********/
void	Sound::update(void)
{
	if ( channel_max > 0 ) {
		set_command(-1, COMMAND_UPDATE);
	}
}

/********************************************************
    再生
		引数	_channel = チャンネル番号
				_data    = サウンドデータ
				_size    = サウンドデータサイズ
				_loop    = ループ回数（0:無限ループ）
				_vol     = 音量
 *******************************************************/
void	Sound::prepare(int _channel, const void* _data, uint32_t _size, int _loop, float _vol)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_PREPARE, _data, _size, _loop, _vol);
}

void	Sound::play(int _channel)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_PLAY);
}

void	Sound::play(int _channel, const void* _data, uint32_t _size, int _loop, float _vol)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_PLAY, _data, _size, _loop, _vol);
}

void	Sound::play(void)
{
	set_command(-1, COMMAND_PLAY);				// 全て再生開始
}

/*******************************************
    停止
		引数	_channel = チャンネル番号
				_cnt     = フェード時間
 *******************************************/
void	Sound::stop(int _channel, int _cnt)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_STOP, nullptr, (uint32_t)_cnt);
}

void	Sound::stop(void)
{
	set_command(-1, COMMAND_STOP);				// 全て停止
}

/*********************************************************
    音量設定
		引数	_channel = チャンネル番号
				_vol     = 音量（0.0:最小 ～ 1.0:最大）
 *********************************************************/
void	Sound::set_volume(int _channel, float _vol)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_VOLUME, nullptr, 0, 0, _vol);
}

/*****************************************************
    マスター音量設定
		引数	_vol = 音量（0.0:最小 ～ 1.0:最大）
 *****************************************************/
void	Sound::set_master_volume(float _vol)
{
	set_command(-1, COMMAND_VOLUME, nullptr, 0, 0, _vol);
}

/*******************************************************
    連続再生設定
		引数	_channel = チャンネル番号
				_data    = サウンドデータ
				_size    = サウンドデータサイズ
				_loop    = ループ回数（0:無限ループ）
 *******************************************************/
void	Sound::set_next(int _channel, const void* _data, uint32_t _size, int _loop)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_NEXT, _data, _size, _loop);
}

/*********************************************
    一時停止
		引数	_channel = チャンネル番号
 *********************************************/
void	Sound::pause(int _channel)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_PAUSE);
}

void	Sound::pause(void)
{
	set_command(-1, COMMAND_PAUSE);				// 全て一時停止
}

/*********************************************
    再開
		引数	_channel = チャンネル番号
 *********************************************/
void	Sound::resume(int _channel)
{
	assert((0 <= _channel) && (_channel < channel_max));

	set_command(_channel, COMMAND_RESUME);
}

void	Sound::resume(void)
{
	set_command(-1, COMMAND_RESUME);			// 全て再開
}

}
