/***************************

		描画管理

 ***************************/

#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Sprite.h"
#include "swappy/swappyGL.h"


namespace sys
{

EGLDisplay	Renderer::mEglDisplay;
EGLSurface	Renderer::mEglSurface;
EGLContext	Renderer::mEglContext;
EGLConfig	Renderer::mEglConfig;
EGLint const*	Renderer::egl_attribs;

int		Renderer::screen_width;					// 画面解像度
int		Renderer::screen_height;
int		Renderer::limit_width;					// 表示画面解像度
int		Renderer::limit_height;
Rect<float>		Renderer::screen_rect;			// 実画面
FrameBuffer*	Renderer::frame_buffer;			// フレームバッファ
ShaderProgram*	Renderer::shader;				// シェーダプログラム
GLfloat const*	Renderer::mat_projection;		// 透視変換行列

Renderer::Current	Renderer::current;			// カレント


/************
    初期化
 ************/
void	Renderer::create(const AppParam& _param)
{
	static const
	EGLint	attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_STENCIL_SIZE, 0,
		EGL_NONE
	};

	mEglDisplay	= EGL_NO_DISPLAY;
	mEglSurface	= EGL_NO_SURFACE;
	mEglContext	= EGL_NO_CONTEXT;
	mEglConfig	= 0;
	egl_attribs	= _param.egl_attribs ? _param.egl_attribs : attribs;

	screen_width	= _param.screen_width;
	screen_height	= _param.screen_height;
	limit_width		= _param.limit_width;
	limit_height	= _param.limit_height;

	shader			= nullptr;
	frame_buffer	= nullptr;

	CTexture::init_cache(_param.texture_cache);						// テクスチャキャッシュ初期化
}

void	Renderer::resume(ANativeWindow* _window)
{
	init_OpenGL(_window);											// OpenGL初期化

	if ( shader == nullptr ) {
		shader = new ShaderProgram[SHADER_MAX];
		create_shader();											// シェーダ初期化
	}
	frame_buffer = new FrameBuffer(limit_width, limit_height);		// フレームバッファ作成
}

/********************
    シェーダ初期化
 ********************/
void	Renderer::create_shader(void)
{
	{						// SHADER_PLAIN（テクスチャ無し）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"uniform mat4 projection;"
					"attribute vec4 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
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

		shader[SHADER_PLAIN].create(gVertexShader, gFragmentShader);
	}

	{						// SHADER_TEXTURE（テクスチャ有り）
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"uniform mat4 projection;"
					"attribute vec4 position;"
					"attribute vec4 color;"
					"varying vec4 vColor;"
					"attribute vec2 texcoord;"
					"varying vec2 vTexcoord;"
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

		shader[SHADER_TEXTURE].create(gVertexShader, gFragmentShader);
	}

	{						// SHADER_SIMPLE
		static const
		char	gVertexShader[] = 					// 頂点シェーダプログラム
					"attribute vec4 position;"
					"attribute vec2 texcoord;"
					"varying vec2 vTexcoord;"
					"void main() {"
						"gl_Position = position;"
						"vTexcoord = texcoord;"
					"}";

		static const
		char	gFragmentShader[] = 				// フラグメントシェーダプログラム
					"precision mediump float;"
					"varying vec2 vTexcoord;"
					"uniform sampler2D texture;"
					"void main() {"
						"gl_FragColor = texture2D(texture, vTexcoord);"
					"}";

		shader[SHADER_SIMPLE].create(gVertexShader, gFragmentShader);
	}
}


/**********
    終了
 **********/
void	Renderer::pause(void)
{
	CTexture::clear_cache();						// テクスチャキャッシュクリア
	if ( frame_buffer ) {
		delete	frame_buffer;						// フレームバッファ削除
		frame_buffer = nullptr;
	}
}

void	Renderer::release(void)
{
	delete[]	shader;								// シェーダ削除
	quit_OpenGL();									// OpenGL終了
}


/******************
    OpenGL初期化
 ******************/
void	Renderer::init_OpenGL(ANativeWindow* _window)
{
	if ( (mEglDisplay != EGL_NO_DISPLAY) && (mEglSurface != EGL_NO_SURFACE) && (mEglContext != EGL_NO_CONTEXT) ) {
		return;										// 初期化済み
	}

	if ( mEglDisplay == EGL_NO_DISPLAY ) {
		LOGI("Renderer: initializing display.");
		mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if ( !eglInitialize(mEglDisplay, 0, 0) ) {
			LOGE("Renderer: failed to init display, error %d", eglGetError());
			return;
		}
	}
	if ( mEglContext == EGL_NO_CONTEXT ) {
		LOGI("Renderer: initializing context.");

		EGLint	numConfigs;

		eglChooseConfig(mEglDisplay, egl_attribs, nullptr, 0, &numConfigs);

		static const
		EGLint	attribList[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

		EGLConfig	_configs[numConfigs];

		eglChooseConfig(mEglDisplay, egl_attribs, _configs, numConfigs, &numConfigs);
		for (int i = 0; i < numConfigs; i++) {
			mEglConfig = _configs[i];
			mEglContext = eglCreateContext(mEglDisplay, mEglConfig, nullptr, attribList);
			if ( mEglContext != EGL_NO_CONTEXT ) {
				mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, _window, nullptr);
				if ( mEglSurface != EGL_NO_SURFACE ) {
					LOGI("Renderer: successfully initialized context.");
					break;
				}
				else {
					eglDestroyContext(mEglDisplay, mEglContext);
					mEglContext = EGL_NO_CONTEXT;
				}
			}
		}
		if ( mEglContext == EGL_NO_CONTEXT ) {
			LOGE("Failed to create EGL context, EGL error %d", eglGetError());
			return;
		}
	}
	if ( mEglSurface == EGL_NO_SURFACE ) {
		LOGI("Renderer: initializing surface.");
		mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, _window, nullptr);
		if ( mEglSurface == EGL_NO_SURFACE ) {
			LOGE("Failed to create EGL surface, EGL error %d", eglGetError());
			return;
		}
		LOGI("Renderer: successfully initialized surface.");
	}

	if ( !eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) ) {
		LOGE("Renderer: eglMakeCurrent failed, EGL error %d", eglGetError());
		return;
	}
}

/********************
    サーフェス削除
 ********************/
void	Renderer::kill_surface(void)
{
	LOGI("Renderer: killing surface.");
	eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if ( mEglSurface != EGL_NO_SURFACE ) {
		eglDestroySurface(mEglDisplay, mEglSurface);
		mEglSurface = EGL_NO_SURFACE;
	}
	LOGI("Renderer: Surface killed successfully.");
}

/****************
    OpenGL終了
 ****************/
void	Renderer::quit_OpenGL(void)
{
	kill_surface();

	LOGI("Renderer: killing context.");
	if ( mEglContext != EGL_NO_CONTEXT ) {
		eglDestroyContext(mEglDisplay, mEglContext);
		mEglContext = EGL_NO_CONTEXT;
	}
	LOGI("Renderer: Context killed successfully.");

	LOGI("Renderer: killing display.");
	if ( mEglDisplay != EGL_NO_DISPLAY ) {
		LOGI("Renderer: terminating display now.");
		eglTerminate(mEglDisplay);
		mEglDisplay = EGL_NO_DISPLAY;
	}
	LOGI("Renderer: display killed successfully.");
}


/****************
    描画前処理
 ****************/
void	Renderer::prepare(ANativeWindow* _window)
{
	init_OpenGL(_window);

	frame_buffer->bind();							// フレームバッファ使用
	current.init();									// カレント初期化
	Sprite::set_color();							// スプライト初期化

	glEnable(GL_BLEND);								// αブレンド初期化
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
}

/****************
    描画後処理
 ****************/
void	Renderer::update(void)
{
	glFlush();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);


	// フレームバッファテクスチャ描画
	EGLint	width, height;
	GLsizei	vw, vh;

	eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width);
	eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height);
	screen_rect.x = (float)width/2;
	screen_rect.y = (float)height/2;
	if ( width*screen_height < height*screen_width ) {				// 横長（上下カット）
		screen_rect.w = (float)width;
		screen_rect.h = (float)width*screen_height/screen_width;
		vw = width*limit_width/screen_width;
		vh = width*limit_height/screen_width;
	}
	else {															// 縦長（左右カット）
		screen_rect.w = (float)height*screen_width/screen_height;
		screen_rect.h = (float)height;
		vw = height*limit_width/screen_height;
		vh = height*limit_height/screen_height;
	}
	glViewport((width - vw)/2, (height - vh)/2, vw, vh);

	use_shader(SHADER_SIMPLE);
	frame_buffer->Texture::bind();
	set_vertex();
	set_texcoord();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


	if ( !SwappyGL_swap(mEglDisplay, mEglSurface) ) {
		LOGE("Renderer: SwappyGL_swap failed, EGL error %d", eglGetError());
	}
}


/*************************************
    透視変換行列設定
		引数	_mat = 透視変換行列
 *************************************/
void	Renderer::set_projection(GLfloat const* _mat)
{
	mat_projection = _mat;
	if ( current.shader && (current.shader->projection >= 0) ) {
		glUniformMatrix4fv(current.shader->projection, 1, GL_FALSE, _mat);
	}
}

/*********************************
    シェーダ使用
		引数	_sd = シェーダ
		戻り値	シェーダ
 *********************************/
ShaderProgram*	Renderer::use_shader(ShaderProgram* _sd)
{
	if ( current.shader != _sd ) {					// シェーダ切り替え
		if ( current.shader ) {
			current.shader->unuse();
		}
		current.shader = _sd;
		_sd->use(mat_projection);
		current.vertex = nullptr;
		current.texcoord = nullptr;
		current.color = nullptr;
	}
	return	_sd;
}

/*************************************************
    テクスチャ使用
		引数	target = テクスチャ種類
				tex    = テクスチャオブジェクト
 *************************************************/
void	Renderer::bind_texture(GLenum target, GLuint tex)
{
	if ( tex != current.texture ) {
		glBindTexture(target, tex);
		current.texture = tex;
	}
}

/********************************
    カラー設定
		引数	color = カラー
 ********************************/
void	Renderer::set_color(GLubyte const* color)
{
	if ( color != current.color ) {
		glVertexAttribPointer(current.shader->color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, color);
		current.color = color;
	}
}

void	Renderer::set_color(GLfloat const* color)
{
	if ( color != (GLfloat*)current.color ) {
		glVertexAttribPointer(current.shader->color, 4, GL_FLOAT, GL_FALSE, 0, color);
		current.color = (GLubyte*)color;
	}
}

void	Renderer::set_color(void)
{
	static const
	uint32_t	_color[4] =
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
	if ( coord != current.texcoord ) {
		glVertexAttribPointer(current.shader->texcoord, 2, GL_FLOAT, GL_FALSE, 0, coord);
		current.texcoord = coord;
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
	if ( vertex != current.vertex ) {
		glVertexAttribPointer(current.shader->position, 2, GL_FLOAT, GL_FALSE, 0, vertex);
		current.vertex = vertex;
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

}
