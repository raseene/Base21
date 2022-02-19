/********************************

		システムエンジン

 ********************************/

#include "Engine.h"
#include "Renderer.h"
#include "TouchPanel.h"
#include "Sound.h"
#include "Scene.h"

#include "swappy/swappyGL.h"
#include <time.h>


namespace sys
{

struct android_app*		Engine::app;		// アプリ
JNIEnv*		Engine::mJniEnv;
int			Engine::frame_rate;				// フレームレート
Scene*		Engine::scene;					// シーン
int			Engine::phase;					// 実行段階
uint32_t	Engine::screen_state;			// 画面の状態


/*** 画面の状態 *******/
enum
{
//	FOCUS	= (1 << 0),
	FOCUS	= 0,
	VISIBLE	= (1 << 1),
	WINDOW	= (1 << 2),
};

/*** 実行段階 *******/
enum
{
	RESUME,
	START_APP,
	RUN,
	PAUSE,
	EXIT,
};


/*******************************************
    初期化
		引数	_param = アプリパラメータ
		戻り値	TRUE ：成功
				FALSE：失敗
 *******************************************/
bool	Engine::create(struct android_app* _app, const AppParam& _param)
{
	app = _app;
	app->activity->vm->AttachCurrentThread(&mJniEnv, nullptr);

	phase = RESUME;										// 実行段階
	screen_state = 0;									// 画面の状態
	app->onAppCmd = handle_cmd;							// コマンド処理関数

	frame_rate = _param.frame_rate;						// フレームレート

	Renderer::create(_param);							// 描画管理作成
	Sound::create(_param.sound_channel);				// サウンド管理作成
	TouchManager::create(_param.touch_max);				// タッチパネル管理作成
	Scene::create_manager();							// シーン共通処理作成

	LOGI("Calling SwappyGL_init");
	SwappyGL_init(mJniEnv, app->activity->javaGameActivity);
	SwappyGL_setSwapIntervalNS(UINT64_C(1000000000)/frame_rate);

	timespec	_t;
	clock_gettime(CLOCK_REALTIME, &_t);					// 現在時刻取得
	srand(_t.tv_sec + _t.tv_nsec);						// 乱数初期化

	while ( screen_state != (FOCUS | VISIBLE | WINDOW) ) {			// 開始待ち
		if ( !poll_all() ) {
			return	false;
		}
	}
	Renderer::resume(app->window);						// 描画管理初期化
	Sound::resume_manager();							// サウンド管理初期化
	TouchManager::resume(android_app_swap_input_buffers(app));		// タッチパネル管理初期化
	return	true;
}

/**********
    終了
 **********/
void	Engine::release(void)
{
	SwappyGL_destroy();
	TouchManager::release();							// タッチパネル管理終了
	Sound::release();									// サウンド管理終了
	Renderer::release();								// 描画管理終了
	if ( mJniEnv ) {
		app->activity->vm->DetachCurrentThread();
		mJniEnv = nullptr;
	}
}

/*********************************
    コールバック実行
			戻り値	TRUE ：実行
					FALSE：終了
 *********************************/
bool	Engine::poll_all(void)
{
	int		_events;
	struct android_poll_source*		_source;

	while ( (ALooper_pollAll((screen_state == (FOCUS | VISIBLE | WINDOW)) ? 0 : -1, nullptr, &_events, (void**)&_source)) >= 0 ) {
		if ( _source ) {
			_source->process(app, _source);
		}
		if ( app->destroyRequested ) {
			return	false;
		}
	}
	return	true;
}

/******************
    コマンド処理
 ******************/
void	Engine::handle_cmd(struct android_app* _app, int32_t _cmd)
{
	switch ( _cmd ) {
	  case APP_CMD_SAVE_STATE :
		LOGI("NativeEngine: APP_CMD_SAVE_STATE");
		break;

	  case APP_CMD_INIT_WINDOW :
		LOGI("NativeEngine: APP_CMD_INIT_WINDOW");
		if ( (phase < PAUSE) && _app->window ) {
			screen_state |= WINDOW;
			SwappyGL_setWindow(_app->window);
		}
		break;

	  case APP_CMD_TERM_WINDOW :
		LOGI("NativeEngine: APP_CMD_TERM_WINDOW");
		if ( phase != EXIT ) {
			Renderer::kill_surface();
		}
		screen_state &= ~WINDOW;
		break;

	  case APP_CMD_GAINED_FOCUS :
		LOGI("NativeEngine: APP_CMD_GAINED_FOCUS");
		screen_state |= FOCUS;
		break;

	  case APP_CMD_LOST_FOCUS :
		LOGI("NativeEngine: APP_CMD_LOST_FOCUS");
		screen_state &= ~FOCUS;
		break;

	  case APP_CMD_PAUSE :
		LOGI("NativeEngine: APP_CMD_PAUSE");
		if ( scene ) {
			scene->pause();
		}
		TouchManager::pause();							// タッチパネル管理停止
		Sound::pause_manager();							// サウンド管理停止
		Renderer::pause();								// 描画管理停止
		if ( phase != EXIT ) {
			phase = PAUSE;
		}
		break;

	  case APP_CMD_RESUME :
		LOGI("NativeEngine: APP_CMD_RESUME");
		if ( phase != EXIT ) {
			phase = RESUME;
		}
		break;

	  case APP_CMD_START :
		LOGI("NativeEngine: APP_CMD_START");
		screen_state |= VISIBLE;
		break;

	  case APP_CMD_STOP :
		LOGI("NativeEngine: APP_CMD_STOP");
		screen_state &= ~VISIBLE;
		break;

	  case APP_CMD_WINDOW_RESIZED :
	  case APP_CMD_CONFIG_CHANGED :
		LOGI("NativeEngine: %s", _cmd == APP_CMD_WINDOW_RESIZED ? "APP_CMD_WINDOW_RESIZED" : "APP_CMD_CONFIG_CHANGED");
		break;

	  case APP_CMD_LOW_MEMORY :
		LOGI("NativeEngine: APP_CMD_LOW_MEMORY");
		break;

	  case APP_CMD_CONTENT_RECT_CHANGED :
		LOGI("NativeEngine: APP_CMD_CONTENT_RECT_CHANGED");
		break;

	  case APP_CMD_WINDOW_REDRAW_NEEDED :
		LOGI("NativeEngine: APP_CMD_WINDOW_REDRAW_NEEDED");
		break;

	  case APP_CMD_WINDOW_INSETS_CHANGED :
		LOGI("NativeEngine: APP_CMD_WINDOW_INSETS_CHANGED");
		break;

	  case APP_CMD_DESTROY :
		LOGI("NativeEngine: APP_CMD_DESTROY");
		break;

	  default :
		LOGI("NativeEngine: (unknown command).");
		break;
	}
}


/*************************************
    実行
		引数	_scene = 開始シーン
 *************************************/
void	Engine::run(Scene* _scene)
{
	struct timespec		_ts;
	time_t	_t, _time0, _time1;

	phase = START_APP;
	scene = _scene;
	while ( poll_all() ) {
		switch ( phase ) {
		  case RESUME :
			Renderer::resume(app->window);				// 描画管理初期化
			Sound::resume_manager();					// サウンド管理初期化
			TouchManager::resume(android_app_swap_input_buffers(app));		// タッチパネル管理初期化

		  case START_APP :
			scene->resume();
			clock_gettime(CLOCK_MONOTONIC, &_ts);
			_time0 = _ts.tv_sec*1000000 + _ts.tv_nsec/1000;
			_time1 = 1000000;
			phase = RUN;

		  case RUN :
			{
				Renderer::prepare(app->window);			// 描画前処理

				clock_gettime(CLOCK_MONOTONIC, &_ts);
				_t = _ts.tv_sec*1000000 + _ts.tv_nsec/1000;
				_time1 += (_t - _time0)*frame_rate;
				if ( _time1 > 4*1000000 - 1 ) {
					_time1 = 4*1000000 - 1;
				}
				_time0 = _t;
				int		_loop = _time1/1000000;
				_time1 -= _loop*1000000;

				Scene*	_next = scene;
				for (; (_loop > 0) && (_next == scene); _loop--) {
					android_input_buffer*	_input = android_app_swap_input_buffers(app);

					Sound::update();					// サウンド処理
					TouchManager::update(_input);		// タッチパネル処理
					Scene::update_manager(_input);		// シーン共通処理
					_next = scene->update();
				}
				scene->draw();

				Renderer::update();						// 描画後処理

				if ( _next != scene ) {
					delete	scene;
					scene = _next;
					if ( scene ) {						// シーン切り替え
						phase = START_APP;
					}
					else {								// 終了
						phase = EXIT;
						GameActivity_finish(app->activity);
					}
				}
			}
			break;

		  case PAUSE :
		  case EXIT :
			break;
		}
	}
}

}
