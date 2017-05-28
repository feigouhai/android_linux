/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_buffer.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/30
  Description   : msg_buffer.c header file
  History       :
  1.Date        : 2008/01/30
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_BUFFER_H__
#define __MSG_BUFFER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#include "msg_queue.h"
#include "linux_list.h"

typedef struct hiQNODE_S
{
    void *priv_data;
    struct list_head list;
} QNODE_S;

typedef struct hiMB_HEAD_S
{
    int  flag;   /*used flag 1: used, 0: unused*/
    unsigned int buffer_id;      /*msg buffer id*/
    unsigned int block_id;      /*msg buffer block id*/
    QNODE_S node;
} MB_HEAD_S;

#define MB_HEAD_LEN sizeof(MB_HEAD_S)

#define MB_NAME_SIZE    16

typedef size_t VTOP_MSG_BLKSIZETAB[VTOP_MSG_BL_BUTT];
typedef char MSG_BUFFER_NAME[MB_NAME_SIZE];

typedef struct
{
    VTOP_MSG_BLOCKNUM   blockNum;
    VTOP_MSG_BLKSIZETAB blockSize;
    const char *name;
}VTOP_MSG_S_Buffer;

typedef void* MB_HANDLE;

void*  MB_Alloc( MB_HANDLE handle, size_t size );
void MB_Deinit( MB_HANDLE handle );
void MB_Free( MB_HANDLE handle, void *p );
QNODE_S* MB_GetQNode(unsigned char *pMbBody);
unsigned int MB_GetTotalNum( MB_HANDLE handle );
unsigned int MB_GetTotalSize( MB_HANDLE handle );
unsigned int MB_GetTotalUsed( MB_HANDLE handle );
unsigned int MB_GetUsed( MB_HANDLE handle, unsigned int buffer_id );
int MB_Init( MB_HANDLE *pHandle, VTOP_MSG_S_Buffer *pBuffer);
void MB_ShowState(MB_HANDLE handle);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_BUFFER_H__ */
