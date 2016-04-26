#ifndef	___SOUND_H___
#define	___SOUND_H___

#include "common.h"

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
	char*		data;			// データ
	u32			size;			// データサイズ
	int			loop;			// ループカウンタ
	void*		file_data;		// 終了時に解放するデータ
	SoundData*	next;			// 次のデータ

		SoundData(char* _data, u32 _size, int _loop, void* _file = NULL)		// コンストラクタ
		{
			data = _data;
			size = _size;
			loop = _loop;
			file_data = _file;
			next = NULL;
		}
		~SoundData()							// デストラクタ
		{
			if ( file_data ) {
				free(file_data);
			}
			if ( next ) {
				delete	next;
			}
		}
	void	set_next(SoundData* _next)			// 次のデータを設定
			{
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

	static void		init_engine(void);				// 初期化
	static void		quit_engine(void);				// 終了
	static Bool		set_master_volume(float);		// マスター音量設定


private :

	SLObjectItf						outputMixObject;			// 出力オブジェクト
	SLObjectItf						bqPlayerObject;				// プレイヤーオブジェクト
	SLPlayItf						bqPlayerPlay;				// インタフェース
	SLAndroidSimpleBufferQueueItf	bqPlayerBufferQueue;		// バッファキューインタフェース
	SLVolumeItf						bqPlayerVolume;				// 音量インタフェース

	int				state;				// 状態
	float			volume;				// 音量
	float			fade_volume;		// フェードアウト音量
	int				format;				// データフォーマット
	SoundData*		sound_data;			// サウンドデータ

	OggVorbis_File	ov_file;			// OggVorbisファイル情報
	long			ov_pos;				// 現在位置
	char*			pcm_buffer;			// PCMデータ展開バッファ

public :

// 状態
enum
{
	STOPPED,			// 停止中
	PREPARED,			// 準備
	PLAYING,			// 再生中
	PAUSED,				// 一時停止
};

static const u32	FILE_ASSET = 0xffffffff;			// assetファイル指定


		SoundPlayer(void);				// コンストラクタ
		~SoundPlayer();					// デストラクタ
	void	play(const void*, u32, int _loop = 1, float _vol = 1.0f);		// 再生
	void	play(void);
	void	prepare(const void*, u32, int _loop = 1, float _vol = 1.0f);	// 再生準備
	void	set_next(const void*, u32, int _loop = 1);						// 連続再生設定
	void	stop(int _cnt = 0);												// 停止
	void	set_volume(float);												// 音量設定
	void	set_volume(void);
	void	pause(Bool _f = TRUE);											// 一時停止
	void	resume(int _state = PAUSED);									// 再開
	void	update(void);													// 稼働
	int		get_state(void)													// 状態取得
			{
				return	state;
			}

	size_t	ov_read(void*, size_t, size_t);			// メモリ読み込み
	int		ov_seek(ogg_int64_t, int);				// メモリシーク
	int		ov_close(void);							// メモリクローズ
	long	ov_tell(void);							// メモリ位置通達

	void	callback_wav(void);						// 再生コールバック
	void	callback_ogg(void);
};


/******************
    サウンド管理
 ******************/
class SoundManager
{
	static SoundPlayer*		player;				// プレイヤー

public :

	static void		init(void);					// 初期化
	static void		quit(void);					// 終了
	static void		update(void);				// 稼働

	static void		play(int, const void*, u32, int _loop = 1, float _vol = 1.0f);			// 再生
	static void		play(int _channel, const char* _file, int _loop = 1, float _vol = 1.0f)
					{
						play(_channel, _file, sys::SoundPlayer::FILE_ASSET, _loop, _vol);
					}
	static void		play(int);
	static void		prepare(int, const void*, u32, int _loop = 1, float _vol = 1.0f);		// 再生準備
	static void		prepare(int _channel, const char* _file, int _loop = 1, float _vol = 1.0f)
					{
						prepare(_channel, _file, sys::SoundPlayer::FILE_ASSET, _loop, _vol);
					}
	static void		play(void);																// 全て再生
	static void		stop(int, int _cnt = 0);	// 停止
	static void		stop(void);					// 全て停止
	static void		set_volume(int, float);		// 音量設定
	static void		set_master_volume(float);	// マスター音量設定
	static void		set_next(int, const void*, u32, int _loop = 1);							// 連続再生設定
	static void		pause(int);					// 一時停止
	static void		pause(void);				// 全て一時停止
	static void		pause_system(void);			// システムによる一時停止
	static void		resume(int);				// 再開
	static void		resume(void);				// 全て再開
	static void		resume_system(void);		// システムによる再開
	static int		get_state(int _n)			// 状態取得
					{
						return	player[_n].get_state();
					}

	static void		set_command(int, int, const void* _dat = NULL, u32 _size = 0, int _loop = 0, float _volume = 0.0f);
												// Javaにコマンドを送る
	static void		get_command(int, int, void*, u32, int, float);
												// Javaからコマンドを受け取って実行
};

}

#endif
/********************* End of File ********************************/
