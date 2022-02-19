#ifndef	___SYS_TOUCH_PANEL_H___
#define	___SYS_TOUCH_PANEL_H___

#include "common.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"


namespace sys
{

/**********************
    タッチパネル管理
 **********************/
class TouchManager
{
	static int	touch_max;				// マルチタッチ数

public :

	static void		create(int);						// 作成
	static void		resume(android_input_buffer*);		// 初期化
	static void		pause(void) {}						// 停止
	static void		release(void);						// 終了
	static void		update(android_input_buffer*);		// 稼働


enum
{
	TOUCH	= (1 << 0),			// タッチ
	TRIGGER	= (1 << 1),			// 初回タッチ
	REPEAT	= (1 << 2),			// リピート
	RELEASE	= (1 << 3),			// リリース
	RAW		= (1 << 7),			// 生データ
};

	uint32_t	flag;					// タッチ状態フラグ
	float		x, y;					// タッチ位置
	float		move_x, move_y;			// 移動距離
	float		prev_x, prev_y;			// 前回のタッチ位置
	int			repeat_cnt;				// リピート用カウンタ

	void	update(void);				// 稼働

	bool	is_touch(void)
			{
				return	(bool)(flag & TOUCH);
			}
	bool	is_touch(int _sx, int _sy, int _ex, int _ey)
			{
				return	((flag & TOUCH) && (x >= (float)_sx) && (x < (float)_ex) && (y >= (float)_sy) && (y < (float)_ey));
			}
	bool	is_trigger(void)
			{
				return	(bool)(flag & TRIGGER);
			}
	bool	is_trigger(int _sx, int _sy, int _ex, int _ey)
			{
				return	((flag & TRIGGER) && (x >= (float)_sx) && (x < (float)_ex) && (y >= (float)_sy) && (y < (float)_ey));
			}
	bool	is_repeat(void)
			{
				return	(bool)(flag & REPEAT);
			}
	bool	is_repeat(int _sx, int _sy, int _ex, int _ey)
			{
				return	((flag & REPEAT) && (x >= (float)_sx) && (x < (float)_ex) && (y >= (float)_sy) && (y < (float)_ey));
			}
	bool	is_release(void)
			{
				return	(bool)(flag & RELEASE);
			}
};

extern TouchManager*	TouchPanel;

}
#endif
