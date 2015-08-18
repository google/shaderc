LOCAL_PATH := $(call my-dir)/glslang
GLSLANG_LOCAL_PATH := $(call my-dir)/glslang

include $(CLEAR_VARS)
LOCAL_MODULE:=SPIRV
LOCAL_CXXFLAGS:=-std=c++11
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:= \
	SPIRV/GlslangToSpv.cpp \
	SPIRV/SpvBuilder.cpp \
	SPIRV/SPVRemapper.cpp \
	SPIRV/doc.cpp \
	SPIRV/disassemble.cpp

LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) $(GLSLANG_LOCAL_PATH)/glslang/SPIRV
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/glslang/SPIRV
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=OSDependent
LOCAL_CXXFLAGS:=-std=c++11
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:=glslang/OSDependent/Linux/ossource.cpp
LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) $(GLSLANG_LOCAL_PATH)/glslang/OSDependent/Linux/
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/glslang/OSDependent/Linux/
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=OGLCompiler
LOCAL_CXXFLAGS:=-std=c++11
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:=OGLCompilersDLL/InitializeDll.cpp
LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/OGLCompiler
LOCAL_STATIC_LIBRARIES:=OSDependent
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=glslang
LOCAL_CXXFLAGS:=-std=c++11
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
#TODO(awoloszyn) This creates the glslang_tab.cpp/h files in the source tree.
#                Figure out if there is a way in the android build system
#                to put them somewhere else.
$(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang_tab.cpp : $(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang.y
	@bison --defines=$(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang_tab.cpp.h -t $(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang.y -o $(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang_tab.cpp
	@echo "[$(TARGET_ARCH_ABI)] Grammar: glslang_tab.cc <= glslang.y"

$(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/Scan.cpp:$(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent/glslang_tab.cpp

LOCAL_SRC_FILES:= \
		glslang/MachineIndependent/glslang_tab.cpp \
		glslang/GenericCodeGen/CodeGen.cpp \
		glslang/GenericCodeGen/Link.cpp \
		glslang/MachineIndependent/Constant.cpp \
		glslang/MachineIndependent/InfoSink.cpp \
		glslang/MachineIndependent/Initialize.cpp \
		glslang/MachineIndependent/Intermediate.cpp \
		glslang/MachineIndependent/intermOut.cpp \
		glslang/MachineIndependent/IntermTraverse.cpp \
		glslang/MachineIndependent/limits.cpp \
		glslang/MachineIndependent/linkValidate.cpp \
		glslang/MachineIndependent/parseConst.cpp \
		glslang/MachineIndependent/ParseHelper.cpp \
		glslang/MachineIndependent/PoolAlloc.cpp \
		glslang/MachineIndependent/reflection.cpp \
		glslang/MachineIndependent/RemoveTree.cpp \
		glslang/MachineIndependent/Scan.cpp \
		glslang/MachineIndependent/ShaderLang.cpp \
		glslang/MachineIndependent/SymbolTable.cpp \
		glslang/MachineIndependent/Versions.cpp \
		glslang/MachineIndependent/preprocessor/PpAtom.cpp \
		glslang/MachineIndependent/preprocessor/PpContext.cpp \
		glslang/MachineIndependent/preprocessor/Pp.cpp \
		glslang/MachineIndependent/preprocessor/PpMemory.cpp \
		glslang/MachineIndependent/preprocessor/PpScanner.cpp \
		glslang/MachineIndependent/preprocessor/PpSymbols.cpp \
		glslang/MachineIndependent/preprocessor/PpTokens.cpp

LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_STATIC_LIBRARIES:=OSDependent OGLCompiler SPIRV
include $(BUILD_STATIC_LIBRARY)

