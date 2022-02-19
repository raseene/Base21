#ifndef	___SYS_SCENE_H___
#define	___SYS_SCENE_H___

#include "common.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"
#include "Engine.h"


namespace sys
{

/************
    シーン
 ************/
class Scene
{
public :

	virtual	~Scene() {}							// デストラクタ

	virtual void	resume(void) {}				// 再開
	virtual void	pause(void) {}				// 一時停止

	virtual Scene*	update(void)				// 実行
					{
						if ( common_counter % 100 == 0 ) {
							LOGI("Scene Update... %d", common_counter);
						}
						return	this;
					}
	virtual void	draw(void) {}				// 描画


// キー種類
enum
{
	KEY_BACK = (1 << 0),			// バックキー
};

	static uint32_t		common_counter;			// 汎用カウンタ
	static uint32_t		key_status;				// キー入力状態

	static void		create_manager(void)							// 共通処理作成
					{
						common_counter = 0;
						key_status = 0;
					}
	static void		update_manager(android_input_buffer*);			// 共通処理稼働

	static std::unique_ptr<uint8_t>		load_asset(const char*, size_t* size = nullptr);		// assetファイル読み込み
};


/**********************
    Java関数呼び出し
 **********************/
struct JavaAccessor
{
	JNIEnv*		env;
	jclass		clazz;

		JavaAccessor(void)				// コンストラクタ
		{
			Engine::app->activity->vm->AttachCurrentThread(&env, nullptr);
			clazz = env->GetObjectClass(Engine::app->activity->javaGameActivity);
		}
		~JavaAccessor()					// デストラクタ
		{
			Engine::app->activity->vm->DetachCurrentThread();
		}
};

}
#endif
