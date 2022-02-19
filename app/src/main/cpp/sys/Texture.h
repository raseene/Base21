#ifndef	___SYS_TEXTURE_H___
#define	___SYS_TEXTURE_H___

#include "common.h"
#include <GLES2/gl2.h>


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
		}
		Texture(short _w, short _h, const uint8_t* _data)
		{
			texture	= 0;
			create(FORMAT_RGBA, _w, _h, _data);
		}
		Texture(short _format, short _w, short _h, const uint8_t* _data)
		{
			texture	= 0;
			create(_format, _w, _h, _data);
		}
		~Texture()							// デストラクタ
		{
			release();
		}
	void	create(const uint8_t*);			// 作成
	void	create(short, short, short, const uint8_t*);
	void	set_image(const uint8_t*);
	void	release(void);					// 削除
	virtual void	bind(void);				// 使用
	void	load(const uint8_t*);			// データ読み込み
	void	load_png(const uint8_t*);		// PNG読み込み
	void	load_pkm(const uint8_t*);		// PKM読み込み
	virtual void	load(void) {};
};


/******************************
    キャッシュ対応テクスチャ
 ******************************/
class CTexture : public Texture
{
public :

	static uint32_t		cache_limit;		// キャッシュ使用メモリサイズ最大値
	static uint32_t		cache_mem_size;		// キャッシュ使用メモリサイズ
	static CTexture*	cache_top;			// 先頭
	static CTexture*	cache_last;			// 終端

	static void		init_cache(uint32_t _limit)			// キャッシュ初期化
					{
						cache_limit = _limit;
						cache_mem_size = 0;
						cache_top = nullptr;
					}
	static void		clear_cache(void);					// キャッシュクリア
#ifndef	NDEBUG
	static void		list_texture(void);					// テクスチャリスト
#endif

// リソース種類
enum
{
	RES_MEMORY	= 0,		// メモリ
	RES_ASSET,				// assetsファイル
};

private :

	short			type;			// リソース種類
	const void*		data;			// リソースデータ
	uint32_t		mem_size;		// VRAM占有サイズ

	CTexture*		prev;			// リスト構造
	CTexture*		next;

public :

		CTexture(void) {}					// コンストラクタ
		CTexture(short _type, const void* _data)
		{
			set(_type, _data);
		}
		~CTexture()							// デストラクタ
		{
			release();
		}

	void	set(short _type, const void* _data)			// 設定
			{
				release();
				type = _type;
				data = _data;
			}
	void	set(const char* _file)
			{
				set(RES_ASSET, _file);
			}
	void	bind(void);						// 使用
	void	load(void);						// データ読み込み
	void	create(const uint8_t*);			// 作成
	void	release(void);					// 削除
};

}
#endif
