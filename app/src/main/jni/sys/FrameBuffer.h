#ifndef	___FRAME_BUFFER_H___
#define	___FRAME_BUFFER_H___

#include "common.h"
#include "Texture.h"


namespace sys
{

/********************************
    フレームバッファテクスチャ
 ********************************/
class FrameBuffer : public Texture
{
	GLuint		frame_buffer;			// フレームバッファオブジェクト
	GLfloat		mat_projection[4*4];	// 透視変換行列

public :

		FrameBuffer(void)				// コンストラクタ
		{
			frame_buffer = 0;
		}
		FrameBuffer(int _w, int _h)
		{
			make(_w, _h);
		}
		~FrameBuffer()					// デストラクタ
		{
			clear();
		}

	void	make(int, int);				// 作成
	void	make(void)
			{
				make(width, height);
			}
	void	clear(void);				// 削除
	void	bind(void);					// 使用
};

}

#endif
/********************* End of File ********************************/
