/***************************

		描画管理

 ***************************/

#include "Renderer.h"
#include "Sprite.h"


namespace sys
{

ShaderProgram*	Renderer::shader;						// シェーダプログラム
ShaderProgram*	Renderer::current_shader;				// 使用中シェーダ
GLfloat const*	Renderer::mat_projection;				// 透視変換行列
SRect			Renderer::screen_rect;					// 画面解像度
SRect			Renderer::limit_rect;					// 表示画面解像度
FrameBuffer*	Renderer::frame_buffer = NULL;			// フレームバッファ
GLuint			Renderer::current_texture;				// 使用中テクスチャ
GLubyte const*	Renderer::current_color;				// 設定中カラー
GLfloat const*	Renderer::current_texcoord;				// 設定中UV座標
GLfloat const*	Renderer::current_vertex;				// 設定中頂点座標
u8*				Renderer::prim_buffer = NULL;			// プリミティブ用汎用バッファ
u32				Renderer::prim_p;
int				Renderer::fade_bright;					// 画面の明るさ
int				Renderer::fade_speed;					// フェードの速さ
Bool			Renderer::draw_flag;					// 描画フラグ


/******************************************
    初期化
		引数	init_flag = 初期化フラグ
 ******************************************/
void	Renderer::create(Bool init_flag)
{
	create_shader();												// シェーダ初期化
	frame_buffer = new FrameBuffer(LIMIT_WIDTH, LIMIT_HEIGHT);		// フレームバッファ作成

	prim_buffer = (PRIM_BUF_SIZE > 0) ? (u8*)memalign(4, PRIM_BUF_SIZE) : NULL;			// プリミティブ用汎用バッファ
	prim_p = 0;

	TexCache::create();												// テクスチャキャッシュ初期化

	if ( init_flag ) {
		fade_bright	= BRIGHT_INIT;									// 画面の明るさ
		fade_speed	= 0;											// フェードの速さ
	}
}

/************************************************
    画面サイズ設定
		引数	width, height = 端末画面サイズ
 ************************************************/
void	Renderer::set_screen(int width, int height)
{
	if ( width*SCREEN_HEIGHT < height*SCREEN_WIDTH ) {				// 横長（上下カット）
		screen_rect.w = width;
		screen_rect.h = width*SCREEN_HEIGHT/SCREEN_WIDTH;
		limit_rect.w  = width*LIMIT_WIDTH/SCREEN_WIDTH;
		limit_rect.h  = width*LIMIT_HEIGHT/SCREEN_WIDTH;
	}
	else {															// 縦長（左右カット）
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

/********************
    シェーダ初期化
 ********************/
void	Renderer::create_shader(void)
{
	shader = new ShaderProgram[SHADER_MAX];

	{						// SHADER_PLAIN（テクスチャ無し）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"uniform mat3 projection;"
					"attribute vec3 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
					"void main() {"
						"gl_Position = vec4(projection*position, 1.0);"
						"vColor = color;"
					"}";

		static const
		char	gFragmentShader[] = 				// フラグメントシェーダプログラム
					"precision mediump float;"
					"varying vec4 vColor;"
					"void main() {"
						"gl_FragColor = vColor;"
					"}";

		shader[SHADER_PLAIN].create(gVertexShader, gFragmentShader);
	}

	{						// SHADER_TEXTURE（テクスチャ有り）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"uniform mat3 projection;"
					"attribute vec3 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
					"attribute vec2 texcoord;"
					"varying vec2 vTexcoord;"
					"void main() {"
						"gl_Position = vec4(projection*position, 1.0);"
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

		shader[SHADER_TEXTURE].create(gVertexShader, gFragmentShader);
	}

	{						// SHADER_SIMPLE
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"attribute vec3 position;"
					"attribute vec2 texcoord;"
					"varying vec2 vTexcoord;"
					"void main() {"
						"gl_Position = vec4(position, 1.0);"
						"vTexcoord = texcoord;"
					"}";

		static const
		char	gFragmentShader[] = 				// フラグメントシェーダプログラム
					"precision mediump float;"
					"uniform vec4 color;"
					"varying vec2 vTexcoord;"
					"uniform sampler2D texture;"
					"void main() {"
						"gl_FragColor = texture2D(texture, vTexcoord)*color;"
					"}";

		shader[SHADER_SIMPLE].create(gVertexShader, gFragmentShader);
	}
}

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
		GLchar*	_buf;
		GLint	_len;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &_len);
		if ( (_len > 0) && (_buf = new GLchar[_len]) ) {
			glGetProgramInfoLog(program, _len, NULL, _buf);			// エラーログ取得
			LOGE("Could not link program:\n%s\n", _buf);
			delete[]	_buf;
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


/**********
    終了
 **********/
void	Renderer::release(void)
{
	if ( prim_buffer ) {
		free(prim_buffer);							// プリミティブ用汎用バッファ
		prim_buffer = NULL;
	}
	if ( frame_buffer ) {
		delete		frame_buffer;					// フレームバッファ削除
		frame_buffer = NULL;
		delete[]	shader;							// シェーダ削除
	}
	TexCache::release();							// テクスチャキャッシュ削除
}

void	ShaderProgram::release(void)
{
	if ( program && Renderer::is_active() ) {
		glDeleteProgram(program);
	}
	program = 0;
}


/************************************
    稼働（前処理）
		引数	_draw = 描画フラグ
 ************************************/
void	Renderer::update(Bool _draw)
{
	draw_flag = _draw;					// 描画フラグ

	current_shader		= NULL;
	current_texture		= 0;
	current_color		= NULL;
	current_texcoord	= NULL;
	current_vertex		= NULL;
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
	}
	else if ( fade_speed < 0 ) {		// フェードアウト
		fade_bright += fade_speed;
		if ( fade_bright <= 0 ) {
			fade_bright	= 0;
			fade_speed	= 0;
		}
	}

	if ( draw_flag ) {
		// フレームバッファテクスチャ描画
		static const
		GLfloat	_vertices[] =
				{
					-1.0f,  1.0f,
					 1.0f,  1.0f,
					-1.0f, -1.0f,
					 1.0f, -1.0f,
				};
		static GLfloat	_color[RGBA] = {1.0f, 1.0f, 1.0f, 1.0f};

		glViewport(limit_rect.x, limit_rect.y, limit_rect.w, limit_rect.h);
		frame_buffer->Texture::bind();
		_color[R] = _color[G] = _color[B] = fade_bright/255.0f;
		glUniform4fv(use_shader(SHADER_SIMPLE)->color, 1, _color);
		set_vertex(_vertices);
		set_texcoord();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		current_shader->unuse();
		glFlush();
	}
}

/************************************
    シェーダ使用
		引数	_sd = シェーダ番号
		戻り値	シェーダ
 ************************************/
ShaderProgram*	Renderer::use_shader(ShaderProgram* _sd)
{
	if ( current_shader != _sd ) {					// シェーダ切り替え
		if ( current_shader ) {
			current_shader->unuse();
		}
		current_shader = _sd;
		current_shader->use(mat_projection);
		current_vertex = NULL;
		current_texcoord = NULL;
		current_color = NULL;
	}
	return	current_shader;
}

void	ShaderProgram::use(const GLfloat* mat_projection)
{
	glUseProgram(program);
	if ( (projection >= 0) && mat_projection ) {
		glUniformMatrix3fv(projection, 1, GL_FALSE, mat_projection);
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

/*************************************************
    テクスチャ使用
		引数	target = テクスチャ種類
				tex    = テクスチャオブジェクト
 *************************************************/
void	Renderer::bind_texture(GLenum target, GLuint tex)
{
	if ( tex != current_texture ) {
		glBindTexture(target, tex);
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

void	Renderer::set_color(GLfloat const* color)
{
	if ( color != (GLfloat*)current_color ) {
		glVertexAttribPointer(current_shader->color, 4, GL_FLOAT, GL_FALSE, 0, color);
		current_color = (GLubyte*)color;
	}
}

void	Renderer::set_color(void)
{
	static const
	u32		_color[4] =
			{
				RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
			};

	set_color(_color);
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

void	Renderer::set_texcoord(void)
{
	static const
	GLfloat		_texcoords[4][XY] =
				{
					{0.0f, 0.0f},
					{1.0f, 0.0f},
					{0.0f, 1.0f},
					{1.0f, 1.0f},
				};

	set_texcoord(&_texcoords[0][0]);
}

/***********************************
    頂点座標設定
		引数	vertex = 頂点座標
 ***********************************/
void	Renderer::set_vertex(GLfloat const* vertex)
{
	if ( vertex != current_vertex ) {
		glVertexAttribPointer(current_shader->position, 2, GL_FLOAT, GL_FALSE, 0, vertex);
		current_vertex = vertex;
	}
}

void	Renderer::set_vertex(void)
{
	static const
	GLfloat		_vertices[4][XY] =
				{
					{-1.0f, -1.0f},
					{ 1.0f, -1.0f},
					{-1.0f,  1.0f},
					{ 1.0f,  1.0f},
				};

	set_vertex(&_vertices[0][0]);
}

/****************************************
    プリミティブバッファ取得
			引数	_size = 使用サイズ
			戻り値	バッファ
 ****************************************/
void*	Renderer::get_prim_buffer(u32 _size)
{
	assert(prim_buffer);

	if ( prim_p + _size > PRIM_BUF_SIZE ) {
		prim_p = 0;
	}

	void*	_ret = (void*)(prim_buffer + prim_p);

	prim_p += _size;
	return	_ret;
}

}

/**************** End of File ******************************************/
