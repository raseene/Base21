/*********************************************

		フレームバッファテクスチャ

 *********************************************/

#include "FrameBuffer.h"
#include "Renderer.h"


namespace sys
{

/********************************
    作成
		引数	_width  = 幅
				_height = 高さ
 ********************************/
void	FrameBuffer::create(int _width, int _height)
{
	if ( frame_buffer ) {
		if ( (width == _width) && (height == _height) ) {
			return;
		}
		release();
	}

	glGenFramebuffers(1, &frame_buffer);					// フレームバッファオブジェクト作成
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	format	= FORMAT_RGB;			// テクスチャフォーマット
	width	= _width;				// 幅
	height	= _height;				// 高さ
	Texture::create(NULL);			// テクスチャ作成
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);	// フレームバッファにアタッチ

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = 0; i < 4*4; i++) {							// 透視変換行列初期化
		mat_projection[i] = 0.0f;
	}
	mat_projection[0]  =  2.0f/width;
	mat_projection[5]  =  2.0f/height;
	mat_projection[10] =  1.0f;
	mat_projection[15] =  1.0f;
}

/**********
    削除
 **********/
void	FrameBuffer::release(void)
{
	if ( frame_buffer ) {
		if ( Renderer::is_active() ) {
			glDeleteFramebuffers(1, &frame_buffer);
		}
		frame_buffer = 0;
		Texture::release();
	}
}

/**********
    使用
 **********/
void	FrameBuffer::bind(void)
{
	if ( frame_buffer == 0 ) {
		assert(width > 0);
		create(width, height);								// 再作成
	}

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glViewport(0, 0, width, height);						// ビューポート設定
	Renderer::mat_projection = mat_projection;				// 透視変換行列設定
	if ( Renderer::current_shader ) {
		glUniformMatrix4fv(Renderer::current_shader->projection, 1, GL_FALSE, mat_projection);
	}
	glFlush();
}

}

/**************** End of File ******************************************/
