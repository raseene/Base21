#ifndef	___TEXTURE_H___
#define	___TEXTURE_H___

#include "common.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


namespace sys
{

/****************
    テクスチャ
 ****************/
class Texture
{
public :

// ピクセルフォーマット
enum
{
	FORMAT_RGBA,
	FORMAT_RGB,
	FORMAT_RGB565,
	FORMAT_ETC,
};

	GLuint	texture;						// テクスチャオブジェクト
	short	format;							// テクスチャフォーマット
	short	width, height;					// サイズ

		Texture(void)						// コンストラクタ
		{
			texture	= 0;
			width	= 0;
		}
		Texture(short _w, short _h)
		{
			texture	= 0;
			format	= FORMAT_RGBA;
			width	= _w;
			height	= _h;
		}
		~Texture()							// デストラクタ
		{
			release();
		}
	void	create(const u8*);				// 作成
	void	release(void);					// 削除
	void	bind(void);						// 使用
	void	load(const u8*);				// データ読み込み
	void	load_png(const u8*);			// PNG読み込み
	void	load_pkm(const u8*);			// PKM読み込み
	void	draw(float _x = 0.0f, float _y = 0.0f);		// 描画
};


/**************************
    テクスチャキャッシュ
 **************************/
class TexCache : public Texture
{
	static TexCache**	cache;				// キャッシュ
	static int			cache_num;			// キャッシュ数
	static u32			cache_mem_size;		// キャッシュ使用メモリサイズ

	static void		clear_cache(void);		// 最後尾のテクスチャを削除

public :

// リソース種類
enum
{
	RES_MEMORY	= 0,		// メモリ
	RES_ASSET,				// assetsファイル

	RES_UPDATE	= 0x4000,	// 更新あり
};

	static void			create(void);						// 初期化
	static void			release(void);						// 削除
	static Texture*		get_texture(short, const void*);	// テクスチャ取得

	short			type;			// リソース種類
	const void*		data;			// リソースデータ
	u32				mem_size;		// VRAM占有サイズ

		TexCache(short, const void*);		// コンストラクタ
};

}

#endif
/********************* End of File ********************************/
