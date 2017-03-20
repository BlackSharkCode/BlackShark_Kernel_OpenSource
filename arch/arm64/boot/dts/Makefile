dts-dirs += arm
#dts-dirs += qcom
ifeq ($(CONFIG_ARCH_SDM845),y)
define find_bs_dts_dirs
$(addprefix $(1),$(shell find $(srctree)/$(src)/$(1) -type d -printf "%f\n" | tail -n +2))
endef

dts-dirs += $(call find_bs_dts_dirs,blackshark/shark/)
endif

subdir-y	:= $(dts-dirs)

dtstree		:= $(srctree)/$(src)

dtb-$(CONFIG_OF_ALL_DTBS) := $(patsubst $(dtstree)/%.dts,%.dtb, $(foreach d,$(dts-dirs), $(wildcard $(dtstree)/$(d)/*.dts)))

always		:= $(dtb-y)

targets += dtbs

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
targets += $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := dts/*.dtb *.dtb
