
#===============================================================================
# rule
#===============================================================================
.PHONY: rootbox_prepare rootbox_compose  rootbox_strip rootbox_install img_build
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++
rootbox_install: rootbox_prepare rootbox_compose rootbox_strip img_build

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
rootbox_prepare:
	@echo -e "\033[34m Prepare for making image... \033[0m"
	rm -rf $(IMAGE_DIR)/rootbox
	fakeroot cp -arf $(SDK_PUB_DIR)/$(CFG_CHIP_TYPE)/rootbox      $(IMAGE_DIR)
	tar -czvf $(IMAGE_DIR)/audio_lib.tar.gz -C                    $(TARGET_GLOBAL_LD_EXTERN_DIRS)/ .

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
rootbox_compose:
	@echo -e "\033[34m Install config files to rootbox... \033[0m"
	fakeroot mkdir -p $(ROOTBOX_DIR)/data
	fakeroot mkdir -p $(ROOTBOX_DIR)/usr/data
	-cp -rf $(TARGET_CONFIG_DIR)/*                                $(ROOTBOX_DIR)/usr/data
	-cp -rf $(PROJECT_DIR)/data/appcfg.ini                        $(ROOTBOX_DIR)/usr/data
	-cp -rf $(PROJECT_DIR)/data/syscfg.ini                        $(ROOTBOX_DIR)/usr/data
	-cp -rf $(PROJECT_DIR)/data/dtv                               $(ROOTBOX_DIR)/usr/data
	-cp -rf $(PROJECT_DIR)/data/frontend                          $(ROOTBOX_DIR)/usr/data
	-cp -rf $(PROJECT_DIR)/res                                    $(ROOTBOX_DIR)/usr/bin
ifeq ($(CFG_SINGLE_PROCESS_ENABLE),y)
	-cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/S98user-Single  $(ROOTBOX_DIR)/etc/init.d/S98user
else
	-cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/S98user         $(ROOTBOX_DIR)/etc/init.d/S98user
endif
	-cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/S99init         $(ROOTBOX_DIR)/etc/init.d
	-cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/rcS             $(ROOTBOX_DIR)/etc/init.d

	@echo -e "\033[34m Install fushion.ko to rootbox... \033[0m"
	-cp -rf $(TARGET_GLOBAL_LD_SHARED_DIRS)/fusion.ko             $(ROOTBOX_DIR)/kmod
	-cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/load            $(ROOTBOX_DIR)/kmod

	@echo -e "\033[34m Install lib files to rootbox... \033[0m"
	#rm -rf $(ROOTBOX_DIR)/usr/lib/*
	-cp -arf $(TARGET_GLOBAL_LD_SHARED_DIRS)/*.so                 $(ROOTBOX_DIR)/usr/lib/
	-cp -arf $(TARGET_GLOBAL_LD_SHARED_DIRS)/*.so.*               $(ROOTBOX_DIR)/usr/lib/
	-cp -arf $(TARGET_GLOBAL_LD_SHARED_DIRS)/higo-adp             $(ROOTBOX_DIR)/usr/lib/
	-cp -arf $(TARGET_GLOBAL_LD_SHARED_DIRS)/directfb-1.6-0       $(ROOTBOX_DIR)/usr/lib/
ifeq ($(CFG_HI_NATIVE_GSTREAMER_ENABLE),y)
	-cp -arf $(TARGET_GLOBAL_LD_SHARED_DIRS)/gstplugins           $(ROOTBOX_DIR)/usr/lib/
	-cp -arf $(TARGET_BIN_DIR)/gst-launch-1.0                $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/gst-inspect-1.0                $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/gst-typefind-1.0                $(ROOTBOX_DIR)/usr/bin
endif
	-cp -arf $(TARGET_GLOBAL_LD_EXTERN_DIRS)/*.so                 $(ROOTBOX_DIR)/usr/lib/

	@echo -e "\033[34m Install modular drm plugin to rootbox... \033[0m"
	fakeroot mkdir -p $(ROOTBOX_DIR)/vendor/lib/mediadrm
	-mv -f $(ROOTBOX_DIR)/usr/lib/libplayreadydrmplugin.so $(ROOTBOX_DIR)/vendor/lib/mediadrm
	@echo -e "\033[34m Install binary files to rootbox... \033[0m"
	-cp -arf $(TARGET_BIN_DIR)/adbd                $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/logcat              $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_apm         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_manager     $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_media       $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_system      $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_resmgr      $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_dtv         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_drm         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_dth         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/service_dcas        $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/zygote              $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/sysui               $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_sysui           $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/launcher            $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_launcher        $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/mediaCenter         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_mediacenter     $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/setting             $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_setting         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/dtv                 $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_dtv             $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/system_time_setting    $(ROOTBOX_DIR)/usr/bin
ifeq ($(CFG_HI_PPPOE_SUPPORT),y)
	fakeroot mkdir -p $(ROOTBOX_DIR)/etc/ppp
	-cp -rf  $(PLATFORM_DIR)/hal/scripts/pppoe     $(ROOTBOX_DIR)/usr/data
	-cp -arf $(TARGET_BIN_DIR)/pppd                $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/pppoe               $(ROOTBOX_DIR)/usr/bin
endif
ifeq (${CFG_HI_NATIVE_BROWSER_ENABLE},y)
	-cp -arf $(TARGET_BIN_DIR)/browser_ui          $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_CONFIG_DIR)/font/*           $(ROOTBOX_DIR)/
	-cp -arf $(TARGET_BIN_DIR)/res_browser         $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/browser             $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/res_browser_full    $(ROOTBOX_DIR)/usr/bin
	-cp -arf $(TARGET_BIN_DIR)/browser_full        $(ROOTBOX_DIR)/usr/bin
ifneq (${CFG_HI_NATIVE_HIGV_ENABLE},y)
	-mv $(ROOTBOX_DIR)/usr/lib/directfb-1.6-0/inputdrivers/libdirectfb_lirc.so $(ROOTBOX_DIR)/usr/lib/directfb-1.6-0/inputdrivers/libdirectfb_lirc.so_bak
	-mv $(ROOTBOX_DIR)/usr/lib/directfb-1.6-0/inputdrivers/libdirectfb_keyboard.so $(ROOTBOX_DIR)/usr/lib/directfb-1.6-0/inputdrivers/libdirectfb_keyboard.so_bak
endif
	-@$(TARGET_STRIP)                              $(ROOTBOX_DIR)/usr/bin/browser/content_shell
	-@$(TARGET_STRIP)                              $(ROOTBOX_DIR)/usr/bin/browser/libcontent_shell_lib.so
	-@$(TARGET_STRIP)                              $(ROOTBOX_DIR)/usr/bin/browser_ui
	-@$(TARGET_STRIP)                              $(ROOTBOX_DIR)/usr/bin/browser_full
endif
	-cp -rf $(PROJECT_DIR)/data/appcfg_s.ini       $(ROOTBOX_DIR)/usr/data/appcfg.ini
	-@$(TARGET_STRIP)                              $(ROOTBOX_DIR)/usr/lib/lib*

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
rootbox_strip:
	find $(ROOTBOX_DIR) -name "*.svn"| xargs rm -rf
	find $(ROOTBOX_DIR) -name "*.a"| xargs rm -rf

#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
img_build:
	@echo -e "\033[34m Make rootbox image... \033[0m"
ifeq ($(CFG_HI_APPFS_EXT4), y)
	 -cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/profiles/profile $(ROOTBOX_DIR)/etc/profile
	$(TOOL_MKEXT4FS) -l $(CFG_HI_APPFS_EXT4_SIZE)M -s $(IMAGE_DIR)/rootfs_$(CFG_HI_APPFS_EXT4_SIZE)M.ext4 $(ROOTBOX_DIR)
endif
ifeq ($(CFG_HI_APPFS_YAFFS), y)
	 -cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/profiles/profile $(ROOTBOX_DIR)/etc/profile
	$(TOOL_MKYAFFS) $(ROOTBOX_DIR) $(IMAGE_DIR)/rootfs.yaffs $(CFG_HI_NAND_PAGE_SIZE)k $(CFG_HI_NAND_ECC_TYPE)bit
endif
ifeq ($(CFG_HI_APPFS_SQUASHFS), y)
	 -cp -rf $(PLATFORM_DIR)/$(CFG_SDK_TYPE)_patch/profiles/profile_data_yaffs $(ROOTBOX_DIR)/etc/profile
	$(TOOL_MKSQUASHFS) $(ROOTBOX_DIR) $(IMAGE_DIR)/rootfs.squashfs -no-fragments -noappend -noI -comp gzip -b $(CFG_HI_NAND_BLOCK_SIZE)k
ifeq ($(CFG_HI_DCAS_ENABLE), y)
	cp -f $(IMAGE_DIR)/rootfs.squashfs       $(SDK_PUB_DIR)/$(CFG_CHIP_TYPE)/image
	cp -f $(IMAGE_DIR)/bootargs_squashfs.bin $(SDK_PUB_DIR)/$(CFG_CHIP_TYPE)/image/bootargs.bin
	make -C $(FLYFISH_SDK_PATH)/HiSTBSDKV1R5-L/tools/linux/utils/advca install
	cp -f $(SDK_PUB_DIR)/advca_image/SignOutput/fastboot-burn.bin_Sign.img   $(IMAGE_DIR)
	cp -f $(SDK_PUB_DIR)/advca_image/SignOutput/bootargs.bin_Sign.img        $(IMAGE_DIR)/bootargs_squashfs.bin_Sign.img
	cp -f $(SDK_PUB_DIR)/advca_image/SignOutput/trustedcore.img_Sign.img     $(IMAGE_DIR)
	cp -f $(SDK_PUB_DIR)/advca_image/SignOutput/hi_kernel.bin_Sign.img       $(IMAGE_DIR)
	cp -f $(SDK_PUB_DIR)/advca_image/SignOutput/rootfs.squashfs.sig          $(IMAGE_DIR)
endif
	$(TOOL_MKYAFFS) $(ROOTBOX_DIR)/data $(IMAGE_DIR)/data.yaffs $(CFG_HI_NAND_PAGE_SIZE)k $(CFG_HI_NAND_ECC_TYPE)bit
endif

