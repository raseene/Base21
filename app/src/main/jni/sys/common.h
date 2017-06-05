#ifndef	___COMMON_H___
#define	___COMMON_H___

#include <jni.h>
#include <assert.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define	FALSE	0
#define	TRUE	1

typedef char				s8;
typedef unsigned char		u8;
typedef short				s16;
typedef unsigned short		u16;
typedef long				s32;
typedef unsigned long		u32;
typedef long long			s64;
typedef unsigned long long	u64;
typedef int					Bool;

struct Rect
{
	float	x, y, w, h;

	void	set(float _x, float _y, float _w, float _h)
			{
				x = _x;
				y = _y;
				w = _w;
				h = _h;
			}
};

struct SRect
{
	short	x, y, w, h;

	void	set(short _x, short _y, short _w, short _h)
			{
				x = _x;
				y = _y;
				w = _w;
				h = _h;
			}
};

// 座標
enum
{
	X	= 0,
	Y,
	Z,
	XY	= 2,
	XYZ,
};

// カラー
enum
{
	R	= 0,
	G,
	B,
	A,
	RGB	= 3,
	RGBA,
};

#define	RGBA(_r, _g, _b, _a)	(((u32)(_a) << 24) | ((u32)(_b) << 16) | ((u32)(_g) << 8) | (u32)(_r))
#define	RGB(_r, _g, _b)			(0xff000000 | ((u32)(_b) << 16) | ((u32)(_g) << 8) | (u32)(_r))
#define	ALPHA(_a)				(((u32)(_a) << 24) | 0xffffff)

#include "../def.h"


#ifdef	NDEBUG
#define  LOGI(...)  ((void)0)
#define  LOGE(...)  ((void)0)
#else
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  "INFO",  __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, "ERROR", __VA_ARGS__)
#endif


namespace sys
{
extern JavaVM*	g_JavaVM;								// JavaVM情報

extern AAssetManager*	asset_manager;					// asset読み込み用

void*	load_asset(const char*, u32* size = NULL);		// assetファイル読み込み


// キー種類
enum
{
	KEY_BACK	= 1,			// バックキー
	KEY_YES,					// ダイアログ用
	KEY_NO,
};

extern u32	common_counter;		// 汎用カウンタ
extern int	key_status;			// キー入力状態
}

#endif
/***************** End of File ***********************************/
