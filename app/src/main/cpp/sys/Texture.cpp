/***************************

		テクスチャ

 ***************************/

#include "Texture.h"
#include "Renderer.h"
#include "Scene.h"

#include <libpng/jni/png.h>


namespace sys
{

/*****************************************
    作成
		引数	data = テクスチャデータ
 *****************************************/
void	Texture::create(const uint8_t* _data)
{
	release();
	glGenTextures(1, &texture);					// テクスチャオブジェクト作成
	set_image(_data);
}

void	Texture::create(short _format, short _width, short _height, const uint8_t* _data)
{
	format = _format;
	width  = _width;
	height = _height;
	create(_data);
}

void	Texture::set_image(const uint8_t* _data)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch ( format ) {
	  case FORMAT_RGBA :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _data);
		break;

	  case FORMAT_RGB :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, _data);
		break;

	  case FORMAT_RGB565 :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, _data);
		break;

	  case FORMAT_ETC :
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, width*height/2, _data);
		break;

	  default :
		assert(false);
		break;
	}
}

/**********
    削除
 **********/
void	Texture::release(void)
{
	if ( texture ) {
		glDeleteTextures(1, &texture);
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
void	Texture::load(const uint8_t* _data)
{
	switch ( *((uint32_t*)_data) ) {
	  case 0x474e5089 :				// PNG
		load_png(_data);
		break;

	  case 0x204d4b50 :				// PKM
		load_pkm(_data);
		break;

	  default :						// 生データ
		{
			const short*	p = (const short*)_data;

			assert(p[0] == 0x0000);
			format	= p[1];
			width	= p[2];
			height	= p[3];
			create((const uint8_t*)_data + 8);
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
	uint8_t**	_p = (uint8_t**)png_get_io_ptr(png_ptr);		// データポインタ

	memcpy(data, *_p, length);
	*_p += length;
}

/**********************************
    PNG読み込み
		引数	data = PNGデータ
 **********************************/
void	Texture::load_png(const uint8_t* data)
{
	png_structp		_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	assert(_png);

	png_infop		_info = png_create_info_struct(_png);
	assert(_info);

	png_set_read_fn(_png, nullptr, png_read);
	png_init_io(_png, (png_FILE_p)&data);
	png_read_info(_png, _info);

	png_uint_32		_width, _height;
	int				_bit_depth, _color_type;

	png_get_IHDR(_png, _info, &_width, &_height, &_bit_depth, &_color_type, nullptr, nullptr, nullptr);
	switch ( _color_type ) {
	  case PNG_COLOR_TYPE_RGB :
		format = FORMAT_RGB;
		break;

	  case PNG_COLOR_TYPE_PALETTE :
		png_set_palette_to_rgb(_png);
		png_read_update_info(_png, _info);
		png_get_IHDR(_png, _info, &_width, &_height, &_bit_depth, &_color_type, nullptr, nullptr, nullptr);
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
	uint8_t*	_buf = (uint8_t*)memalign(4, _row_bytes*height);
	png_bytep	_rows[height];

	assert(_buf != nullptr);
	for (int i = 0; i < height; i++) {
		_rows[i] = _buf + i*_row_bytes;
	}
	png_read_image(_png, _rows);

	create(_buf);				// テクスチャ作成

	png_destroy_read_struct(&_png, &_info, nullptr);
	free(_buf);
}


/**********************************
    PKM読み込み
		引数	data = PKMデータ
 **********************************/
void	Texture::load_pkm(const uint8_t* data)
{
	format	= FORMAT_ETC;
	width	= (uint16_t)(((uint16_t)data[8] << 8) | data[9]);
	height	= (uint16_t)(((uint16_t)data[10] << 8) | data[11]);
	create(data + 0x10);
}



uint32_t	CTexture::cache_limit;			// キャッシュ使用メモリサイズ最大値
uint32_t	CTexture::cache_mem_size;		// キャッシュ使用メモリサイズ
CTexture*	CTexture::cache_top;			// 先頭
CTexture*	CTexture::cache_last;			// 終端


/**********
    使用
 **********/
void	CTexture::bind(void)
{
	load();								// データ読み込み
	Texture::bind();
}

/********************
    データ読み込み
 ********************/
void	CTexture::load(void)
{
	if ( texture == 0 ) {
		switch ( type ) {
		  case RES_MEMORY :				// メモリ
			Texture::load((const uint8_t*)data);
			break;

		  case RES_ASSET :				// assetsファイル
			Texture::load(Scene::load_asset((const char*)data, nullptr).get());
			break;

		  default :
			assert(false);
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

		prev = nullptr;					// リスト先頭に追加
		next = cache_top;
		if ( next ) {
			next->prev = this;
		}
		else {
			cache_last = this;
		}
		cache_top = this;

		cache_mem_size += mem_size;
		while ( cache_mem_size > cache_limit ) {
			cache_last->release();
		}
	}
	else if ( prev ) {					// 先頭以外
		prev->next = next;
		if ( next ) {
			next->prev = prev;
		}
		else {
			cache_last = prev;
		}

		prev = nullptr;					// リスト先頭に移動
		next = cache_top;
		next->prev = this;
		cache_top = this;
	}
}

/**********
    削除
 **********/
void	CTexture::release(void)
{
	if ( texture ) {
		Texture::release();

		if ( prev ) {
			prev->next = next;
			if ( next ) {
				next->prev = prev;
			}
			else {
				cache_last = prev;
			}
		}
		else {
			cache_top = next;
			if ( next ) {
				next->prev = nullptr;
			}
		}
		cache_mem_size -= mem_size;
	}
}

/**********************
    キャッシュクリア
 **********************/
void	CTexture::clear_cache(void)
{
	CTexture*	_tex = cache_top;

	while ( _tex ) {
		_tex->Texture::release();
		_tex = _tex->next;
	}
	cache_mem_size = 0;
	cache_top = nullptr;
}


#ifndef	NDEBUG
/**********************
    テクスチャリスト
 **********************/
void	CTexture::list_texture(void)
{
	CTexture*	_tex = cache_top;

	LOGI("--- Texture List ---");
	while ( _tex ) {
		switch ( _tex->type ) {
		  case RES_MEMORY :
			LOGI("%x：%d", _tex->data, _tex->mem_size);
			break;

		  case RES_ASSET :
			LOGI("%s：%d", (char*)_tex->data, _tex->mem_size);
			break;
		}
		_tex = _tex->next;
	}
}

#endif

}
