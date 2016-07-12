#ifndef	___RENDERER_H___
#define	___RENDERER_H___

#include "common.h"
#include "FrameBuffer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


namespace sys
{

/************************
    シェーダプログラム
 ************************/
struct ShaderProgram
{
	GLuint	program;			// プログラムオブジェクト
	GLint	position;			// 座標
	GLint	color;				// カラー
	GLuint	projection;			// 透視変換
	GLuint	texture;			// テクスチャ
	GLint	texcoord;			// テクスチャUV座標

		ShaderProgram(void)					// コンストラクタ
		{
			program = 0;
		}
		ShaderProgram(char const* v_source, char const* f_source)
		{
			create(v_source, f_source);
		}
		~ShaderProgram()					// デストラクタ
		{
			release();
		}
	void	create(GLuint, GLuint);			// 初期化
	void	create(char const*, char const*);
	void	use(const GLfloat*);			// 使用
	void	unuse(void);
	void	release(void);					// 終了

	static GLuint	load_shader(GLenum, const char*);		// シェーダ作成
};

/**************
    描画管理
 **************/
class Renderer
{
	static ShaderProgram*	shader;						// シェーダプログラム

	static GLuint			current_texture;			// 使用中テクスチャ
	static GLubyte const*	current_color;				// 設定中カラー
	static GLfloat const*	current_texcoord;			// 設定中UV座標

	static u8*	prim_buffer;							// プリミティブ用汎用バッファ
	static u32	prim_p;

	static GLubyte	screen_color[4*4];					// 画面描画カラー
	static int		fade_bright;						// 画面の明るさ
	static int		fade_speed;							// フェードの速さ

	static void		create_shader(void);				// シェーダ初期化

public :

// シェーダ種類
enum
{
	SHADER_PLAIN	= 0,
	SHADER_TEXTURE,
	SHADER_MAX
};

	static SRect			screen_rect;				// 画面解像度
	static SRect			limit_rect;					// 表示画面解像度
	static FrameBuffer*		frame_buffer;				// フレームバッファ
	static ShaderProgram*	current_shader;				// 使用中シェーダ
	static GLfloat const*	mat_projection;				// 透視変換行列
	static Bool				draw_flag;					// 描画フラグ

	static void		create(Bool);						// 初期化
	static void		set_screen(int, int);				// 画面サイズ設定
	static void		release(void);						// 終了
	static void		update(Bool);						// 稼働（前処理）
	static void		draw(void);							// 描画（後処理）
	static ShaderProgram*	use_shader(ShaderProgram*);	// シェーダ使用
	static ShaderProgram*	use_shader(int _n)
							{
								return	use_shader(&shader[_n]);
							}
	static void		bind_texture(GLenum, GLuint);		// テクスチャ使用
	static void		bind_texture(GLuint tex)
					{
						bind_texture(GL_TEXTURE_2D, tex);
					}
	static void		set_color(GLubyte const*);			// カラー設定
	static void		set_color(u32 const* _color)
					{
						set_color((GLubyte const*)_color);
					}
	static void		set_texcoord(GLfloat const*);		// テクスチャUV座標設定
	static void		set_vertex(GLfloat const*);			// 頂点座標設定
	static void		set_vertex(GLfloat const* _vertex, GLfloat const* _coord, GLubyte const* _color)
					{
						set_vertex(_vertex);
						set_texcoord(_coord);
						set_color(_color);
					}
	static void		set_vertex(GLfloat const* _vertex, GLfloat const* _coord, u32 const* _color)
					{
						set_vertex(_vertex, _coord, (GLubyte const*)_color);
					}
	static void*	get_prim_buffer(u32);				// プリミティブバッファ取得
	static Bool		is_active(void)						// 稼働中か
					{
						return	(Bool)prim_buffer;
					}

	static void		fade_in(int _cnt = FRAME_RATE/2)			// フェードイン
					{
						fade_speed = (_cnt > 0) ? (254 + _cnt)/_cnt : 255;
					}
	static void		fade_out(int _cnt = FRAME_RATE/2)			// フェードアウト
					{
						fade_speed = (_cnt > 0) ? -(254 + _cnt)/_cnt : -255;
					}
	static int		get_bright(void)							// 画面明るさ取得
					{
						return	fade_bright;
					}
	static void		set_bright(int _bright = 0)					// 画面明るさ設定
					{
						fade_bright = _bright;
					}
};

}

#endif
/********************* End of File ********************************/
