package sys;

import android.app.KeyguardManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.widget.FrameLayout;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import sys.SoundManager;


/********************
    アクティビティ
 ********************/
public class BaseActivity extends FragmentActivity
{
	static {
		System.loadLibrary("native");
    }


	private final static int	NATIVE_PRIORITY	= 7;			// nativeスレッド優先度

	public final static int		KEY_BACK	= 1;				// バックキー
	public final static int		KEY_YES		= 2;				// ダイアログ用
	public final static int		KEY_NO		= 3;


	protected final Object	obj_sync = new Object();
	protected FrameLayout	base_layout;						// ベースレイアウト
	private BaseView		surface_view;						// ビュー
	private int				phase;								// 実行段階
	protected int			screen_width;						// 画面の大きさ
	protected int			screen_height;

	private ScheduledExecutorService	executor;				// 定期実行管理
	private ScheduledFuture<?>			future;

	private short[]			touch_status = new short[5*3];		// タッチパネル状態
	protected int			key_status;							// キー入力状態


	public native int		initNative(boolean _init, int _w, int _h, AssetManager _mgr);	// native部初期化
	public native void		quitNative();													// native部終了
	public native void		pauseNative();													// native部一時停止
	public native boolean	updateNative(short _touch[], int _key);							// native部稼働


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

		if ( _savedInstanceState == null ) {
			phase = 0;
			SoundManager.init();										// サウンド管理初期化
		}
		else {
			phase = 1;
		}

		if ( _base == null ) {
			base_layout = new FrameLayout(this);						// ベースレイアウト
			setContentView(base_layout);
		}
		else {
			base_layout	= _base;
		}

		executor = Executors.newSingleThreadScheduledExecutor(new ThreadFactory()		// 定期実行管理
		{
			@Override
			public Thread	newThread(Runnable r)
			{
				Thread	thread = new Thread(r);

				thread.setPriority(NATIVE_PRIORITY);
				return	thread;
			}
		});

		// デフォルト 画面アトリビュート
		final int[]		configAttribs =
		{
			EGL10.EGL_RED_SIZE,		8,
			EGL10.EGL_GREEN_SIZE,	8,
			EGL10.EGL_BLUE_SIZE,	8,
			EGL10.EGL_DEPTH_SIZE,	0,
			EGL10.EGL_STENCIL_SIZE,	0,
			EGL10.EGL_NONE
		};

		surface_view = new BaseView(new BaseRenderer(), (_attribs != null) ? _attribs : configAttribs);		// ビュー作成
		base_layout.addView(surface_view, 0);
	}

	/**********
	    終了
	 **********/
	@Override
	public void		finish()
	{
		phase = -1;
		super.finish();
	}

	@Override
	protected void	onDestroy()
	{
		if ( phase < 0 ) {
			quitNative();							// native部終了
			SoundManager.quit();					// サウンド管理終了
		}
		if ( executor != null ) {
			executor.shutdown();
			executor = null;
		}
		super.onDestroy();
	}

	/**************
	    一時停止
	 **************/
	@Override
	protected void	onPause()
	{
		super.onPause();
		if ( future != null ) {						// 定期実行停止
			future.cancel(false);
			future = null;
		}
		if ( receiver != null ) {
			unregisterReceiver(receiver);			// レシーバー登録を解除
			receiver = null;
		}
		synchronized (obj_sync)
		{
			surface_view.onPause();
			pauseNative();							// native部一時停止
		}
		getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);		// スリープ禁止解除
	}

	/**********
	    再開
	 **********/
	@Override
	protected void	onResume()
	{
		super.onResume();

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);		// スリープ禁止
		for (int i = 0; i < 5; i++) {				// タッチパネル状態クリア
			touch_status[i*3] = 0;
		}
		if ( ((KeyguardManager)getSystemService(Context.KEYGUARD_SERVICE)).inKeyguardRestrictedInputMode() ) {
			receiver = new UnLockReceiver();
			registerReceiver(receiver, new IntentFilter(Intent.ACTION_USER_PRESENT));			// スクリーンロック解除待ち
		}
		else {
			surface_view.onResume();
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
			surface_view.onResume();
		}
	}

	/**********
	    稼働
	 **********/
	protected void	update()
	{
		surface_view.requestRender();				// 描画リクエスト
	}


	/********************
		タッチイベント
	 ********************/
	public boolean	_onTouchEvent(final MotionEvent event)
	{
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
		return true;
	}

	/********************
	    バックキー入力
	 ********************/
	@Override
	public void		onBackPressed()
	{
		key_status = KEY_BACK;
	}


	/************
	    ビュー
	 ************/
	class BaseView extends GLSurfaceView
	{
		public BaseView(BaseRenderer _renderer, int[] _attribs)
		{
			super(getApplication());
			setEGLContextClientVersion(2);					// OpenGL ES 2.0使用
			setEGLConfigChooser(new ConfigChooser(_attribs));
			setRenderer(_renderer);
			setRenderMode(RENDERMODE_WHEN_DIRTY);
		}

		class ConfigChooser implements GLSurfaceView.EGLConfigChooser
		{
			private int[]	attribs;

			public ConfigChooser(int[] _attribs)
			{
				attribs = _attribs;
			}

			@Override
			public EGLConfig	chooseConfig(EGL10 egl, EGLDisplay display)
			{
				EGLConfig	config;

				config = search(egl, display, attribs);
				if ( config != null ) {
					return	config;
				}
				return	search(egl, display, new int[] {EGL10.EGL_NONE});
			}

			private EGLConfig	search(EGL10 egl, EGLDisplay display, final int[] attribs)
			{
				int[]	num_config = new int[1];

				egl.eglChooseConfig(display, attribs, null, 0, num_config);
				if ( num_config[0] <= 0 ) {
					return	null;
				}

				EGLConfig[]		configs = new EGLConfig[num_config[0]];

				egl.eglChooseConfig(display, attribs, configs, num_config[0], num_config);


				final int		EGL_CONTEXT_CLIENT_VERSION = 0x3098;
				final int[]		attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2,	EGL10.EGL_NONE};

				EGLContext	context;
				EGLSurface	surface;

				for (EGLConfig config : configs) {
					context = egl.eglCreateContext(display, config, EGL10.EGL_NO_CONTEXT, attrib_list);		// Context作成チェック
					if ( (context != null) && (context != EGL10.EGL_NO_CONTEXT) ) {
						egl.eglDestroyContext(display, context);

						surface = egl.eglCreateWindowSurface(display, config, getHolder(), null);			// Surface作成チェック
						if ( (surface != null) && (surface != EGL10.EGL_NO_SURFACE) ) {
							egl.eglDestroySurface(display, surface);
							return	config;
						}
					}
				}
				return	null;
			}
		}

		/********************
			タッチイベント
		 ********************/
		@Override
		public boolean	onTouchEvent(final MotionEvent event)
		{
			if ( future == null ) {
				return	false;
			}
			return	_onTouchEvent(event);
		}
	}


	/****************
	    レンダラー
	 ****************/
	class BaseRenderer implements GLSurfaceView.Renderer
	{
		public void	onSurfaceCreated(GL10 _gl, EGLConfig _config) {}

		public void	onSurfaceChanged(GL10 gl, int width, int height)
		{
			screen_width  = width;
			screen_height = height;
			synchronized (obj_sync)
			{
				if ( future == null ) {
					int		period = initNative((phase == 0), width, height, getAssets());		// native部初期化

					phase      = 1;
					key_status = 0;
					future = executor.scheduleAtFixedRate(new Runnable()	// 定期実行開始
					{
						@Override
						public void	run()
						{
							update();
						}
					},
					0, period, TimeUnit.MILLISECONDS);
				}
			}
		}

		public void	onDrawFrame(GL10 gl)
		{
			synchronized (obj_sync)
			{
				if ( (future != null) && (phase > 0) ) {
					int		_key = key_status;

					key_status = 0;
					if ( !updateNative(touch_status, _key) ) {				// native部稼働
						finish();
					}
				}
			}
		}
	}
}

/***************** End of File ***************************************************/