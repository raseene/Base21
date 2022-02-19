package com.example.base;

import com.google.androidgamesdk.GameActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.core.view.WindowCompat;

import android.util.Log;


/********************
    アクティビティ
 ********************/
public class AppActivity extends GameActivity
{
	static {
		System.loadLibrary("native");
	}

	static public AppActivity	app;


	/**********
	    開始
	 **********/
	@Override
	protected void	onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		WindowCompat.setDecorFitsSystemWindows(getWindow(), true);
		app = this;
	}

	/**********
	    終了
	 **********/
	@Override
	protected void	onDestroy()
	{
		app = null;
		super.onDestroy();
	}


	public final static int		KEY_YES		= (1 << 0);			// ダイアログ用
	public final static int		KEY_NO		= (1 << 1);
	public final static int		KEY_CANCEL	= (1 << 2);

	public native void	clickKey(int _key);						// ダイアログキー結果

	/**************
	    終了確認
	 **************/
	static
	public void		open_dialog()
	{
		(new ExitDialogFragment()).show(app.getFragmentManager(), "exit");			// 終了確認ダイアログ
	}

	/********************
	    終了ダイアログ
	 ********************/
	public static class ExitDialogFragment extends DialogFragment
	{
		@Override
		public Dialog	onCreateDialog(Bundle savedInstanceState)
		{
			return	(new AlertDialog.Builder(app))
						.setMessage("アプリを終了しますか")
						.setPositiveButton("終了",
							new DialogInterface.OnClickListener()
							{
								@Override
								public void		onClick(DialogInterface dialog, int which)
								{
									app.clickKey(KEY_YES);
								}
							})
						.setNegativeButton("キャンセル",
							new DialogInterface.OnClickListener()
							{
								@Override
								public void		onClick(DialogInterface dialog, int which)
								{
									app.clickKey(KEY_NO);
								}
							})
						.create();
		}

		@Override
		public void		onDismiss(DialogInterface dialog)
		{
			super.onDismiss(dialog);
			app.clickKey(KEY_CANCEL);
		}
	}
}
