LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libcommon
LOCAL_SRC_FILES := svr_proc.c \
                   hi_time.c  \
                   hi_udisk.c \
                   scsiexe.c \
                   hi_timer.c \
                   peabstm.c \
                   pereltm.c \
                   petimer.c \
                   tsklet.c \
                   xmem.c

LOCAL_SRC_FILES += hi_adp_file.c \
                   hi_adp_memmng.c \
                   hi_adp_mutex.c \
                   hi_adp_sem.c \
                   hi_adp_string.c \
                   hi_adp_thread.c \
                   hi_adp_threadmng.c \
                   hi_adp_time.c

LOCAL_SRC_FILES += hi_ref_mod.c

LOCAL_SRC_FILES += hi_msg_queue.c \
                   msg_buffer.c \
                   msg_com.c \
                   msg_connq.c \
                   msg_log.c \
                   msg_misc.c \
                   msg_priorq.c \
                   msg_proc.c \
                   msg_queue.c \
                   msg_rspmng.c \
                   msg_socket.c \
                   msg_submng.c \
                   msg_syncq.c \
                   msg_task.c \
                   msg_taskmng.c

LOCAL_CFLAGS := -D_GNU_SOURCE

LOCAL_C_INCLUDES := $(FLYFISH_TOP)/base/include/utils
LOCAL_C_INCLUDES += $(FLYFISH_TOP)/base/include
LOCAL_C_INCLUDES += $(FLYFISH_TOP)/base/include/common
LOCAL_C_INCLUDES += $(FLYFISH_TOP)/target/include

include $(BUILD_SHARED_LIBRARY)
