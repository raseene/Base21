/*******************************

		スプライト

 *******************************/

#include "Sprite.h"
#include "Renderer.h"


namespace sys
{

u32 const*	Sprite::spr_color;			// カラー


/**************************************
    設定
		引数	_tex    = テクスチャ
				_coord  = UV座標
				_origin = 原点位置
 **************************************/
void	Sprite::set(Texture* _tex, SRect const* _coord, int _origin)
{
	res_type = -1;						// リソース設定済み

	texture	= _tex;						// テクスチャ
	texcoord[0][X] = texcoord[2][X] = (GLfloat)_coord->x/_tex->width;		// UV座標
	texcoord[0][Y] = texcoord[1][Y] = (GLfloat)_coord->y/_tex->height;
	texcoord[1][X] = texcoord[3][X] = (GLfloat)(_coord->x + _coord->w)/_tex->width;
	texcoord[2][Y] = texcoord[3][Y] = (GLfloat)(_coord->y + _coord->h)/_tex->height;
	width	= _coord->w;				// 幅
	height	= _coord->h;				// 高さ
	set_origin(_origin);				// 原点位置設定
	set_shader(Renderer::SHADER_TEXTURE);
}

void	Sprite::set(Texture* _tex, int _origin)
{
	res_type = -1;						// リソース設定済み

	texture	= _tex;						// テクスチャ
	texcoord[0][X] = texcoord[2][X] = 0.0f;			// UV座標
	texcoord[0][Y] = texcoord[1][Y] = 0.0f;
	texcoord[1][X] = texcoord[3][X] = 1.0f;
	texcoord[2][Y] = texcoord[3][Y] = 1.0f;
	width	= _tex->width;				// 幅
	height	= _tex->height;				// 高さ
	set_origin(_origin);				// 原点位置設定
	set_shader(Renderer::SHADER_TEXTURE);
}

void	Sprite::set(short _width, short _height, int _origin)
{
	res_type = -1;						// リソース設定済み

	texture	= NULL;						// テクスチャ無し
	width	= _width;					// 幅
	height	= _height;					// 高さ
	set_origin(_origin);				// 原点位置設定
	set_shader(Renderer::SHADER_PLAIN);
}

/******************************************
    設定（リソース指定）
		引数	_type   = リソース種類
				_data   = リソースデータ
				_coord  = UV座標
				_origin = 原点位置
 ******************************************/
void	Sprite::set(short _type, const void* _data, SRect const* _coord, int _origin)
{
	set(TexCache::get_texture(_type, _data), _coord, _origin);
	res_type = _type;					// リソース設定
	res_data = _data;
}

void	Sprite::set(short _type, const void* _data, int _origin)
{
	set(TexCache::get_texture(_type, _data), _origin);
	res_type = _type;					// リソース設定
	res_data = _data;
}

/************************************
    原点位置設定
		引数	_origin = 原点位置
 ************************************/
void	Sprite::set_origin(int _origin)
{
	switch ( _origin & 0x0f ) {
	  case X_CENTER :				// センタリング
		ox = width/2;
		break;

	  case X_LEFT :					// 左揃え
		ox = 0;
		break;

	  case X_RIGHT :				// 右揃え
		ox = width;
		break;
	}

	switch ( _origin & 0xf0 ) {
	  case Y_CENTER :				// センタリング
		oy = height/2;
		break;

	  case Y_TOP :					// 上揃え
		oy = 0;
		break;

	  case Y_BOTTOM :				// 下揃え
		oy = height;
		break;
	}
}


/***********************************
    カラー設定
		引数	_color = カラー値
 ***********************************/
void	Sprite::set_color(u32 _color)
{
	if ( _color == 0xffffffff ) {			// デフォルト
		static const
		u32		color_white[] =
				{
					0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
				};

		spr_color = color_white;
	}
	else if ( (_color != spr_color[0]) || (_color != spr_color[1]) || (_color != spr_color[2]) || (_color != spr_color[3]) ) {
		u32*	_p = (u32*)Renderer::get_prim_buffer(sizeof(u32)*4);		// カラーバッファ

		_p[0] = _color;
		_p[1] = _color;
		_p[2] = _color;
		_p[3] = _color;
		spr_color = _p;
	}
}

void	Sprite::set_color(u32 const* _color)
{
	u32*	_p = (u32*)Renderer::get_prim_buffer(sizeof(u32)*4);			// カラーバッファ

	_p[0] = _color[0];
	_p[1] = _color[1];
	_p[2] = _color[2];
	_p[3] = _color[3];
	spr_color = _p;
}


/********************
    テクスチャ設定
 ********************/
void	Sprite::bind_texture(void)
{
	if ( shader >= 0 ) {
		Renderer::use_shader((int)shader);					// シェーダ
	}
	if ( texture ) {				// テクスチャ有り
		if ( res_type >= 0 ) {								// テクスチャキャッシュ
			texture = TexCache::get_texture(res_type, res_data);			// テクスチャ取得
			res_type &= ~TexCache::RES_UPDATE;
		}
		texture->bind();									// テクスチャ
		Renderer::set_texcoord(&texcoord[0][0]);			// UV座標
	}
	Renderer::set_color((GLubyte*)spr_color);				// カラー
}

/***********************************
    描画
		引数	_x, _y   = 位置
				_sx, _sy = 拡大率
				_rot     = 回転角
 ***********************************/
void	Sprite::draw(float _x, float _y)
{
	GLfloat*	_vertex = (GLfloat*)Renderer::get_prim_buffer(sizeof(GLfloat)*4*XY);		// 頂点バッファ

	_vertex[0*XY + X] = _vertex[2*XY + X] = _x - ox;
	_vertex[0*XY + Y] = _vertex[1*XY + Y] = _y - oy;
	_vertex[1*XY + X] = _vertex[3*XY + X] = _x + (width - ox);
	_vertex[2*XY + Y] = _vertex[3*XY + Y] = _y + (height - oy);

	bind_texture();
	Renderer::set_vertex(_vertex);							// 頂点
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);					// 描画
}

void	Sprite::draw(float _x, float _y, float _sx, float _sy)
{
	GLfloat*	_vertex = (GLfloat*)Renderer::get_prim_buffer(sizeof(GLfloat)*4*XY);		// 頂点バッファ

	_vertex[0*XY + X] = _vertex[2*XY + X] = _x - _sx*ox;
	_vertex[0*XY + Y] = _vertex[1*XY + Y] = _y - _sy*oy;
	_vertex[1*XY + X] = _vertex[3*XY + X] = _x + _sx*(width - ox);
	_vertex[2*XY + Y] = _vertex[3*XY + Y] = _y + _sy*(height - oy);

	bind_texture();
	Renderer::set_vertex(_vertex);							// 頂点
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);					// 描画
}

void	Sprite::draw(float _x, float _y, float _sx, float _sy, float _rot)
{
	GLfloat*	_vertex = (GLfloat*)Renderer::get_prim_buffer(sizeof(GLfloat)*4*XY);		// 頂点バッファ

	float	_cos = cosf(_rot), _sin = sinf(_rot),
			_cx0, _sx0, _cy0, _sy0, _cx1, _sx1, _cy1, _sy1;

	_cx0 = -_sx*ox; _sx0 = _sin*_cx0; _cx0 *= _cos;
	_cy0 = -_sy*oy; _sy0 = _sin*_cy0; _cy0 *= _cos;
	_cx1 = _sx*(width - ox); _sx1 = _sin*_cx1; _cx1 *= _cos;
	_cy1 = _sy*(height - oy); _sy1 = _sin*_cy1; _cy1 *= _cos;

	_vertex[0*XY + X] = _x + _cx0 - _sy0;
	_vertex[0*XY + Y] = _y + _cy0 + _sx0;
	_vertex[1*XY + X] = _x + _cx1 - _sy0;
	_vertex[1*XY + Y] = _y + _cy0 + _sx1;
	_vertex[2*XY + X] = _x + _cx0 - _sy1;
	_vertex[2*XY + Y] = _y + _cy1 + _sx0;
	_vertex[3*XY + X] = _x + _cx1 - _sy1;
	_vertex[3*XY + Y] = _y + _cy1 + _sx1;

	bind_texture();
	Renderer::set_vertex(_vertex);							// 頂点
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);					// 描画
}

void	Sprite::draw(float const* vertex)
{
	GLfloat*	_vertex = (GLfloat*)Renderer::get_prim_buffer(sizeof(GLfloat)*4*XY);		// 頂点バッファ

	for (int i = 0; i < 4*XY; i++) {
		_vertex[i] = vertex[i];
	}

	bind_texture();
	Renderer::set_vertex(_vertex);							// 頂点
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);					// 描画
}

}

/******************* End of File *******************************************************/
