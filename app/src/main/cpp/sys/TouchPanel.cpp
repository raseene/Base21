/***********************************

		タッチパネル管理

 ***********************************/

#include	"TouchPanel.h"
#include	"Renderer.h"
#include	"Engine.h"


namespace sys
{

TouchManager*	TouchPanel;				// タッチパネル状態

int		TouchManager::touch_max;		// マルチタッチ数


/***************************************
    初期化
		引数	_max = マルチタッチ数
 ***************************************/
void	TouchManager::create(int _max)
{
	touch_max = _max;
	TouchPanel = new TouchManager[touch_max];
}

void	TouchManager::resume(android_input_buffer* _input)
{
	if ( _input && (_input->motionEventsCount > 0) ) {
		android_app_clear_motion_events(_input);
	}
	for (int i = 0; i < touch_max; i++) {
		TouchPanel[i].flag = 0x00;
	}
}

/**********
    終了
 **********/
void	TouchManager::release(void)
{
	delete[]	TouchPanel;
}


/**********
    稼働
 **********/
void	TouchManager::update(android_input_buffer* _input)
{
	for (int i = 0; i < touch_max; i++) {				// 準備
		TouchPanel[i].prev_x = TouchPanel[i].x;
		TouchPanel[i].prev_y = TouchPanel[i].y;
	}

	if ( _input ) {										// 入力イベント
		for (size_t i = 0; i < _input->motionEventsCount; i++) {
			GameActivityMotionEvent*	_event = &_input->motionEvents[i];
			GameActivityPointerAxes*	_pt = &_event->pointers[(_event->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT];

			if ( _pt->id < touch_max ) {
				TouchManager*	_touch = &TouchPanel[_pt->id];

				switch ( _event->action & AMOTION_EVENT_ACTION_MASK ) {
				  case AMOTION_EVENT_ACTION_DOWN :
				  case AMOTION_EVENT_ACTION_POINTER_DOWN :
					_touch->flag |= RAW;
					break;

				  case AMOTION_EVENT_ACTION_UP :
				  case AMOTION_EVENT_ACTION_POINTER_UP :
					_touch->flag &= ~RAW;
					break;
				}
			}

			for (int j = 0, t = 0; j < touch_max; j++) {
				TouchManager*	_touch = &TouchPanel[j];

				if ( _touch->flag & (RAW | TOUCH) ) {
					_pt = &_event->pointers[t++];
					_touch->x = (GameActivityPointerAxes_getX(_pt) - sys::Renderer::screen_rect.x)*sys::Renderer::screen_width/sys::Renderer::screen_rect.w;
					_touch->y = (GameActivityPointerAxes_getY(_pt) - sys::Renderer::screen_rect.y)*sys::Renderer::screen_height/sys::Renderer::screen_rect.h;
				}
			}
		}
		android_app_clear_motion_events(_input);
	}

	for (int i = 0; i < touch_max; i++) {				// 各タッチ稼働
		TouchPanel[i].update();
	}
}

void	TouchManager::update(void)
{
	if ( flag & RAW ) {									// タッチ中
		if ( !(flag & TOUCH) ) {						// 初回
			flag = TOUCH | TRIGGER | REPEAT | RAW;
			repeat_cnt = Engine::frame_rate/2;
			move_x = 0.0f;
			move_y = 0.0f;
			prev_x = x;
			prev_y = y;
		}
		else {											// 継続
			flag = TOUCH | RAW;
			if ( --repeat_cnt == 0 ) {					// リピート
				flag |= REPEAT;
				repeat_cnt = Engine::frame_rate/7;
			}
			move_x = x - prev_x;
			move_y = y - prev_y;
		}
	}
	else {												// 非タッチ
		flag = (flag & TOUCH) ? RELEASE : 0;
	}
}

}
