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
		~ShaderProgram()					// デストラクタ
		{
			quit();
		}
	void	init(GLuint, GLuint, Bool);		// 初期化
	void	use(const GLfloat*);			// 使用
	void	unuse(void);
	void	quit(void);						// 終了
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

	static void		initShader(void);					// シェーダ初期化
	static GLuint	loadShader(GLenum, const char*);	// シェーダ作成

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

	static void		init(int, int);						// 初期化
	static void		quit(void);							// 終了
	static void		update(void);						// 稼働（前処理）
	static void		draw(void);							// 描画（後処理）
	static ShaderProgram*	use_shader(int);			// シェーダ使用
	static void		bind_texture(GLuint);				// テクスチャ使用
	static void		set_color(GLubyte const*);			// カラー設定
	static void		set_texcoord(GLfloat const*);		// テクスチャUV座標設定
	static void		set_vertex(GLfloat const*);			// 頂点座標設定
	static void*	get_prim_buffer(u32);				// プリミティブバッファ取得
	static Bool		is_active(void)						// 稼働中か
					{
						return	(Bool)prim_buffer;
					}

	static void		fade_in(int _cnt = 500/FRAME_PERIOD)		// フェードイン
					{
						fade_speed = (_cnt > 0) ? (254 + _cnt)/_cnt : 255;
					}
	static void		fade_out(int _cnt = 500/FRAME_PERIOD)		// フェードアウト
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
