/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_submng.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/29
  Last Modified :
  Description   : 订阅表数据表管理
  Function List :
              SUBTABCalcUnitSize
              SUBTabGetAddr
              SUBTABInitUnit
              SUBTABLock
              SUBTABunLock
              SUB_TAB_Add
              SUB_TAB_CalcSize
              SUB_TAB_DeInit
              SUB_TAB_Del
              SUB_TAB_GetMaxSubNum
              SUB_TAB_GetNum
              SUB_TAB_Init
              SUB_TAB_IsExist
              SUB_TAB_Sub2Task
              SUB_TAB_Task2Sub
  History       :
  1.Date        : 2008/1/29
    Author      : z42136
    Modification: Created file

******************************************************************************/

#include "common/hi_adp_mutex.h"

#include "common/msg_interface.h"
#include "msg_misc.h"

#include "msg_log.h"

#include "msg_submng.h"
#include "msg_taskmng.h"

typedef struct hiSUB_TAB_S
{
    VTOP_MSG_TYPE msgType;          /* 订阅的消息类型 */
    VTOP_TASK_ID *taskIdList;

    // VTOP_TASK_ID array[maxTaskNumInSub]
} MSG_TAB_SUB_S;

static HI_ThreadMutex_S s_SubTabLock = HI_MUTEX_INITIALIZER;

#define SUBTABLock()         \
    HI_MutexLock(&s_SubTabLock)

#define SUBTABunLock()    \
    HI_MutexUnLock(&s_SubTabLock)

#define SUBUnit(subTab, unitIndex, taskInSub)    \
    (MSG_TAB_SUB_S *)((unsigned int)subTab + unitIndex*SUBTABCalcUnitSize(taskInSub))

int SUBGetAddr(MSG_SHM_S ** pMsgShm, MSG_TAB_SUB_S ** pSubTab)
{
    *pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, *pMsgShm, NULL, VTOP_MSG_ERR_SHMATTCH);

    *pSubTab = (MSG_TAB_SUB_S *)((unsigned int)(*pMsgShm) + (*pMsgShm)->subTabPos);

    return 0;
}

static unsigned int SUBTABCalcUnitSize(unsigned int maxTaskInSub)
{
    return ( sizeof(VTOP_MSG_TYPE) +         // msgType
             sizeof(VTOP_TASK_ID *) +        // taskIdList
             sizeof(VTOP_TASK_ID)*maxTaskInSub);    // taskId array
}

unsigned int SUB_TAB_CalcSize(unsigned int maxSub, unsigned int maxTaskInSub)
{
    if((maxSub == 0) || (maxTaskInSub == 0))
    {
        MSG_INFO(LOG_MODULE, 0, "maxSub and maxTaskInSub must be larger than zero!");
        return 0;
    }

    return (maxSub * SUBTABCalcUnitSize(maxTaskInSub));
}

static void SUBTABInitUnit(MSG_TAB_SUB_S * pUnit, unsigned int maxTaskInSub)
{
    unsigned int index;

    pUnit->msgType = VTOP_INVALID_MSG_TYPE;
    pUnit->taskIdList = (VTOP_TASK_ID *)((unsigned int)pUnit + sizeof(VTOP_MSG_TYPE) + sizeof(VTOP_TASK_ID *));

    for ( index = 0 ; index < maxTaskInSub ; index++ )
    {
        pUnit->taskIdList[index] = VTOP_INVALID_TASK_ID;
    }

    return;
}

int SUB_TAB_Init(void)
{
    int ret;
    unsigned int index;

    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_NOTSMALL(LOG_MODULE, 0, ret, 0, VTOP_MSG_ERR_SHMATTCH);

    for ( index = 0 ; index < pMsgShm->maxSubNum; index++ )
    {
        pUnit = SUBUnit(pSubTab, index, pMsgShm->maxTaskNumInSub);
        SUBTABInitUnit(pUnit, pMsgShm->maxTaskNumInSub);
    }

    //ret = SEM_Init(VTOP_MSG_SEM_SUB_KEY);
    ret = HI_MutexInit(&s_SubTabLock, NULL);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return 0;
}

int SUB_TAB_DeInit(void)
{
    int ret;
    unsigned int maxSub, maxTaskInSub;
    unsigned int index;

    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_SUB_S * pSubTab = NULL;

    ret = SUB_TAB_GetMaxSubNum(&maxSub, &maxTaskInSub);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    for ( index = 0 ; index < maxSub ; index++ )
    {
        SUBTABInitUnit(&pSubTab[index], maxTaskInSub);
    }

    //SEM_DeInit(VTOP_MSG_SEM_SUB_KEY);
    HI_MutexDestroy(&s_SubTabLock);

    return 0;
}

#if 0
int SUB_TAB_GetNum(void)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;

    unsigned int index;
    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;

    int subNum = 0;

    pMsgShm = SHM_GetAddr();
    ASSERT_RETURN(pMsgShm!=NULL, VTOP_MSG_ERR_SHMATTCH);

    pSubTab = SUBTabGetAddr();
    ASSERT_RETURN(pSubTab!=NULL, VTOP_MSG_ERR_SHMATTCH);

    ret = SUBTABLock();
    ASSERT_SUCCESS(ret);

    for ( index = 0 ; index < pMsgShm->maxSubNum; index++ )
    {
        pUnit = SUBUnit(pSubTab, index, pMsgShm->maxTaskNumInSub);
        if(pUnit->msgType != VTOP_INVALID_MSG_TYPE)
        {
        subNum += 1;
        }
    }

    ret = SUBTABunLock();
    ASSERT_SUCCESS(ret);

    return subNum;
}
#endif

int SUB_TAB_Add(VTOP_MSG_TYPE msgType, VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_SUB_S * pSubTab = NULL;

    unsigned int index, unitIndex = 0;
    MSG_TAB_SUB_S * pUnit = NULL;

    HI_BOOL bSubExist = HI_FALSE;
    HI_BOOL bTaskExist = HI_FALSE;

    bTaskExist = TASK_TAB_IsTaskIdExist(taskId);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, bTaskExist, HI_FALSE, VTOP_MSG_ERR_TASKNOTEXIST);

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    bSubExist = SUB_TAB_IsExist(msgType, &unitIndex);

    ret = SUBTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    if(bSubExist == HI_TRUE)
    {
        /* 订阅表已经存在 */

        /* 检查task是否已经存在 */
        pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);
        for ( index = 0 ; index < pMsgShm->maxTaskNumInSub; index++ )
        {
            if(pUnit->taskIdList[index] == taskId)
        {
        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return VTOP_MSG_ERR_SUBEXIST;
        }
        }

        /* 加入到订阅表 */
        for ( index = 0 ; index < pMsgShm->maxTaskNumInSub; index++ )
        {
            if(pUnit->taskIdList[index] == VTOP_INVALID_TASK_ID)
        {
        pUnit->taskIdList[index] = taskId;
        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return 0;
        }
        }

        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        // err: 订阅表已经满了, BIGBUG
        return VTOP_MSG_ERR_SUBMEMBERFULL;
    }
    else
    {
        /* 订阅表不存在 */
        for ( unitIndex = 0 ; unitIndex < pMsgShm->maxSubNum; unitIndex++ )
        {
        pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);
            if(pUnit->msgType == VTOP_INVALID_MSG_TYPE)
        {
        pUnit->msgType = msgType;
        pUnit->taskIdList[0] = taskId;

        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return 0;
        }
        }

        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return VTOP_MSG_ERR_SUBFULL;
    }

    /* code can't reach here */
    //ret = SUBTABunLock();
    //ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    //return -1;
}

int SUB_TAB_Del(VTOP_MSG_TYPE msgType, VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;

    unsigned int index, unitIndex = 0;
    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;

    HI_BOOL bSubExist = HI_FALSE;
    HI_BOOL bTaskExist = HI_FALSE;

    bTaskExist = TASK_TAB_IsTaskIdExist(taskId);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, bTaskExist, HI_FALSE, VTOP_MSG_ERR_TASKNOTEXIST);

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    bSubExist = SUB_TAB_IsExist(msgType, &unitIndex);
    ASSERT_EQUAL(LOG_MODULE, 0, bSubExist, HI_TRUE, VTOP_MSG_ERR_SUBNOTEXIST);

    ret = SUBTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);

    /* 在订阅表中删除该任务 */
    for ( index = 0 ; index < pMsgShm->maxTaskNumInSub; index++ )
    {
        if(pUnit->taskIdList[index] == taskId)
        {
        pUnit->taskIdList[index] = VTOP_INVALID_TASK_ID;
        break;
        }
    }
    if(index >= pMsgShm->maxTaskNumInSub)
    {
        /* 没有发现该任务 */
        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return VTOP_MSG_ERR_SUBNOTEXIST;
    }

    /* 检查该订阅表中是否还有消息任务，如果没有则删除该订阅表 */
    for ( index = 0 ; index < pMsgShm->maxTaskNumInSub; index++ )
    {
        if(pUnit->taskIdList[index] != VTOP_INVALID_TASK_ID)
        {
        break;
        }
    }
    if ( index >= pMsgShm->maxTaskNumInSub )
    {
        /* 订阅表中没有消息任务了 */
        pUnit->msgType = VTOP_INVALID_MSG_TYPE;
    }

    ret = SUBTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return 0;
}

int SUB_TAB_Sub2Task(
        VTOP_MSG_TYPE msgType,
        VTOP_TASK_ID *taskIdList,
        unsigned int *pNum)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;

    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;
    unsigned int index, unitIndex = 0;
    HI_BOOL bSubExist = HI_FALSE;

    unsigned int taskNum = 0;

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    bSubExist = SUB_TAB_IsExist(msgType, &unitIndex);
    ASSERT_EQUAL(LOG_MODULE, 0, bSubExist, HI_TRUE, VTOP_MSG_ERR_SUBNOTEXIST);

    ret = SUBTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);
    for ( index = 0; index < pMsgShm->maxTaskNumInSub; index++ )
    {
        if(pUnit->taskIdList[index] != VTOP_INVALID_TASK_ID)
        {
        taskIdList[taskNum++] = pUnit->taskIdList[index];
        }

        if(taskNum >= *pNum)
        {
        break;
        }
    }

    *pNum = taskNum;

    ret = SUBTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return 0;
}

int SUB_TAB_Task2Sub(
        VTOP_TASK_ID taskId,
        VTOP_MSG_TYPE * msgTypeList,
        unsigned int *pNum)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;

    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;
    unsigned int index, unitIndex;

    unsigned int subNum = 0;
    HI_BOOL bTaskExist = HI_FALSE;

    bTaskExist = TASK_TAB_IsTaskIdExist(taskId);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, bTaskExist, HI_FALSE, VTOP_MSG_ERR_TASKNOTEXIST);

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = SUBTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    for ( unitIndex = 0 ; unitIndex < pMsgShm->maxSubNum; unitIndex++ )
    {
        pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);
        if(pUnit->msgType != VTOP_INVALID_MSG_TYPE)
        {
        for ( index = 0; index < pMsgShm->maxTaskNumInSub; index++ )
        {
            if(pUnit->taskIdList[index] == taskId)
        {
        msgTypeList[subNum++] = pUnit->msgType;
        }

        if(subNum >= *pNum)
        {
        goto break_search;
        }
        }
        }
    }

break_search:
    *pNum = subNum;

    ret = SUBTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return 0;
}

HI_BOOL SUB_TAB_IsExist(VTOP_MSG_TYPE msgType, unsigned int * pIndex)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;

    unsigned int unitIndex;
    MSG_TAB_SUB_S * pSubTab = NULL;
    MSG_TAB_SUB_S * pUnit = NULL;

    ret = SUBGetAddr(&pMsgShm, &pSubTab);
    ASSERT_NOTSMALL(LOG_MODULE, 0, ret, 0, HI_FALSE);

    ret = SUBTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    for ( unitIndex = 0 ; unitIndex < pMsgShm->maxSubNum; unitIndex++ )
    {
        pUnit = SUBUnit(pSubTab, unitIndex, pMsgShm->maxTaskNumInSub);
        if(pUnit->msgType == msgType)
        {
        *pIndex = unitIndex;
        ret = SUBTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return HI_TRUE;
        }
    }

    ret = SUBTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return HI_FALSE;
}
