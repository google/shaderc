THIRD_PARTY_PATH := $(call my-dir)

GLSLANG_LOCAL_PATH := $(THIRD_PARTY_PATH)/glslang
LOCAL_PATH := $(GLSLANG_LOCAL_PATH)

GLSLANG_OS_FLAGS := -DGLSLANG_OSINCLUDE_UNIX
# AMD extensions are turned on by default in upstream Glslang.
GLSLANG_DEFINES:= -DAMD_EXTENSIONS $(GLSLANG_OS_FLAGS)

include $(CLEAR_VARS)
LOCAL_MODULE:=SPIRV
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti $(GLSLANG_DEFINES)
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:= \
	SPIRV/GlslangToSpv.cpp \
	SPIRV/InReadableOrder.cpp \
	SPIRV/Logger.cpp \
	SPIRV/SPVRemapper.cpp \
	SPIRV/SpvBuilder.cpp \
	SPIRV/disassemble.cpp \
	SPIRV/doc.cpp

LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) $(GLSLANG_LOCAL_PATH)/glslang/SPIRV
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/glslang/SPIRV
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=OSDependent
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti $(GLSLANG_DEFINES)
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:=glslang/OSDependent/Unix/ossource.cpp
LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) $(GLSLANG_LOCAL_PATH)/glslang/OSDependent/Unix/
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/glslang/OSDependent/Unix/
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:=OGLCompiler
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti $(GLSLANG_DEFINES)
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)
LOCAL_SRC_FILES:=OGLCompilersDLL/InitializeDll.cpp
LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)/OGLCompiler
LOCAL_STATIC_LIBRARIES:=OSDependent
include $(BUILD_STATIC_LIBRARY)


# Build Glslang's HLSL parser library.
include $(CLEAR_VARS)
LOCAL_MODULE:=HLSL
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti
LOCAL_SRC_FILES:= \
		hlsl/hlslAttributes.cpp \
		hlsl/hlslGrammar.cpp \
		hlsl/hlslOpMap.cpp \
		hlsl/hlslParseables.cpp \
		hlsl/hlslParseHelper.cpp \
		hlsl/hlslScanContext.cpp \
		hlsl/hlslTokenStream.cpp
LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) \
	$(GLSLANG_LOCAL_PATH)/hlsl
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

GLSLANG_OUT_PATH=$(if $(call host-path-is-absolute,$(TARGET_OUT)),$(TARGET_OUT),$(abspath $(TARGET_OUT)))

LOCAL_MODULE:=glslang
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti $(GLSLANG_DEFINES)
LOCAL_EXPORT_C_INCLUDES:=$(GLSLANG_LOCAL_PATH)

LOCAL_SRC_FILES:= \
		glslang/GenericCodeGen/CodeGen.cpp \
		glslang/GenericCodeGen/Link.cpp \
		glslang/MachineIndependent/Constant.cpp \
		glslang/MachineIndependent/glslang_tab.cpp \
		glslang/MachineIndependent/InfoSink.cpp \
		glslang/MachineIndependent/Initialize.cpp \
		glslang/MachineIndependent/Intermediate.cpp \
		glslang/MachineIndependent/intermOut.cpp \
		glslang/MachineIndependent/IntermTraverse.cpp \
		glslang/MachineIndependent/iomapper.cpp \
		glslang/MachineIndependent/limits.cpp \
		glslang/MachineIndependent/linkValidate.cpp \
		glslang/MachineIndependent/parseConst.cpp \
		glslang/MachineIndependent/ParseContextBase.cpp \
		glslang/MachineIndependent/ParseHelper.cpp \
		glslang/MachineIndependent/PoolAlloc.cpp \
		glslang/MachineIndependent/propagateNoContraction.cpp \
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

LOCAL_C_INCLUDES:=$(GLSLANG_LOCAL_PATH) \
	$(GLSLANG_LOCAL_PATH)/glslang/MachineIndependent \
	$(GLSLANG_OUT_PATH)
LOCAL_STATIC_LIBRARIES:=OSDependent OGLCompiler SPIRV HLSL
include $(BUILD_STATIC_LIBRARY)


SPVTOOLS_LOCAL_PATH := $(THIRD_PARTY_PATH)/spirv-tools
LOCAL_PATH := $(SPVTOOLS_LOCAL_PATH)
SPVTOOLS_OUT_PATH=$(if $(call host-path-is-absolute,$(TARGET_OUT)),$(TARGET_OUT),$(abspath $(TARGET_OUT)))
SPVHEADERS_LOCAL_PATH := $(THIRD_PARTY_PATH)/spirv-tools/external/spirv-headers

# Locations of grammar files.
SPV_CORE10_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/1.0/spirv.core.grammar.json
SPV_CORE11_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/1.1/spirv.core.grammar.json
SPV_GLSL_GRAMMAR=$(SPVHEADERS_LOCAL_PATH)/include/spirv/1.0/extinst.glsl.std.450.grammar.json
# OpenCL grammar has not yet been published to SPIRV-Headers
SPV_OPENCL_GRAMMAR=$(SPVTOOLS_LOCAL_PATH)/source/extinst-1.0.opencl.std.grammar.json

define gen_spvtools_grammar_tables
$(call generate-file-dir,$(1)/core.insts-1.0.inc)
$(1)/core.insts-1.0.inc $(1)/operand.kinds-1.0.inc $(1)/glsl.std.450.insts-1.0.inc $(1)/opencl.std.insts-1.0.inc: \
        $(SPVTOOLS_LOCAL_PATH)/utils/generate_grammar_tables.py \
        $(SPV_CORE10_GRAMMAR) \
        $(SPV_GLSL_GRAMMAR) \
        $(SPV_OPENCL_GRAMMAR)
		@$(HOST_PYTHON) $(SPVTOOLS_LOCAL_PATH)/utils/generate_grammar_tables.py \
		                --spirv-core-grammar=$(SPV_CORE10_GRAMMAR) \
		                --extinst-glsl-grammar=$(SPV_GLSL_GRAMMAR) \
		                --extinst-opencl-grammar=$(SPV_OPENCL_GRAMMAR) \
		                --core-insts-output=$(1)/core.insts-1.0.inc \
		                --glsl-insts-output=$(1)/glsl.std.450.insts-1.0.inc \
		                --opencl-insts-output=$(1)/opencl.std.insts-1.0.inc \
		                --operand-kinds-output=$(1)/operand.kinds-1.0.inc
		@echo "[$(TARGET_ARCH_ABI)] Grammar v1.0   : instructions & operands <= grammar JSON files"
$(1)/core.insts-1.1.inc $(1)/operand.kinds-1.1.inc: \
        $(SPVTOOLS_LOCAL_PATH)/utils/generate_grammar_tables.py \
        $(SPV_CORE11_GRAMMAR)
		@$(HOST_PYTHON) $(SPVTOOLS_LOCAL_PATH)/utils/generate_grammar_tables.py \
		                --spirv-core-grammar=$(SPV_CORE11_GRAMMAR) \
		                --core-insts-output=$(1)/core.insts-1.1.inc \
		                --operand-kinds-output=$(1)/operand.kinds-1.1.inc
		@echo "[$(TARGET_ARCH_ABI)] Grammar v1.1   : instructions & operands <= grammar JSON files"
$(SPVTOOLS_LOCAL_PATH)/source/opcode.cpp: $(1)/core.insts-1.0.inc $(1)/core.insts-1.1.inc
$(SPVTOOLS_LOCAL_PATH)/source/operand.cpp: $(1)/operand.kinds-1.0.inc $(1)/operand.kinds-1.1.inc
$(SPVTOOLS_LOCAL_PATH)/source/ext_inst.cpp: $(1)/glsl.std.450.insts-1.0.inc $(1)/opencl.std.insts-1.0.inc
endef
$(eval $(call gen_spvtools_grammar_tables,$(SPVTOOLS_OUT_PATH)))

define gen_spvtools_build_version_inc
$(call generate-file-dir,$(1)/dummy_filename)
$(1)/build-version.inc: \
        $(SPVTOOLS_LOCAL_PATH)/utils/update_build_version.py \
        $(SPVTOOLS_LOCAL_PATH)/CHANGES
		@$(HOST_PYTHON) $(SPVTOOLS_LOCAL_PATH)/utils/update_build_version.py \
		                $(SPVTOOLS_LOCAL_PATH) $(1)/build-version.inc
		@echo "[$(TARGET_ARCH_ABI)] Generate       : build-version.inc <= CHANGES"
$(SPVTOOLS_LOCAL_PATH)/source/software_version.cpp: $(1)/build-version.inc
endef
$(eval $(call gen_spvtools_build_version_inc,$(SPVTOOLS_OUT_PATH)))

define gen_spvtools_generators_inc
$(call generate-file-dir,$(1)/dummy_filename)
$(1)/generators.inc: \
        $(SPVTOOLS_LOCAL_PATH)/utils/generate_registry_tables.py \
        $(SPVHEADERS_LOCAL_PATH)/include/spirv/spir-v.xml
		@$(HOST_PYTHON) $(SPVTOOLS_LOCAL_PATH)/utils/generate_registry_tables.py \
		                --xml=$(SPVHEADERS_LOCAL_PATH)/include/spirv/spir-v.xml \
				--generator-output=$(1)/generators.inc
		@echo "[$(TARGET_ARCH_ABI)] Generate       : generators.inc <= spir-v.xml"
$(SPVTOOLS_LOCAL_PATH)/source/opcode.cpp: $(1)/generators.inc
endef
$(eval $(call gen_spvtools_generators_inc,$(SPVTOOLS_OUT_PATH)))

include $(CLEAR_VARS)
LOCAL_MODULE := SPIRV-Tools
LOCAL_C_INCLUDES := \
		$(SPVTOOLS_LOCAL_PATH)/include \
		$(SPVTOOLS_LOCAL_PATH)/source \
		$(SPVTOOLS_LOCAL_PATH)/external/spirv-headers/include \
		$(SPVTOOLS_OUT_PATH)
LOCAL_EXPORT_C_INCLUDES := \
		$(SPVTOOLS_LOCAL_PATH)/include
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti
LOCAL_SRC_FILES:= \
		source/assembly_grammar.cpp \
		source/binary.cpp \
		source/diagnostic.cpp \
		source/disassemble.cpp \
		source/ext_inst.cpp \
		source/libspirv.cpp \
		source/name_mapper.cpp \
		source/opcode.cpp \
		source/operand.cpp \
		source/parsed_operand.cpp \
		source/print.cpp \
		source/software_version.cpp \
		source/spirv_endian.cpp \
		source/spirv_target_env.cpp \
		source/table.cpp \
		source/text.cpp \
		source/text_handler.cpp \
		source/util/parse_number.cpp \
		source/val/basic_block.cpp \
		source/val/construct.cpp \
		source/val/function.cpp \
		source/val/instruction.cpp \
		source/val/validation_state.cpp \
		source/validate.cpp \
		source/validate_cfg.cpp \
		source/validate_datarules.cpp \
		source/validate_id.cpp \
		source/validate_instruction.cpp \
		source/validate_layout.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SPIRV-Tools-opt
LOCAL_C_INCLUDES := \
		$(SPVTOOLS_LOCAL_PATH)/include \
		$(SPVTOOLS_LOCAL_PATH)/source \
		$(SPVTOOLS_LOCAL_PATH)/external/spirv-headers/include
LOCAL_CXXFLAGS:=-std=c++11 -fno-exceptions -fno-rtti
LOCAL_STATIC_LIBRARIES:=SPIRV-Tools
LOCAL_SRC_FILES:= \
		source/opt/build_module.cpp \
		source/opt/def_use_manager.cpp \
		source/opt/eliminate_dead_constant_pass.cpp \
		source/opt/fold_spec_constant_op_and_composite_pass.cpp \
		source/opt/freeze_spec_constant_value_pass.cpp \
		source/opt/function.cpp \
		source/opt/instruction.cpp \
		source/opt/ir_loader.cpp \
		source/opt/module.cpp \
		source/opt/optimizer.cpp \
		source/opt/pass_manager.cpp \
		source/opt/set_spec_constant_default_value_pass.cpp \
		source/opt/strip_debug_info_pass.cpp \
		source/opt/type_manager.cpp \
		source/opt/types.cpp \
		source/opt/unify_const_pass.cpp
include $(BUILD_STATIC_LIBRARY)
