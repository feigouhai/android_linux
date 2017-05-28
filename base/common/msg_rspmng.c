/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_rspmng.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/03/15
  Description   :
  History       :
  1.Date        : 2009/03/15
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include "hi_type.h"
#include "common/hi_adp_mutex.h"
#include "msg_rspmng.h"
#include "msg_log.h"

typedef enum hiMSG_RSPMNG_STATE_E
{
    MSG_RSPMNG_STATE_FREE = 0x0,          /* Filter已空闲           */
    MSG_RSPMNG_STATE_IDLE,                /* Filter已分配           */
    MSG_RSPMNG_STATE_ACTIVE,              /* Filter已使能           */
    MSG_RSPMNG_STATE_BUTT
} MSG_RSPMNG_STATE_E;

typedef struct hiMSG_RSPMNG_S
{
    HI_U8 buf[MAX_MSG_RSP_SIZE + MSG_INNER_HEAD_LEN];
    MSG_RSPMNG_STATE_E   enState;           /* 过滤器状态                 */
    HI_ThreadMutex_S lock;               /* Filter属性信息                   */
} MSG_RSPMNG_S;

static MSG_RSPMNG_S s_MsgRspMng[MAX_MSG_RSP_NUM];
static HI_BOOL s_bFilterInited = HI_FALSE;
static HI_ThreadMutex_S s_filterLock = HI_MUTEX_INITIALIZER;

#define FLTR_LOCK()     (void)HI_MutexLock(&s_filterLock)
#define FLTR_UNLOCK()   (void)HI_MutexUnLock(&s_filterLock)

#define FLTR_HANDLE_LOCK(handle)     (void)HI_MutexLock(&s_MsgRspMng[handle].lock)
#define FLTR_HANDLE_UNLOCK(handle)   (void)HI_MutexUnLock(&s_MsgRspMng[handle].lock)

#define FLTR_CHECK_INITED() do{ \
    if ( s_bFilterInited == HI_FALSE )    \
    {\
        return HI_FAILURE;\
    }\
}while(0)

#define FLTR_CHECK_HANDLE_VALID(handle) do{\
        if((handle) < 0 || (handle) >= MAX_MSG_RSP_NUM)\
        {\
            return HI_FAILURE;\
        }\
    }while(0)

#define FLTR_CHECK_PARAM_VALID(exp) do{\
        if(!(exp))\
        {\
            return HI_FAILURE;\
        }\
    }while(0)

#define FLTR_IsStateFree(handle)   (s_MsgRspMng[handle].enState == MSG_RSPMNG_STATE_FREE)
#define FLTR_IsStateIdle(handle)   (s_MsgRspMng[handle].enState == MSG_RSPMNG_STATE_IDLE)
#define FLTR_IsStateActive(handle) (s_MsgRspMng[handle].enState == MSG_RSPMNG_STATE_ACTIVE)

#define FLTR_CHECK_FREE(handle) do{\
    if ( FLTR_IsStateFree(handle) ) \
    {\
        return HI_FAILURE;\
    }\
}while(0)

/*****************************************************************************
 Prototype       : MSG_RSPMNG_Init
 Description     : ...
 Input           : HI_VOID
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_RSPMNG_Init( HI_VOID )
{
    HI_S32 i;

    if ( s_bFilterInited == HI_TRUE )
    {
        return HI_SUCCESS;
    }
    for ( i = 0 ; i < MAX_MSG_RSP_NUM ; i++ )
    {
        memset(&s_MsgRspMng[i], 0, sizeof(s_MsgRspMng[i]));
        s_MsgRspMng[i].enState = MSG_RSPMNG_STATE_FREE;
        HI_MutexInit(&s_MsgRspMng[i].lock, NULL);
    }
    s_bFilterInited = HI_TRUE;
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_Deinit
 Description     : ...
 Input           : HI_VOID
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_VOID MSG_RSPMNG_Deinit( HI_VOID )
{
    if ( s_bFilterInited == HI_FALSE )
    {
        return ;
    }
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_New
 Description     : ...
 Input           : None
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
HI_S32  MSG_RSPMNG_New( MSG_RSPMNG_HANDLE *pHandle )
{
    HI_S32 i;
    ASSERT_NOT_EQUAL(LOG_MODULE, VTOP_INVALID_TASK_ID, pHandle, NULL, HI_FAILURE);

    /*初始化状态检查*/
    FLTR_CHECK_INITED();

    FLTR_LOCK();
    for ( i = 0 ; i < MAX_MSG_RSP_NUM ; i++ )
    {
        if ( FLTR_IsStateFree(i) )
        {
            s_MsgRspMng[i].enState = MSG_RSPMNG_STATE_IDLE;
            *pHandle = i;
            FLTR_UNLOCK();
            return HI_SUCCESS;
        }
    }
    FLTR_UNLOCK();
    return HI_FAILURE;
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_Delete
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_RSPMNG_Delete( MSG_RSPMNG_HANDLE handle )
{
    /*初始化状态检查*/
    FLTR_CHECK_INITED();

    FLTR_CHECK_HANDLE_VALID(handle);

    FLTR_HANDLE_LOCK(handle);

    s_MsgRspMng[handle].enState = MSG_RSPMNG_STATE_FREE;

    FLTR_HANDLE_UNLOCK(handle);

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_GetState
 Description     : ...
 Input           : ...
 Output          : None
 Return Value    : MSG_RSPMNG_STATE_E
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
MSG_RSPMNG_STATE_E MSG_RSPMNG_GetState( MSG_RSPMNG_HANDLE handle )
{
    MSG_RSPMNG_STATE_E state;

    // 内部接口不做检查
    /*初始化状态检查*/
    //FLTR_CHECK_INITED();
    //FLTR_CHECK_HANDLE_VALID(handle);

    FLTR_HANDLE_LOCK(handle);

    state = s_MsgRspMng[handle].enState;

    FLTR_HANDLE_UNLOCK(handle);

    return state;
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_GetData
 Description     : ...
 Input           : None
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
HI_S32  MSG_RSPMNG_GetData( MSG_RSPMNG_HANDLE handle, VTOP_MSG_S_InnerBlock **ppInnerMsg )
{
    FLTR_CHECK_HANDLE_VALID(handle);

    FLTR_HANDLE_LOCK(handle);

    if ( !FLTR_IsStateActive(handle) )
    {
        FLTR_HANDLE_UNLOCK(handle);
        return HI_FAILURE;
    }

    *ppInnerMsg = (VTOP_MSG_S_InnerBlock*)s_MsgRspMng[handle].buf;

    FLTR_HANDLE_UNLOCK(handle);

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_RSPMNG_PutData
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/15
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_RSPMNG_PutData( MSG_RSPMNG_HANDLE handle, VTOP_MSG_S_InnerBlock *pInnerMsg)
{
    HI_U8 *pBuf = NULL;

    /*初始化状态检查*/
    FLTR_CHECK_INITED();

    FLTR_CHECK_HANDLE_VALID(handle);

    if ( pInnerMsg->msg.ulMsgLen > MAX_MSG_RSP_SIZE )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_MSGSIZE, "err Size:%u!", pInnerMsg->msg.ulMsgLen);
    }

    FLTR_HANDLE_LOCK(handle);

    if ( !FLTR_IsStateIdle(handle) )
    {
        MSG_ERROR(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "err state!");
        FLTR_HANDLE_UNLOCK(handle);
    }

    pBuf = s_MsgRspMng[handle].buf;
    MSG_COPY2BUF(pBuf, pInnerMsg);
    s_MsgRspMng[handle].enState = MSG_RSPMNG_STATE_ACTIVE;

    FLTR_HANDLE_UNLOCK(handle);
    return HI_SUCCESS;
}
