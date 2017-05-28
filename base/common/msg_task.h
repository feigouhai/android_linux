/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_task.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/2/24
  Last Modified :
  Description   : msg_task.c header file
  Function List :
  History       :
  1.Date        : 2008/2/24
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifndef __MSG_TASK_H__
#define __MSG_TASK_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "linux_list.h"

/* 用于将消息任务的pMsgHandler、pTaskLog保存到DS中 */
typedef struct
{
    VTOP_TASK_ID            taskId;
    PFUNC_VTOP_MSG_Handler     pMsgHandler;
    PFUNC_VTOP_MSG_Log         pTaskLog;

    int  Sndfd;
    int  Rcvfd;

    struct list_head list;
}MSGTASK_DATA;

extern int addTaskData(MSGTASK_DATA * pTaskData);
extern int delTaskData(VTOP_TASK_ID taskId);
extern MSGTASK_DATA * getTaskData(VTOP_TASK_ID     taskId);

int  VTOP_MSG_RegisterX(
        const VTOP_MSG_TASKNAME taskName,
        PFUNC_VTOP_MSG_Handler pMsgHandler,
        PFUNC_VTOP_MSG_Log  pTaskLog,
        VTOP_TASK_ID * pTaskId,
        TASK_OWNER_E owner);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MSG_TASK_H__ */
