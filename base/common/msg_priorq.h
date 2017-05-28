/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_priorq.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/26
  Description   : msg_priorq.c header file
  History       :
  1.Date        : 2008/01/26
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_PRIORQ_H__
#define __MSG_PRIORQ_H__

#include "msg_misc.h"
#include "msg_queue.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define VTOP_MSG_PRIO_MAX 5
#define VTOP_MSG_PRIO_RSP 4
#define VTOP_MSG_PRIO_SYN 3

void  PRIORQ_Deinit(void);
int PRIORQ_Get(VTOP_MSG_S_InnerBlock **ppMsg);
int PRIORQ_GetNum(void);
int PRIORQ_Init(VTOP_MSG_BLOCKNUM blockNum);
int PRIORQ_Put(const VTOP_MSG_S_InnerBlock *pMsg);
void  PRIORQ_ShowState(void);
size_t PRIORQ_GetMbNum(void);
void PRIORQ_Free(VTOP_MSG_S_InnerBlock *pMsg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_PRIORQ_H__ */
