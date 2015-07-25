
#include "common.h"
#include "Renderer.h"
#include "Sprite.h"
#include "TouchPanel.h"
#include "Sound.h"


// スプライト
enum
{
	SPR_PHOTO = 0,			// 背景
	SPR_BALL_BLUE,			// 青ビー玉
	SPR_BALL_GREEN,			// 緑ビー玉
	SPR_BALL_RED,			// 赤ビー玉
	SPR_BALL_YELLOW,		// 黄ビー玉
	SPR_MAX,
};

static sys::Sprite*	sprite;		// スプライト


// 状態
enum
{
	PHASE_MAIN,				// メイン
	PHASE_DIALOG,			// 終了確認
	PHASE_EXIT,				// 終了
};

static int	phase;				// 状態
static int	cnt;				// カウンタ
static int	end_cnt;			// 終了カウンタ


/************
    初期化
 ************/
void	init_app(void)
{
	struct SprDef
	{
		const char*		tex_name;		// テクスチャファイル名
		SRect			coord;			// UV座標
	};

	static const
	SprDef	spr_def[SPR_MAX] =
			{
				{"photo.pkm",		{  0,   0, 640, 1136}},		// 背景
				{"biidama.png",		{  0,   0,  60,   60}},		// 青ビー玉
				{"biidama.png",		{ 60,   0,  60,   60}},		// 緑ビー玉
				{"biidama.png",		{  0,  60,  60,   60}},		// 赤ビー玉
				{"biidama.png",		{ 60,  60,  60,   60}},		// 黄ビー玉
			};

	sprite = new sys::Sprite[SPR_MAX];				// スプライト初期化
	for (int i = 0; i < SPR_MAX; i++) {
		sprite[i].set(sys::TexCache::RES_ASSET, spr_def[i].tex_name, &spr_def[i].coord);
	}


	phase	= PHASE_MAIN;		// 状態
	cnt		= 0;				// カウンタ
	end_cnt	= 0;				// 終了カウンタ
}

/**********
    終了
 **********/
void	quit_app(void)
{
	delete[]	sprite;
}


/************************
    終了確認ダイアログ
 ************************/
static
void	open_dialog(void)
{
	JNIEnv*		env;
	Bool		attach_flag = FALSE;

	if ( sys::g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) < 0 ) {
		if ( sys::g_JavaVM->AttachCurrentThread(&env, NULL) < 0 ) {
			return;
		}
		attach_flag = TRUE;
	}

	jclass	clazz = env->FindClass("app/AppActivity");

	if ( clazz ) {
		jmethodID	mid = env->GetStaticMethodID(clazz, "open_dialog", "()V");

		if ( mid ) {
			env->CallStaticVoidMethod(clazz, mid);
		}
	}
	if ( attach_flag ) {
		sys::g_JavaVM->DetachCurrentThread();
	}
}

/******************************
    稼働
		戻り値	アプリ続行か
 ******************************/
Bool	update_app(void)
{
	switch ( phase ) {
	  case PHASE_MAIN :				// メイン
		cnt++;
		if ( sys::key_status == sys::KEY_BACK ) {		// バックキー
			open_dialog();								// 終了確認ダイアログ
			phase = PHASE_DIALOG;
		}
		break;

	  case PHASE_DIALOG :			// 終了確認ダイアログ
		switch ( sys::key_status ) {
		  case sys::KEY_YES :
			sys::Renderer::fade_out(60);
			end_cnt = 60;
			phase = PHASE_EXIT;
			break;

		  case sys::KEY_NO :
			phase = PHASE_MAIN;
			break;
		}
		break;

	  case PHASE_EXIT :				// 終了
		if ( --end_cnt == 0 ) {
			return	FALSE;								// アプリ終了
		}
		break;
	}


	sprite[SPR_PHOTO].draw(0.0f, 0.0f);					// 背景
	for (int i = 0; i < 4; i++) {						// ビー玉
		int		t = ((cnt + i*15) % 60) - 30;

		sprite[SPR_BALL_BLUE + i].draw(((i*180 + cnt*4) % 720) - 360, 400 - (30*30 - t*t)/2);
	}

	return	TRUE;
}

/**************
    一時停止
 **************/
void	pause_app(void)
{
}

/**********
    再開
 **********/
void	resume_app(void)
{
}

/**************** End of File *************************************************/