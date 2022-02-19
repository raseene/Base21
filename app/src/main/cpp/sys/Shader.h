#ifndef	___SYS_SHADER_H___
#define	___SYS_SHADER_H___

#include "common.h"
#include <GLES2/gl2.h>


namespace sys
{

/************************
    シェーダプログラム
 ************************/
class ShaderProgram
{
	GLuint	program;			// プログラムオブジェクト

	static GLuint	load_shader(GLenum, const char*);			// シェーダ作成

public :

	GLint	projection;			// 透視変換
	GLint	position;			// 座標
	GLint	color;				// カラー
	GLint	texture;			// テクスチャ
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
	void	release(void);					// 終了
	void	use(const GLfloat*);			// 使用
	void	unuse(void);
};

}
#endif