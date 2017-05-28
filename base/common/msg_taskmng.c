/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_taskmng.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/23
  Last Modified :
  Description   : 消息任务表管理
  Function List :
              TASK_TAB_Add
              TASK_TAB_Del
              TASK_TAB_GetNum
              TASK_TAB_GetStat
              TASK_TAB_GetTaskList
              TASK_TAB_Id2Name
              TASK_TAB_IsExist
              TASK_TAB_Name2Id
              TASK_TAB_StatRcv
              TASK_TAB_StatSF
              TASK_TAB_StatSS
  History       :
  1.Date        : 2008/1/23
    Author      : z42136
    Modification: Created file

******************************************************************************/

#include "common/hi_adp_mutex.h"

#include "common/msg_interface.h"

#include "msg_log.h"
#include "msg_taskmng.h"
#include "msg_buffer.h"

typedef struct hiTASK_TAB_S
{
    VTOP_MSG_TASKNAME taskName;    /* 消息任务名 */
    VTOP_TASK_ID taskId;        /* 消息任务ID */

    TASK_OWNER_E    owner;      /* 消息任务的属主信息*/

    VTOP_MSG_TYPE   msgtype;    /* 合并的消息类型，如果为无效值不做消息合并 */

    MSG_QUEUE_HANDLE   handle;  /*任务对应的消息队列*/

}MSG_TAB_TASK_S;

static HI_ThreadMutex_S s_TaskTabLock = HI_MUTEX_INITIALIZER;

#define TASKTABLock()    \
    HI_MutexLock(&s_TaskTabLock)

#define TASKTABunLock()    \
    HI_MutexUnLock(&s_TaskTabLock)

static MSG_TAB_TASK_S * g_TaskTabAddr = NULL;
static MSG_TAB_TASK_S * TASKTabGetAddr(void)
{
    MSG_SHM_S * pMsgShm = NULL;

    if(g_TaskTabAddr)
        return g_TaskTabAddr;

    pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsgShm, NULL, 0);

    g_TaskTabAddr = (void *)((unsigned int)pMsgShm + pMsgShm->taskTabPos);

#if 0
    SVR_LOG_INFO("pMsgShm=%p, pMsgShm->taskTabPos=%x, g_TaskTabAddr=%p\n",
            pMsgShm, pMsgShm->taskTabPos, g_TaskTabAddr);
#endif

    return g_TaskTabAddr;
}

int TASKGetAddr(MSG_SHM_S ** pMsgShm, MSG_TAB_TASK_S ** pTaskTab)
{
    *pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, *pMsgShm, NULL, VTOP_MSG_ERR_SHMATTCH);

    *pTaskTab = TASKTabGetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, *pTaskTab, NULL, VTOP_MSG_ERR_SHMATTCH);

    return 0;
}

unsigned int TASK_TAB_CalcSize(unsigned int maxTask)
{
    if(maxTask == 0)
    {
        return 0;
    }

    return (maxTask * sizeof(MSG_TAB_TASK_S));
}

static void TASKTABInitUnit(MSG_TAB_TASK_S * pUnit)
{
    memset(pUnit, 0, sizeof(MSG_TAB_TASK_S));
    pUnit->taskId = VTOP_INVALID_TASK_ID;
    pUnit->handle = MSG_QUEUE_INVALID_HANDLE;

    return;
}

//int TASK_TAB_Init(unsigned int maxTask, void * pTaskTabAddr)
int TASK_TAB_Init(void)
{
    int ret;
    unsigned int index;

    MSG_TAB_TASK_S * pTaskTab = NULL;
    MSG_SHM_S * pMsgShm = NULL;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_MSG_ERR_SHMATTCH);

    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        TASKTABInitUnit(&pTaskTab[index]);
    }

    //ret = SEM_Init(VTOP_MSG_SEM_TASK_KEY);
    ret = HI_MutexInit(&s_TaskTabLock, NULL);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return 0;
}

int TASK_TAB_DeInit(void)
{
    int ret;
    unsigned int maxTask;
    unsigned int index;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    ret = TASK_TAB_GetMaxTaskNum(&maxTask);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    pTaskTab = TASKTabGetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pTaskTab, NULL, VTOP_MSG_ERR_SHMATTCH);

    g_TaskTabAddr = NULL;

    for ( index = 0 ; index < maxTask ; index++ )
    {
        TASKTABInitUnit(&pTaskTab[index]);
    }

    //SEM_DeInit(VTOP_MSG_SEM_TASK_KEY);
    HI_MutexDestroy(&s_TaskTabLock);

    return 0;
}

/*****************************************************************************
 Prototype    : TASK_TAB_GetTaskList
 Description  : 获取任务表中所有的消息任务ID列表
 Input        : VTOP_TASK_ID *taskIdList
                int *pNum
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
int TASK_TAB_GetTaskList(VTOP_TASK_ID *taskIdList, unsigned int *pNum)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    unsigned int taskNum = 0;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = TASKTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    for ( index = 0 ; index < pMsgShm->maxTaskNum ; index++ )
    {
        if(pTaskTab[index].taskId != VTOP_INVALID_TASK_ID)
        {
        taskIdList[taskNum++] = pTaskTab[index].taskId;
        }

        if(taskNum >= *pNum)
        {
        break;
        }
    }

    *pNum = taskNum;

    ret = TASKTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);
    return 0;
}

/*****************************************************************************
 Prototype    : TASK_TAB_Add
 Description  : 向共享内存任务表中添加一个消息任务
 Input        : VTOP_MSG_TASKNAME taskName
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
VTOP_TASK_ID TASK_TAB_Add(const VTOP_MSG_TASKNAME taskName)
{
#if 0
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    ret = TASKTABLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == VTOP_INVALID_TASK_ID)
        {
        pTaskTab[index].taskId = pMsgShm->counter_taskId;
            if(pMsgShm->counter_taskId >= VTOP_MAX_TASK_ID)
            {
               pMsgShm->counter_taskId = 0;
            }
            else
            {
               pMsgShm->counter_taskId ++;
            }

        strncpy(pTaskTab[index].taskName, taskName, sizeof(VTOP_MSG_TASKNAME)-1);
            pTaskTab[index].owner = TASK_OWNER_TASK;

        ret = TASKTABunLock();
        ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

        return pTaskTab[index].taskId;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    return VTOP_INVALID_TASK_ID;
#endif

    return TASK_TAB_Add2(taskName, TASK_OWNER_TASK);
}

/*****************************************************************************
 Prototype    : TASK_TAB_Del
 Description  : 从共享内存任务表中删除一个消息任务
 Input        : VTOP_TASK_ID taskId
 Output       : None
 Return Value : 加入成功的话，就返回消息任务的句柄; 否则返回NULL
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
int TASK_TAB_Del(VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = TASKTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);
    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
            MSG_QueueDeinit(pTaskTab[index].handle);
        memset((void *)&pTaskTab[index], 0, sizeof(MSG_TAB_TASK_S));
        pTaskTab[index].taskId = VTOP_INVALID_TASK_ID;
            pTaskTab[index].handle = MSG_QUEUE_INVALID_HANDLE;

        ret |= TASKTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return 0;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return VTOP_MSG_ERR_TASKNOTEXIST;
}

/*****************************************************************************
 Prototype    : TASK_TAB_Id2Name
 Description  : 通过消息任务名字获取消息任务的ID
 Input        : VTOP_TASK_ID taskId
 Output       : VTOP_MSG_TASKNAME taskName
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
int TASK_TAB_Id2Name(VTOP_TASK_ID taskId, VTOP_MSG_TASKNAME taskName)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = TASKTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);
    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
        strncpy(taskName, pTaskTab[index].taskName, sizeof(VTOP_MSG_TASKNAME));
        taskName[sizeof(VTOP_MSG_TASKNAME)-1] = '\0';
        ret = TASKTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);
        return 0;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return VTOP_MSG_ERR_TASKNOTEXIST;
}

/*****************************************************************************
 Prototype    : TASK_TAB_Name2Id
 Description  : 通过消息任务ID来获取消息任务的名字
 Input        : VTOP_MSG_TASKNAME taskName
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
VTOP_TASK_ID TASK_TAB_Name2Id(const VTOP_MSG_TASKNAME taskName)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    VTOP_TASK_ID ret_id;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    ret = TASKTABLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);
    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(strncmp(pTaskTab[index].taskName, taskName, VTOP_MSG_MAX_TASK_NAME_LEN) == 0)
        {
        ret_id = pTaskTab[index].taskId;

        ret = TASKTABunLock();
        ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

        return ret_id;
        }
    }
    ret = TASKTABunLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    return VTOP_INVALID_TASK_ID;
}

/*****************************************************************************
 Prototype    : TASK_TAB_IsTaskExist
 Description  : 判断给定名字的消息任务是否存在
 Input        : VTOP_MSG_TASKNAME taskName
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
HI_BOOL TASK_TAB_IsTaskExist(const VTOP_MSG_TASKNAME taskName)
{
    if(TASK_TAB_Name2Id(taskName) == VTOP_INVALID_TASK_ID)
    {
        return HI_FALSE;
    }
    else
    {
        return HI_TRUE;
    }
}

/*****************************************************************************
 Prototype    : TASK_TAB_IsTaskIdExist
 Description  : 判断给定Id的消息任务是否存在
 Input        : VTOP_MSG_TASKNAME taskName
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2008/1/23
    Author       : z42136
    Modification : Created function

*****************************************************************************/
HI_BOOL TASK_TAB_IsTaskIdExist(VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;
    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    ret = TASKTABLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
        ret = TASKTABunLock();
        ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

        return HI_TRUE;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    return HI_FALSE;
}

/*****************************************************************************
 Prototype       : TASK_TAB_Add2
 Description     : 选择任务类型加入任务表
 Input           : taskName  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/3/4
    Author       : qushen
    Modification : Created function

*****************************************************************************/
VTOP_TASK_ID TASK_TAB_Add2(const VTOP_MSG_TASKNAME taskName, TASK_OWNER_E owner)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    ret = TASKTABLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == VTOP_INVALID_TASK_ID)
        {
        pTaskTab[index].taskId = pMsgShm->counter_taskId;
            if(pMsgShm->counter_taskId >= VTOP_MAX_TASK_ID)
            {
               pMsgShm->counter_taskId = 0;
            }
            else
            {
               pMsgShm->counter_taskId ++;
            }
            pTaskTab[index].msgtype = VTOP_INVALID_MSG_TYPE;
            strncpy(pTaskTab[index].taskName, taskName, sizeof(VTOP_MSG_TASKNAME)-1);
            pTaskTab[index].taskName[sizeof(VTOP_MSG_TASKNAME)-1] = '\0';
            pTaskTab[index].owner = owner;
            #define MSG_DEFAULT_QUEUE_SIZE (32*1024)
            ret = MSG_QueueInit(pTaskTab[index].taskName, MSG_DEFAULT_QUEUE_SIZE, &pTaskTab[index].handle);

        ret |= TASKTABunLock();
        ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

        return pTaskTab[index].taskId;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, VTOP_INVALID_TASK_ID);

    return VTOP_INVALID_TASK_ID;
}

/*****************************************************************************
 Prototype       : TASK_TAB_GetHandle
 Description     : 获取消息ID对应的消息队列句柄
 Input           : taskId  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
MSG_QUEUE_HANDLE TASK_TAB_GetHandle(VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;
    unsigned int index;
    MSG_QUEUE_HANDLE handle = MSG_QUEUE_INVALID_HANDLE;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    ret = TASKTABLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
            handle = pTaskTab[index].handle;
        ret = TASKTABunLock();
        ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

        return handle;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS_RETURN(LOG_MODULE, 0, ret, HI_FALSE);

    return MSG_QUEUE_INVALID_HANDLE;
}

int TASK_TAB_SetMerge(VTOP_TASK_ID taskId, VTOP_MSG_TYPE msgtype)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = TASKTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);
    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
            pTaskTab[index].msgtype = msgtype;

        ret = TASKTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);
        return 0;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return VTOP_MSG_ERR_TASKNOTEXIST;
}

VTOP_MSG_TYPE TASK_TAB_GetMerge(VTOP_TASK_ID taskId)
{
    int ret;
    MSG_SHM_S * pMsgShm = NULL;
    MSG_TAB_TASK_S * pTaskTab = NULL;
    VTOP_MSG_TYPE msgtype;

    unsigned int index;

    ret = TASKGetAddr(&pMsgShm, &pTaskTab);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    ret = TASKTABLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);
    for ( index = 0 ; index < pMsgShm->maxTaskNum; index++ )
    {
        if(pTaskTab[index].taskId == taskId)
        {
            msgtype = pTaskTab[index].msgtype;
        ret = TASKTABunLock();
        ASSERT_SUCCESS(LOG_MODULE, 0, ret);

        return msgtype;
        }
    }

    ret = TASKTABunLock();
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    return VTOP_MSG_ERR_TASKNOTEXIST;
}
