/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_taskmng.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/24
  Last Modified :
  Description   : msg_taskmng.c header file
  Function List :
  History       :
  1.Date        : 2008/1/24
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifndef __MSG_TASKMNG_H__
#define __MSG_TASKMNG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "msg_queue.h"

HI_BOOL TASK_TAB_IsTraceTask( VTOP_TASK_ID taskId );

VTOP_TASK_ID TASK_TAB_Add2(const VTOP_MSG_TASKNAME taskName, TASK_OWNER_E owner);

extern int TASK_TAB_Init(void);
extern int TASK_TAB_DeInit(void);
extern unsigned int TASK_TAB_CalcSize(unsigned int maxTask);

extern VTOP_TASK_ID TASK_TAB_Add(const VTOP_MSG_TASKNAME taskName);
extern int TASK_TAB_Del(VTOP_TASK_ID taskId);

extern int TASK_TAB_GetTaskList(VTOP_TASK_ID *taskIdList, unsigned int *pNum);

extern HI_BOOL TASK_TAB_IsTaskExist(const VTOP_MSG_TASKNAME taskName);
extern HI_BOOL TASK_TAB_IsTaskIdExist(VTOP_TASK_ID taskId);

extern int TASK_TAB_Id2Name(VTOP_TASK_ID taskId, VTOP_MSG_TASKNAME taskName);
extern VTOP_TASK_ID TASK_TAB_Name2Id(const VTOP_MSG_TASKNAME taskName);

MSG_QUEUE_HANDLE TASK_TAB_GetHandle(VTOP_TASK_ID taskId);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MSG_TASKMNG_H__ */
