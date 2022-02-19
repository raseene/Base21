#ifndef	___SYS_SPRITE_H___
#define	___SYS_SPRITE_H___

#include	"common.h"
#include	"Renderer.h"
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

	static GLfloat	spr_vertex[4][XY];		// 頂点
	static uint32_t	spr_color[4];			// カラー

	static void		set_color(uint32_t _color = RGBA(0xff, 0xff, 0xff, 0xff));			// カラー設定
	static void		set_color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 0xff)
					{
						set_color(RGBA(_r, _g, _b, _a));
					}
	static void		set_color(uint32_t const*);


	Texture*	texture;					// テクスチャ
	GLfloat		texcoord[4][XY];			// UV座標
	short		width, height;				// 大きさ
	short		ox, oy;						// 原点
	ShaderProgram*	shader;					// シェーダー

	void	set(Texture*, Rect<short> const*, int _origin = X_CENTER | Y_CENTER);		// 設定
	void	set(Texture*, int _origin = X_CENTER | Y_CENTER);							// 設定（テクスチャに合わせた大きさ）
	void	set(short, short, int _origin = X_CENTER | Y_CENTER);						// 設定（テクスチャ無し）
	void	set_size(short _w, short _h, int _origin = X_CENTER | Y_CENTER)				// サイズ設定
			{
				width  = _w;
				height = _h;
				set_origin(_origin);
			}
	void	set_origin(int _origin = X_CENTER | Y_CENTER);								// 原点位置設定
	void	set_shader(ShaderProgram* _shader)											// シェーダ設定
			{
				shader = _shader;
			}
	void	set_shader(int _shader)
			{
				set_shader(&Renderer::shader[_shader]);
			}

	void	bind_texture();					// テクスチャ設定
	void	draw(float, float);				// 描画
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
