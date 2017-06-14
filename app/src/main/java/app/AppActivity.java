package app;

import sys.BaseActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;


/********************
    アクティビティ
 ********************/
public class AppActivity extends BaseActivity
{
	static public AppActivity	app;


	/**********
	    開始
	 **********/
	@Override
	protected void	onCreate(Bundle _savedInstanceState)
	{
		super.onCreate(_savedInstanceState);
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


	/**************
	    終了確認
	 **************/
	static
	public void		open_dialog()
	{
		ExitDialogFragment	_dlg = new ExitDialogFragment();			// 終了確認ダイアログ

		_dlg.show(app.getSupportFragmentManager(), "dialog");
	}

	/********************
	    終了ダイアログ
	 ********************/
	public static class ExitDialogFragment extends DialogFragment
	{
		@Override
		public Dialog	onCreateDialog(Bundle savedInstanceState)
		{
			return	app.onCreateDialog();
		}

		@Override
		public void		onDismiss(DialogInterface dialog)
		{
			super.onDismiss(dialog);

			if ( app.key_status == 0 ) {
				app.key_status = KEY_NO;
			}
		}
	}

	public Dialog	onCreateDialog()
	{
		AlertDialog.Builder		builder = new AlertDialog.Builder(this);

		builder.setMessage("アプリを終了しますか");
		builder.setPositiveButton("終了",
			new DialogInterface.OnClickListener()
			{
				@Override
				public void		onClick(DialogInterface dialog, int which)
				{
					key_status = KEY_YES;
				}
			});
		builder.setNegativeButton("キャンセル",
			new DialogInterface.OnClickListener()
			{
				@Override
				public void		onClick(DialogInterface dialog, int which)
				{
					key_status = KEY_NO;
				}
			});

		return	builder.create();
	}
}

/***************** End of File ***************************************************/