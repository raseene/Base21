/********************************

		タッチパネル

 ********************************/

#ifndef	___TOUCH_PANEL_H___
#define	___TOUCH_PANEL_H___

#include "common.h"


namespace sys
{

/**********************
    タッチパネル管理
 **********************/
class TouchManager
{
public :

	static void		init_manager(void);					// 管理システム初期化
	static void		quit_manager(void);					// 管理システム終了
	static void		update_manager(short const*);		// 管理システム稼働

private :

	int		repeat_cnt;			// リピート用カウンタ
	int		prev_x, prev_y;		// 前回のタッチ位置

public :

static const int	TOUCH_MAX = 5;						// マルチタッチ数

enum
{
	TOUCH	= (1 << 0),			// タッチ
	TRIGGER	= (1 << 1),			// 初回タッチ
	REPEAT	= (1 << 2),			// リピート
	RELEASE	= (1 << 3),			// リリース
};

	u32		flag;				// タッチ状態フラグ
	int		x, y;				// タッチ位置
	int		move_x, move_y;		// 移動距離

		TouchManager(void)				// コンストラクタ
		{
			flag = 0x00;
		}
	void	update(short const*);		// 稼働

	Bool	is_touch(void)
			{
				return	(Bool)(flag & TOUCH);
			}
	Bool	is_touch(short _sx, short _sy, short _ex, short _ey)
			{
				return	((flag & TOUCH) && (x >= _sx) && (x < _ex) && (y >= _sy) && (y < _ey));
			}
	Bool	is_trigger(void)
			{
				return	(Bool)(flag & TRIGGER);
			}
	Bool	is_trigger(short _sx, short _sy, short _ex, short _ey)
			{
				return	((flag & TRIGGER) && (x >= _sx) && (x < _ex) && (y >= _sy) && (y < _ey));
			}
	Bool	is_repeat(void)
			{
				return	(Bool)(flag & REPEAT);
			}
	Bool	is_repeat(short _sx, short _sy, short _ex, short _ey)
			{
				return	((flag & REPEAT) && (x >= _sx) && (x < _ex) && (y >= _sy) && (y < _ey));
			}
	Bool	is_release(void)
			{
				return	(Bool)(flag & RELEASE);
			}
};

extern TouchManager*	TouchPanel;

}

#endif
/*************** End of File ****************************************/
