#include <stdio.h>
#include <malloc.h>     /*lint -esym(766, malloc.h) */
#include <stdarg.h>

#include "utils/Logger.h"
#include "common/hi_ref_errno.h"
#include "common/hi_adp_mutex.h"
#include "common/hi_ref_mod.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */
//SVR_MODULE_DECLARE("COMMONMOD")
typedef struct tagMOD_INFO_S
{
    HI_U32 u32ModId;
    HI_CHAR aszModName[HI_MOD_NAME_LEN];
} MOD_INFO_S;

static HI_BOOL s_bMode_init = HI_FALSE;
static MOD_INFO_S s_astModInfo[HI_MOD_MAX_NUM];
static HI_ThreadMutex_S s_stMod_mutex = HI_MUTEX_INITIALIZER;

/***************************** Macro & Variable Definition *******************/

// check param is valid
#define MOD_CHK_PARA(val)        \
do                                  \
{                                   \
    if ((val))                      \
    {                               \
        HI_LOGE("para invalid");    \
        return HI_EINVAL;           \
    };                              \
} while (0)

#define MOD_CHK_RETURN_ERR(val, ret )        \
do                                              \
{                                               \
    if ((val))                                  \
    {                                           \
        HI_LOGE("value invalid");               \
        return ret;                             \
    }                                           \
} while (0)

#define MODDOFUNC_RETURN( func )            \
do                                              \
{                                               \
    HI_S32 ret = 0;                             \
    ret = func;                                 \
    if (ret != HI_SUCCESS)                      \
    {                                           \
        HI_LOGE("CALL %s, ret is %d ", # func, ret);  \
        return ret;                             \
    };                                          \
} while (0)

static HI_VOID MOD_Init()
{
    HI_U32 i = 0;

    if (HI_FALSE == s_bMode_init)
    {
        s_bMode_init = HI_TRUE;
        memset(s_astModInfo, 0, sizeof(MOD_INFO_S) * HI_MOD_MAX_NUM);

        for (i = HI_MOD_ID_NULL; i < HI_MOD_MAX_NUM; i++)
        {
            s_astModInfo[i].u32ModId = HI_MOD_INVALID_ID;
            snprintf(s_astModInfo[i].aszModName, sizeof(s_astModInfo[i].aszModName),"%s", "");
        }

        s_astModInfo[HI_MOD_ID_NULL].u32ModId = HI_MOD_ID_NULL;
        snprintf(s_astModInfo[HI_MOD_ID_NULL].aszModName, sizeof(s_astModInfo[HI_MOD_ID_NULL].aszModName),"%s", "MOD_NULL");
    }
}

const HI_CHAR * HI_MOD_GetNameById(HI_U32 u32ModID)
{
    const HI_CHAR * pszModName;

    (HI_VOID)HI_MutexLock(&s_stMod_mutex);
    MOD_Init();

    if (u32ModID >= (HI_U32)HI_MOD_MAX_NUM)
    {
        pszModName = (const HI_CHAR *)"";
    }
    else
    {
        pszModName = (const HI_CHAR *)(s_astModInfo[u32ModID].aszModName);
    }

    if (HI_NULL == pszModName)
    {
        pszModName = (const HI_CHAR *)"";
    }

    (HI_VOID)HI_MutexUnLock(&s_stMod_mutex);

    return pszModName;
}

HI_S32 HI_MOD_GetIdByName(HI_CHAR * pszModName, HI_U32 * pu32ModId)
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_BOOL bFind = HI_FALSE;
    HI_U32 i = 0;

    MOD_CHK_RETURN_ERR(NULL == pszModName || NULL == pu32ModId, HI_FAILURE);

    (HI_VOID)HI_MutexLock(&s_stMod_mutex);
    MOD_Init();

    for (i = 0; i < HI_MOD_MAX_NUM; i++)
    {
        if (HI_MOD_INVALID_ID == s_astModInfo[i].u32ModId)
        {
            continue;
        }

        HI_LOGD("### mod name1 = %s, pszModName = %s \n",
            s_astModInfo[i].aszModName, pszModName);
        if (!strncmp(s_astModInfo[i].aszModName, pszModName, HI_MOD_NAME_LEN))
        {
            bFind = HI_TRUE;
            break;
        }
    }

    if (HI_TRUE == bFind)
    {
        s32Ret = HI_SUCCESS;
        *pu32ModId = i;
    }
    else
    {
        *pu32ModId = HI_MOD_ID_NULL;
    }

    (HI_VOID)HI_MutexUnLock(&s_stMod_mutex);

    return s32Ret;
}

HI_S32 HI_MOD_Register(HI_U32 u32ModId, HI_CHAR * pszModName)
{
    MOD_CHK_PARA(u32ModId >= HI_MOD_MAX_NUM);
    MOD_CHK_RETURN_ERR(NULL == pszModName, HI_FAILURE);
    MOD_CHK_RETURN_ERR(!strncmp(pszModName, "", HI_MOD_NAME_LEN), HI_FAILURE);
    MOD_CHK_RETURN_ERR(HI_MOD_NAME_LEN < strlen(pszModName), HI_FAILURE);

    (HI_VOID)HI_MutexLock(&s_stMod_mutex);
    MOD_Init();

    if (HI_MOD_INVALID_ID != s_astModInfo[u32ModId].u32ModId)
    {
        HI_LOGD("### this mod already register, name = %s \n", s_astModInfo[u32ModId].aszModName);
        (HI_VOID)HI_MutexUnLock(&s_stMod_mutex);
        return HI_SUCCESS;
    }

    s_astModInfo[u32ModId].u32ModId = u32ModId;
    memcpy(s_astModInfo[u32ModId].aszModName, pszModName, strlen(pszModName));

    HI_LOGD("### reg mod id = %d, mod name = %s \n", u32ModId, pszModName);

    (HI_VOID)HI_MutexUnLock(&s_stMod_mutex);

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __cplusplus */
