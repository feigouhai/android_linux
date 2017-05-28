/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_socket.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/18
  Description   :
  History       :
  1.Date        : 2008/01/18
    Author      : qushen
    Modification: Created file

  2.Date        : 2008/02/04
    Author      : qushen
    Modification: change file name to msg_socket.c

******************************************************************************/

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "msg_misc.h"
#include "msg_log.h"
#include "msg_socket.h"

/************************************************

消息通信 模块对外接口

*************************************************/

//...服务端的bind，accept，listen不再列出
