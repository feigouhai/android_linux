/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_misc.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/31
  Last Modified :
  Description   : msg.c header file
  Function List :
  History       :
  1.Date        : 2008/1/31
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifndef __MSG_MISC_H__
#define __MSG_MISC_H__

#include "common/msg_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifdef __MSG_DEBUG__
#define VTOP_MSG_VERSION "V1.1.0.0(D)"
#else
#define VTOP_MSG_VERSION "V1.1.0.0(R)"
#endif

#define VTOP_MSG_MAX_VERSION_SIZE    32

/* ���屾��SOCKETʹ�õ��ļ��� */
//#define MSG_SOCKET_DOMAIN "/var/.s3Pmsgdomain"
#define MSG_SOCKET_DOMAIN 46326

typedef enum hiTASK_OWNER_E
{
    TASK_OWNER_TASK  = 0,   /*��ͨ��Ϣ����*/
    TASK_OWNER_BUTT
} TASK_OWNER_E;

#if 0
#define VTOP_MSG_PRJ_ID        1

/* ������Ϣ��������� */
#define VTOP_MSG_NAME_PROC  ".s3Pmsgproc"

/* ����ÿ�����ݱ����ź�����ʹ�õ��ļ��� */
#define VTOP_MSG_NAME_SEM_TAB_TASK  ".s3Pmsgtask"
#define VTOP_MSG_NAME_SEM_TAB_SUB      ".s3Pmsgsub"
#define VTOP_MSG_NAME_SEM_TAB_GROUP ".s3Pmsggroup"
#else
/* ������Ϣ��������� */
#define VTOP_MSG_PROC_KEY           0xB2790105

/* ����ÿ�����ݱ����ź�����ʹ�õ��ļ��� */
#define VTOP_MSG_SEM_TASK_KEY       0xF1800813
#define VTOP_MSG_SEM_SUB_KEY        0xAA791019
#endif

/* ������ϢϵͳĬ������ֵ */
#define VTOP_MSG_DEFAULT_MAX_TASK        32
#define VTOP_MSG_DEFAULT_MAX_SUB        64

#define VTOP_MSG_DEFAULT_MAX_MEM        (256*1024)

/* 0xffff0001 ~ 0xfffffffe reserved for internal */
#define VTOP_MAX_TASK_ID        0xffff0000

/* msg flag: not for user */
#define VTOP_MSG_FLG_DSC_RCV    0x00001000  /* ������Ϣͷ���� */
#define VTOP_MSG_FLG_DSC_SND    0x00002000  /* ������Ϣͷ���� */
#define VTOP_MSG_FLG_DSC_RCV_BYE    0x00008000  /* ������Ϣͷ�������� */

/* msg flag: not for user */
#define VTOP_MSG_isRCVMsg(flag)     ((flag) & VTOP_MSG_FLG_DSC_RCV)
#define VTOP_MSG_isSNDMsg(flag)     ((flag) & VTOP_MSG_FLG_DSC_SND)

/*check if a exist then use a, or use value b*/
#define GET_VALUE(a, b) ((a) ? (a) : (b))

typedef struct
{
    unsigned int  maxTaskNum;          /* ��Ϣϵͳ�����Ϣ������ */

    unsigned int  maxSubNum;          /* ��Ϣϵͳ����ı��� */
    unsigned int  maxTaskNumInSub;      /* ÿ���ı����������Ϣ������ */

    unsigned int  taskTabPos;        /* ��Ϣ���������λ�� */
    unsigned int  subTabPos;         /* ���ı�����λ��*/

    VTOP_TASK_ID counter_taskId;
    VTOP_MSG_ID  counter_msgId;

    char    version[VTOP_MSG_MAX_VERSION_SIZE];
} MSG_SHM_S;

typedef struct
{
    VTOP_MSG_S_Block msg;   // �û���Ϣ��
    VTOP_MSG_ID resp2id;    // ���msgΪ��Ӧ��Ϣ����resp2id��Ϊԭ��Ϣid
    unsigned int magic;
    int privData; //ͬ��Ӧ����Ϣ���
}VTOP_MSG_S_InnerBlock;

#define MSG_INNER_HEAD_LEN sizeof(VTOP_MSG_S_InnerBlock)

#define MSG_MAX_LEN (MSG_INNER_HEAD_LEN + VTOP_MSG_MAX_MSG_SIZE)

typedef struct hiMSG_SND_BUF_S
{
    size_t sntSize;
    size_t msgSize;
    unsigned char  buffer[MSG_MAX_LEN];
} MSG_SND_BUF_S;

/* ����˽��ͷ�ļ� */

#define MSG_COPY2BUF(pBuf, pMsg)    \
    do{     \
        memcpy(pBuf, pMsg, MSG_INNER_HEAD_LEN);\
        if((pMsg)->msg.ulMsgLen > 0)\
        {       \
            memcpy(pBuf + MSG_INNER_HEAD_LEN, (pMsg)->msg.pMsgBody, (pMsg)->msg.ulMsgLen);\
            ((VTOP_MSG_S_InnerBlock *)pBuf)->msg.pMsgBody = pBuf + MSG_INNER_HEAD_LEN; \
        }   \
    }while(0)

#define VTOP_MSG_MAGIC 0x7fff

HI_BOOL SHM_Exist(void);
int SHM_Init(VTOP_MSG_S_Config * pMsgConfig);
int SHM_DeInit(void);

extern MSG_SHM_S * SHM_GetAddr(void);

extern int TASK_TAB_GetMaxTaskNum(unsigned int * pMaxTask);
extern int SUB_TAB_GetMaxSubNum(unsigned int * pMaxSub, unsigned int * pMaxTaskInSub);

extern int getMsgID(VTOP_MSG_ID * pMsgID);

extern void VTOP_MSG_ShowTitle(void);
extern void VTOP_MSG_Info(VTOP_MSG_S_InnerBlock * pMsg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*  __MSG_MISC_H__  */
