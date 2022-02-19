#ifndef	___SCENE_TEST_H___
#define	___SCENE_TEST_H___

#include "sys/common.h"
#include "sys/Scene.h"
#include "sys/Sprite.h"


/************
    テスト
 ************/
class SceneTest : public sys::Scene
{
	int		phase;					// 状態
	int		cnt;					// カウンタ

	sys::CTexture*	texture;		// テクスチャ
	sys::Sprite*	sprite;			// スプライト

	void	open_dialog(void);			// 終了確認ダイアログ

public :

		SceneTest(void);				// コンストラクタ
		~SceneTest();					// デストラクタ

	sys::Scene*		update(void);		// 稼働
	void			draw(void);			// 描画
};

#endif
