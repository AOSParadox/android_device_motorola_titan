LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),titan)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := \
    system/core/init \
    device/qcom/common/init

LOCAL_CFLAGS := -Wall -DANDROID_TARGET=\"$(TARGET_BOARD_PLATFORM)\"
LOCAL_SRC_FILES := init_titan.cpp
LOCAL_MODULE := libinit_titan
include $(BUILD_STATIC_LIBRARY)

endif
