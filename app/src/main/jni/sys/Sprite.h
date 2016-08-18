#ifndef	___SPRITE_H___
#define	___SPRITE_H___

#include	"common.h"
#include	"Texture.h"


namespace sys
{

/****************
    スプライト
 ****************/
class Sprite
{
public :

// 原点位置
enum
{
	X_CENTER	= 0x00,			// センタリング
	X_LEFT		= 0x01,			// 左揃え
	X_RIGHT		= 0x02,			// 右揃え
	Y_CENTER	= 0x00,			// センタリング
	Y_TOP		= 0x10,			// 上揃え
	Y_BOTTOM	= 0x20,			// 下揃え
};

	static GLfloat	spr_vertex[4][XY];			// 頂点
	static u32		spr_color[4];				// カラー

	static void		set_color(u32 _color = 0xffffffff);		// カラー設定
	static void		set_color(u8 _r, u8 _g, u8 _b, u8 _a = 0xff)
					{
						set_color(RGBA(_r, _g, _b, _a));
					}
	static void		set_color(u32 const*);


	Texture*	texture;			// テクスチャ
	GLfloat		texcoord[4][XY];	// UV座標
	short		width, height;		// 大きさ
	short		ox, oy;				// 原点
	short		shader;				// シェーダー

	short		res_type;			// リソース種類
	const void*	res_data;			// リソースデータ

	void	set(Texture*, SRect const*, int _origin = X_CENTER | Y_CENTER);				// 設定
	void	set(Texture*, int _origin = X_CENTER | Y_CENTER);							// 設定（テクスチャに合わせた大きさ）
	void	set(short, short, int _origin = X_CENTER | Y_CENTER);						// 設定（テクスチャ無し）
	void	set(short, const void*, SRect const*, int _origin = X_CENTER | Y_CENTER);	// 設定（リソース指定）
	void	set(short, const void*, int _origin = X_CENTER | Y_CENTER);
	void	set_size(short _w, short _h, int _origin = X_CENTER | Y_CENTER)				// サイズ設定
			{
				width  = _w;
				height = _h;
				set_origin(_origin);
			}
	void	set_origin(int _origin = X_CENTER | Y_CENTER);								// 原点位置設定
	void	set_shader(int _shader)														// シェーダ設定
			{
				shader = (short)_shader;
			}

	void	bind_texture();			// テクスチャ設定
	void	draw(float, float);		// 描画
	void	draw(float, float, float, float);
	void	draw(float _x, float _y, float _scl)
			{
				draw(_x, _y, _scl, _scl);
			}
	void	draw(float, float, float, float, float);
	void	draw(float const*);
};

}

#endif
/****************** End of File ***************************************/
