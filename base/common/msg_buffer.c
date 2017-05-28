/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_buffer.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/24
  Description   :
  History       :
  1.Date        : 2008/01/24
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "msg_misc.h"
#include "msg_queue.h"
#include "msg_buffer.h"
#include "msg_log.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define MSG_MAX_BLK_NUM  (16*1024)
#define MSG_MAX_BLK_SIZE (256*1024)

typedef struct hiMSG_BUFFER_S
{
    unsigned char  **pMsg;
    size_t blocksize;
    unsigned int blocknum;
    size_t buffersize; /*buffersize = blocksize * blocknum*/
    unsigned int used;
} MSG_BUFFER_S;

typedef struct hiMB_CONTEXT_S
{
    MSG_BUFFER_S mb[VTOP_MSG_BL_BUTT];
    size_t total_size;
    unsigned int total_num;
    unsigned int mb_num;
    unsigned char *s_pMsgBuf;
    MSG_BUFFER_NAME mbName;
} MB_CONTEXT_S;

#define MB_BLOCK_SIZE(level, pBlockSize) ( MB_HEAD_LEN + pBlockSize[level] )

/*消息缓冲buffer组的块单位大小，强制以升序排列，设置为0代表该缓冲buffer组不启用，
为0的无效缓冲组必须放在有效块大小之后, 有效的数据块必须连续(为了较少搜索次数)*/
/*加入buffer type connq， 为连接队列使用. 连接队列buffer级别只有一级*/
/*!!!重要: buffer设置必须强制遵循上面规则*/
#if 0
static const size_t block_size_tab[BUFFER_TYPE_BUTT][VTOP_MSG_BL_BUTT] = \
{
    {256, 512, 1024, 4096},
    {CONN_BLK_SIZE, 0, 0, 0}
};
static const size_t default_block_num[BUFFER_TYPE_BUTT][VTOP_MSG_BL_BUTT] = \
{
    {256, 128, 64, 16},
    {((256 + 128 + 64 + 16) + VTOP_MSG_DEFAULT_MAX_TASK), 0, 0, 0}
};
static const char *s_mbName[BUFFER_TYPE_BUTT] = {"MSG ", "CONN"};
static MB_CONTEXT_S s_struMBContext[BUFFER_TYPE_BUTT];
static unsigned char *s_pMsgBuf[BUFFER_TYPE_BUTT] = {NULL, NULL};
#endif

/*****************************************************************************
 Prototype       : MB_Init
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/24
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int MB_Init( MB_HANDLE *pHandle, VTOP_MSG_S_Buffer *pBuffer)
{
    unsigned int i, j;
    //unsigned int mb_num;
    size_t each_mb_size[VTOP_MSG_BL_BUTT], buffer_size;
    MB_HEAD_S  *pMBH;
    unsigned char *pMbHead[VTOP_MSG_BL_BUTT];
    const size_t *pBlockNum  = NULL;
    const size_t *pBlockSize = NULL;
    size_t bufLevel;
    MB_CONTEXT_S *pMbContext = NULL;

    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pHandle, NULL, VTOP_MSG_ERR_INVALIDPARA);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pBuffer, NULL, VTOP_MSG_ERR_INVALIDPARA);

    /*check blocknum参数*/
    if ( pBuffer->blockNum[VTOP_MSG_BL_0] > MSG_MAX_BLK_NUM
      || pBuffer->blockNum[VTOP_MSG_BL_1] > MSG_MAX_BLK_NUM
      || pBuffer->blockNum[VTOP_MSG_BL_2] > MSG_MAX_BLK_NUM
      || pBuffer->blockNum[VTOP_MSG_BL_3] > MSG_MAX_BLK_NUM )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "Blk num too big!");
    }
    else if ( pBuffer->blockNum[VTOP_MSG_BL_0] == 0
           && pBuffer->blockNum[VTOP_MSG_BL_1] == 0
           && pBuffer->blockNum[VTOP_MSG_BL_2] == 0
           && pBuffer->blockNum[VTOP_MSG_BL_3] == 0 )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "Blk num all zero!");
    }
    else
    {
        pBlockNum = pBuffer->blockNum;
    }

    /*check blocksize参数*/
    if ( pBuffer->blockSize[VTOP_MSG_BL_0] > MSG_MAX_BLK_SIZE
      || pBuffer->blockSize[VTOP_MSG_BL_1] > MSG_MAX_BLK_SIZE
      || pBuffer->blockSize[VTOP_MSG_BL_2] > MSG_MAX_BLK_SIZE
      || pBuffer->blockSize[VTOP_MSG_BL_3] > MSG_MAX_BLK_SIZE )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "Blk size too big!");
    }
#if 0
    else if ( pBuffer->blockSize[VTOP_MSG_BL_0] == 0
           && pBuffer->blockSize[VTOP_MSG_BL_1] == 0
           && pBuffer->blockSize[VTOP_MSG_BL_2] == 0
           && pBuffer->blockSize[VTOP_MSG_BL_3] == 0 )
    {
        return VTOP_MSG_ERR_INVALIDPARA;
    }
#endif
    else
    {
        for ( i = 0, bufLevel = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
        {
            if ( pBuffer->blockSize[i] > 0 )
            {
                bufLevel++;
            }
        }
        if ( bufLevel == 0 )
        {
            MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "Blk size all zero!");
        }
        for ( i = 0 ; i < bufLevel ; i++ )
        {
            if ( pBuffer->blockSize[i] == 0 )
            {
                return VTOP_MSG_ERR_INVALIDPARA;
            }
        }
        for ( i = 1 ; i < bufLevel ; i++ )
        {
            if ( pBuffer->blockSize[i-1] >= pBuffer->blockSize[i] )
            {
                MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA,
                    "Block size order %u %u!", pBuffer->blockSize[i-1], pBuffer->blockSize[i]);
            }
        }
        pBlockSize = pBuffer->blockSize;
    }

    pMbContext = (MB_CONTEXT_S *)malloc(sizeof(MB_CONTEXT_S));
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMbContext, NULL, VTOP_MSG_ERR_NOMEM);
    memset(pMbContext, 0, sizeof(MB_CONTEXT_S));

    //bufLevel = mb_num;
    memset(each_mb_size, 0, sizeof(each_mb_size));
    /*calc each block size and total buffer size*/
    for ( i = 0, buffer_size = 0 ; i < bufLevel ; i++ )
    {
        each_mb_size[i] = pBlockNum[i] * MB_BLOCK_SIZE(i, pBlockSize);
        buffer_size += each_mb_size[i];
    }

    /*统一分配一大块消息内存，再由各个队列细分*/
    pMbContext->s_pMsgBuf = (unsigned char*)malloc(buffer_size);
    if ( pMbContext->s_pMsgBuf == NULL )
    {
        free(pMbContext);
        pMbContext = NULL;
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "no mem!");
    }
    memset(pMbContext->s_pMsgBuf, 0, buffer_size);
    memset(pMbHead, 0, sizeof(pMbHead));

    for ( pMbHead[0] = pMbContext->s_pMsgBuf, i = 1 ; i < bufLevel ; i++ )
    {
        pMbHead[i] = pMbHead[i-1] + each_mb_size[i-1];
        //printf("each_mb_size[%u] = %#x, pMbHead[%u] = %p\n", i, each_mb_size[i], i, pMbHead[i]);
    }
    ASSERT(LOG_MODULE, 0, \
        (pMbHead[i-1] + each_mb_size[i-1]) == (pMbContext->s_pMsgBuf + buffer_size));

    /*init mb context*/
    pMbContext->mb_num = bufLevel;
    strncpy(pMbContext->mbName, pBuffer->name, MB_NAME_SIZE);
    pMbContext->mbName[MB_NAME_SIZE-1]='\0';
    for ( i = 0 ; i < bufLevel ; i++ )
    {
        pMbContext->mb[i].blocksize  = MB_BLOCK_SIZE(i, pBlockSize);
        //pMbContext->mb[i].blocksize  = pBlockSize[i];
        pMbContext->mb[i].blocknum   = pBlockNum[i];
        pMbContext->mb[i].buffersize = \
            pMbContext->mb[i].blocksize * pMbContext->mb[i].blocknum;
        pMbContext->mb[i].used = 0;
        if ( pMbContext->mb[i].buffersize > 0 )
        {
            pMbContext->mb[i].pMsg = \
                (unsigned char**)malloc(pMbContext->mb[i].blocknum * sizeof(unsigned char*));
            if ( pMbContext->mb[i].pMsg == NULL )
            {
                MB_Deinit(pMbContext);
                MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "no mem!");
            }
            for ( j = 0 ; j < pMbContext->mb[i].blocknum ; j++ )
            {
                pMbContext->mb[i].pMsg[j] = pMbHead[i] + j*pMbContext->mb[i].blocksize;
                pMBH = (MB_HEAD_S*)pMbContext->mb[i].pMsg[j];
                pMBH->buffer_id = i;
                pMBH->block_id = j;
                #if 0
                SVR_LOG_INFO("[%u %u, flag: %d, Addr:%p]\n", pMBH->buffer_id, pMBH->block_id, pMBH->flag, pMbContext->mb[i].pMsg[j]);
                MSG_INFO("MSG BUFFER: [%d, %d, %p]",
                    pMBH->buffer_id, pMBH->block_id, pMbContext->mb[i].pMsg[j] + MB_HEAD_LEN);
                #endif
            }
        }
        //MSG_INFO("<mb=%d: blocknum=%d, blocksize=%d>",i, pMbContext->mb[i].blocknum, pMbContext->mb[i].blocksize);
        pMbContext->total_num += pMbContext->mb[i].blocknum;
        pMbContext->total_size += (pMbContext->mb[i].blocknum * pMbContext->mb[i].blocksize);
    }
    //MSG_INFO("<all: %d %d>", pMbContext->total_num, pMbContext->total_size);
    ASSERT(LOG_MODULE, 0, buffer_size == pMbContext->total_size);
    *pHandle = pMbContext;
    return 0;
}

/*****************************************************************************
 Prototype       : MB_Deinit
 Description     : ...
 Input           : void
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/24
    Author       : qushen
    Modification : Created function

*****************************************************************************/
void MB_Deinit( MB_HANDLE handle )
{
    unsigned int i;
    size_t bufLevel;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;

    if ( pMbContext == NULL )
    {
        return;
    }

    bufLevel = pMbContext->mb_num;
    for ( i = 0 ; i < bufLevel ; i++ )
    {
        if ( pMbContext->mb[i].pMsg != NULL )
        {
            free(pMbContext->mb[i].pMsg);
            pMbContext->mb[i].pMsg = NULL;
        }
    }
    free(pMbContext->s_pMsgBuf);
    pMbContext->s_pMsgBuf = NULL;
    free(pMbContext);
    pMbContext = NULL;
}

/*****************************************************************************
 Prototype       : MB_Alloc
 Description     :
 Input           : size: 申请的消息Body的大小
 Output          : None
 Return Value    : 返回整个消息头的指针，后面紧跟消息Body
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/18
    Author       : qushen
    Modification : Created function

*****************************************************************************/
void*  MB_Alloc( MB_HANDLE handle, size_t size )
{
    unsigned int i;
    MSG_BUFFER_S *pMsgBuf;
    MB_HEAD_S *pMBH;
    size_t bufLevel;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;

    bufLevel = pMbContext->mb_num;
    for ( i = 0 ; i < bufLevel; i++ )
    {
        if ( pMbContext->mb[i].buffersize > 0 )
        {
            if ( (size + MB_HEAD_LEN) <= pMbContext->mb[i].blocksize )
            {
                break;
            }
        }
    }
    if ( i == bufLevel )
    {
        MSG_ERROR(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "not find match size: %d", size);
        return NULL;
    }

    pMsgBuf = &pMbContext->mb[i];
    for ( i = 0 ; i < pMsgBuf->blocknum; i++ )
    {
        pMBH = (MB_HEAD_S*)pMsgBuf->pMsg[i];
        if ( pMBH->flag == 0 )
        {
            pMBH->flag = 1;
            pMsgBuf->used++;
//MSG_INFO("Alloc addr: [%d, %d, %p]", pMBH->buffer_id, pMBH->block_id, pMsgBuf->pMsg[i] + MB_HEAD_LEN);
            return pMsgBuf->pMsg[i] + MB_HEAD_LEN;
        }
    }
    return NULL;
}

QNODE_S* MB_GetQNode(unsigned char *pMbBody)
{
    MB_HEAD_S *pMBH;
    pMBH = (MB_HEAD_S *)(pMbBody - MB_HEAD_LEN);
    return &(pMBH->node);
}

/*****************************************************************************
 Prototype       : MB_Free
 Description     : ...
 Input           : void *p
 Output          : None
 Return Value    : void
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/18
    Author       : qushen
    Modification : Created function

*****************************************************************************/
void MB_Free( MB_HANDLE handle, void *p )
{
    MB_HEAD_S *pMBH;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;

    ASSERT(LOG_MODULE, 0, p != NULL);

    pMBH = (MB_HEAD_S *)((unsigned char*)p - MB_HEAD_LEN);
    ASSERT(LOG_MODULE, 0, pMBH->flag == 1);

    pMBH->flag = 0;
    pMbContext->mb[pMBH->buffer_id].used--;
    //HI_LOG_DEBUG("Free addr: [%d, %d, %p]", pMBH->buffer_id, pMBH->block_id, p);
    return;
}

/*****************************************************************************
 Prototype       : MB_GetTotalUsed
 Description     : ...
 Input           : void
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/25
    Author       : qushen
    Modification : Created function

*****************************************************************************/
unsigned int MB_GetTotalUsed( MB_HANDLE handle )
{
    unsigned int i;
    unsigned int total_used = 0;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;

    for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
    {
        if ( pMbContext->mb[i].buffersize > 0 )
        {
            total_used += pMbContext->mb[i].used;
        }
    }
    return total_used;
}

unsigned int MB_GetTotalSize( MB_HANDLE handle )
{
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;
    return pMbContext->total_size;
}

unsigned int MB_GetTotalNum( MB_HANDLE handle )
{
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;
    return pMbContext->total_num;
}

/*****************************************************************************
 Prototype       : MB_GetUsed
 Description     : 获取bufer组的使用情况
 Input           : None
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/29
    Author       : qushen
    Modification : Created function

*****************************************************************************/
unsigned int MB_GetUsed( MB_HANDLE handle, unsigned int buffer_id )
{
    unsigned int i, used;
    MSG_BUFFER_S *pMsgBuf;
    MB_HEAD_S *pMBH;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;
#if 0
    for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
    {
        if ( block_size_tab[type][i] > 0 )
        {
            pUsed[i] = pMbContext->mb[i].used;
        }
        else
        {
            pUsed[i] = 0;
        }
    }
#endif
    pMsgBuf = &pMbContext->mb[buffer_id];
    for ( i = 0, used = 0 ; i < pMsgBuf->blocknum; i++ )
    {
        pMBH = (MB_HEAD_S*)pMsgBuf->pMsg[i];
        if ( pMBH->flag == 1 )
        {
            used++;
        }
        //SVR_LOG_INFO("pMBH->flag = %d\n", pMBH->flag);
    }
    return used;
}

void MB_ShowState(MB_HANDLE handle)
{
#ifdef __MSG_DEBUG__
    unsigned int i;
    MSG_BUFFER_S *pMsgBuf;
    MB_CONTEXT_S *pMbContext = (MB_CONTEXT_S *)handle;

    SVR_LOG_INFO("\n****************  MB \"%s\" ShowState  *****************\n", pMbContext->mbName);
    SVR_LOG_INFO("  index     blksize     blkused      blknum        type\n");
    for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
    {
        pMsgBuf = &pMbContext->mb[i];
        //SVR_LOG_INFO("    %2u         %4u         %4u         %4u      %s\n",
            //i, pMsgBuf->blocksize, pMsgBuf->used, pMsgBuf->blocknum, s_mbName[type]);
        SVR_LOG_INFO("%7u%12u%12u%12u      %s\n",
            i, pMsgBuf->blocksize, MB_GetUsed(handle, i), pMsgBuf->blocknum, pMbContext->mbName);
    }
    SVR_LOG_INFO("********************************************************\n");
    fflush(stdout);
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
