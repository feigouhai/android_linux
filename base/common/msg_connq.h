/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_connq.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/30
  Description   : msg_connq.c header file
  History       :
  1.Date        : 2008/01/30
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_CONNQ_Task_H__
#define __MSG_CONNQ_Task_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "msg_misc.h"
#include "msg_queue.h"

int CONNQ_Task_Add(int fd, VTOP_TASK_ID taskId);
void CONNQ_Task_Deinit(void);
int CONNQ_Task_Del(int fd);
void CONNQ_Task_DelAll(void);
int CONNQ_Task_Init(size_t conn_num);
int CONNQ_Task_Send(const VTOP_MSG_S_InnerBlock *pMsg);
void CONNQ_Task_SendAll(const VTOP_MSG_S_InnerBlock *pMsg);
void CONNQ_Task_ShowState(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_CONNQ_Task_H__ */
