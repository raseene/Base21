package sys;

import java.util.concurrent.ArrayBlockingQueue;
import android.util.Log;


/******************
    サウンド管理
 ******************/
public class SoundManager implements Runnable
{
	static private SoundManager		manager;							// サウンド管理タスク
	static private Thread			thread;								// スレッド
	static private ArrayBlockingQueue<SoundCommand>		queue;			// コマンドキュー


	/************
	    初期化
	 ************/
	static
	public void		init()
	{
		queue	= new ArrayBlockingQueue<SoundCommand>(32, true);		// キュー作成
		manager	= new SoundManager();
		thread	= new Thread(manager);
//		thread.setPriority(9);
		thread.start();
	}

	/**********
	    終了
	 **********/
	static
	public void		quit()
	{
		if ( thread != null ) {
			stop_command();
		}
		manager	= null;
	}

	/******************
	    コマンド予約
	 ******************/
	static
	public void		set_command(short _channel, short _command, int _data, int _size, short _loop, float _volume)
	{
		try {
			queue.put(new SoundCommand(_channel, _command, _data, _size, _loop, _volume));
		}
		catch (InterruptedException e) {}
	}

	/******************
		コマンド停止
	 ******************/
	static
	public void		stop_command()
	{
		if ( thread != null ) {
			queue.clear();
			set_command((short)-1, (short)-1, 0, 0, (short)0, 0.0f);
			try {
				thread.join();
			}
			catch (InterruptedException e) {}
			queue.clear();
			queue	= null;
			thread	= null;
		}
	}


	public native void	sendSoundCommand(short _channel, short _command, int _data, int _size, short _loop, float _volume);

	/**********
	    稼働
	 **********/
	public void		run()
	{
		SoundCommand	_command;

		while ( true ) {
			try {
				_command = queue.take();					// コマンド取得
				if ( _command != null ) {					// コマンド実行
					if ( _command.command < 0 ) {
						return;
					}
					sendSoundCommand(_command.channel, _command.command, _command.data, _command.size, _command.loop, _command.volume);
				}
			}
			catch (InterruptedException e) {
				queue.clear();
			}
		}
	}
}


/**********
    命令
 **********/
class SoundCommand
{
	public short	channel;		// チャンネル
	public short	command;		// コマンド
	public int		data;			// データ
	public int		size;			// サイズ
	public short	loop;			// ループ回数
	public float	volume;			// 音量

	/********************
	    コンストラクタ
	 ********************/
	public SoundCommand(short _channel, short _command, int _data, int _size, short _loop, float _volume)
	{
		channel	= _channel;
		command	= _command;
		data	= _data;
		size	= _size;
		loop	= _loop;
		volume	= _volume;
	}
}

/******************** End of File *******************************************************/