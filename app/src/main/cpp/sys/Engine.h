#ifndef	___SYS_ENGINE_H___
#define	___SYS_ENGINE_H___

#include "common.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"
#include <EGL/egl.h>


namespace sys
{

/**********************
    アプリパラメータ
 **********************/
struct AppParam
{
	int				frame_rate		= 60;				// フレームレート

	int				screen_width	= 640;				// 画面解像度
	int				screen_height	= 960;
	int				limit_width		= 640;				// 表示画面解像度
	int				limit_height	= 1280;
	uint32_t		texture_cache	= 32*1024*1024;		// テクスチャキャッシュ容量
	EGLint const*	egl_attribs		= nullptr;			// EGLアトリビュート

	int				touch_max		= 5;				// マルチタッチ数

	int				sound_channel	= 8;				// サウンドチャンネル数
};

class Scene;			// シーン

/**********************
    システムエンジン
 **********************/
class Engine
{
	static int			phase;						// 実行段階
	static uint32_t		screen_state;				// 画面の状態

	static bool		poll_all(void);									// コールバック実行
	static void		handle_cmd(struct android_app*, int32_t);		// コマンド処理

public :

	static struct android_app*	app;				// アプリ
	static JNIEnv*	mJniEnv;
	static Scene*	scene;							// シーン

	static int		frame_rate;						// フレームレート

	static bool		create(struct android_app*, const AppParam&);	// 作成
	static void		release(void);									// 終了
	static void		run(Scene*);									// 実行
};

}
#endif
