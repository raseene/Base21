TOP_LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(TOP_LOCAL_PATH)/sys/libpng/jni/Android.mk

include $(CLEAR_VARS)
include $(TOP_LOCAL_PATH)/sys/Tremor/Android.mk

LOCAL_PATH := $(TOP_LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE    := native

LOCAL_STATIC_LIBRARIES := libpng libtremor

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/sys/ \
	$(LOCAL_PATH)/sys/Tremor \

LOCAL_SRC_FILES := \
	AppMain.cpp \
	sys/SysMain.cpp \
	sys/Renderer.cpp \
	sys/Texture.cpp \
	sys/Sprite.cpp \
	sys/FrameBuffer.cpp \
	sys/TouchPanel.cpp \
	sys/Sound.cpp \

LOCAL_LDLIBS    :=  -llog -lGLESv2 -landroid -lz -lOpenSLES

LOCAL_ARM_MODE  := arm

include $(BUILD_SHARED_LIBRARY)
