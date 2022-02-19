/***************************

		シェーダ

 ***************************/

#include "Shader.h"


namespace sys
{

/*************************************************
    シェーダプログラム作成
		引数	v_shader = 頂点シェーダ
				f_shader = フラグメントシェーダ
 *************************************************/
void	ShaderProgram::create(GLuint v_shader, GLuint f_shader)
{
	program = glCreateProgram();									// プログラムオブジェクト作成
	assert(program != 0);
	glAttachShader(program, v_shader);								// 頂点シェーダアタッチ
	glAttachShader(program, f_shader);								// フラグメントシェーダアタッチ

    GLint	_linked = GL_FALSE;

	glLinkProgram(program);											// リンク
	glGetProgramiv(program, GL_LINK_STATUS, &_linked);				// リンク結果取得
	if ( !_linked ) {												// リンク失敗
		GLint	_len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &_len);
		if ( _len > 0 ) {
			GLchar	_buf[_len];
			glGetProgramInfoLog(program, _len, nullptr, _buf);		// エラーログ取得
			LOGE("Could not link program:\n%s\n", _buf);
		}
		glDeleteProgram(program);
		program = 0;
		return;
	}

	position	= glGetAttribLocation(program, "position");			// 座標
	color		= glGetAttribLocation(program, "color");			// カラー
	if ( color < 0 ) {
		color	= glGetUniformLocation(program, "color");
	}
	projection	= glGetUniformLocation(program, "projection");		// 透視変換
	texture		= glGetUniformLocation(program, "texture");			// テクスチャ
	texcoord	= glGetAttribLocation(program, "texcoord");			// テクスチャUV座標
}

void	ShaderProgram::create(char const* v_source, char const* f_source)
{
	GLuint	vert_shader, frag_shader;

	vert_shader = load_shader(GL_VERTEX_SHADER, v_source);			// 頂点シェーダ
	frag_shader = load_shader(GL_FRAGMENT_SHADER, f_source);		// フラグメントシェーダ

	create(vert_shader, frag_shader);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}

/*******************************************
    シェーダ作成
		引数	type   = シェーダ種類
				source = プログラムソース
		戻り値	シェーダオブジェクト
 *******************************************/
GLuint	ShaderProgram::load_shader(GLenum type, const char* source)
{
    GLuint	_shader = glCreateShader(type);							// シェーダオブジェクト作成
    GLint	_compiled = 0;

	assert(_shader != 0);
	glShaderSource(_shader, 1, &source, nullptr);					// プログラムソース設定
	glCompileShader(_shader);										// コンパイル
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &_compiled);			// コンパイル結果取得
	if ( !_compiled ) {												// コンパイル失敗
		GLint	_len;
		glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &_len);
		if ( _len > 0 ) {
			GLchar	_buf[_len];
			glGetShaderInfoLog(_shader, _len, nullptr, _buf);		// エラーログ取得
			LOGE("Could not compile shader %d:\n%s\n", type, _buf);
		}
		glDeleteShader(_shader);
		return	0;
	}
	return	_shader;
}


/**********
    終了
 **********/
void	ShaderProgram::release(void)
{
	if ( program ) {
		glDeleteProgram(program);
		program = 0;
	}
}


/******************
    シェーダ使用
 ******************/
void	ShaderProgram::use(const GLfloat* _mat)
{
	glUseProgram(program);
	if ( (projection >= 0) && _mat ) {
		glUniformMatrix4fv(projection, 1, GL_FALSE, _mat);
	}
	if ( position >= 0 ) {
		glEnableVertexAttribArray(position);
	}
	if ( color >= 0 ) {
		glEnableVertexAttribArray(color);
	}
	if ( texcoord >= 0 ) {
		glEnableVertexAttribArray(texcoord);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(texture, 0);
	}
	glFlush();
}

void	ShaderProgram::unuse(void)
{
	if ( texcoord >= 0 ) {
		glDisableVertexAttribArray(texcoord);
	}
	if ( color >= 0 ) {
		glDisableVertexAttribArray(color);
	}
	if ( position >= 0 ) {
		glDisableVertexAttribArray(position);
	}
}

}
