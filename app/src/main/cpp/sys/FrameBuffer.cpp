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
	Texture::create(FORMAT_RGB, _width, _height, nullptr);		// テクスチャ作成
	glGenFramebuffers(1, &frame_buffer);						// フレームバッファオブジェクト作成
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);		// フレームバッファにアタッチ
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = 0; i < 4; i++) {								// 透視変換行列初期化
		for (int j = 0; j < 4; j++) {
			mat_projection[i][j] = 0.0f;
		}
	}
	mat_projection[0][0] =  2.0f/width;
	mat_projection[1][1] = -2.0f/height;
	mat_projection[2][2] =  1.0f;
	mat_projection[3][3] =  1.0f;
}

/**********
    削除
 **********/
void	FrameBuffer::release(void)
{
	if ( frame_buffer ) {
		glDeleteFramebuffers(1, &frame_buffer);
	}
	Texture::release();
}

/**********
    使用
 **********/
void	FrameBuffer::bind(void)
{
	assert(frame_buffer != 0);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glViewport(0, 0, width, height);							// ビューポート設定
	Renderer::set_projection(&mat_projection[0][0]);			// 透視変換行列設定
	glFlush();
}

}
