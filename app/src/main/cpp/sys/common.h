#ifndef	___SYS_COMMON_H___
#define	___SYS_COMMON_H___

#include <jni.h>
#include <assert.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <memory>
#include <vector>


template <class T>
struct Rect
{
	T	x, y, w, h;

	void	set(T _x, T _y, T _w, T _h)
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

#define	RGBA(_r, _g, _b, _a)	(((uint32_t)(_a) << 24) | ((uint32_t)(_b) << 16) | ((uint32_t)(_g) << 8) | (uint32_t)(_r))
#define	RGB(_r, _g, _b)			(0xff000000 | ((uint32_t)(_b) << 16) | ((uint32_t)(_g) << 8) | (uint32_t)(_r))
#define	ALPHA(_a)				(((uint32_t)(_a) << 24) | 0xffffff)


#ifdef	NDEBUG
#define  LOGI(...)  ((void)0)
#define  LOGE(...)  ((void)0)
#else
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  "INFO",  __VA_ARGS__);
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, "ERROR", __VA_ARGS__);
#endif

#endif
