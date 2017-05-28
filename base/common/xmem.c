/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : xmem.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/09/17
  Description   :
  History       :
  1.Date        : 2009/09/17
    Author      : dvb
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "hi_argparser.h"
#include "hi_adp_mutex.h"
#include "hi_adp_memmng.h"
#include "xmem.h"
#include "linux_list.h"

#define XMEM_MALLOC(size) HI_MEMMNG_MALLOC(HI_MOD_ID_NULL, size)
#define XMEM_FREE(pAddr) HI_MEMMNG_FREE(HI_MOD_ID_NULL, pAddr)

enum XMEM_STATUS
{
    STATUS_UNAVAILABLE = -1,
    STATUS_AVAILABLE = 0,
};

#if 0
 #define    X_PRINT(fmt...) do {SVR_LOG_INFO(fmt);} while (0)
#else
 #define    X_PRINT(fmt...) do {} while (0)
#endif

#define MEM_ALIGN_SIZE(size) (((size) + 3) & 0xFFFFFFFC)

//content node for a section of EIT data
struct content_t
{
    struct content_t *next;
    HI_U32            len; //content buf len(shall >= section data len)
    HI_U8             addr[0]; //section buf addr
};

typedef struct ALLOC_OBJ
{
    HI_ThreadMutex_S alloc_mutex;
    HI_U8*           buffer;
    HI_U32           buf_len;
    struct content_t *free_list;
    HI_S32 status;

    HI_U32 modid;
    HI_U32 alloc_times;
    HI_U32 free_times;
    struct list_head list;
} ALLOC_OBJ_S;

#define alloc_enter_mutex(pAllocObj) HI_MutexLock(&(pAllocObj)->alloc_mutex)
#define alloc_leave_mutex(pAllocObj) HI_MutexUnLock(&(pAllocObj)->alloc_mutex)

#define GARBAGE_LEN_THRESHOLD 20

static struct list_head s_xmemList;

static HI_VOID x_init(HI_VOID)
{
    static HI_BOOL bInited = HI_FALSE;

    if (bInited == HI_TRUE)
    {
        return;
    }

    INIT_LIST_HEAD(&s_xmemList);
    bInited = HI_TRUE;
}

/*
 * alloc a content, it includes both content_struct and section_data area.
 */
static struct content_t *alloc_content(struct content_t **head, HI_U32 len)
{
    struct content_t *pre = NULL;
    struct content_t *new_node = NULL;
    struct content_t *pre_new_node = NULL;
    struct content_t *node = *head;
    HI_U32 len_limit1 = len + GARBAGE_LEN_THRESHOLD;

    HI_U32 size = MEM_ALIGN_SIZE(len + sizeof(struct content_t));
    HI_U32 len_limit2 = size + 2 * GARBAGE_LEN_THRESHOLD;

    X_PRINT("alloc from the free list method 1!");

    while (node != NULL)
    {
        //check if wihin [len, len + threshold]
        if (node->len >= len)
        {
            if (node->len <= len_limit1)
            {
                //del the node from the list
                if (pre == NULL)
                {
                    //head one
                    *head = node->next;
                }
                else
                {
                    pre->next = node->next;
                }

                node->next = NULL;
                return node;
            }
            else if ((new_node == NULL) || (node->len < new_node->len)) //get the shortest one
            {
                new_node = node;
                pre_new_node = pre;
            }
        }

        pre  = node;
        node = node->next;
    }

    //found one > len + threshold
    if (new_node != NULL)
    {
        // >= len + 2*threshold, divide it
        if (new_node->len >= len_limit2)
        {
            node = new_node;

            node->len -= size;
            new_node = (struct content_t*)(node->addr + node->len);
            new_node->len  = size - sizeof(struct content_t);
            new_node->next = NULL;
            return new_node;
        }
        else    //(len + threshold, len + 2*threshold)
        {
            //del the node from the list
            if (pre_new_node == NULL)
            {
                //head one
                *head = new_node->next;
            }
            else
            {
                pre_new_node->next = new_node->next;
            }

            new_node->next = NULL;
            return new_node;
        }
    }

    X_PRINT("alloc a content node failed!");
    return NULL;
}

/*
 * add the content node to the list
 */
static void add_content_list(struct content_t **head, struct content_t *node)
{
    node->next = (*head);
    *head = node;
}

/*
 * defrag a content node with the free list
 */
static HI_S32 defrag_content(struct content_t **head, struct content_t *node)
{
    struct content_t *list = *head;
    struct content_t *pre = NULL;
    HI_U32 addr;

    if ((list == NULL) || (node == NULL))
    {
        return HI_FAILURE;
    }

    addr = (HI_U32)(node->addr + node->len);
    while (list != NULL)
    {
        if ((HI_U32)(list->addr + list->len) == (HI_U32)node)
        {
            // list <-- node
            list->len += (sizeof(struct content_t) + node->len);
            X_PRINT("list: 0x%X <-- node: 0x%X\n", list, node);
            return HI_SUCCESS;
        }
        else if (addr == (HI_U32)list)
        {
            node->len += (sizeof(struct content_t) + list->len);
            node->next = list->next;
            if (pre == NULL)
            {
                *head = node;
            }
            else
            {
                pre->next = node;
            }

            return HI_SUCCESS;
        }

        pre  = list;
        list = list->next;
    }

    X_PRINT("defrag failed!\nnode: 0x%X\n", node);
    return HI_FAILURE;
}

/*
 * defrag the free list
 */
static void free_content(struct content_t **head, struct content_t *node)
{
    node->next = NULL;

    if (HI_SUCCESS != defrag_content(head, node))
    {
        add_content_list(head, node);
    }
}

void *x_alloc(HI_HANDLE handle, HI_U32 size)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;
    struct content_t *node;

    alloc_enter_mutex(pAllocObj);

    node = alloc_content(&pAllocObj->free_list, size);
    if (node != NULL)
    {
        pAllocObj->alloc_times++;
    }

    alloc_leave_mutex(pAllocObj);

    if (node == NULL)
    {
        pAllocObj->status = STATUS_UNAVAILABLE;
        return NULL;
    }

    return (void*)((HI_U32)node + sizeof(struct content_t));
}

HI_S32 x_free(HI_HANDLE handle, void *addr)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;
    struct content_t *node;

    if (addr == NULL)
    {
        return -1;
    }

    alloc_enter_mutex(pAllocObj);

    node = (struct content_t*)((HI_U32)addr - sizeof(struct content_t));

    free_content(&pAllocObj->free_list, node);

    pAllocObj->status = STATUS_AVAILABLE;

    pAllocObj->free_times++;

    alloc_leave_mutex(pAllocObj);

    return 0;
}

HI_S32 x_init_mem(HI_U32 modid, HI_U32 buf_len, HI_HANDLE *pHandle)
{
    ALLOC_OBJ_S *pAllocObj;

    x_init();

    if ((pHandle == NULL) || (buf_len < sizeof(struct content_t)))
    {
        X_PRINT("%s: parameter error!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    pAllocObj = (ALLOC_OBJ_S *)HI_MEMMNG_MALLOC(modid, sizeof(ALLOC_OBJ_S));
    if (pAllocObj == NULL)
    {
        X_PRINT("%s: parameter error!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    pAllocObj->buffer = (HI_U8 *)HI_MEMMNG_MALLOC(modid, buf_len);
    if (pAllocObj->buffer == NULL)
    {
        X_PRINT("%s: parameter error!\n", __FUNCTION__);
        XMEM_FREE(pAllocObj);
        return HI_FAILURE;
    }

    HI_MutexInit(&pAllocObj->alloc_mutex, NULL);
    pAllocObj->buf_len   = buf_len;
    pAllocObj->free_list = (struct content_t*)pAllocObj->buffer;
    pAllocObj->free_list->next = NULL;
    pAllocObj->free_list->len = buf_len - sizeof(struct content_t);
    pAllocObj->status = STATUS_AVAILABLE;
    pAllocObj->modid = modid;
    pAllocObj->alloc_times = 0;
    pAllocObj->free_times = 0;
    list_add_tail(&pAllocObj->list, &s_xmemList);

    *pHandle = (HI_HANDLE)pAllocObj;

    X_PRINT("x malloc buffer: 0x%X, len: %d\n", pAllocObj->buffer, buf_len);

    return HI_SUCCESS;
}

HI_S32 x_reset_mem(HI_HANDLE handle)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;

    alloc_enter_mutex(pAllocObj);

    pAllocObj->free_list = (struct content_t*)pAllocObj->buffer;
    pAllocObj->free_list->next = NULL;
    pAllocObj->free_list->len = pAllocObj->buf_len - sizeof(struct content_t);
    pAllocObj->status = STATUS_AVAILABLE;

    alloc_leave_mutex(pAllocObj);

    return HI_SUCCESS;
}

HI_S32 x_release_mem(HI_HANDLE handle)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;

    alloc_enter_mutex(pAllocObj);

    if (pAllocObj->buffer != NULL)
    {
        HI_MEMMNG_FREE(pAllocObj->modid, pAllocObj->buffer);
        pAllocObj->buffer = NULL;
    }

    pAllocObj->buf_len   = 0;
    pAllocObj->free_list = NULL;
    pAllocObj->status = STATUS_UNAVAILABLE;

    alloc_leave_mutex(pAllocObj);

    HI_MutexDestroy(&pAllocObj->alloc_mutex);

    list_del(&pAllocObj->list);

    HI_MEMMNG_FREE(pAllocObj->modid, pAllocObj);

    return HI_SUCCESS;
}

HI_S32 x_get_mem_status(HI_HANDLE handle)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;

    return pAllocObj->status;
}

HI_S32 x_get_free_mem_info(HI_HANDLE handle, HI_U32 *pFreeSize, HI_U32 *pFreeNum)
{
    ALLOC_OBJ_S *pAllocObj = (ALLOC_OBJ_S *)handle;
    HI_U32 free_size = 0, free_num = 0;
    struct content_t *free_list;

    for (free_list = pAllocObj->free_list; free_list != NULL; free_list = free_list->next)
    {
        free_num++;
        free_size += free_list->len;
    }

    *pFreeNum  = free_num;
    *pFreeSize = free_size;
    return HI_SUCCESS;
}
