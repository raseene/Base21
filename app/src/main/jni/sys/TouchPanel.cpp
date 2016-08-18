/***********************************

		タッチパネル管理

 ***********************************/

#include	"TouchPanel.h"
#include	"Renderer.h"


namespace sys
{

TouchManager*	TouchPanel;

/************
    初期化
 ************/
void	TouchManager::create_manager(void)
{
	TouchPanel = new TouchManager[TOUCH_MAX];
}

/**********
    終了
 **********/
void	TouchManager::release_manager(void)
{
	delete[]	TouchPanel;
}

/***********************************************
    稼働
		引数	_status[0] = タッチしているか
				_status[1] = X座標
				_status[2] = Y座標
 ***********************************************/
void	TouchManager::update_manager(short const* _status)
{
	for (int i = 0; i < TOUCH_MAX; i++) {
		TouchPanel[i].update(_status);
		_status += 3;
	}
}

void	TouchManager::update(short const* _status)
{
	if ( _status[0] ) {						// タッチ中
		// タッチ位置補正
		x = (_status[1] - Renderer::screen_rect.x)*SCREEN_WIDTH/Renderer::screen_rect.w - SCREEN_WIDTH/2;
		y = (_status[2] - Renderer::screen_rect.y)*SCREEN_HEIGHT/Renderer::screen_rect.h - SCREEN_HEIGHT/2;
		if ( !(flag & TOUCH) ) {					// 初回
			flag = TOUCH | TRIGGER | REPEAT;
			repeat_cnt = TOUCH_REPEAT1;
			move_x = 0;
			move_y = 0;
			prev_x = x;
			prev_y = y;
		}
		else {										// 継続
			flag = TOUCH;
			if ( --repeat_cnt == 0 ) {				// リピート
				flag |= REPEAT;
				repeat_cnt = TOUCH_REPEAT2;
			}
			move_x	= x - prev_x;
			move_y	= y - prev_y;
			prev_x	= x;
			prev_y	= y;
		}
	}
	else {									// 非タッチ
		flag = (flag & TOUCH) ? RELEASE : 0;
	}
}

}

/*************** End of File *****************************************/
