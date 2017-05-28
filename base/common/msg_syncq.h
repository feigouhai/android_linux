/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_syncq.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/04/03
  Description   : msg_syncq.c header file
  History       :
  1.Date        : 2008/04/03
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_SYNCQ_H__
#define __MSG_SYNCQ_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "msg_misc.h"
#include "msg_queue.h"

int CONNQ_Sync_Add(int fd, VTOP_MSG_ID msgId);
void CONNQ_Sync_Deinit(void);
int CONNQ_Sync_Del(int fd);
void CONNQ_Sync_DelAll(void);
int CONNQ_Sync_Init(size_t conn_num);
int CONNQ_Sync_MsgId2Fd(VTOP_MSG_ID msgId);
void CONNQ_Sync_ShowState(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_SYNCQ_H__ */
