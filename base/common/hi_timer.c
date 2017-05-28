/*****************************************************************************
*                      Copyright    , 2009-2050, Hisilicon Tech. Co., Ltd.
*                                   ALL RIGHTS RESERVED
* FileName: hi_timer.c
* Description: timer组件对外接口实现
*
* History:
* Version   Date                Author            DefectNum      Description
* main\1    2009-3-17     chenling102556      NULL          Create this file.
******************************************************************************/

/*include header files*/
#include "hi_type.h"
#include <pthread.h>
#include <common/petimer.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

static HI_VOID * g_timerEntry = NULL;

petimer_t  HI_TIMER_Create(HI_VOID (*timeout)(HI_VOID *), HI_S32 mode)
{
    pthread_t tid;

    if(NULL == g_timerEntry)
    {
        petimer_start(&tid, NULL, &g_timerEntry);
    }

    return  petimer_create_reltimer(timeout, mode);
}

HI_S32  HI_TIMER_Destroy(petimer_t timer)
{
    return  petimer_free_reltimer(g_timerEntry, timer);
}

HI_S32 HI_TIMER_Disable(petimer_t timer)
{
    return petimer_stop_reltimer_safe(g_timerEntry, timer, NULL, NULL);
}

HI_S32 HI_TIMER_Enable (petimer_t timer, HI_U32 msec)
{
   return  petimer_start_reltimer(g_timerEntry, timer, msec, (HI_VOID *)timer);
}

HI_U32 HI_TIMER_EnableEx (petimer_t timer, HI_U32 msec, HI_VOID * args)
{
   return  petimer_start_reltimer(g_timerEntry, timer, msec, args);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* __cplusplus */
