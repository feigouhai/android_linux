LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= logcat.cpp event.logtags

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_MODULE:= logcat

LOCAL_C_INCLUDES := $(FLYFISH_TOP)/base/include
LOCAL_CPPFLAGS := -include $(LOCAL_PATH)/../../include/arch/linux-arm/AndroidConfig.h \
                  -I $(LOCAL_PATH)/../../include/arch/linux_arm

include $(BUILD_EXECUTABLE)
