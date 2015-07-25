LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE	:= libtremor
LOCAL_SRC_FILES :=\
	block.c \
	codebook.c \
	floor0.c \
	floor1.c \
	info.c \
	mapping0.c \
	mdct.c \
	registry.c \
	res012.c \
	sharedbook.c \
	synthesis.c \
	vorbisfile.c \
	window.c \
	src/bitwise.c \
	src/framing.c

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)
