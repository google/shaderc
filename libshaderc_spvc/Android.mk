LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx
LOCAL_MODULE:=shaderc_spvc
LOCAL_EXPORT_C_INCLUDES:=$(LOCAL_PATH)/include
LOCAL_SRC_FILES:=src/spvc.cc \
                 src/spvc_private.cc
LOCAL_C_INCLUDES:=$(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES:=shaderc_util SPIRV-Tools SPIRV-Tools-opt spirv-cross-glsl
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti -DNV_EXTENSIONS -DENABLE_HLSL=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++11
LOCAL_EXPORT_LDFLAGS:=-latomic
include $(BUILD_STATIC_LIBRARY)
