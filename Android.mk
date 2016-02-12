ROOT_SHADERC_PATH := $(call my-dir)

include $(ROOT_SHADERC_PATH)/third_party/Android.mk
include $(ROOT_SHADERC_PATH)/libshaderc_util/Android.mk
include $(ROOT_SHADERC_PATH)/libshaderc/Android.mk

ALL_LIBS:=libglslang.a \
	libOGLCompiler.a \
	libOSDependent.a \
	libshaderc.a \
	libshaderc_util.a \
	libSPIRV.a \
	libSPIRV-Tools.a

define gen_libshaderc
$(1)/combine.ar: $(addprefix $(1)/, $(ALL_LIBS))
	@echo "create libshaderc_combined.a" > $(1)/combine.ar
	$(foreach lib,$(ALL_LIBS),
		@echo "addlib $(lib)" >> $(1)/combine.ar
	)
	@echo "save" >> $(1)/combine.ar
	@echo "end" >> $(1)/combine.ar

$(1)/libshaderc_combined.a: $(addprefix $(1)/, $(ALL_LIBS)) $(1)/combine.ar
	@echo "[$(TARGET_ARCH_ABI)] Combine: libshaderc_combined.a <= $(ALL_LIBS)"
	@cd $(1) && $(2)ar -M < combine.ar && cd -
	@$(2)objcopy --strip-debug $(1)/libshaderc_combined.a

libshaderc_combined:$(1)/libshaderc_combined.a
endef

$(eval $(call gen_libshaderc,$(TARGET_OUT),$(TOOLCHAIN_PREFIX)))
