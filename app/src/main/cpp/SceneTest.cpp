/***************************

		テスト

 ***************************/

#include "SceneTest.h"
#include "sys/Sound.h"
#include "sys/TouchPanel.h"


// テクスチャ
enum
{
	TEX_PHOTO = 0,			// 背景
	TEX_BALL,				// ビー玉
	TEX_MAX,
};

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


// 状態
enum
{
	PHASE_MAIN,				// メイン
	PHASE_DIALOG,			// 終了確認
	PHASE_EXIT,				// 終了
};


/********************
    コンストラクタ
 ********************/
SceneTest::SceneTest(void)
{
	static const
	char*	tex_name[TEX_MAX] =
	{
		"photo.pkm",
		"biidama.png",
	};

	texture = new sys::CTexture[TEX_MAX];				// テクスチャ作成
	for (int i = 0; i < TEX_MAX; i++) {
		texture[i].set(tex_name[i]);
	}


	struct SprDef
	{
		int				tex;			// テクスチャ番号
		Rect<short>		coord;			// UV座標
	};

	static const
	SprDef	spr_def[SPR_MAX] =
	{
		{TEX_PHOTO,		{  0,   0, 640, 1136}},			// 背景
		{TEX_BALL,		{  0,   0,  60,   60}},			// 青ビー玉
		{TEX_BALL,		{ 60,   0,  60,   60}},			// 緑ビー玉
		{TEX_BALL,		{  0,  60,  60,   60}},			// 赤ビー玉
		{TEX_BALL,		{ 60,  60,  60,   60}},			// 黄ビー玉
	};

	sprite = new sys::Sprite[SPR_MAX];					// スプライト作成
	for (int i = 0; i < SPR_MAX; i++) {
		sprite[i].set(&texture[spr_def[i].tex], &spr_def[i].coord);
	}


	phase	= PHASE_MAIN;				// 状態
	cnt		= 0;						// カウンタ
}

/******************
    デストラクタ
 ******************/
SceneTest::~SceneTest()
{
	delete[]	sprite;
	delete[]	texture;
}


static const int	KEY_YES    = (1 << 0);				// ダイアログ用
static const int	KEY_NO     = (1 << 1);
static const int	KEY_CANCEL = (1 << 2);

static int	key_dialog;					// ダイアログ結果

extern "C"
{
JNIEXPORT void JNICALL	Java_com_example_base_AppActivity_clickKey(JNIEnv*, jobject, jint);
}

/******************************
    ダイアログで押されたキー
 ******************************/
JNIEXPORT void JNICALL	Java_com_example_base_AppActivity_clickKey(JNIEnv*, jobject, jint _key)
{
	LOGI("Dialog Key %d", _key);
	key_dialog |= (int)_key;
}

/**********
    稼働
 **********/
sys::Scene*		SceneTest::update(void)
{
	switch ( phase ) {
	  case PHASE_MAIN :				// メイン
		cnt++;
		if ( key_status & KEY_BACK ) {					// バックキー
			open_dialog();								// 終了確認ダイアログ
			phase = PHASE_DIALOG;
		}
		break;

	  case PHASE_DIALOG :			// 終了確認
		if ( key_dialog & KEY_YES ) {
			return	nullptr;							// アプリ終了
		}
		else if ( key_dialog ) {
			phase = PHASE_MAIN;							// キャンセル
		}
		break;
	}
	return	this;
}

/************************
    終了確認ダイアログ
 ************************/
void	SceneTest::open_dialog(void)
{
	key_dialog = 0;

	sys::JavaAccessor	_java;

	jmethodID	_mid = _java.env->GetStaticMethodID(_java.clazz, "open_dialog", "()V");
	_java.env->CallStaticVoidMethod(_java.clazz, _mid);
}

/**********
    描画
 **********/
void	SceneTest::draw(void)
{
	sprite[SPR_PHOTO].draw(0.0f, 0.0f);					// 背景
	for (int i = 0; i < 4; i++) {						// ビー玉
		int		t = ((cnt + i*15) % 60) - 30;

		sprite[SPR_BALL_BLUE + i].draw(((i*180 + cnt*4) % 720) - 360, 400 - (30*30 - t*t)/2);
	}
}
