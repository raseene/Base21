/***************************

		サウンド管理

 ***************************/

#include "Sound.h"


namespace sys
{

SLObjectItf		SoundPlayer::engineObject = NULL;		// エンジンオブジェクト
SLEngineItf		SoundPlayer::engineEngine;				// インタフェース
float			SoundPlayer::master_volume;				// マスター音量

/********************
    エンジン初期化
 ********************/
void	SoundPlayer::create_engine(void)
{
	SLresult	result;

	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);			// エンジンオブジェクト作成
	assert(SL_RESULT_SUCCESS == result);

	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);		// リアライズ
	assert(SL_RESULT_SUCCESS == result);

	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	assert(SL_RESULT_SUCCESS == result);									// インタフェース取得

	master_volume = 1.0f;				// マスター音量初期化
}

/******************
    エンジン終了
 ******************/
void	SoundPlayer::release_engine(void)
{
	if ( engineObject ) {
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
	}
}


/********************
    コンストラクタ
 ********************/
SoundPlayer::SoundPlayer(void)
{
	SLresult	result;

	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, NULL, NULL);
	assert(SL_RESULT_SUCCESS == result);									// 出力オブジェクト作成

	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);									// リアライズ

	pcm_buffer		= new char[PCM_BUF_SIZE];
	bqPlayerObject	= NULL;
	sound_data		= NULL;

	state = STOPPED;
}

/******************
    デストラクタ
 ******************/
SoundPlayer::~SoundPlayer()
{
	stop();
	(*outputMixObject)->Destroy(outputMixObject);
	if ( sound_data ) {
		delete	sound_data;
	}
	delete[]	pcm_buffer;
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
	if ( ptr == NULL ) {
		return	0;
	}

	size_t	_count = (size_t)((sound_data->size - ov_pos)/size);

	if ( _count >= nmemb ) {
		_count = nmemb;
	}
	else if ( _count <= 0 ) {
		return	0;
	}
	size *= _count;
	memcpy(ptr, sound_data->data + ov_pos, size);
	ov_pos += size;

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
		ov_pos += offset;
		break;

	  case SEEK_END :
		ov_pos = sound_data->size + offset;
		break;

	  case SEEK_SET :
		ov_pos = offset;
		break;

	  default :
		return	-1;
	}

	if ( ov_pos > sound_data->size ) {
		ov_pos = sound_data->size;
		return	-1;
	}
	else if ( ov_pos < 0 ) {
		ov_pos = 0;
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
	return	ov_pos;
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

/**********************
    再生コールバック
 **********************/
static
void	bqPlayerCallbackWAV(SLAndroidSimpleBufferQueueItf, void* context)
{
	((SoundPlayer*)context)->callback_wav();
}

void	SoundPlayer::callback_wav(void)
{
	if ( sound_data->loop == 1 ) {
		if ( sound_data ) {
			if ( sound_data->next == NULL ) {			// 終了
				delete	sound_data;
				sound_data = NULL;
			}
			else {										// 連続再生
				SoundData*	_next = sound_data->next;

				sound_data->next = NULL;
				delete	sound_data;
				sound_data = _next;
				(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, sound_data->data, (SLuint32)sound_data->size);
				return;
			}
		}
		(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);									// 停止
		state = STOPPED;
		return;
	}
	if ( sound_data->loop > 1 ) {
		sound_data->loop--;
	}
	(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, sound_data->data, (SLuint32)sound_data->size);		// 再生開始
}

static
void	bqPlayerCallbackOGG(SLAndroidSimpleBufferQueueItf, void* context)
{
	((SoundPlayer*)context)->callback_ogg();
}

void	SoundPlayer::callback_ogg(void)
{
	SLuint32	read_size;

	if ( bqPlayerObject == NULL ) {
		return;
	}
	while ( TRUE ) {
		read_size = ::ov_read(&ov_file, pcm_buffer, PCM_BUF_SIZE, NULL);
		if ( read_size > 0 ) {
			(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pcm_buffer, read_size);
			return;
		}

		if ( sound_data->loop != 1 ) {
			::ov_pcm_seek(&ov_file, 0);				// ループ
			if ( sound_data->loop > 1 ) {
				sound_data->loop--;
			}
		}
		else if ( sound_data->next == NULL ) {		// 終了
			break;
		}
		else {
			SoundData*	_next = sound_data->next;	// 連続再生

			sound_data->next = NULL;
			delete	sound_data;
			sound_data = _next;

			ov_pos = 0;
			if ( ::ov_open_callbacks(this, &ov_file, NULL, 0, callbacks) ) {
				LOGE("ov_open_callbacks ERROR!");
				::ov_clear(&ov_file);
				break;
			}
		}
	}

	(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);			// 終了
	if ( sound_data ) {
		delete	sound_data;
		sound_data = NULL;
	}
	state = STOPPED;
}


// データフォーマット
enum
{
	FORMAT_WAV,
	FORMAT_OGG,
};

struct WaveFormat
{
	s8		riff[4];
	u32		total_size;
	s8		fmt[8];
 	u32		fmt_size;
	u16		format;
	u16		channel;
	u32		rate;
	u32		avgbyte;
	u16		block;
	u16		bit;
	s8		data[4];
	u32		data_size;
};

/****************************************************
    再生
		引数	_data = サウンドデータ
				_size = サウンドデータサイズ
				_loop = ループ回数（0:無限ループ）
				_vol  = 音量
 ****************************************************/
void	SoundPlayer::prepare(const void* _data, u32 _size, int _loop, float _vol)
{
	stop();

	void*	_file_data;

	switch ( _size ) {
	  case FILE_ASSET :				// assetファイル
		_file_data = load_asset((char*)_data, &_size);
		_data = _file_data;
		break;

	  default :						// メモリ
		_file_data = NULL;
		break;
	}


	SLDataLocator_AndroidSimpleBufferQueue	loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM						format_pcm;
	SLDataSource							audioSrc = {&loc_bufq, &format_pcm};

	switch ( *((u32*)_data) ) {						// データフォーマット
	  case 0x46464952 :			// WAV
		{
			format = FORMAT_WAV;

			WaveFormat*		_info = (WaveFormat*)_data;

			format_pcm.formatType		= SL_DATAFORMAT_PCM;
			format_pcm.numChannels		= (SLuint32)_info->channel;
			format_pcm.samplesPerSec	= (SLuint32)_info->rate*1000;
			format_pcm.bitsPerSample	= (SLuint32)_info->bit;
			format_pcm.containerSize	= (SLuint32)_info->bit;
			format_pcm.channelMask		= (_info->channel == 1) ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
			format_pcm.endianness		= SL_BYTEORDER_LITTLEENDIAN;

			sound_data = new SoundData((char*)((u32)_data + sizeof(WaveFormat)), _info->data_size, _loop, _file_data);
		}
		break;

	  case 0x5367674f :			// OGG
		{
			format = FORMAT_OGG;

			sound_data = new SoundData((char*)_data, _size, _loop, _file_data);
			ov_pos = 0;								// 現在位置
			if ( ::ov_open_callbacks(this, &ov_file, NULL, 0, callbacks) ) {
				LOGE("ov_open_callbacks ERROR!");
				::ov_clear(&ov_file);
				delete	sound_data;
				return;
			}

			vorbis_info*	_info = ::ov_info(&ov_file, -1);

			format_pcm.formatType		= SL_DATAFORMAT_PCM;
			format_pcm.numChannels		= (SLuint32)_info->channels;
			format_pcm.samplesPerSec	= (SLuint32)_info->rate*1000;
			format_pcm.bitsPerSample	= (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16;
			format_pcm.containerSize	= (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16;
			format_pcm.channelMask		= (_info->channels == 1) ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
			format_pcm.endianness		= SL_BYTEORDER_LITTLEENDIAN;
		}
		break;

	  default :
		assert(FALSE);
		break;
	}

	SLDataLocator_OutputMix		loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	SLDataSink					audioSnk = {&loc_outmix, NULL};

	const SLInterfaceID		ids[3] = {SL_IID_PLAY, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
	const SLboolean			req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

	SLresult	result;

	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 3, ids, req);
	if ( SL_RESULT_SUCCESS != result ) {									// プレイヤーオブジェクト作成
		bqPlayerObject = NULL;
		return;
	}

	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);	// リアライズ
	assert(SL_RESULT_SUCCESS == result);

	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	assert(SL_RESULT_SUCCESS == result);									// インタフェース取得

	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);									// バッファキューインタフェース

	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	assert(SL_RESULT_SUCCESS == result);									// 音量インタフェース
	set_volume(_vol);														// 音量設定

	switch ( format ) {
	  case FORMAT_WAV :			// WAV
		{
			result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallbackWAV, this);
			assert(SL_RESULT_SUCCESS == result);							// 再生コールバック設定

			(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, sound_data->data, (SLuint32)sound_data->size);
		}
		break;

	  case FORMAT_OGG :			// OGG
		{
			result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallbackOGG, this);
			assert(SL_RESULT_SUCCESS == result);							// 再生コールバック設定

			callback_ogg();													// データ読み込み
		}
		break;
	}
	state = PREPARED;
}

void	SoundPlayer::play(void)
{
	if ( bqPlayerObject ) {
		state = PLAYING;
		(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);			// 再生開始
	}
}

void	SoundPlayer::play(const void* _data, u32 _size, int _loop, float _vol)
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
void	SoundPlayer::set_next(const void* _data, u32 _size, int _loop)
{
	if ( sound_data == NULL ) {				// すでに鳴り終わっている可能性
		play(_data, _size, _loop);
		return;
	}

	void*	_file_data;

	switch ( _size ) {
	  case FILE_ASSET :				// assetファイル
		_file_data = load_asset((char*)_data, &_size);
		_data = _file_data;
		break;

	  default :						// メモリ
		_file_data = NULL;
		break;
	}

	switch ( format ) {
	  case FORMAT_WAV :						// WAVデータ
		assert(*((u32*)_data) == 0x46464952);
		{
			WaveFormat*		_info = (WaveFormat*)_data;

			sound_data->set_next(new SoundData((char*)((u32)_data + sizeof(WaveFormat)), _info->data_size, _loop, _file_data));
		}
		break;

	  case FORMAT_OGG :						// OGGデータ
		assert(*((u32*)_data) == 0x5367674f);
		sound_data->set_next(new SoundData((char*)_data, _size, _loop, _file_data));
		break;
	}
}


/*************************************
    停止
		引数	_cnt = フェード時間
 *************************************/
void	SoundPlayer::stop(int _cnt)
{
	if ( bqPlayerObject ) {					// 再生中
		if ( (_cnt == 0) || (volume == 0.0f) ) {
			(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);			// 停止状態
			(*bqPlayerObject)->Destroy(bqPlayerObject);
			bqPlayerObject = NULL;
			if ( format == FORMAT_OGG ) {
				::ov_clear(&ov_file);
				format = -1;
			}
			if ( sound_data ) {
				delete	sound_data;
				sound_data = NULL;
			}
		}
		else {
			fade_volume = volume/_cnt;
			return;
		}
	}
	state = STOPPED;
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
Bool	SoundPlayer::set_master_volume(float _vol)
{
	if ( master_volume != _vol ) {
		master_volume = _vol;
		return	TRUE;
	}
	return	FALSE;
}


/*********************************************
    一時停止
		引数	_f = 状態フラグを変更するか
 *********************************************/
void	SoundPlayer::pause(Bool _f)
{
	if ( state == PLAYING ) {				// 再生中
		if ( bqPlayerObject ) {
			(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);		// 一時停止状態
		}
		if ( _f ) {
			state = PAUSED;
		}
	}
}

/***********************************
    再開
		引数	_state = 再開条件
 ***********************************/
void	SoundPlayer::resume(int _state)
{
	if ( state == _state ) {
		play();
	}
}


/**********
    稼働
 **********/
void	SoundPlayer::update(void)
{
	switch ( state ) {
	  case STOPPED :						// 停止中
		if ( bqPlayerObject ) {	
			(*bqPlayerObject)->Destroy(bqPlayerObject);
			bqPlayerObject = NULL;
		}
		break;

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



SoundPlayer*	SoundManager::player = NULL;			// プレイヤー

/************************
    サウンド管理初期化
 ************************/
void	SoundManager::create(void)
{
	SoundPlayer::create_engine();						// サウンドエンジン初期化
	player = new SoundPlayer[SOUND_CHANNEL_MAX];		// プレイヤー
}

/**********
    終了
 **********/
void	SoundManager::release(void)
{
	{				// コマンド停止
		JNIEnv*		env;
		Bool		attach_flag = FALSE;

		if ( g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) < 0 ) {
			if ( g_JavaVM->AttachCurrentThread(&env, NULL) < 0 ) {
				goto error;
			}
			attach_flag = TRUE;
		}

		jclass	clazz = env->FindClass("sys/SoundManager");

		if ( clazz ) {
			jmethodID	mid = env->GetStaticMethodID(clazz, "stop_command", "()V");

			if ( mid ) {
				env->CallStaticVoidMethod(clazz, mid);
			}
		}
		if ( attach_flag ) {
			g_JavaVM->DetachCurrentThread();
		}
	}

error :
	if ( player ) {
		delete[]	player;
		player = NULL;
	}
	SoundPlayer::release_engine();						// サウンドエンジン終了
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
	COMMAND_PAUSE_SYS,
	COMMAND_RESUME,			// 再開
	COMMAND_RESUME_SYS,
};

/**********
    稼働
 **********/
void	SoundManager::update(void)
{
	set_command(-1, COMMAND_UPDATE);
}

/********************************************************
    再生
		引数	_channel = チャンネル番号
				_data    = サウンドデータ
				_size    = サウンドデータサイズ
				_loop    = ループ回数（0:無限ループ）
				_vol     = 音量
 *******************************************************/
void	SoundManager::prepare(int _channel, const void* _data, u32 _size, int _loop, float _vol)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_PREPARE, _data, _size, _loop, _vol);
}

void	SoundManager::play(int _channel)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_PLAY);
}

void	SoundManager::play(int _channel, const void* _data, u32 _size, int _loop, float _vol)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_PLAY, _data, _size, _loop, _vol);
}

void	SoundManager::play(void)
{
	set_command(-1, COMMAND_PLAY);				// 全て再生開始
}

/*******************************************
    停止
		引数	_channel = チャンネル番号
				_cnt     = フェード時間
 *******************************************/
void	SoundManager::stop(int _channel, int _cnt)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_STOP, NULL, (u32)_cnt);
}

void	SoundManager::stop(void)
{
	set_command(-1, COMMAND_STOP);				// 全て停止
}

/*********************************************************
    音量設定
		引数	_channel = チャンネル番号
				_vol     = 音量（0.0:最小 ～ 1.0:最大）
 *********************************************************/
void	SoundManager::set_volume(int _channel, float _vol)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_VOLUME, NULL, 0, 0, _vol);
}

/*****************************************************
    マスター音量設定
		引数	_vol = 音量（0.0:最小 ～ 1.0:最大）
 *****************************************************/
void	SoundManager::set_master_volume(float _vol)
{
	set_command(-1, COMMAND_VOLUME, NULL, 0, 0, _vol);
}

/*******************************************************
    連続再生設定
		引数	_channel = チャンネル番号
				_data    = サウンドデータ
				_size    = サウンドデータサイズ
				_loop    = ループ回数（0:無限ループ）
 *******************************************************/
void	SoundManager::set_next(int _channel, const void* _data, u32 _size, int _loop)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_NEXT, _data, _size, _loop);
}

/*********************************************
    一時停止
		引数	_channel = チャンネル番号
 *********************************************/
void	SoundManager::pause(int _channel)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_PAUSE);
}

void	SoundManager::pause(void)
{
	set_command(-1, COMMAND_PAUSE);				// 全て一時停止
}

void	SoundManager::pause_system(void)		// システムによる一時停止
{
	set_command(-1, COMMAND_PAUSE_SYS);
}

/*********************************************
    再開
		引数	_channel = チャンネル番号
 *********************************************/
void	SoundManager::resume(int _channel)
{
	assert((0 <= _channel) && (_channel < SOUND_CHANNEL_MAX));

	set_command(_channel, COMMAND_RESUME);
}

void	SoundManager::resume(void)
{
	set_command(-1, COMMAND_RESUME);			// 全て再開
}

void	SoundManager::resume_system(void)		// システムによる再開
{
	set_command(-1, COMMAND_RESUME_SYS);
}


/***********************************************
    コマンドをJavaに送る
			引数	_channel = チャンネル番号
					_command = コマンド
					_data    = データ
					_size    = サイズ
					_loop    = ループ回数
					_volume  = 音量
 ***********************************************/
void	SoundManager::set_command(int _channel, int _command, const void* _data, u32 _size, int _loop, float _volume)
{
	JNIEnv*		env;
	Bool		attach_flag = FALSE;

	if ( g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) < 0 ) {
		if ( g_JavaVM->AttachCurrentThread(&env, NULL) < 0 ) {
			return;
		}
		attach_flag = TRUE;
	}

	jclass	clazz = env->FindClass("sys/SoundManager");

	if ( clazz ) {
		jmethodID	mid = env->GetStaticMethodID(clazz, "set_command", "(SSIISF)V");

		if ( mid ) {
			env->CallStaticVoidMethod(clazz, mid,	(short)_channel, (short)_command, (int)_data, (int)_size, (short)_loop, _volume);
		}
	}
	if ( attach_flag ) {
		g_JavaVM->DetachCurrentThread();
	}
}

/********************************
    Javaからコマンドを受け取る
 ********************************/
extern "C"
{
JNIEXPORT void JNICALL	Java_sys_SoundManager_sendSoundCommand(JNIEnv*, jobject, jshort, jshort, jint, jint, jshort, jfloat);
}

JNIEXPORT void JNICALL	Java_sys_SoundManager_sendSoundCommand(JNIEnv*, jobject,
								jshort _channel, jshort _command, jint _data, jint _size, jshort _loop, jfloat _volume)
{
	SoundManager::get_command((int)_channel, (int)_command, (void*)_data, (u32)_size, (int)_loop, (float)_volume);
}

void	SoundManager::get_command(int _channel, int _command, void* _data, u32 _size, int _loop, float _volume)
{
	switch ( _command ) {
	  case COMMAND_UPDATE :
		for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {
			player[i].update();
		}
		break;

	  case COMMAND_PREPARE :
		player[_channel].prepare(_data, _size, _loop, _volume);
		break;

	  case COMMAND_PLAY :
		if ( _channel >= 0 ) {
			if ( _size ) {
				player[_channel].prepare(_data, _size, _loop, _volume);
			}
			player[_channel].play();
		}
		else {
			for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {			// 全て再開
				player[i].play();
			}
		}
		break;

	  case COMMAND_STOP :
		if ( _channel >= 0 ) {
			player[_channel].stop((int)_size);
		}
		else {
			for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {			// 全て停止
				player[i].stop();
			}
		}
		break;

	  case COMMAND_VOLUME :
		if ( _channel >= 0 ) {
			player[_channel].set_volume(_volume);
		}
		else if ( SoundPlayer::set_master_volume(_volume) ) {		// マスター音量設定
			for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {
				player[i].set_volume();								// 音量再設定
			}
		}
		break;

	  case COMMAND_NEXT :
		player[_channel].set_next(_data, _size, _loop);
		break;

	  case COMMAND_PAUSE :
		if ( _channel >= 0 ) {
			player[_channel].pause(TRUE);
		}
		else {
			for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {			// 全て一時停止
				player[i].pause(TRUE);
			}
		}
		break;

	  case COMMAND_PAUSE_SYS :
		for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {				// システムによる再開
			player[i].pause(FALSE);
		}
		break;

	  case COMMAND_RESUME :
		if ( _channel >= 0 ) {
			player[_channel].resume();
		}
		else if ( _size ) {
			for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {			// 全て再開
				player[i].resume();
			}
		}
		break;

	  case COMMAND_RESUME_SYS :
		for (int i = 0; i < SOUND_CHANNEL_MAX; i++) {				// システムによる再開
			player[i].resume(SoundPlayer::PLAYING);
		}
		break;
	}
}

}

/**************** End of File ******************************************/
