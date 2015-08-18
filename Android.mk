ROOT_SHADERC_PATH := $(call my-dir)

include $(ROOT_SHADERC_PATH)/third_party/Android.mk
include $(ROOT_SHADERC_PATH)/libshaderc_util/Android.mk
include $(ROOT_SHADERC_PATH)/libshaderc/Android.mk

