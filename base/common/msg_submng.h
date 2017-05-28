/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_submng.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/24
  Last Modified :
  Description   : msg_submng.c header file
  Function List :
  History       :
  1.Date        : 2008/1/24
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifndef __MSG_SUBMNG_H__
#define __MSG_SUBMNG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern int SUB_TAB_Init(void);
extern int SUB_TAB_DeInit(void);
extern unsigned int SUB_TAB_CalcSize(unsigned int maxSub, unsigned int maxTaskInSub);

extern int SUB_TAB_Add(VTOP_MSG_TYPE msgType, VTOP_TASK_ID taskId);
extern int SUB_TAB_Del(VTOP_MSG_TYPE msgType, VTOP_TASK_ID taskId);

extern HI_BOOL SUB_TAB_IsExist(VTOP_MSG_TYPE msgType, unsigned int * pIndex);

extern int SUB_TAB_Sub2Task(
        VTOP_MSG_TYPE msgType,
        VTOP_TASK_ID *taskIdList,
        unsigned int *pNum);

extern int SUB_TAB_Task2Sub(
        VTOP_TASK_ID taskId,
        VTOP_MSG_TYPE * msgTypeList,
        unsigned int *pNum);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MSG_SUBMNG_H__ */
