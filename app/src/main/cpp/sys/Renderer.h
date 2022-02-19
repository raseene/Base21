#ifndef	___SYS_RENDERER_H___
#define	___SYS_RENDERER_H___

#include "common.h"
#include "Engine.h"
#include "Shader.h"
#include "FrameBuffer.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


namespace sys
{

/**************
    描画管理
 **************/
class Renderer
{
	struct Current				// カレント
	{
		ShaderProgram*	shader;					// シェーダ
		GLuint			texture;				// テクスチャ
		GLubyte const*	color;					// カラー
		GLfloat const*	texcoord;				// UV座標
		GLfloat const*	vertex;					// 頂点座標

		void	init(void)
		{
			shader		= nullptr;
			texture		= 0;
			color		= nullptr;
			texcoord	= nullptr;
			vertex		= nullptr;
		}
	};

	static Current	current;					// カレント


public :

// シェーダ種類
enum
{
	SHADER_PLAIN	= 0,
	SHADER_TEXTURE,
	SHADER_SIMPLE,
	SHADER_MAX
};

	static EGLDisplay	mEglDisplay;
	static EGLSurface	mEglSurface;
	static EGLContext	mEglContext;
	static EGLConfig	mEglConfig;
	static EGLint const*	egl_attribs;

	static int	screen_width;					// 画面解像度
	static int	screen_height;
	static int	limit_width;					// 表示画面解像度
	static int	limit_height;
	static Rect<float>	screen_rect;			// 実画面
	static FrameBuffer*		frame_buffer;		// フレームバッファ
	static ShaderProgram*	shader;				// シェーダプログラム
	static GLfloat const*	mat_projection;		// 透視変換行列


	static void		create(const AppParam&);					// 作成
	static void		resume(ANativeWindow*);						// 初期化
	static void		create_shader(void);						// シェーダ作成
	static void		pause(void);								// 停止
	static void		release(void);								// 終了
	static void		init_OpenGL(ANativeWindow*);				// OpenGL初期化
	static void		kill_surface(void);							// サーフェス削除
	static void		quit_OpenGL(void);							// OpenGL終了

	static void		prepare(ANativeWindow*);					// 描画前処理
	static void		update(void);								// 描画後処理

	static void		set_projection(GLfloat const*);				// 透視変換行列設定
	static ShaderProgram*	use_shader(ShaderProgram*);			// シェーダ使用
	static ShaderProgram*	use_shader(int _n)
							{
								return	use_shader(&shader[_n]);
							}
	static void		bind_texture(GLenum, GLuint);				// テクスチャ使用
	static void		bind_texture(GLuint tex)
					{
						bind_texture(GL_TEXTURE_2D, tex);
					}
	static void		set_color(GLubyte const*);					// カラー設定
	static void		set_color(uint32_t const* _color)
					{
						set_color((GLubyte const*)_color);
					}
	static void		set_color(GLfloat const*);
	static void		set_color(void);
	static void		set_texcoord(GLfloat const*);				// テクスチャUV座標設定
	static void		set_texcoord(void);
	static void		set_vertex(GLfloat const*);					// 頂点座標設定
	static void		set_vertex(void);
	static void		set_vertex(GLfloat const* _vertex, GLfloat const* _coord, GLubyte const* _color)
					{
						set_vertex(_vertex);
						set_texcoord(_coord);
						set_color(_color);
					}
	static void		set_vertex(GLfloat const* _vertex, GLfloat const* _coord, uint32_t const* _color)
					{
						set_vertex(_vertex, _coord, (GLubyte const*)_color);
					}
	static void		set_vertex(GLfloat const* _vertex, GLfloat const* _coord, GLfloat const* _color)
					{
						set_vertex(_vertex);
						set_texcoord(_coord);
						set_color(_color);
					}
};

}
#endif
