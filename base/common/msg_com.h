/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : MSG_com.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/22
  Description   : MSG_Send.c header file
  History       :
  1.Date        : 2008/01/22
    Author      : qushen
    Modification: Created file

  2.Date        : 2008/02/04
    Author      : qushen
    Modification: change file name to msg_com.h

******************************************************************************/

#ifndef __MSG_COM_H__
#define __MSG_COM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "msg_misc.h"

int MSG_Send(int fd, VTOP_MSG_S_InnerBlock *pMsg, int timeout);
int MSG_Recv( int fd, unsigned char *pBuf, int timeout );

int MSG_RecvConnect( int *pFd, VTOP_TASK_ID taskId );
void  MSG_RecvDisconnect( int fd );
int MSG_SendAsyncMsg( VTOP_MSG_S_Block *pMsg, unsigned int asynMsgFlag );
int MSG_SendSyncMsg( VTOP_MSG_S_Block *pMsg, VTOP_MSG_S_Block** ppMsgResp, int timeout );
int MSG_SendSyncRsp( int fd,
                       VTOP_MSG_S_Block* pMsg, VTOP_MSG_S_Block *pMsgResp  );

HI_S32 MSG_PublishMsg( VTOP_MSG_S_Block *pMsg );

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_SEND_H__ */
