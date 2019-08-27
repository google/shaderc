LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:=shaderc_util
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti -DNV_EXTENSIONS -DENABLE_HLSL=1
LOCAL_EXPORT_C_INCLUDES:=$(LOCAL_PATH)/include
LOCAL_SRC_FILES:=src/args.cc \
                src/compiler.cc \
		src/file_finder.cc \
		src/io.cc \
		src/message.cc \
		src/resources.cc \
		src/shader_stage.cc \
		src/spirv_tools_wrapper.cc \
		src/version_profile.cc
LOCAL_STATIC_LIBRARIES:=SPIRV SPIRV-Tools-opt
LOCAL_C_INCLUDES:=$(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)
