package sys;

import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.AssetManager;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.WindowManager;
import android.widget.FrameLayout;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;


/********************
    アクティビティ
 ********************/
public class BaseActivity extends FragmentActivity implements Runnable
{
	static {
		System.loadLibrary("native");
    }


	private final static int	NATIVE_PRIORITY = android.os.Process.THREAD_PRIORITY_MORE_FAVORABLE;		// nativeスレッド優先度

	public final static int		KEY_BACK = 1;					// バックキー
	public final static int		KEY_YES  = 2;					// ダイアログ用
	public final static int		KEY_NO   = 3;

	protected final static int	PHASE_RUN      = 0;				// 実行中
	protected final static int	PHASE_INIT     = 1;				// 初期化
	protected final static int	PHASE_CONTINUE = 2;				// 再開
	protected final static int	PHASE_STOP     = 3;				// 中断
	protected final static int	PHASE_FINISH   = 4;				// 終了


	protected FrameLayout	base_layout;						// ベースレイアウト
	protected BaseView		base_view;							// ビュー
	protected int			phase;								// 実行段階
	protected int			screen_width, screen_height;		// 画面の大きさ
	protected final Object	sync_native = new Object();

	private ScheduledExecutorService	executor;				// 定期実行管理
	private ScheduledFuture<?>			future;
	private long			time0, time1;
	protected int			frame_rate;							// フレームレート

	private short[]			touch_status = new short[5*3];		// タッチパネル状態
	protected int			key_status = 0;						// キー入力状態


	public native int		initNative(boolean _init, AssetManager _mgr);			// native部初期化
	public native void		setScreenNative(int _w, int _h);						// native部画面サイズ設定
	public native void		quitNative();											// native部終了
	public native void		pauseNative();											// native部一時停止
	public native boolean	updateNative(boolean draw, short _touch[], int _key);	// native部稼働


	/**********
	    開始
	 **********/
	@Override
	protected void	onCreate(Bundle _savedInstanceState)
	{
		onCreate2(_savedInstanceState, null, null);
	}

	protected void	onCreate2(Bundle _savedInstanceState, FrameLayout _base)
	{
		onCreate2(_savedInstanceState, _base, null);
	}

	protected void	onCreate2(Bundle _savedInstanceState, FrameLayout _base, int[] _attribs)
	{
		super.onCreate(_savedInstanceState);
		phase = (_savedInstanceState == null) ? PHASE_INIT : PHASE_CONTINUE;

		if ( _attribs == null ) {
			_attribs = (new int[]
						{									// デフォルト 画面アトリビュート
							EGL10.EGL_RED_SIZE,		8,
							EGL10.EGL_GREEN_SIZE,	8,
							EGL10.EGL_BLUE_SIZE,	8,
							EGL10.EGL_DEPTH_SIZE,	0,
							EGL10.EGL_STENCIL_SIZE,	0,
							EGL10.EGL_NONE
						});
		}
		base_view = new BaseView((Build.VERSION.SDK_INT < 14), _attribs);		// ベースビュー

		if ( _base == null ) {
			base_layout = base_view;
			setContentView(base_view);
		}
		else {
			base_layout = _base;
			base_layout.addView(base_view, 0);
		}
	}

	/**********
	    終了
	 **********/
	@Override
	public void		finish()
	{
		phase = PHASE_FINISH;
		super.finish();
	}

	@Override
	protected void	onDestroy()
	{
		if ( phase == PHASE_FINISH ) {
			quitNative();							// native部終了
		}
		super.onDestroy();
	}

	/**********
	    再開
	 **********/
	@Override
	protected void	onResume()
	{
		super.onResume();

		executor = Executors.newSingleThreadScheduledExecutor();	// 描画スレッド管理

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);					// スリープ禁止
		for (int i = 0; i < 5; i++) {				// タッチパネル状態クリア
			touch_status[i*3] = 0;
		}
		if ( ((KeyguardManager)getSystemService(Context.KEYGUARD_SERVICE)).inKeyguardRestrictedInputMode() ) {
			receiver = new UnLockReceiver();
			registerReceiver(receiver, new IntentFilter(Intent.ACTION_USER_PRESENT));			// スクリーンロック解除待ち
		}
		else {
			start();
		}
	}

	private UnLockReceiver	receiver = null;						// スクリーンロック解除検知用

	private class UnLockReceiver extends BroadcastReceiver
	{
		@Override
		public void		onReceive(Context context, Intent intent)
		{
			unregisterReceiver(receiver);							// レシーバー登録を解除
			receiver = null;
			start();
		}
	}

	/**************
	    一時停止
	 **************/
	@Override synchronized
	protected void	onPause()
	{
		super.onPause();
		if ( phase == PHASE_RUN ) {
			phase = PHASE_STOP;
		}
		if ( future != null ) {						// 定期実行停止
			future.cancel(false);
			future = null;
		}
		try {
			executor.submit(new Runnable()
			{
				@Override
				public void		run()
				{
					base_view.quitGL();				// OpenGL終了
				}
			}).get();
		}
		catch (InterruptedException | ExecutionException e) {}
		executor.shutdown();
		executor = null;
		if ( phase >= PHASE_STOP ) {
			pauseNative();							// native部一時停止
		}
		if ( receiver != null ) {
			unregisterReceiver(receiver);			// レシーバー登録を解除
			receiver = null;
		}
		getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);		// スリープ禁止解除
	}

	/********************************
	    開始
			戻り値	フレームレート
	 ********************************/
	synchronized
	public void		start()
	{
		if ( (executor == null) || (base_view.native_window == null) ) {
			return;
		}

		try {
			executor.submit(new Runnable()
			{
				@Override
				public void		run()
				{
					android.os.Process.setThreadPriority(NATIVE_PRIORITY);
					base_view.initGL()	;															// OpenGL初期化
					synchronized (sync_native) {
						frame_rate = initNative((phase == PHASE_INIT), getAssets());				// native部初期化
					}
					phase = PHASE_RUN;

					time0 = System.currentTimeMillis();
					time1 = 0;
				}
			}).get();
		}
		catch (InterruptedException | ExecutionException e) {}

		future = executor.scheduleAtFixedRate(this, 0, 1000/frame_rate, TimeUnit.MILLISECONDS);		// 定期実行開始
	}

	/*********
	    稼働
	 **********/
	@Override
	public void		run()
	{
		if ( phase == PHASE_RUN ) {
			long	_t = System.currentTimeMillis();
			int		_loop;

			time1 += (_t - time0)*frame_rate;
			if ( time1 > 4*1000 - 1 ) {
				time1 = 4*1000 - 1;
			}
			time0 = _t;
			_loop = (int)time1/1000;
			if ( _loop > 0 ) {
				time1 -= _loop*1000;

				base_view.swap();
				synchronized (sync_native) {
					for (; (_loop > 0) && (phase == PHASE_RUN); _loop--) {
						int		_key = key_status;

						key_status = 0;
						if ( !updateNative((_loop == 1), touch_status, _key) ) {	// native部稼働
							finish();
							break;
						}
					}
				}
			}
		}
	}

	/********************
	    画面サイズ設定
	 ********************/
	public void		set_screen(int _width, int _height)
	{
		screen_width  = _width;
		screen_height = _height;
		synchronized (sync_native) {
			setScreenNative(_width, _height);
		}
	}


	/********************
		タッチイベント
	 ********************/
	public boolean	_onTouchEvent(final MotionEvent event)
	{
		if ( phase != PHASE_RUN ) {
			return	false;
		}

		int		_action = event.getAction();
		int		_index, _id;

		switch ( _action & MotionEvent.ACTION_MASK ) {
		  case MotionEvent.ACTION_DOWN :
		  case MotionEvent.ACTION_POINTER_DOWN :
			_index	= (_action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
			_id		= event.getPointerId(_index)*3;
			if ( _id < 5*3 ) {
				touch_status[_id + 1] = (short)event.getX(_index);		// X座標
				touch_status[_id + 2] = (short)event.getY(_index);		// Y座標
				touch_status[_id + 0] = 1;								// タッチ中
			}
			break;

		  case MotionEvent.ACTION_MOVE :
			for (_index = 0; _index < event.getPointerCount(); _index++) {
				_id = event.getPointerId(_index)*3;
				if ( _id < 5*3 ) {
					touch_status[_id + 1] = (short)event.getX(_index);	// X座標
					touch_status[_id + 2] = (short)event.getY(_index);	// Y座標
					touch_status[_id + 0] = 1;							// タッチ中
				}
			}
			break;

		  case MotionEvent.ACTION_UP :
		  case MotionEvent.ACTION_POINTER_UP :
			_index	= (_action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
			_id		= event.getPointerId(_index)*3;
			if ( _id < 5*3 ) {
				touch_status[_id + 0] = 0;								// 非タッチ
			}
			break;
		}
		return	true;
	}

	/********************
	    バックキー入力
	 ********************/
	@Override
	public void		onBackPressed()
	{
		key_status = KEY_BACK;
	}


	/**********************
	    グラフィック取得
	 **********************/
	public EGL10	getEgl()
	{
		return	base_view.mEgl;
	}

	public EGLDisplay	getEglDisplay()
	{
		return	base_view.mEglDisplay;
	}

	public EGLContext	getEglContext()
	{
		return	base_view.mEglContext;
	}

	public EGLSurface	getEglSurface()
	{
		return	base_view.mEglSurface;
	}


	/************
	    ビュー
	 ************/
	class BaseView extends FrameLayout
	{
		public EGL10		mEgl;
 		public EGLDisplay	mEglDisplay;
 		public EGLContext	mEglContext;
		public EGLSurface	mEglSurface;
		public Object		native_window;
		private int[]		config_attribs;

		private static final int	EGL_OPENGL_ES2_BIT = 4;
		private static final int	EGL_CONTEXT_CLIENT_VERSION = 0x3098;


		/************************************************
		    コンストラクタ
				引数	_kind    = true ：SurfaceView
						           false：TextureView
						_attribs = 画面アトリビュート
		 ************************************************/
		public BaseView(boolean _kind, int[] _attribs)
		{
			super(getApplication());
			config_attribs = _attribs;

			if ( _kind ) {					// SurfaceView
				SurfaceView		_view = new SurfaceView(getApplication());

				_view.getHolder().addCallback(new SurfaceHolder.Callback()
				{
					@Override
				    public void surfaceCreated(SurfaceHolder holder) {}

					@Override synchronized
				    public void surfaceDestroyed(SurfaceHolder holder)
					{
						native_window = null;
					}

					@Override
				    public void surfaceChanged(SurfaceHolder _holder, int format, int _width, int _height)
					{
						set_screen(_width, _height);
						set_surface(_holder);
					}
				});
				addView(_view);
			}
			else {							// TextureView
				TextureView		_view = new TextureView(getApplication());

				_view.setSurfaceTextureListener(new TextureView.SurfaceTextureListener()
				{
					@Override
					public void		onSurfaceTextureAvailable(SurfaceTexture _surface, int _width, int _height)
					{
						set_screen(_width, _height);
						set_surface(_surface);
					}

					@Override
					public void		onSurfaceTextureSizeChanged(SurfaceTexture surface, int _width, int _height)
					{
						set_screen(_width, _height);
					}

					@Override synchronized
					public boolean	onSurfaceTextureDestroyed(SurfaceTexture surface)
					{
						native_window = null;
						return	true;
					}

					@Override
					public void		onSurfaceTextureUpdated(SurfaceTexture surface) {}
				});
				addView(_view);
			}
		}

		/**********
		    開始
		 **********/
		synchronized
		private void	set_surface(Object _window)
		{
			if ( native_window == null ) {
				native_window = _window;
				start();
			}
		}

		/**********
		    稼働
		 **********/
		private void	swap()
		{
			if ( !mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) ) {
				throw new RuntimeException("eglMakeCurrent failed");
			}
			mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
		}

		/********************
			タッチイベント
		 ********************/
		@Override
		public boolean	onTouchEvent(final MotionEvent event)
		{
			return	_onTouchEvent(event);
		}


		/******************
		    OpenGL初期化
		 ******************/
		private void	initGL()
		{
			mEgl = (EGL10)EGLContext.getEGL();
			mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
			if ( mEglDisplay == EGL10.EGL_NO_DISPLAY ) {
				throw new RuntimeException("eglGetDisplay failed");
			}
			if ( !mEgl.eglInitialize(mEglDisplay, new int[2]) ) {
				throw new RuntimeException("eglInitialize failed");
			}
			if ( (search_config(config_attribs) == null) && (search_config(new int[] {EGL10.EGL_NONE}) == null) ) {
				throw new RuntimeException("eglCreateWindowSurface failed");
			}

			mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
		}

		private EGLConfig	search_config(final int[] attribs)
		{
			int[]	num_config = new int[1];

			mEgl.eglChooseConfig(mEglDisplay, attribs, null, 0, num_config);
			if ( num_config[0] <= 0 ) {
				return	null;
			}

			EGLConfig[]		configs = new EGLConfig[num_config[0]];

			mEgl.eglChooseConfig(mEglDisplay, attribs, configs, num_config[0], num_config);

			final int[]		attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2,	EGL10.EGL_NONE};

			for (EGLConfig config : configs) {
				mEglContext = mEgl.eglCreateContext(mEglDisplay, config, EGL10.EGL_NO_CONTEXT, attrib_list);	// Context作成
				if ( (mEglContext != null) && (mEglContext != EGL10.EGL_NO_CONTEXT) ) {
					mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, config, native_window, null);		// Surface作成
					if ( (mEglSurface != null) && (mEglSurface != EGL10.EGL_NO_SURFACE) ) {
						return	config;
					}
					mEgl.eglDestroyContext(mEglDisplay, mEglContext);
				}
			}
			return	null;
		}

		/****************
		    OpenGL終了
		 ****************/
		private void	quitGL()
		{
			if ( !mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) ) {
				throw new RuntimeException("eglMakeCurrent failed");
			}
			GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
			mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);

			if ( mEglContext != EGL10.EGL_NO_CONTEXT ) {
				mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
				mEgl.eglDestroyContext(mEglDisplay, mEglContext);
				mEglContext	= EGL10.EGL_NO_CONTEXT;
			}
			if ( mEglSurface != EGL10.EGL_NO_SURFACE ) {
				mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
				mEglSurface = EGL10.EGL_NO_SURFACE;
			}
			if ( mEglDisplay != EGL10.EGL_NO_DISPLAY ) {
				mEgl.eglTerminate(mEglDisplay);
				mEglDisplay = EGL10.EGL_NO_DISPLAY;
			}
		}
	}
}

/***************** End of File ***************************************************/