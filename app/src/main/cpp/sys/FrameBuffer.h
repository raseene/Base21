#ifndef	___SYS_FRAME_BUFFER_H___
#define	___SYS_FRAME_BUFFER_H___

#include "common.h"
#include "Texture.h"


namespace sys
{

/********************************
    フレームバッファテクスチャ
 ********************************/
class FrameBuffer : public Texture
{
	GLuint		frame_buffer;				// フレームバッファオブジェクト
	GLfloat		mat_projection[4][4];		// 透視変換行列

public :

		FrameBuffer(void)				// コンストラクタ
		{
			frame_buffer = 0;
		}
		FrameBuffer(int _w, int _h)
		{
			create(_w, _h);
		}
		~FrameBuffer()					// デストラクタ
		{
			release();
		}

	void	create(int, int);			// 作成
	void	release(void);				// 削除
	void	bind(void);					// 使用
};

}
#endif
