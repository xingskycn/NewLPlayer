LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := player
LOCAL_SRC_FILES := player.cpp PlayerData.cpp
LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid
LOCAL_SHARED_LIBRARIES := libswresample libavformat libavcodec libswscale libavutil 
LOCAL_CXXFLAGS := -D__STDC_CONSTANT_MACROS

include $(BUILD_SHARED_LIBRARY)
$(call import-module,ffmpeg/android/arm)
