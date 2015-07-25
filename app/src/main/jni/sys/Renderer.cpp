/***************************

		描画管理

 ***************************/

#include "Renderer.h"
#include "Sprite.h"


namespace sys
{

ShaderProgram*	Renderer::shader;							// シェーダプログラム
ShaderProgram*	Renderer::current_shader;					// 使用中シェーダ
GLfloat const*	Renderer::mat_projection;					// 透視変換行列
SRect			Renderer::screen_rect;						// 画面解像度
SRect			Renderer::limit_rect;						// 表示画面解像度
FrameBuffer*	Renderer::frame_buffer;						// フレームバッファ
GLuint			Renderer::current_texture;					// 使用中テクスチャ
GLubyte const*	Renderer::current_color;					// 設定中カラー
GLfloat const*	Renderer::current_texcoord;					// 設定中UV座標
u8*				Renderer::prim_buffer = NULL;				// プリミティブ用汎用バッファ
u32				Renderer::prim_p;
GLubyte			Renderer::screen_color[4*4];				// 画面描画カラー
int				Renderer::fade_bright;						// 画面の明るさ
int				Renderer::fade_speed;						// フェードの速さ


/************************************************
    初期化
		引数	width, height = 端末画面サイズ
 ************************************************/
void	Renderer::init(int width, int height)
{
	initShader();													// シェーダ初期化
	frame_buffer = new FrameBuffer(LIMIT_WIDTH, LIMIT_HEIGHT);		// フレームバッファ作成

	if ( width > 0 ) {
		if ( width*SCREEN_HEIGHT < height*SCREEN_WIDTH ) {			// 横長（上下カット）
			screen_rect.w = width;
			screen_rect.h = width*SCREEN_HEIGHT/SCREEN_WIDTH;
			limit_rect.w  = width*LIMIT_WIDTH/SCREEN_WIDTH;
			limit_rect.h  = width*LIMIT_HEIGHT/SCREEN_WIDTH;
		}
		else {														// 縦長（左右カット）
			screen_rect.w = height*SCREEN_WIDTH/SCREEN_HEIGHT;
			screen_rect.h = height;
			limit_rect.w  = height*LIMIT_WIDTH/SCREEN_HEIGHT;
			limit_rect.h  = height*LIMIT_HEIGHT/SCREEN_HEIGHT;
		}
		screen_rect.x = (width - screen_rect.w)/2;
		screen_rect.y = (height - screen_rect.h)/2;
		limit_rect.x  = (width - limit_rect.w)/2;
		limit_rect.y  = (height - limit_rect.h)/2;
	}

	prim_buffer = (u8*)memalign(4, PRIM_BUF_SIZE);					// プリミティブ用汎用バッファ
	prim_p = 0;

	TexCache::init();												// テクスチャキャッシュ初期化

	memset(screen_color, 0xff, 4*4);								// スクリーン描画カラー初期化
	fade_bright	= 0;												// 画面の明るさ
	fade_speed	= 0;												// フェードの速さ
}

/********************
    シェーダ初期化
 ********************/
void	Renderer::initShader(void)
{
	shader = new ShaderProgram[SHADER_MAX];

	GLuint	vert_shader, frag_shader;

	{						// SHADER_PLAIN（テクスチャ無し）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"attribute vec4 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
					"uniform mat4 projection;"
					"void main() {"
						"gl_Position = projection*position;"
						"vColor = color;"
					"}";

		static const
		char	gFragmentShader[] = 				// フラグメントシェーダプログラム
					"precision mediump float;"
					"varying vec4 vColor;"
 					"void main() {"
						"gl_FragColor = vColor;"
					"}";

		vert_shader = loadShader(GL_VERTEX_SHADER, gVertexShader);			// 頂点シェーダ
		frag_shader = loadShader(GL_FRAGMENT_SHADER, gFragmentShader);		// フラグメントシェーダ

		shader[SHADER_PLAIN].init(vert_shader, frag_shader, FALSE);			// シェーダプログラム作成
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
	}

	{						// SHADER_TEXTURE（テクスチャ有り）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"attribute vec4 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
					"attribute vec2 texcoord;"
					"varying vec2 vTexcoord;"
					"uniform mat4 projection;"
					"void main() {"
						"gl_Position = projection*position;"
						"vColor = color;"
						"vTexcoord = texcoord;"
					"}";

		static const
		char	gFragmentShader[] = 				// フラグメントシェーダプログラム
					"precision mediump float;"
					"varying vec4 vColor;"
					"varying vec2 vTexcoord;"
					"uniform sampler2D texture;"
 					"void main() {"
						"gl_FragColor = texture2D(texture, vTexcoord)*vColor;"
					"}";

		vert_shader = loadShader(GL_VERTEX_SHADER, gVertexShader);			// 頂点シェーダ
		frag_shader = loadShader(GL_FRAGMENT_SHADER, gFragmentShader);		// フラグメントシェーダ

		shader[SHADER_TEXTURE].init(vert_shader, frag_shader, TRUE);		// シェーダプログラム作成
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
	}
}

/*******************************************
    シェーダ作成
		引数	type   = シェーダ種類
				source = プログラムソース
		戻り値	シェーダオブジェクト
 *******************************************/
GLuint	Renderer::loadShader(GLenum type, const char* source)
{
    GLuint	_shader = glCreateShader(type);						// シェーダオブジェクト作成
    GLint	_compiled = 0;

	assert(_shader != 0);
	glShaderSource(_shader, 1, &source, NULL);					// プログラムソース設定
	glCompileShader(_shader);									// コンパイル
	glGetShaderiv(_shader, GL_COMPILE_STATUS, &_compiled);		// コンパイル結果取得
	if ( !_compiled ) {											// コンパイル失敗
		GLchar*	_buf;
		GLint	_len;

		glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &_len);
		if ( (_len > 0) && (_buf = new GLchar[_len]) ) {
			glGetShaderInfoLog(_shader, _len, NULL, _buf);		// エラーログ取得
			LOGE("Could not compile shader %d:\n%s\n", type, _buf);
			delete[]	_buf;
		}
		glDeleteShader(_shader);
		return	0;
	}
	return	_shader;
}

/***************************************************
    シェーダプログラム作成
		引数	v_shader = 頂点シェーダ
				f_shader = フラグメントシェーダ
				tex_flag = テクスチャを使用するか
 ***************************************************/
void	ShaderProgram::init(GLuint v_shader, GLuint f_shader, Bool tex_flag)
{
	program = glCreateProgram();								// プログラムオブジェクト作成
	assert(program != 0);
	glAttachShader(program, v_shader);							// 頂点シェーダアタッチ
	glAttachShader(program, f_shader);							// フラグメントシェーダアタッチ

    GLint	_linked = GL_FALSE;

	glLinkProgram(program);										// リンク
	glGetProgramiv(program, GL_LINK_STATUS, &_linked);			// リンク結果取得
	if ( !_linked ) {											// リンク失敗
		GLchar*	_buf;
		GLint	_len;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &_len);
		if ( (_len > 0) && (_buf = new GLchar[_len]) ) {
			glGetProgramInfoLog(program, _len, NULL, _buf);		// エラーログ取得
			LOGE("Could not link program:\n%s\n", _buf);
			delete[]	_buf;
		}
		glDeleteProgram(program);
		program = 0;
		return;
	}

	position	= glGetAttribLocation(program, "position");			// 座標
	color		= glGetAttribLocation(program, "color");			// カラー
	projection	= glGetUniformLocation(program, "projection");		// 透視変換
	texture		= glGetUniformLocation(program, "texture");			// テクスチャ
	texcoord	= glGetAttribLocation(program, "texcoord");			// テクスチャUV座標
}


/**********
    終了
 **********/
void	Renderer::quit(void)
{
	if ( prim_buffer ) {
		free(prim_buffer);							// プリミティブ用汎用バッファ
		prim_buffer = NULL;
		delete		frame_buffer;					// フレームバッファ削除
		delete[]	shader;							// シェーダ削除
	}
	TexCache::quit();								// テクスチャキャッシュ削除
}

void	ShaderProgram::quit(void)
{
	if ( program && Renderer::is_active() ) {
		glDeleteProgram(program);
	}
	program = 0;
}


/********************
    稼働（前処理）
 ********************/
void	Renderer::update(void)
{
	current_shader		= NULL;
	current_texture		= 0;
	current_color		= NULL;
	current_texcoord	= NULL;
	frame_buffer->bind();				// フレームバッファ使用
	Sprite::set_color();				// スプライト初期化

	glEnable(GL_BLEND);					// αブレンド初期化
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
}

/********************
    描画（後処理）
 ********************/
void	Renderer::draw(void)
{
	glFlush();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);


	if ( fade_speed > 0 ) {				// フェードイン
		fade_bright += fade_speed;
		if ( fade_bright >= 255 ) {
			fade_bright	= 255;
			fade_speed	= 0;
		}
		for (int i = 0; i < 4*3; i++) {
			screen_color[i + i/3] = (GLubyte)fade_bright;
		}
	}
	else if ( fade_speed < 0 ) {		// フェードアウト
		fade_bright += fade_speed;
		if ( fade_bright <= 0 ) {
			fade_bright	= 0;
			fade_speed	= 0;
		}
		for (int i = 0; i < 4*3; i++) {
			screen_color[i + i/3] = (GLubyte)fade_bright;
		}
	}


	static const
	GLfloat	_projection[4*4] =
			{
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f,
			};

	static const
	GLfloat	_texcoords[] =
			{
				0.0f, 0.0f,
				1.0f, 0.0f,
				0.0f, 1.0f,
				1.0f, 1.0f
			};

	static const
	GLfloat	_vertices[] =
			{
				-1.0f, -1.0f,
				 1.0f, -1.0f,
				-1.0f,  1.0f,
				 1.0f,  1.0f,
			};

	ShaderProgram*	_shader = use_shader(SHADER_TEXTURE);

	// フレームバッファテクスチャ描画
	glViewport(limit_rect.x, limit_rect.y, limit_rect.w, limit_rect.h);
	glUniformMatrix4fv(_shader->projection, 1, GL_FALSE, _projection);
	glVertexAttribPointer(_shader->position, 2, GL_FLOAT, GL_FALSE, 0, _vertices);
	glVertexAttribPointer(_shader->texcoord, 2, GL_FLOAT, GL_FALSE, 0, _texcoords);
	glVertexAttribPointer(_shader->color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, screen_color);
	glBindTexture(GL_TEXTURE_2D, frame_buffer->texture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	current_shader->unuse();
	glFlush();
}

/************************************
    シェーダ使用
		引数	num = シェーダ番号
		戻り値	シェーダ
 ************************************/
ShaderProgram*	Renderer::use_shader(int num)
{
	if ( current_shader != &shader[num] ) {			// シェーダ切り替え
		if ( current_shader ) {
			current_shader->unuse();
		}
		current_shader = &shader[num];
		current_shader->use(mat_projection);
	}
	return	current_shader;
}

void	ShaderProgram::use(const GLfloat* mat_projection)
{
	glUseProgram(program);
	if ( (projection != GL_INVALID_OPERATION) && mat_projection ) {
		glUniformMatrix4fv(projection, 1, GL_FALSE, mat_projection);
	}
	if ( position != GL_INVALID_OPERATION ) {
		glEnableVertexAttribArray(position);
	}
	if ( color != GL_INVALID_OPERATION ) {
		glEnableVertexAttribArray(color);
	}
	if ( texcoord != GL_INVALID_OPERATION ) {
		glEnableVertexAttribArray(texcoord);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	else {
		glDisable(GL_TEXTURE_2D);
	}
	glFlush();
}

void	ShaderProgram::unuse(void)
{
	if ( texcoord != GL_INVALID_OPERATION ) {
		glDisableVertexAttribArray(texcoord);
	}
	if ( color != GL_INVALID_OPERATION ) {
		glDisableVertexAttribArray(color);
	}
	if ( position != GL_INVALID_OPERATION ) {
		glDisableVertexAttribArray(position);
	}
}

/**********************************************
    テクスチャ使用
		引数	tex = テクスチャオブジェクト
 **********************************************/
void	Renderer::bind_texture(GLuint tex)
{
	if ( tex != current_texture ) {
		glBindTexture(GL_TEXTURE_2D, tex);
		current_texture = tex;
	}
}

/********************************
    カラー設定
		引数	color = カラー
 ********************************/
void	Renderer::set_color(GLubyte const* color)
{
	if ( color != current_color ) {
		glVertexAttribPointer(current_shader->color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, color);
		current_color = color;
	}
}

/********************************
    テクスチャUV座標設定
		引数	coord = UV座標
 ********************************/
void	Renderer::set_texcoord(GLfloat const* coord)
{
	if ( coord != current_texcoord ) {
		glVertexAttribPointer(current_shader->texcoord, 2, GL_FLOAT, GL_FALSE, 0, coord);
		current_texcoord = coord;
	}
}

/***********************************
    頂点座標設定
		引数	vertex = 頂点座標
 ***********************************/
void	Renderer::set_vertex(GLfloat const* vertex)
{
	glVertexAttribPointer(current_shader->position, 2, GL_FLOAT, GL_FALSE, 0, vertex);
}

/****************************************
    プリミティブバッファ取得
			引数	_size = 使用サイズ
			戻り値	バッファ
 ****************************************/
void*	Renderer::get_prim_buffer(u32 _size)
{
	if ( prim_p + _size > PRIM_BUF_SIZE ) {
		prim_p = 0;
	}

	void*	_ret = (void*)(prim_buffer + prim_p);

	prim_p += _size;
	return	_ret;
}

}

/**************** End of File ******************************************/
