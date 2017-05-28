/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_rspmng.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/03/15
  Description   : msg_rspmng.c header file
  History       :
  1.Date        : 2009/03/15
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_RSPMNG_H__
#define __MSG_RSPMNG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "msg_misc.h"

typedef  HI_S32 MSG_RSPMNG_HANDLE;

#define MSG_RSPMNG_INVALID_HANDLE (-1)

#define MAX_MSG_RSP_NUM  128

#define MAX_MSG_RSP_SIZE 512

extern HI_VOID MSG_RSPMNG_Deinit( HI_VOID );
extern HI_S32 MSG_RSPMNG_Delete( MSG_RSPMNG_HANDLE handle );
extern HI_S32  MSG_RSPMNG_GetData( MSG_RSPMNG_HANDLE handle, VTOP_MSG_S_InnerBlock **ppInnerMsg );
extern HI_S32 MSG_RSPMNG_Init( HI_VOID );
extern HI_S32  MSG_RSPMNG_New( MSG_RSPMNG_HANDLE *pHandle );
extern HI_S32 MSG_RSPMNG_PutData( MSG_RSPMNG_HANDLE handle, VTOP_MSG_S_InnerBlock *pInnerMsg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_RSPMNG_H__ */
