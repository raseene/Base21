/***************************

		テクスチャ

 ***************************/

#include "Texture.h"
#include "Renderer.h"
#include "Sprite.h"

#include <libpng/jni/png.h>


namespace sys
{

/*****************************************
    作成
		引数	data = テクスチャデータ
 *****************************************/
void	Texture::create(const u8* data)
{
	release();

	glGenTextures(1, &texture);					// テクスチャオブジェクト作成
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch ( format ) {
	  case FORMAT_RGBA :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		break;

	  case FORMAT_RGB :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		break;

	  case FORMAT_RGB565 :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
		break;

	  case FORMAT_ETC :
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, width*height/2, data);
		break;

	  default :
		assert(FALSE);
		break;
	}
}

/**********
    削除
 **********/
void	Texture::release(void)
{
	if ( texture ) {
		if ( Renderer::is_active() ) {
			glDeleteTextures(1, &texture);
		}
		texture = 0;
	}
}

/**********
    使用
 **********/
void	Texture::bind(void)
{
	assert(texture);
	Renderer::bind_texture(texture);
}


/************************************
    データ読み込み
		引数	_data = 画像データ
 ************************************/
void	Texture::load(const u8* _data)
{
	switch ( *((u32*)_data) ) {
	  case 0x474e5089 :				// PNG
		load_png(_data);
		break;

	  case 0x204d4b50 :				// PKM
		load_pkm(_data);
		break;

	  default :
		if ( *((const short*)_data + 0) == 0x0000 ) {		// 生データ
			format	= *((const short*)_data + 1);
			width	= *((const short*)_data + 2);
			height	= *((const short*)_data + 3);
			create((const u8*)_data + 8);
		}
		else {
			assert(FALSE);
		}
		break;
	}
}


/********************************
    メモリ読み込みコールバック
 ********************************/
static
void	png_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	u8**	_p = (u8**)png_get_io_ptr(png_ptr);			// データポインタ

	memcpy(data, *_p, length);
	*_p += length;
}

/**********************************
    PNG読み込み
		引数	data = PNGデータ
 **********************************/
void	Texture::load_png(const u8* data)
{
	png_structp		_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	assert(_png);

	png_infop		_info = png_create_info_struct(_png);
	assert(_info);

	png_set_read_fn(_png, NULL, png_read);
	png_init_io(_png, (png_FILE_p)&data);
	png_read_info(_png, _info);

	png_uint_32		_width, _height;
	int				_bit_depth, _color_type;

	png_get_IHDR(_png, _info, &_width, &_height, &_bit_depth, &_color_type, NULL, NULL, NULL);
	switch ( _color_type ) {
	  case PNG_COLOR_TYPE_RGB :
		format = FORMAT_RGB;
		break;

	  case PNG_COLOR_TYPE_PALETTE :
		png_set_palette_to_rgb(_png);
		png_read_update_info(_png, _info);
		png_get_IHDR(_png, _info, &_width, &_height, &_bit_depth, &_color_type, NULL, NULL, NULL);
		format = (_color_type == PNG_COLOR_TYPE_RGB) ? FORMAT_RGB : FORMAT_RGBA;
		break;

	  case PNG_COLOR_TYPE_GRAY :
		png_set_gray_to_rgb(_png);
		png_read_update_info(_png, _info);
		format = FORMAT_RGB;
		break;

	  case PNG_COLOR_TYPE_GRAY_ALPHA :
		png_set_gray_to_rgb(_png);
		png_read_update_info(_png, _info);
		format = FORMAT_RGBA;
		break;

	  default :
		format = FORMAT_RGBA;
		break;
	}

	width  = (short)_width;
	height = (short)_height;

	int			_row_bytes = width*((format == FORMAT_RGBA) ? 4 : 3);
	u8*			_buf = (u8*)memalign(4, _row_bytes*height);
	png_bytep	_rows[height];

	assert(_buf != NULL);
	for (int i = 0; i < height; i++) {
		_rows[i] = _buf + i*_row_bytes;
	}
	png_read_image(_png, _rows);

	create(_buf);				// テクスチャ作成

	png_destroy_read_struct(&_png, &_info, NULL);
	free(_buf);
}


/**********************************
    PKM読み込み
		引数	data = PKMデータ
 **********************************/
void	Texture::load_pkm(const u8* data)
{
	format	= FORMAT_ETC;
	width	= (u16)(((u16)data[8] << 8) | data[9]);
	height	= (u16)(((u16)data[10] << 8) | data[11]);
	create(data + 0x10);
}


TexCache**	TexCache::cache = NULL;			// キャッシュ
int			TexCache::cache_num;			// キャッシュ数
u32			TexCache::cache_mem_size;		// キャッシュ使用メモリサイズ

/**********************
    キャッシュ初期化
 **********************/
void	TexCache::create(void)
{
	cache			= new TexCache *[TEX_CACHE_NUM];		// キャッシュ作成
	cache_num		= 0;
	cache_mem_size	= 0;
}

/********************
    キャッシュ削除
 ********************/
void	TexCache::release(void)
{
	if ( cache ) {
		for (int i = 0; i < cache_num; i++) {				// テクスチャ削除
			delete	cache[i];
		}
		delete[]	cache;
		cache = NULL;
	}
}

/****************************************
    テクスチャ取得
		引数	_type = リソース種類
				_data = リソースデータ
 ****************************************/
Texture*	TexCache::get_texture(short _type, const void* _data)
{
	int		i, j;

	for (i = 0; i < cache_num; i++) {
		if ( (_data == cache[i]->data) && ((_type & ~RES_UPDATE) == cache[i]->type) ) {		// 登録済み
			if ( _type & RES_UPDATE ) {				// 更新あり
				delete	cache[i];
				for (j = i; j > 0; j--) {
					cache[j] = cache[j - 1];
				}
				cache[0] = new TexCache(_type & ~RES_UPDATE, _data);
			}
			else if ( i > 0 ) {
				TexCache*	_tmp = cache[i];

				for (j = i; j > 0; j--) {
					cache[j] = cache[j - 1];
				}
				cache[0] = _tmp;					// 先頭に登録
			}
			return	(Texture*)cache[0];
		}
	}

	// 新規登録
	if ( cache_num == TEX_CACHE_NUM ) {				// 枚数オーバー
		clear_cache();
	}
	j = cache_num/2;
	for (i = cache_num; i > j; i--) {
		cache[i] = cache[i - 1];
	}
	cache[j] = new TexCache(_type & ~RES_UPDATE, _data);		// リソース読み込み
	cache_num++;
	cache_mem_size += cache[j]->mem_size;
	while ( cache_mem_size > TEX_CACHE_MEM ) {		// メモリサイズオーバー
		clear_cache();
	}
	return	(Texture*)cache[j];
}

/******************************
    最後尾のテクスチャを削除
 ******************************/
void	TexCache::clear_cache(void)
{
	cache_num--;
	cache_mem_size -= cache[cache_num]->mem_size;
	delete	cache[cache_num];						// テクスチャ削除
}

/****************************************
    リソース読み込み
		引数	_type = リソース種類
				_data = リソースデータ
 ****************************************/
TexCache::TexCache(short _type, const void* _data)
{
	type = _type;					// リソース種類
	data = _data;					// リソースデータ

	switch ( _type ) {
	  case RES_MEMORY :				// メモリ
		load((const u8*)_data);
		break;

	  case RES_ASSET :				// assetsファイル
		{
			u8*	_p = (u8*)load_asset((const char*)_data);

			load(_p);
			free(_p);
		}
		break;

	  default :
		assert(FALSE);
		break;
	}
	switch ( format ) {				// VRAM占有サイズ計算
	  case FORMAT_RGBA :
		mem_size = width*height*4;
		break;

	  case FORMAT_RGB :
		mem_size = width*height*3;
		break;

	  case FORMAT_ETC :
		mem_size = width*height/2;
		break;
	}
}


/*********************************
    描画
		引数	_color = カラー
 *********************************/
void	Texture::draw(GLfloat const* _color)
{
	Texture::bind();
	glUniform4fv(Renderer::use_shader(Renderer::SHADER_SIMPLE)->color, 1, _color);
	Renderer::set_vertex();
	Renderer::set_texcoord();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void	Texture::draw(void)
{
	static const
	GLfloat		_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	draw(_color);
}

void	Texture::draw(u32 _color)
{
	GLfloat		_p[4];

	_p[0] = (_color & 0xff)/255.0f;
	_p[1] = ((_color >> 8) & 0xff)/255.0f;
	_p[2] = ((_color >> 16) & 0xff)/255.0f;
	_p[3] = ((_color >> 24) & 0xff)/255.0f;
	draw(_p);
}

}

/**************** End of File ******************************************/
