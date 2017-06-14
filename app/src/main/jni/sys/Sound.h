#ifndef	___SOUND_H___
#define	___SOUND_H___

#include "common.h"
#include <pthread.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "Tremor/ivorbisfile.h"


namespace sys
{

/********************
    サウンドデータ
 ********************/
struct SoundData
{
// データフォーマット
enum
{
	WAV,
	OGG,
};

static const u32	FILE_ASSET = 0xffffffff;			// assetファイル指定

	int				format;			// データフォーマット
	const void*		data;			// データ
	u32				size;			// データサイズ
	int				loop;			// ループカウンタ
	long			position;		// 再生位置
	const void*		file_data;		// 終了時に解放するデータ
	SoundData*		next;			// 次のデータ

		SoundData(const void*, u32, int);		// コンストラクタ
		~SoundData()							// デストラクタ
		{
			if ( file_data ) {
				free((void*)file_data);
			}
			if ( next ) {
				delete	next;
			}
		}
	void	set_next(SoundData* _next)			// 次のデータを設定
			{
				assert(_next->format == format);
				if ( next ) {
					next->set_next(_next);
				}
				else {
					next = _next;
				}
			}
};

/************************
    サウンドプレイヤー
 ************************/
class SoundPlayer
{
public :
	static const int	PCM_BUF_SIZE = 0x1000;

	static SLObjectItf	engineObject;				// エンジンオブジェクト
	static SLEngineItf	engineEngine;				// インタフェース
	static float		master_volume;				// マスター音量

	static void		create_engine(void);			// 初期化
	static void		release_engine(void);			// 終了
	static Bool		set_master_volume(float);		// マスター音量設定


private :

	SLObjectItf						outputMixObject;			// 出力オブジェクト
	SLObjectItf						bqPlayerObject;				// プレイヤーオブジェクト
	SLPlayItf						bqPlayerPlay;				// インタフェース
	SLAndroidSimpleBufferQueueItf	bqPlayerBufferQueue;		// バッファキューインタフェース
	SLVolumeItf						bqPlayerVolume;				// 音量インタフェース
	SLDataFormat_PCM				format_pcm;					// PCMデータフォーマット

	int					state;			// 状態
	float				volume;			// 音量
	float				fade_volume;	// フェードアウト音量
	SoundData*			sound_data;		// サウンドデータ
	pthread_mutex_t		mutex;

	OggVorbis_File		ov_file;		// OggVorbisファイル情報
	char*				pcm_buffer;		// PCMデータ展開バッファ

public :

// 状態
enum
{
	STOPPED,			// 停止中
	PREPARED,			// 準備
	PLAYING,			// 再生中
	PAUSED,				// 一時停止
};

		SoundPlayer(void);				// コンストラクタ
		~SoundPlayer();					// デストラクタ
	void	create(void);				// 初期化
	void	destroy(void);				// 終了
	void	play(const void*, u32, int _loop = 1, float _vol = 1.0f);		// 再生
	void	play(void);
	void	prepare(const void*, u32, int _loop = 1, float _vol = 1.0f);	// 再生準備
	void	prepare(void);
	void	set_next(const void*, u32, int _loop = 1);						// 連続再生設定
	void	stop(int _cnt = 0);												// 停止
	void	set_volume(float);												// 音量設定
	void	set_volume(void);
	void	pause(void);													// 一時停止
	void	resume(void);													// 再開
	void	update(void);													// 稼働
	int		get_state(void)													// 状態取得
			{
				return	state;
			}

	size_t	ov_read(void*, size_t, size_t);			// メモリ読み込み
	int		ov_seek(ogg_int64_t, int);				// メモリシーク
	int		ov_close(void);							// メモリクローズ
	long	ov_tell(void);							// メモリ位置通達

	void	callback_play(void);					// 再生コールバック
};


/**********************
    サウンドコマンド
 **********************/
struct SoundCommand
{
	int				channel;		// チャンネル
	int				command;		// コマンド
	const void*		data;			// データ
	u32				size;			// サイズ
	int				loop;			// ループ回数
	float			volume;			// 音量

	void	set(int _channel, int _command, const void* _data, u32 _size, int _loop, float _volume)
			{
				channel	= _channel;
				command	= _command;
				data	= _data;
				size	= _size;
				loop	= _loop;
				volume	= _volume;
			}
};

/******************
    サウンド管理
 ******************/
class SoundManager
{
static const int	QUEUE_SIZE = SOUND_CHANNEL_MAX*4;

	static SoundPlayer*		player;				// プレイヤー

	static pthread_t		thread;				// スレッド
	static pthread_mutex_t	mutex;
	static pthread_cond_t	cond;
	static SoundCommand		queue[QUEUE_SIZE];	// コマンドキュー
	static int volatile		reserve_pos, run_pos;

	static void		start_thread(void);			// スレッド開始
	static void		stop_thread(void);			// スレッド停止
	static void		set_command(int, int, const void* _dat = NULL, u32 _size = 0, int _loop = 0, float _volume = 0.0f);
												// コマンド予約
	static void*	run(void*);					// コマンド実行スレッド

public :

	static void		create(void);				// 初期化
	static void		release(void);				// 終了
	static void		update(void);				// 稼働

	static void		play(int, const void*, u32, int _loop = 1, float _vol = 1.0f);			// 再生
	static void		play(int _channel, const char* _file, int _loop = 1, float _vol = 1.0f)
					{
						play(_channel, _file, SoundData::FILE_ASSET, _loop, _vol);
					}
	static void		play(int);
	static void		prepare(int, const void*, u32, int _loop = 1, float _vol = 1.0f);		// 再生準備
	static void		prepare(int _channel, const char* _file, int _loop = 1, float _vol = 1.0f)
					{
						prepare(_channel, _file, SoundData::FILE_ASSET, _loop, _vol);
					}
	static void		play(void);																// 全て再生
	static void		stop(int, int _cnt = 0);	// 停止
	static void		stop(void);					// 全て停止
	static void		set_volume(int, float);		// 音量設定
	static void		set_master_volume(float);	// マスター音量設定
	static void		set_next(int, const void*, u32, int _loop = 1);							// 連続再生設定
	static void		set_next(int _channel, const void* _file, int _loop = 1)
					{
						set_next(_channel, _file, SoundData::FILE_ASSET, _loop);
					}
	static void		pause(int);					// 一時停止
	static void		pause(void);				// 全て一時停止
	static void		resume(int);				// 再開
	static void		resume(void);				// 全て再開
	static void		pause_system(void);			// システム一時停止
	static void		resume_system(void);		// システム再開
	static int		get_state(int _n)			// 状態取得
					{
						return	player[_n].get_state();
					}
};

}

#endif
/********************* End of File ********************************/
