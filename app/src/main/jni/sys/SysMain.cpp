
#include "common.h"
#include "Renderer.h"
#include "TouchPanel.h"
#include "Sound.h"
#include <time.h>


using namespace sys;


extern "C"
{
JNIEXPORT int JNICALL		Java_sys_BaseActivity_initNative(JNIEnv*, jobject, jboolean, jobject);
JNIEXPORT void JNICALL		Java_sys_BaseActivity_setScreenNative(JNIEnv*, jobject, jint, jint);
JNIEXPORT void JNICALL		Java_sys_BaseActivity_quitNative(JNIEnv*, jobject);
JNIEXPORT void JNICALL		Java_sys_BaseActivity_pauseNative(JNIEnv*, jobject);
JNIEXPORT jboolean JNICALL	Java_sys_BaseActivity_updateNative(JNIEnv*, jobject, jboolean, jbyteArray, jint);
}

void	init_app(void);			// メイン初期化
void	quit_app(void);			// メイン終了
void	pause_app(void);		// メイン一時停止
void	resume_app(void);		// メイン再開
Bool	update_app(Bool);		// メイン稼働


/************
    初期化
 ************/
JNIEXPORT int JNICALL	Java_sys_BaseActivity_initNative(JNIEnv* env, jobject, jboolean init_flag, jobject mgr)
{
	LOGI("initNative");

	asset_manager = AAssetManager_fromJava(env, mgr);		// asset読み込みマネージャー
	assert(asset_manager != NULL);

	Renderer::create(init_flag);				// 描画管理初期化
	if ( TOUCH_MAX > 0 ) {
		TouchManager::create_manager();			// タッチパネル管理初期化
	}

	if ( init_flag ) {
		timespec	_t;

		clock_gettime(CLOCK_REALTIME, &_t);		// 現在時刻取得
		srand(_t.tv_sec + _t.tv_nsec);			// 乱数初期化

		if ( SOUND_CHANNEL_MAX > 0 ) {
			SoundManager::create();				// サウンド初期化
		}
		common_counter = 0;						// 汎用カウンタ
		init_app();								// アプリメイン初期化
	}
	if ( SOUND_CHANNEL_MAX > 0 ) {
		SoundManager::resume_system();			// サウンド再開
	}
	resume_app();								// アプリメイン再開

	return	(FRAME_RATE + TOUCH_MAX*0x100);
}

/********************
    画面サイズ設定
 ********************/
JNIEXPORT void JNICALL		Java_sys_BaseActivity_setScreenNative(JNIEnv*, jobject, jint width, jint height)
{
	LOGI("setScreenNative (%d, %d)", width, height);

	Renderer::set_screen(width, height);
}

/**********
    終了
 **********/
JNIEXPORT void JNICALL	Java_sys_BaseActivity_quitNative(JNIEnv*, jobject)
{
	LOGI("quitNative");

	quit_app();									// アプリメイン終了
	if ( SOUND_CHANNEL_MAX > 0 ) {
		SoundManager::release();				// サウンド終了
	}
}

/**************
    一時停止
 **************/
JNIEXPORT void JNICALL	Java_sys_BaseActivity_pauseNative(JNIEnv*, jobject)
{
	LOGI("pauseNative");

	pause_app();								// アプリメイン一時停止
	if ( SOUND_CHANNEL_MAX > 0 ) {
		SoundManager::pause_system();			// サウンド一時停止
	}
	if ( TOUCH_MAX > 0 ) {
		TouchManager::release_manager();		// タッチパネル管理終了
	}
	Renderer::release();						// 描画管理終了
}

/**********
    稼働
 **********/
JNIEXPORT jboolean JNICALL	Java_sys_BaseActivity_updateNative(JNIEnv* env, jobject, jboolean draw, jbyteArray touch, jint key)
{
	Renderer::update(draw);						// 描画前処理
	if ( SOUND_CHANNEL_MAX > 0 ) {
		SoundManager::update();					// サウンド処理
	}
	key_status = key;							// キー入力状態

	if ( TOUCH_MAX > 0 ) {
		jbyte*	dst = (jbyte*)env->GetPrimitiveArrayCritical(touch, NULL);

		TouchManager::update_manager((short*)dst);		// タッチパネル管理稼働
		env->ReleasePrimitiveArrayCritical(touch, dst, 0);
	}

	if ( update_app(draw) ) {					// アプリメイン稼働
		Renderer::draw(draw);					// 描画後処理
		common_counter++;						// 汎用カウンタ
		rand();
		return	JNI_TRUE;
	}
	return	JNI_FALSE;							// アプリ終了
}


namespace sys
{

u32			common_counter;				// 汎用カウンタ
int			key_status;					// キー入力状態

JavaVM*		g_JavaVM;					// JavaVM情報

extern "C"
jint	JNI_OnLoad(JavaVM* _vm, void*)
{
	g_JavaVM = _vm;				// JavaVM保存

	JNIEnv*		_env;

	if ( _vm->GetEnv((void**)&_env, JNI_VERSION_1_6) != JNI_OK ) {			// JNIのバージョンチェック
		return	-1;
	}
	return	JNI_VERSION_1_6;
}


AAssetManager*	asset_manager;			// asset読み込み用

/****************************************
    assetファイル読み込み
			引数	name = ファイル名
			戻り値	データ
					size：データサイズ
 ****************************************/
void*	load_asset(const char* name, u32* size)
{
	LOGI("load_asset \"%s\"", name);

	AAsset*	_asset = AAssetManager_open(asset_manager, name, AASSET_MODE_BUFFER);

	assert(_asset);

	size_t	_size = AAsset_getLength(_asset);		// ファイルサイズ
	void*	_buf = malloc(_size);					// データバッファ

	AAsset_read(_asset, _buf, _size);				// データ読み込み
	AAsset_close(_asset);
	if ( size ) {
		*size = (u32)_size;
	}
	return	_buf;
}

}

/**************** End of File *************************************************/