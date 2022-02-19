
#include "SceneTest.h"


extern "C"
{
	void	android_main(struct android_app*);
};

/************
    メイン
 ************/
void	android_main(struct android_app* _app)
{
	LOGI("android_main");

	sys::AppParam	_param;

	_param.limit_height = 1136;

	if ( sys::Engine::create(_app, _param) ) {		// システムエンジン初期化
		sys::Engine::run(new SceneTest());			// 実行開始
	}
	sys::Engine::release();							// システムエンジン終了
}
