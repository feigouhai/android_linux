############################## platform compile before #####################
include $(FLYFISH_SDK_PATH)/sdk.mk
include $(FLYFISH_HAL_PATH)/hal.mk
include $(FLYFISH_HAL_SMP_PATH)/sample_hal.mk
############################## platform compile end ########################

############################## base compile before #########################
include $(FLYFISH_TOP)/base/ipc/ipc.mk
include $(FLYFISH_TOP)/base/cutils/cutils.mk
include $(FLYFISH_TOP)/base/utils/utils.mk
include $(FLYFISH_TOP)/base/log/liblog/liblog.mk
include $(FLYFISH_TOP)/base/log/logcat/logcat.mk

ifeq (${CFG_HI_SERVICE_DTV_ENABLE},y)
include $(FLYFISH_TOP)/external/iconv/iconv.mk
endif

include $(FLYFISH_TOP)/base/common/libcommon.mk
############################## base compile end ############################


############################## component compile before ####################
ifeq (${CFG_HI_NATIVE_HIPLAYER_ENABLE},y)
include $(FLYFISH_TOP)/component/player/player.mk
include $(FLYFISH_TOP)/component/charset/charset.mk
include $(FLYFISH_TOP)/component/drmengine/drmengine.mk
include $(FLYFISH_TOP)/component/hisub/hisub.mk
endif

ifeq ($(CFG_HI_NATIVE_GSTREAMER_ENABLE),y)
include $(FLYFISH_TOP)/external/sme-player/sme.mk
endif

include $(FLYFISH_TOP)/external/fastdb/fastdb.mk
include $(FLYFISH_TOP)/component/db/db.mk

############################## dtv module #######################
include $(FLYFISH_TOP)/component/dtv/dtv.mk
include $(FLYFISH_TOP)/framework/dthservice/dthservice.mk
include $(FLYFISH_TOP)/component/dcas/dcas.mk
include $(FLYFISH_TOP)/component/TvosMedia/tvos.mk

ifeq (${CFG_HI_NATIVE_HIGV_ENABLE},y)
include $(FLYFISH_TOP)/component/higv/higv.mk
endif

ifeq (${CFG_HI_SERVICE_DTV_ENABLE},y)
include $(FLYFISH_TOP)/component/dtvstack/dtvstack.mk
endif
############################## component compile end #######################


############################## external compile before #####################
include $(FLYFISH_TOP)/external/openssl/openssl.mk

ifeq (${CFG_HI_TEST_CASE_SUPPORT},y)
include $(FLYFISH_TOP)/external/cppunit/cppunit.mk
endif

ifeq (${CFG_HI_NATIVE_HIPLAYER_ENABLE},y)
include $(FLYFISH_TOP)/external/libbluray/libbluray.mk
include $(FLYFISH_TOP)/external/libflac/libflac.mk
include $(FLYFISH_TOP)/external/ffmpeg/libtlsadp/libtlsadp.mk
include $(FLYFISH_TOP)/external/ffmpeg/ffmpeg.mk
include $(FLYFISH_TOP)/external/ffmpeg/libclientadp/libclientadp.mk
include $(FLYFISH_TOP)/external/ffmpeg/libformat_open/libformat_open.mk
include $(FLYFISH_TOP)/external/ffmpeg/libffmpegadp/libffmpegadp.mk
include $(FLYFISH_TOP)/external/libudf/libudf.mk
endif

ifeq (${CFG_HI_NATIVE_BROWSER_ENABLE},y)
include $(FLYFISH_TOP)/external/expat/expat.mk
include $(FLYFISH_TOP)/external/libmicrohttpd/libmicrohttpd.mk
include $(FLYFISH_TOP)/external/fontconfig/fontconfig.mk
include $(FLYFISH_TOP)/external/browser/browser.mk
include $(FLYFISH_TOP)/external/browser/browser_interface/browser_interface.mk
endif

############################## external compile end ########################

############################## tools compile before ########################
include $(FLYFISH_TOP)/tools/linux/kconfig/kconfig.mk
include $(FLYFISH_TOP)/tools/linux/adb/adb.mk

ifeq (${CFG_HI_NATIVE_HIGV_ENABLE},y)
include $(FLYFISH_TOP)/tools/linux/xml2bin/xml2bin.mk
endif
############################## tools compile end ###########################

############################## framwork compile before #####################
include $(FLYFISH_TOP)/framework/servicemanager/servicemanager.mk
include $(FLYFISH_TOP)/framework/appmanager/appmanager.mk
include $(FLYFISH_TOP)/framework/mediaservice/media.mk
include $(FLYFISH_TOP)/framework/resmanager/resmgr.mk
include $(FLYFISH_TOP)/framework/systemservice/systemservice.mk


ifeq (${CFG_HI_SERVICE_DTV_ENABLE},y)
include $(FLYFISH_TOP)/framework/dtvservice/dtvservice.mk
endif
############################## framwork compile end ########################

############################## test compile end ########################
ifeq (${CFG_HI_TEST_CASE_SUPPORT},y)
include $(FLYFISH_TOP)/base/ipc/test/testCase1/test1.mk
include $(FLYFISH_TOP)/component/db/st/tmss_test/module-test/db_sttest.mk
include $(FLYFISH_TOP)/framework/appmanager/test/apmtest.mk
include $(FLYFISH_TOP)/framework/appmanager/test/st/apm_st.mk
include $(FLYFISH_TOP)/framework/mediaservice/test/mediatest.mk
include $(FLYFISH_TOP)/framework/systemservice/test/systemservicetest.mk
include $(FLYFISH_TOP)/framework/systemservice/test/mountservice_ST/mountservice_st.mk
include $(FLYFISH_TOP)/framework/systemservice/test/systemservice_ST/systemservice_st.mk
include $(FLYFISH_TOP)/framework/mediaservice/test/mediatest.mk
endif
include $(FLYFISH_TOP)/framework/mediaservice/test/mediatest.mk

ifeq (${CFG_HI_PPPOE_SUPPORT},y)
include $(FLYFISH_TOP)/external/pppd/pppd.mk
include $(FLYFISH_TOP)/external/rp-pppoe/pppoe.mk
endif

ifeq (${CFG_HI_SERVICE_DTV_ENABLE},y)
include $(FLYFISH_TOP)/project/test/testself/dtv/dtvservicetest.mk
endif
############################## test compile end ########################

############################## project compile before #####################
ifeq (${CFG_HI_NATIVE_HIGV_ENABLE},y)
include $(FLYFISH_TOP)/project/launcher/launcher.mk
include $(FLYFISH_TOP)/project/mediacenter/mediacenter.mk
include $(FLYFISH_TOP)/project/setting/setting.mk
include $(FLYFISH_TOP)/project/sysui/sysui.mk

ifeq (${CFG_HI_NATIVE_BROWSER_ENABLE},y)
include $(FLYFISH_TOP)/project/browser_ui/browser_ui.mk
endif

ifeq (${CFG_HI_SERVICE_DTV_ENABLE},y)
include $(FLYFISH_TOP)/project/dtv/dtv.mk
endif
endif
############################## project compile end ########################

ifeq (${CFG_HI_NATIVE_BROWSER_ENABLE},y)
include $(FLYFISH_TOP)/project/browser_full/browser_full.mk
ifeq (${CFG_HI_TEST_CASE_SUPPORT},y)
include $(FLYFISH_TOP)/external/browser/test/browser_sttest.mk
endif
endif
