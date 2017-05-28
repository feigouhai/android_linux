/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_socket.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/22
  Description   : msg_socket.c header file
  History       :
  1.Date        : 2008/01/22
    Author      : qushen
    Modification: Created file

  2.Date        : 2008/02/04
    Author      : qushen
    Modification: change file name to msg_socket.h

******************************************************************************/

#ifndef __MSG_SOCKET_H__
#define __MSG_SOCKET_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/*Select 调度最小间隔时间10ms*/
#define MSG_SELECT_INTERVAL   10000

/*消息服务进程Select调度超时时间, 5000s (more than 1 hour)*/
#define MSG_SELECT_PROC   5000

/*消息服务进程接收消息超时时间(ms), 6s*/
#define MSG_PROC_RCV_TIMEOUT   6000

int TC_Bind(void);

int TC_Connect(void);
int TC_Send( int h, const unsigned char *pBuf, size_t len, int timeout);
int TC_Recv( int h, unsigned char *pBuf, size_t len, int timeout);
int TC_Close( int h);
int TC_SendBuffer( int fd, MSG_SND_BUF_S *pSendBuf, int timeout);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_COM_H__ */
