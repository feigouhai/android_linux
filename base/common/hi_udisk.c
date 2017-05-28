#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "Logger.h"
#include "common/linux_list.h"
#include "common/hi_udisk.h"
#include "common/scsiexe.h"

#define UDISK_DEFAULT_ROOT          "/mnt"
#define UDISK_USB_DEV_ROOT          "/dev"
#define UDISK_RESERVE_MAX_FILE_NUM  (2048)
#define UDISK_MAX_MOUNT_STR         (128)

typedef enum
{
    UDISK_PARTITION_STATUS_UNKOWN,
    UDISK_PARTITION_STATUS_MOUNTED,
    UDISK_PARTITION_STATUS_SCAN,
    UDISK_PARTITION_STATUS_SCANMOUNTED,
    UDISK_PARTITION_STATUS_UNKNOWN,
    UDISK_PARTITION_STATUS_BUTT,
} UDISK_PARTITON_STATUS_EVENT_E;

typedef enum
{
    UDISK_PARTITION_OP_GET,
    UDISK_PARTITION_OP_SET,
    UDISK_PARTITION_OP_BUTT,
} UDISK_PARTITON_OP_E;

typedef struct
{
    struct list_head partnode;
    HI_CHAR szDevPath[MAX_UDISK_NAME_LEN];
    //HI_CHAR szMntPath[MAX_UDISK_NAME_LEN];
} UDISK_PARTINFO_S;

typedef struct
{
    struct list_head part_node;

    // for full report
    HI_BOOL bFullReport;
    HI_U64 u64WaterLine;

    HI_UDISK_PARTITIONINFO_S PartInfo;
} UDISK_PartNode_S;

typedef struct
{
    struct list_head disk_node;

    HI_UDISK_DISKINFO_S DiskInfo;
    //HI_U32 host;

    struct list_head part_list_head;
} UDISK_DiskNode_S;

typedef struct
{
    HI_BOOL bUsed;
    UDISK_PARTITON_STATUS_EVENT_E enStatus;
    HI_CHAR szDevPath[MAX_UDISK_NAME_LEN];
} UDISK_DISKINFO_S;

typedef struct
{
    HI_CHAR szFileSystemType[20];
} UDISK_PARTITIONTYPE_S;

LIST_HEAD(m_UDisk_DiskListHead);
LIST_HEAD(m_UDisk_PartROListHead);

static pthread_mutex_t m_UDisk_mutex;

#define UDISK_Lock()    pthread_mutex_lock(&m_UDisk_mutex)
#define UDISK_UnLock()  pthread_mutex_unlock(&m_UDisk_mutex)

#define UDISK_MALLOC(size)     malloc(size) //HI_MEMMNG_MALLOC(HI_SVR_MODID_UDISK, size)   //malloc(size)
#define UDISK_FREE(pAddr)      free(pAddr)   //HI_MEMMNG_FREE(HI_SVR_MODID_UDISK, pAddr)    //free(pAddr)

#define UDISK_DOFUNC( func )                    \
    do                                              \
    {                                               \
        HI_S32 ret = 0;                             \
        ret = func;                                 \
        if (ret != HI_SUCCESS)                      \
        {                                           \
            HI_LOGE("CALL %s, ret is %d ", # func, ret);  \
        };                                          \
    } while (0)

typedef struct
{
    HI_UDISK_EVENT_CALLBACK cb[HI_UDISK_EVENT_BUTT];
    HI_VOID* pPrivate[HI_UDISK_EVENT_BUTT];
} UDISK_Event_S;

static HI_CHAR* m_UDisk_Root = HI_NULL;
static HI_CHAR acMountParam[HI_UDISK_PARAM_MAX_LEN];

static HI_BOOL s_ReallocMem = HI_FALSE;
static HI_BOOL s_bUseHimount = HI_FALSE;
static HI_BOOL s_bUseblkidtool = HI_FALSE;
static HI_BOOL s_bForceMountNTFS = HI_FALSE;
static UDISK_DISKINFO_S m_DiskInfo[UDISK_MAX_DEV_NUM];
static UDISK_Event_S m_UDisk_Event;
static HI_BOOL s_bUmountAll = HI_FALSE;
struct timeval  m_UDiskStartTime;

static HI_S32 UDISK_EventCallback(HI_UDISK_EVENT_E enEvent, HI_VOID* param);

static HI_VOID UDISK_InitDiskDB()
{
    UDISK_Lock();
    memset(&m_DiskInfo, 0, sizeof(m_DiskInfo));
    UDISK_UnLock();
}

static HI_BOOL UDISK_ISMediaDevice(const char* pDevPath)
{
    HI_S32 i;
    HI_S32 fd;
    HI_BOOL bResult = HI_FALSE;
    HI_CHAR szDiskDevPath[MAX_UDISK_NAME_LEN] = "";

    for (i = 1; i < UDISK_MAX_DEV_NUM; i++)
    {
        memset(szDiskDevPath, '\0', sizeof(szDiskDevPath));
        snprintf(szDiskDevPath, 100, "%s%d", pDevPath, i);

        if (0 == access(szDiskDevPath, F_OK))
        {
            bResult = HI_TRUE;
            break;
        }
    }

    if (!bResult)
    {
        fd = -1;
        fd = open(pDevPath, O_RDONLY);

        if (fd > 0)
        {
            bResult = HI_TRUE;
            close(fd);
        }
    }

    return   bResult;
}

static HI_VOID UDISK_ReportDiskRemainEvent(HI_VOID)
{
    HI_S32 i, j;
    HI_U32 u32Timediff = 0;
    struct timeval stCurTime;
    HI_S32 s32MountedCount = 0;
    HI_UDISK_EVENT_PARAM_UNKOWNFILESYS_S stScanCmp;
    HI_UDISK_EVENT_PARAM_UNKOWNFILESYS_S stUnknown;

    memset(&stScanCmp, 0 , sizeof(stScanCmp));
    memset(&stUnknown, 0 , sizeof(stUnknown));

    UDISK_Lock();

    for (i = 0; i < UDISK_MAX_DEV_NUM; i++)
    {
        if (!m_DiskInfo[i].bUsed
            || 0 != access(m_DiskInfo[i].szDevPath, F_OK))
        {
            continue;
        }

        if (UDISK_PARTITION_STATUS_MOUNTED == m_DiskInfo[i].enStatus)
        {
            s32MountedCount++;
        }
        else if (UDISK_PARTITION_STATUS_SCAN == m_DiskInfo[i].enStatus
                 || UDISK_PARTITION_STATUS_SCANMOUNTED == m_DiskInfo[i].enStatus)
        {
            if (UDISK_PARTITION_STATUS_SCANMOUNTED == m_DiskInfo[i].enStatus)
            {
                s32MountedCount++;
            }

            j = stScanCmp.u32ParitionNum;
            stScanCmp.u32ParitionNum++;
            strncpy(stScanCmp.szDevPath[j], m_DiskInfo[i].szDevPath, MAX_DEV_NAME_LEN - 1);
        }
        else if (UDISK_PARTITION_STATUS_UNKNOWN == m_DiskInfo[i].enStatus)
        {
            j = stUnknown.u32ParitionNum;
            stUnknown.u32ParitionNum++;
            strncpy(stUnknown.szDevPath[j], m_DiskInfo[i].szDevPath, MAX_DEV_NAME_LEN - 1);
        }
    }

    memset(&m_DiskInfo, 0, sizeof(m_DiskInfo));
    UDISK_UnLock();

    if (0 != stScanCmp.u32ParitionNum)
    {
        UDISK_EventCallback(HI_UDISK_EVENT_DISKSCANCOMPLETE, &stScanCmp);
    }

    if (0 == s32MountedCount && 0 != stUnknown.u32ParitionNum)
    {
        gettimeofday(&stCurTime, NULL);
        u32Timediff = (stCurTime.tv_sec - m_UDiskStartTime.tv_sec) * 1000
                      + (stCurTime.tv_usec - m_UDiskStartTime.tv_usec) / 1000;

        if (u32Timediff < 5000)
        {
            sleep(5);
        }

        UDISK_EventCallback(HI_UDISK_EVENT_UNKNOWNFILESYS, &stUnknown);
    }
}

static HI_VOID UDISK_ReportScanEvent(HI_VOID)
{
    HI_S32 i;
    HI_S32 j = -1;
    HI_S32 s32count = 0;
    HI_UDISK_EVENT_PARAM_SCAN_S  param;

    UDISK_Lock();

    for (i = 0; i < UDISK_MAX_DEV_NUM; i++ )
    {
        if (!m_DiskInfo[i].bUsed)
        {
            continue;
        }

        if (UDISK_PARTITION_STATUS_SCAN == m_DiskInfo[i].enStatus
            || UDISK_PARTITION_STATUS_SCANMOUNTED == m_DiskInfo[i].enStatus)
        {
            if (-1 == j)
            {
                j = i;
            }

            s32count++;
        }
    }

    UDISK_UnLock();

    if (1 == s32count)
    {
        UDISK_Lock();
        memset(&param, 0, sizeof(param));
        strncpy(param.szDevPath, m_DiskInfo[j].szDevPath,
                strlen(m_DiskInfo[j].szDevPath));
        UDISK_UnLock();

        UDISK_EventCallback(HI_UDISK_EVENT_DISKSCANREQUEST, &param);
    }
}

static HI_VOID UDISK_RecordPartitionStatus(const HI_CHAR* pDevPath,
        UDISK_PARTITON_STATUS_EVENT_E enStatus)
{
    HI_S32 i;

    if (NULL == pDevPath)
    {
        return;
    }

    UDISK_Lock();

    for (i = 0; i < UDISK_MAX_DEV_NUM; i++ )
    {
        if (m_DiskInfo[i].bUsed
            && (0 == strncmp(m_DiskInfo[i].szDevPath, pDevPath, strlen(pDevPath))))
        {
            m_DiskInfo[i].enStatus = enStatus;
            UDISK_UnLock();
            return;
        }
    }

    for (i = 0; i < UDISK_MAX_DEV_NUM; i++ )
    {
        if (!m_DiskInfo[i].bUsed)
        {
            m_DiskInfo[i].bUsed = HI_TRUE;
            m_DiskInfo[i].enStatus = enStatus;
            strncpy(m_DiskInfo[i].szDevPath, pDevPath, strlen(pDevPath));
            break;
        }
    }

    UDISK_UnLock();
}

static HI_S32 UDISK_GetPartitionFileSystem(const HI_CHAR* pDevPath, UDISK_PARTITIONTYPE_S* pType)
{
    HI_S32 ret;
    HI_CHAR buffer[100];
    HI_CHAR command[100];
    HI_CHAR* pFilesystemType = NULL;
    FILE* fp;

    if (NULL == pDevPath)
    {
        return HI_FAILURE;
    }

    memset(command, '\0', sizeof(command));
    snprintf(command, 100 , "/sbin/blkid -s TYPE %s>/tmp/filesystemType", pDevPath);
    ret = system(command);

    if (0 != ret)
    {
        return HI_FAILURE;
    }

    fp = fopen("/tmp/filesystemType", "rb");

    if (fp == NULL)
    {
        return HI_FAILURE;
    }

    memset(buffer, '\0', sizeof(buffer));
    (HI_VOID)fread(buffer, sizeof(buffer) - 1 , 1, fp);
    fclose(fp);

    pFilesystemType = strchr(buffer, '=');

    if (NULL == pFilesystemType)
    {
        return HI_FAILURE;
    }

    pFilesystemType++; //ignore ==
    pFilesystemType++; //ignore " character

    if (pType && pFilesystemType)
    {
        memset(pType, '\0', sizeof(UDISK_PARTITIONTYPE_S));
        strncpy((HI_CHAR*)pType, pFilesystemType, 4);  //ignore the last " character
    }

    return HI_SUCCESS;;
}

static HI_VOID UDISK_RemoveROList(const HI_CHAR* pDevPath)
{
    UDISK_PARTINFO_S* pPart = HI_NULL;
    UDISK_PARTINFO_S* n = HI_NULL;

    UDISK_Lock();
    list_for_each_entry_safe(pPart, n, &m_UDisk_PartROListHead, partnode)
    {
        if (0 == strncasecmp(pDevPath, pPart->szDevPath, strlen(pDevPath)))
        {
            list_del(&pPart->partnode);
            free(pPart);
        }
    }
    UDISK_UnLock();
}

static HI_S32 UDISK_StatFs(const HI_CHAR* pMnt, HI_U64* pu64TotalSize, HI_U64* pu64AvailSize)
{
    HI_S32 ret;
    struct statfs fs_stat;
    ret = statfs(pMnt, &fs_stat);

    if (ret != 0 )
    {
        HI_LOGE("statfs");
        return HI_FAILURE;
    }

    if (HI_NULL != pu64TotalSize)
    {
        *pu64TotalSize = (HI_U64)((HI_U64)fs_stat.f_blocks * (HI_U64)fs_stat.f_bsize);/*lint !e737 !e647 */
    }

    if (HI_NULL != pu64AvailSize)
    {
        *pu64AvailSize = (HI_U64)((HI_U64)fs_stat.f_bavail * (HI_U64)fs_stat.f_bsize);/*lint !e737 !e647 */
    }

    return HI_SUCCESS;
}

static HI_S32 UDISK_StatDisk(HI_CHAR* pDiskDev, HI_CHAR* pszVendor, HI_CHAR* pszModel)
{
    memcpy(pszVendor, "Unknown", 8);
    memcpy(pszModel, "Unknown", 8);

    return HI_SUCCESS;
}

static HI_S32 UDISK_EventCallback(HI_UDISK_EVENT_E enEvent, HI_VOID* param)
{
    HI_S32 s32Ret = 0;

    if (m_UDisk_Event.cb[enEvent])
    {
        s32Ret = m_UDisk_Event.cb[enEvent](enEvent, param, m_UDisk_Event.pPrivate[enEvent]);
    }

    return s32Ret;
}
static HI_S32 UDISK_MountByFormat(const char* source, const char* target, const char* filesystemtype, const void* data, HI_UDISK_FSTYPE_E* fs_mnt)
{
    HI_S32 ret = 0;
    HI_U32 u32MountFlags = 0;
    char command[UDISK_MAX_MOUNT_STR];

    if (0 == strncasecmp("vfat" , filesystemtype, 4))
    {
        if (s_bUseHimount)
        {
            memset(command, '\0', sizeof(command));
            snprintf(command, sizeof(command) , "himount %s %s", source, target);
            (void)system(command);
        }
        else
        {
            ret = mount(source, target, filesystemtype, 0, NULL);
            //memset(command, 0, sizeof(command));
            //snprintf(command, sizeof(command) , "mount -t vfat %s %s %s", acMountParam, source, target);
            //ret = system(command);
            HI_LOGE("source:%s target:%s filesystemtype:%s, errno = 0x%x", source, target, filesystemtype, errno);

            if (0 != ret && !(-1 == ret && EBUSY == errno))
            {
                /*report to start scan event.*/
                if (s_bUseblkidtool)
                {
                    UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_SCAN);
                    UDISK_ReportScanEvent(); //report scan start event if necessary.
                }

                memset(command, '\0', sizeof(command));
                snprintf(command, sizeof(command) , "dosfsck -a %s", source);
                (void)system(command);

                if (EROFS == errno)
                {
                    u32MountFlags = MS_RDONLY;
                }

                ret = mount(source, target, filesystemtype, u32MountFlags, NULL);

                if (ret && (EROFS == errno))
                {
                    ret = mount(source, target, filesystemtype, u32MountFlags, NULL);//mount ro fs again
                }

                //memset(command, 0, sizeof(command));
                //snprintf(command, sizeof(command) , "mount -t vfat %s %s %s", acMountParam, source, target);
                //ret = system(command);
                HI_LOGE("ret = %d, errno = 0x%x", ret, errno);

                if (0 == ret || (-1 == ret && EBUSY == errno))
                {
                    UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_SCANMOUNTED);
                }

                HI_LOGE(" mount ret = 0x%x, param: %s", ret, acMountParam);
                return ret;
            }
            else
            {
                memset(command, '\0', sizeof(command));
                snprintf(command, sizeof(command) , "mkdir %s/LOST.DIR", target);
                (void)system(command);
            }
        }

        UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_MOUNTED);
        HI_LOGE("mount ret = 0x%x", ret);
        return ret;
    }
    else if (0 == strncasecmp("ntfs" , filesystemtype, 4) || 0 == strncasecmp("tntfs" , filesystemtype, 5))
    {
        memset(command, '\0', sizeof(command));

        if (!s_bForceMountNTFS)
        {
            snprintf(command, sizeof(command) , "mount -t tntfs %s %s %s", acMountParam, source, target);
        }
        else
        {
            snprintf(command, sizeof(command) , "mount -t tntfs -o force %s %s", source, target);
        }

        ret = system(command);

        if (0 != ret)
        {
            /* report disk scan start event.*/
            UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_SCAN);
            UDISK_ReportScanEvent();//report scan start event if necessary.

            memset(command, '\0', sizeof(command));
            snprintf(command, sizeof(command) , "ntfsck -a %s", source);
            (void)system(command);

            memset(command, '\0', sizeof(command));

            if (!s_bForceMountNTFS)
            {
                snprintf(command, sizeof(command) , "mount -t tntfs %s %s %s", acMountParam, source, target);
            }
            else
            {
                snprintf(command, sizeof(command) , "mount -t tntfs -o force %s %s", source, target);
            }

            ret = system(command);

            if (0 == ret)
            {
                UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_SCANMOUNTED);
            }

            return ret;
        }

        UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_MOUNTED);
        return ret;
    }
    else
    {
        /*EXT2/EXT3*/
        ret = mount(source, target, filesystemtype, 0, NULL);
        HI_LOGD("ret = %d, errno = 0x%x", ret, errno);

        /* If mount failed, try again. */
        if (0 != ret)
        {
            memset(command, '\0', sizeof(command));
            snprintf(command, sizeof(command), "mount -t %s %s %s", filesystemtype, source, target);
            ret = system(command);
            HI_LOGD("mount storage again.ret = %d", ret);
        }

        if (0 == ret)
        {
            UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_MOUNTED);
        }
    }

    return ret;
}

static HI_S32 UDISK_Mount(const char* source, const char* target, const char* fs, HI_UDISK_FSTYPE_E* fs_mnt)
{
    HI_S32 s32Ret;
    UDISK_PARTITIONTYPE_S stFormat;

    if (NULL == source || NULL == target)
    {
        return HI_FAILURE;
    }

    /* The specified format mount*/
    if (fs != HI_NULL)
    {
        return UDISK_MountByFormat(source, target, fs, HI_NULL, fs_mnt);
    }

    if (s_bUseblkidtool)
    {
        memset(&stFormat, 0, sizeof(stFormat));
        s32Ret = UDISK_GetPartitionFileSystem(source, &stFormat);

        if (HI_SUCCESS == s32Ret && 0 != strlen(stFormat.szFileSystemType))
        {
            s32Ret = UDISK_MountByFormat(source, target, stFormat.szFileSystemType, HI_NULL, HI_NULL);

            if (HI_SUCCESS != s32Ret)
            {
                UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_UNKNOWN);
            }
            else if (0 == strncasecmp("vfat", stFormat.szFileSystemType, 4) && fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_FAT32;
            }
            else if (0 == strncasecmp("ntfs", stFormat.szFileSystemType, 4) && fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_NTFS;
            }
            else if (0 == strncasecmp("ext2", stFormat.szFileSystemType, 4)  && fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_EXT2;
            }
            else if (0 == strncasecmp("ext3", stFormat.szFileSystemType, 4) && fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_EXT3;
            }
            else if (fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_BUT;
            }

            return s32Ret;
        }
    }
    else  //use enum method to mount disk parititon.
    {
        if (0 == UDISK_MountByFormat(source, target, "vfat", HI_NULL, NULL))
        {
            if (fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_NTFS;
            }

            return HI_SUCCESS;
        }

        if (0 == UDISK_MountByFormat(source, target, "ext3", HI_NULL, NULL))
        {
            if (fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_EXT3;
            }

            return HI_SUCCESS;
        }

        if (0 == UDISK_MountByFormat(source, target, "ext2", HI_NULL, NULL))
        {
            if (fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_EXT2;
            }

            return HI_SUCCESS;
        }

        if (0 == UDISK_MountByFormat(source, target, "tntfs", HI_NULL, NULL))
        {
            if (fs_mnt)
            {
                *fs_mnt = HI_UDISK_FSTYPE_NTFS;
            }

            return HI_SUCCESS;
        }
    }

    UDISK_RecordPartitionStatus(source, UDISK_PARTITION_STATUS_UNKNOWN);
    return HI_FAILURE;
}

static HI_S32 UDISK_Umount(const char* target)
{
    HI_S32 ret;

    /* 2: MNT_DETACH:
    ** Perform a lazy unmount: make the mount point unavailable for new accesses,
    ** and actually perform the unmount when the mount point ceases  to be busy. */
    ret = umount2(target, 2);

    if (ret < 0)
    {
        HI_LOGE("umount %s %d", target, ret);
        HI_LOGD("<ERROR>umount %s failed!\n", target);
    }

    return ret;
}

static HI_S32 UDISK_GetDiskIndexByDevPath(const HI_CHAR* pDevPath, HI_U32* pu32DiskIndex)
{
    HI_U32 disk_index;
    UDISK_DiskNode_S* pos_disk = HI_NULL;

    UDISK_Lock();

    disk_index = 0;
    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pos_disk, &m_UDisk_DiskListHead, disk_node)
    {
        if (strncmp(pDevPath, pos_disk->DiskInfo.szDevPath, MAX_UDISK_NAME_LEN) == 0)
        {
            // disk found!
            *pu32DiskIndex = disk_index;

            UDISK_UnLock();
            return HI_SUCCESS;
        }

        disk_index++;
    }

    // disk not exist!
    UDISK_UnLock();
    return HI_FAILURE;
}

static HI_S32 UDISK_GetPartIndexByDevPath(const HI_CHAR* pDevPath, HI_U32* pu32DiskIndex, HI_U32* pu32PartIndex)
{
    HI_U32 disk_index, part_index;
    UDISK_DiskNode_S* pos_disk = HI_NULL;
    UDISK_PartNode_S* pos_part = HI_NULL;

    UDISK_Lock();

    // Find disk
    disk_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pos_disk, &m_UDisk_DiskListHead, disk_node)
    {
        part_index = 0;
        /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055 -e516 -e613*/
        list_for_each_entry(pos_part, &(pos_disk->part_list_head), part_node)
        {
            if (strncmp(pDevPath, pos_part->PartInfo.szDevPath, MAX_UDISK_NAME_LEN) == 0)
            {
                // partition found
                *pu32DiskIndex = disk_index;
                *pu32PartIndex = part_index;
                UDISK_UnLock();
                return HI_SUCCESS;
            }

            part_index++;
        }
        disk_index++;
    }

    // partition not exist!
    UDISK_UnLock();
    return HI_FAILURE;
}

static HI_VOID UDISK_AttachPart(UDISK_DiskNode_S* pDiskNode)
{
    HI_S32 i = 1, j = 0;
    HI_S32 s32Ret = HI_FAILURE;
    HI_CHAR szFile[20];
    HI_CHAR* pDevName = HI_NULL;
    UDISK_PartNode_S* pPartNode = HI_NULL;
    HI_UDISK_PARTITIONINFO_S* pPart = HI_NULL;
    HI_CHAR szPartDevPath[MAX_UDISK_NAME_LEN] = "";
    HI_CHAR szPartMntPath[MAX_UDISK_NAME_LEN] = "";
    HI_UDISK_EVENT_PARAM_MOUNT_S param;
    HI_BOOL partitioned = HI_FALSE;
    HI_BOOL bPartMounted = HI_FALSE;
    HI_U32 u32MountCount = 0;
    HI_S32 u32Fd = 0;
    HI_S32 s32count = 0;

    memset(&param, 0, sizeof(param));

    for (i = 1; i <= UDISK_MAX_TRY_NUM; i++)
    {
        partitioned = HI_FALSE;
        memset(szFile, '\0', sizeof(szFile));
        memset(szPartDevPath, '\0', sizeof(szPartDevPath));
        memset(szPartMntPath, '\0', sizeof(szPartMntPath));

        snprintf(szFile, sizeof(szFile), "%s%d", pDiskNode->DiskInfo.szDevPath, i);

        s32Ret = access(szFile, F_OK);

        if (HI_SUCCESS != s32Ret)
        {
            if (1 == i)
            {
                for (j = 0; j < 10; j++)
                {
                    usleep(50 * 1000);
                    s32Ret = access(szFile, F_OK);

                    if (HI_SUCCESS == s32Ret)
                    {
                        partitioned = HI_TRUE;
                        break;
                    }
                }

                if (j >= 10)
                {
                    partitioned = HI_FALSE;
                }
            }
            else
            {
                /* if mount after umountAll, try more times for every partitions.*/
                if (HI_TRUE == s_bUmountAll)
                {
                    s32count = 3;   //try times

                    do
                    {
                        usleep(50 * 1000);
                        u32Fd = -1;
                        u32Fd = open(szFile, O_RDONLY);
                        HI_LOGD("try another times for %s: fd = %d ", szFile, u32Fd);

                        if (0 < u32Fd)
                        {
                            HI_LOGD("try another times for %s. success. ", szFile);
                            partitioned = HI_TRUE;
                            close(u32Fd);
                            break;
                        }

                        close(u32Fd);
                        s32count--;
                    } while((3 >= i) && (0 < s32count));    //try to check multiple times for first 3 partitions.
                }
            }


            if (HI_FALSE == partitioned)
            {
                if (i > 1 && i <= UDISK_MAX_TRY_NUM)
                {
                    HI_LOGD("++++ dev %s isn't exist.\n", szFile);
                    continue;
                }
            }
        }
        else
        {
            partitioned = HI_TRUE;
        }

        pDevName = (HI_CHAR*)strrchr(pDiskNode->DiskInfo.szDevPath, '/');

        if (HI_NULL == pDevName)
        {
            HI_LOGE("no device");
            return;
        }

        pDevName++;

        if (HI_TRUE == partitioned)
        {
            snprintf(szPartDevPath, sizeof(szPartDevPath), "%s%d", pDiskNode->DiskInfo.szDevPath, i);
        }
        else
        {
            snprintf(szPartDevPath, sizeof(szPartDevPath), "%s", pDiskNode->DiskInfo.szDevPath);
        }

        snprintf(szPartMntPath, sizeof(szPartMntPath), "%s/%s/%s%d", m_UDisk_Root, pDevName, pDevName, i);

        s32Ret = mkdir(szPartMntPath, 0777);

        if (s32Ret < 0)
        {
            HI_LOGE("mkdir");
            HI_LOGD("<ERROR>make usb disk dir :%s error\r\n", szPartMntPath);

            if (errno == EEXIST)
            {
                HI_LOGD("<FATAL ERROR>%s should not exist!\n", szPartMntPath);
            }
        }

        pPartNode = (UDISK_PartNode_S*)UDISK_MALLOC(sizeof(UDISK_PartNode_S));

        if (pPartNode == HI_NULL)
        {
            break;
        }

        memset(pPartNode, 0, sizeof(UDISK_PartNode_S));

        INIT_LIST_HEAD(&pPartNode->part_node);
        pPartNode->bFullReport = HI_FALSE;
        pPartNode->u64WaterLine = 0;

        pPart = &pPartNode->PartInfo;

        if (1 == i)
        {
            pPart->bMounted = HI_FALSE;
        }

        bPartMounted = HI_FALSE;

        if (UDISK_Mount(szPartDevPath, szPartMntPath, HI_NULL, &pPart->enFSType) >= 0)
        {
            //(HI_VOID)UDISK_StatFs(szPartMntPath, &pPart->u64TotalSize, &pPart->u64AvailSize);
            pPart->bMounted = HI_TRUE;
            bPartMounted = HI_TRUE;
            u32MountCount++;
            HI_LOGD("++++ mount %s %s success\n", szPartDevPath, szPartMntPath);
        }
        else
        {
            HI_LOGD("++++ mount %s %s error\n", szPartDevPath, szPartMntPath);
        }

        if (HI_TRUE == bPartMounted)
        {
            UDISK_Lock();
            list_add_tail(&pPartNode->part_node, &pDiskNode->part_list_head);
            UDISK_UnLock();

            strncpy(pPart->szDevPath, szPartDevPath, MAX_UDISK_NAME_LEN);
            pPart->szDevPath[MAX_UDISK_NAME_LEN - 1] = '\0';
            strncpy(pPart->szMntPath, szPartMntPath, MAX_UDISK_NAME_LEN);
            pPart->szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';

            strncpy(param.szMntPath, pPart->szMntPath, MAX_UDISK_NAME_LEN);
            param.szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';
            (HI_VOID)UDISK_GetPartIndexByDevPath(pPart->szDevPath, &param.u32DiskIndex, &param.u32PartitionIndex);
            UDISK_EventCallback(HI_UDISK_EVENT_MOUNTED, &param);
        }
        else
        {
            if (pPartNode)
            {
                free(pPartNode);
                pPartNode = HI_NULL;
            }

            s32Ret = rmdir(szPartMntPath);

            if (s32Ret)
            {
                HI_LOGE("\nUnable to delete directory\n");
            }
        }

        if (UDISK_MAX_DEV_NUM <= u32MountCount)
        {
            HI_LOGD("mounted partition num reaches maximum");
            break;
        }
    }

    if ((i >= UDISK_MAX_TRY_NUM) && (0 < u32MountCount))//if all partition have been checked and exists partition just been mounted.
    {
        HI_LOGD("++++ All partitions has been mounted.\n");
        memset(param.szMntPath, '\0', sizeof(param.szMntPath));
        strncpy(param.szMntPath, pDiskNode->DiskInfo.szMntPath, MAX_UDISK_NAME_LEN - 1);
        UDISK_EventCallback(HI_UDISK_EVENT_ALLPARTMOUNTED, &param);
    }

    return; /*lint !e429 */
}

static UDISK_DiskNode_S* UDISK_FindDisk(const HI_CHAR* pDevPath)
{
    UDISK_DiskNode_S* pos = HI_NULL;

    UDISK_Lock();

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pos, &m_UDisk_DiskListHead, disk_node)
    {
        if (strncmp(pDevPath, pos->DiskInfo.szDevPath, MAX_UDISK_NAME_LEN) == 0)
        {
            UDISK_UnLock();
            return pos;
        }
    }

    UDISK_UnLock();
    return HI_NULL;
}

static HI_VOID UDISK_UMountDisk(HI_CHAR* pDiskMntPath)
{
    DIR* dir = NULL;
    struct dirent* pEntry = NULL;

    HI_CHAR szPartMntPath[MAX_UDISK_NAME_LEN] = "";

    dir = opendir(pDiskMntPath);

    if (HI_NULL == dir)
    {
        HI_LOGD("dir is null!\n");
        return;
    }

    while (NULL != (pEntry = readdir(dir)))
    {
        if ( (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1))
             || (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1)))
        {
            continue ;
        }

        snprintf(szPartMntPath, sizeof(szPartMntPath), "%s/%s", pDiskMntPath, pEntry->d_name);
        (HI_VOID)UDISK_Umount(szPartMntPath);
        (HI_VOID)rmdir(szPartMntPath);
    }

    (HI_VOID)closedir(dir);
}

static HI_VOID UDISK_UMountDisks(HI_CHAR* root)
{
    DIR* dir = NULL;
    struct dirent* pEntry = NULL;
    HI_CHAR szDiskMntPath[MAX_UDISK_NAME_LEN] = "";

    dir = opendir(root);

    if (dir == NULL)
    {
        HI_LOGD("<ERROR>make sure %s is an valid directory\n", root);
        return ;
    }

    while (NULL != (pEntry = readdir(dir)))
    {
        if ( (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1)) || (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1)))
        {
            continue ;
        }

        if (0 == strncmp(pEntry->d_name, "sd", 2) && (3 == strlen(pEntry->d_name)))
        {
            snprintf(szDiskMntPath, sizeof(szDiskMntPath), "%s/%s", root, pEntry->d_name);
            UDISK_UMountDisk(szDiskMntPath);
            (HI_VOID)rmdir(szDiskMntPath);
        }

    }

    (HI_VOID)closedir(dir);
}

/* Get Disk info (eg. Vender and so on) and partition info (eg. total size and available size). */
static HI_VOID UDISK_GetDiskInfo(UDISK_DiskNode_S* pDiskNode)
{
    HI_UDISK_DISKINFO_S* pDisk = HI_NULL;
    UDISK_PartNode_S* pPartNode = HI_NULL;
    HI_UDISK_PARTITIONINFO_S* pPartition = HI_NULL;

    pDisk = &pDiskNode->DiskInfo;
    (HI_VOID)UDISK_StatDisk(pDisk->szDevPath, pDisk->szVendor, pDisk->szModel);

    list_for_each_entry(pPartNode, &(pDiskNode->part_list_head), part_node)
    {
        pPartition = &pPartNode->PartInfo;
        (HI_VOID)UDISK_StatFs(pPartition->szMntPath, &pPartition->u64TotalSize, &pPartition->u64AvailSize);
    }
}

static HI_VOID UDISK_AttachDisk(const HI_CHAR* pDevPath)
{
    HI_S32 s32Ret = 0;

    HI_CHAR szDiskDevPath[MAX_UDISK_NAME_LEN] = "";
    HI_CHAR szDiskMntPath[MAX_UDISK_NAME_LEN] = "";
    HI_CHAR szTemp[MAX_UDISK_NAME_LEN] = "";

    HI_CHAR* pDevName = HI_NULL;
    UDISK_DiskNode_S* pDiskNode = HI_NULL;
    HI_UDISK_DISKINFO_S* pDisk;
    HI_UDISK_EVENT_PARAM_PLUG_S param;

    memset(szDiskDevPath, '\0', sizeof(szDiskDevPath));
    memset(szDiskMntPath, '\0', sizeof(szDiskMntPath));
    memset(szTemp, '\0', sizeof(szTemp));

    /*Check whether the device has already been mounted, if mounted do noting*/
    /*检查设备是否已经加载，如果已经加载则不做任何处理*/
    if (HI_NULL != UDISK_FindDisk(pDevPath))
    {
        return;
    }

    if (!UDISK_ISMediaDevice(pDevPath))
    {
        return;
    }

    pDevName = (HI_CHAR*)strrchr(pDevPath, '/');

    if (HI_NULL == pDevName)
    {
        return;
    }

    pDevName++;

    snprintf(szDiskMntPath, sizeof(szDiskMntPath), "%s/%s", m_UDisk_Root, pDevName);

    /*Check whether the directory has already been exist, if exist umounted, otherwise created*/
    /* 判断mount的目的目录是否存在。没有则创建之；有则存在异常*/
    s32Ret = mkdir(szDiskMntPath, 0777);

    if (s32Ret < 0)
    {
        if (errno == EEXIST)
        {
            UDISK_UMountDisk(szDiskMntPath);
        }
        else
        {
            // should not go here!!
            HI_LOGE("mkdir");
            HI_LOGD("<ERROR>make usb disk dir :%s error\r\n", szDiskMntPath);
        }
    }

    /*Create disk node*//* 创建新的Disk节点*/
    pDiskNode = (UDISK_DiskNode_S*)UDISK_MALLOC(sizeof(UDISK_DiskNode_S));

    if (pDiskNode == HI_NULL)
    {
        return;
    }

    memset(pDiskNode, 0, sizeof(UDISK_DiskNode_S));

    /* assignment to disk node*/
    pDisk = &pDiskNode->DiskInfo;
    strncpy(pDisk->szDevPath, pDevPath, MAX_UDISK_NAME_LEN);
    pDisk->szDevPath[MAX_UDISK_NAME_LEN - 1] = '\0';
    strncpy(pDisk->szMntPath, szDiskMntPath, MAX_UDISK_NAME_LEN);
    pDisk->szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';

    //(HI_VOID)UDISK_StatDisk(pDisk->szDevPath, pDisk->szVendor, pDisk->szModel);

    INIT_LIST_HEAD(&pDiskNode->part_list_head);

    /*Add to list*/
    UDISK_Lock();
    list_add_tail(&(pDiskNode->disk_node), &m_UDisk_DiskListHead);
    UDISK_UnLock();

    /*notify the plug in event*/
    memset(param.szMntPath, '\0', sizeof(param.szMntPath));
    strncpy(param.szMntPath, pDisk->szMntPath, MAX_UDISK_NAME_LEN);
    param.szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';
    (HI_VOID)UDISK_GetDiskIndexByDevPath(pDisk->szDevPath, &param.u32DiskIndex);
    UDISK_EventCallback(HI_UDISK_EVENT_PLUGIN, &param);

    /*Add the Partitions of disk*/
    UDISK_InitDiskDB();
    UDISK_AttachPart(pDiskNode);
    UDISK_GetDiskInfo(pDiskNode);
    UDISK_ReportDiskRemainEvent();
}

static HI_VOID UDISK_DetachDisk(const HI_CHAR* pDevPath)
{
    HI_S32 s32Ret;
    HI_U32 u32CheckUdiskTimes = 3;

    UDISK_DiskNode_S* pDiskNode = HI_NULL;
    UDISK_PartNode_S* pPartNode = HI_NULL;
    UDISK_PartNode_S* n =  HI_NULL;
    //UDISK_DiskNode_S * pos_disk = HI_NULL;
    HI_UDISK_EVENT_PARAM_PLUG_S  param_plg;
    HI_UDISK_EVENT_PARAM_MOUNT_S param_mnt;

    /*Find the node of disk/partition, delete*//*找到对应的disk/part节点，删除之*/
    pDiskNode = UDISK_FindDisk(pDevPath);

    if (pDiskNode == HI_NULL)
    {
        /*Can`t find return*/
        return;
    }

    UDISK_Lock();
    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry_safe(pPartNode, n, &(pDiskNode->part_list_head), part_node)
    {
        /*if have been mounted, unmounted and delete*//* 如果已经加载，则卸载，并删除目录*/
        if (HI_TRUE == pPartNode->PartInfo.bMounted)
        {
            /* umounted event notified*/
            (HI_VOID)UDISK_GetPartIndexByDevPath(pPartNode->PartInfo.szDevPath, \
                                                 &param_mnt.u32DiskIndex, &param_mnt.u32PartitionIndex);

            memset(param_mnt.szMntPath, 0, sizeof(param_mnt.szMntPath));
            strncpy(param_mnt.szMntPath, pPartNode->PartInfo.szMntPath, MAX_UDISK_NAME_LEN);
            param_mnt.szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';
            UDISK_EventCallback(HI_UDISK_EVENT_UMOUNTED, &param_mnt);

            while (u32CheckUdiskTimes > 0)
            {
                s32Ret = UDISK_Umount(pPartNode->PartInfo.szMntPath);

                if (HI_SUCCESS == s32Ret)
                {
                    break;
                }

                (HI_VOID)sleep(1);
                u32CheckUdiskTimes--;
            }

            (HI_VOID)rmdir(pPartNode->PartInfo.szMntPath);
        }
    }

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry_safe(pPartNode, n, &(pDiskNode->part_list_head), part_node)
    {
        /*delete the partition from the list*//* 从分区链表中删除该分区*/
        list_del(&pPartNode->part_node);
        UDISK_FREE(pPartNode);
    }

    /* plugout event callback*/
    memset(param_plg.szMntPath, '\0', sizeof(param_plg.szMntPath));
    strncpy(param_plg.szMntPath, pDiskNode->DiskInfo.szMntPath, MAX_UDISK_NAME_LEN);
    param_plg.szMntPath[MAX_UDISK_NAME_LEN - 1] = '\0';
    (HI_VOID)UDISK_GetDiskIndexByDevPath(pDiskNode->DiskInfo.szDevPath, &param_plg.u32DiskIndex);
    UDISK_EventCallback(HI_UDISK_EVENT_PLUGOUT, &param_plg);

    /*delete the disk path*//* 删除disk目录*/
    (HI_VOID)rmdir(pDiskNode->DiskInfo.szMntPath);

    /*delete the disk from disk list*//* 从磁盘链表中删除该磁盘*/
    list_del(&pDiskNode->disk_node);

    UDISK_FREE(pDiskNode);

    UDISK_UnLock();
}

static HI_VOID UDISK_UmontAllDisk(HI_VOID)
{
    HI_S32 s32Ret;
    UDISK_DiskNode_S* pos = HI_NULL;
    UDISK_DiskNode_S* n = HI_NULL;

    UDISK_Lock();
    list_for_each_entry_safe(pos, n, &m_UDisk_DiskListHead, disk_node)
    {
        /*If the device can be access. uMount it.*/
        s32Ret = access(pos->DiskInfo.szDevPath, F_OK);

        if (HI_SUCCESS == s32Ret)
        {
            HI_LOGD("++++++ Umount %s+++++++++++\n", pos->DiskInfo.szDevPath);
            UDISK_DetachDisk(pos->DiskInfo.szDevPath);
        }
    }
    UDISK_UnLock();
}

static HI_VOID UDISK_CheckDiskPlugInOut(HI_VOID)
{
    HI_S32 s32Ret = HI_FAILURE;
    DIR* dir = HI_NULL;
    struct dirent* pEntry = NULL;
    HI_CHAR szDiskDevPath[MAX_UDISK_NAME_LEN] = "";
    UDISK_DiskNode_S* pos = HI_NULL;
    UDISK_DiskNode_S* n = HI_NULL;

    UDISK_Lock();
    list_for_each_entry_safe(pos, n, &m_UDisk_DiskListHead, disk_node)
    {
        /*If the device can be access. uMount it.*/
        s32Ret = access(pos->DiskInfo.szDevPath, F_OK);

        if (HI_SUCCESS != s32Ret)
        {
            HI_LOGD("++++++ Umount %s+++++++++++\n", pos->DiskInfo.szDevPath);
            UDISK_DetachDisk(pos->DiskInfo.szDevPath);
            UDISK_RemoveROList(pos->DiskInfo.szDevPath);
        }
    }
    UDISK_UnLock();

    dir = opendir(UDISK_USB_DEV_ROOT);

    if (NULL == dir)
    {
        HI_LOGD("can't open directory %s !\r\n", UDISK_USB_DEV_ROOT);
        return;
    }

    while (NULL != (pEntry = readdir(dir)))
    {
        /*Find sdx*//* 找到 sdx */
        if (0 == strncmp(pEntry->d_name, "sd", 2) && (3 == strlen(pEntry->d_name)))
        {
            memset(szDiskDevPath, '\0', sizeof(szDiskDevPath));
            snprintf(szDiskDevPath, sizeof(szDiskDevPath), "%s/%s", UDISK_USB_DEV_ROOT, pEntry->d_name);
            UDISK_AttachDisk(szDiskDevPath);
        }

        /* Find /dev/sr0, some 3g card(auto mode) as scsi CD-ROM device */
        struct stat statBuf;

        if (0 == strncmp(pEntry->d_name, "sr0", 3) && 0 != stat("/dev/ttyUSB0", &statBuf))
        {
            static HI_S32 s32CheckCount = 0;

            if (0 == s32CheckCount++)
            {
                HI_LOGD("eject /dev/sr0(scsi CD-ROM)\n");
                system("eject /dev/sr0");
            }

            if (s32CheckCount > 10)
            {
                s32CheckCount = 0;
            }
        }
    }

    s_bUmountAll = HI_FALSE;

    (HI_VOID)closedir(dir);
    return ;
}

static HI_VOID UDISK_CheckPartitionFull(HI_VOID)
{
    HI_U32 disk_index, part_index;
    UDISK_DiskNode_S* pDiskNode = HI_NULL;
    UDISK_PartNode_S* pPartNode = HI_NULL;
    HI_U64 u64TotalSize, u64AvailSize;
    HI_UDISK_EVENT_PARAM_FULL_S param;

    UDISK_Lock();

    // Find disk
    disk_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pDiskNode, &m_UDisk_DiskListHead, disk_node)
    {
        // disk found, find partition
        part_index = 0;
        /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055 -e613*/
        list_for_each_entry(pPartNode, &(pDiskNode->part_list_head), part_node)
        {
            if (pPartNode->bFullReport == HI_TRUE)
            {
                if (UDISK_StatFs(pPartNode->PartInfo.szMntPath, &u64TotalSize, &u64AvailSize) == HI_SUCCESS)
                {
                    if (u64AvailSize <= pPartNode->u64WaterLine)
                    {
                        /*FULL event callback*/
                        param.u32DiskIndex = disk_index;
                        param.u32PartitionIndex = part_index;

                        /*Note: maybe deadlock if user used the udisk interface*//* 用户函数回调中，有可能会调用UDISK的接口引起死锁*/
                        UDISK_EventCallback(HI_UDISK_EVENT_FULL, &param);
                    }
                }
            }

            part_index++;
        }
        disk_index++;
    }

    // partition not exist!
    UDISK_UnLock();
}

/* filter whether have the extension,if have return HI_SUCCESS, otherwise return HI_FAILURE */
/* filter包含指定的扩展名，则返回HI_SUCCESS，否则返回HI_FAILURE */
static HI_S32 UDISK_CheckFILTER(const HI_CHAR* filename,
                                HI_UDISK_FILTER_E eFilterType,
                                const HI_CHAR* filter)
{
    HI_CHAR* pSuffix = NULL;
    HI_U32 SuffixLen;
    HI_CHAR* pSuffixFlt = NULL;

    if (HI_NULL == filter)
    {
        return HI_FAILURE;
    }

    // find ".*"
    if (strncmp(filter, ".*", strlen(".*") + 1) == 0)
    {
        return HI_SUCCESS;
    }

    pSuffix = strrchr((char*)filename, '.');

    if (NULL == pSuffix)
    {
        return HI_FAILURE;
    }

    SuffixLen = strlen(pSuffix);

    /*traverses all suffix*//* 遍历flt中所有的suffix */
    for (;;)
    {
        if (pSuffixFlt == NULL)
        {
            /*first *//* 第一个flt*/
            pSuffixFlt = strchr(filter, '.');/*lint !e605*/
        }
        else
        {
            /*the next*//* 后面的flt*/
            pSuffixFlt = strchr(pSuffixFlt + 1, '.');
        }

        /*last filter done*//* 最后一个flt已经处理了 */
        if (pSuffixFlt == NULL)
        {
            return HI_FAILURE;
        }

        if (eFilterType & HI_UDISK_FLT_CASECARE) /*lint !e655 */
        {
            /*Consider case difference*//* 比较时，考虑大小写差异*/
            if (strncmp(pSuffixFlt, pSuffix, SuffixLen) == 0)
            {
                return HI_SUCCESS;
            }
        }
        else
        {
            /*Case-insensitive difference*//* 比较时，忽略大小写差异*/
            if (strncasecmp(pSuffixFlt, pSuffix, SuffixLen) == 0)
            {
                return HI_SUCCESS;
            }
        }
    }
}

static HI_S32 UDISK_AddFileToList(const HI_CHAR* pathname,
                                  const HI_CHAR* filename,
                                  HI_UDISK_FILELIST_S* pFileList)
{
    HI_UDISK_FILEINFO_S* pNewFile = NULL;
    struct stat64 buf;
    HI_CHAR* pos = NULL;

    pNewFile = (HI_UDISK_FILEINFO_S*)UDISK_MALLOC(sizeof(HI_UDISK_FILEINFO_S));

    if ( NULL == pNewFile )
    {
        HI_LOGD("no mem for new file node!\n");
        return HI_UDISK_ERR_NOMEM;
    }

    pNewFile->u32Index = pFileList->u32FileNum;
    pNewFile->u64Size = 0;
    pNewFile->u32CTime = 0;
    pNewFile->pszShortName = UDISK_MALLOC(strlen(filename) + 1);

    if (NULL == pNewFile->pszShortName)
    {
        free(pNewFile);
        return HI_FAILURE;
    }

    pNewFile->pszFullName = UDISK_MALLOC(strlen(pathname) + strlen("/") + strlen(filename) + 1);

    if (NULL == pNewFile->pszFullName)
    {
        free(pNewFile->pszShortName);
        free(pNewFile);
        return HI_FAILURE;
    }

    memset(pNewFile->pszShortName, '\0', strlen(filename) + 1);
    memset(pNewFile->pszFullName, '\0', strlen(pathname) + strlen("/") + strlen(filename) + 1);

    strncpy(pNewFile->pszShortName, filename, strlen(filename) + 1);

    if (0 == strncmp(filename, ".", strlen(".") + 1))
    {
        pNewFile->u32Type = 0;
        strncpy(pNewFile->pszFullName, pathname, strlen(pathname) + 1);
    }
    else if (0 == strncmp(filename, "..", strlen("..") + 1))
    {
        pNewFile->u32Type = 0;
        strncpy(pNewFile->pszFullName, pathname, strlen(pathname) + 1);

        if (0 == strncmp(pNewFile->pszFullName, m_UDisk_Root, strlen(pNewFile->pszFullName) + 1))
        {
            /*Reach USB Disk Root Dir, Enter Upper Layer Dir is forbidden*/
        }
        else if (0 == strncmp(pNewFile->pszFullName, "/", strlen("/") + 1))
        {
            /*Already Linux Root Dir*/
        }
        else
        {
            pos = strrchr(pNewFile->pszFullName, (int)'/');

            if (NULL == pos)
            {
                /*UnHandled Path Name*/
            }
            else if (pos == pNewFile->pszFullName)
            {
                /*Reach Linux Root Dir*/
                *(pos + 1) = '\0';
            }
            else
            {
                *pos = '\0';
            }
        }
    }
    else
    {
        strncpy(pNewFile->pszFullName, pathname, strlen(pathname) + 1);
        strncat(pNewFile->pszFullName, "/", strlen("/") + 1);
        strncat(pNewFile->pszFullName, filename, strlen(filename) + 1);
    }

    /*y00107738 add*/
    if (!stat64(pNewFile->pszFullName, &buf))
    {
        pNewFile->u64Size = buf.st_size;
        pNewFile->u32CTime = (HI_U32)buf.st_ctime;
    }

    if (pFileList->u32FileNum < UDISK_RESERVE_MAX_FILE_NUM)
    {
        pFileList->ppFileList[pFileList->u32FileNum] = pNewFile;
        pFileList->u32FileNum++;
    }
    else
    {
        HI_LOGD("This folder has more than 2048 files, read file fails.\n");
        UDISK_FREE(pNewFile->pszShortName);
        pNewFile->pszShortName = HI_NULL;
        UDISK_FREE(pNewFile->pszFullName);
        pNewFile->pszFullName = HI_NULL;
        UDISK_FREE(pNewFile);
        pNewFile = HI_NULL;
        return HI_FAILURE;
#if 0

        if ((pFileList->u32FileNum < 10240) && (HI_FALSE == s_ReallocMem))
        {
            s_ReallocMem = HI_TRUE;
            pFileList->ppFileList = realloc(pFileList->ppFileList, sizeof(HI_UDISK_FILEINFO_S*) * 10240);
            pFileList->ppFileList[pFileList->u32FileNum] = pNewFile;
            pFileList->u32FileNum++;
        }
        else
        {
            HI_LOGD("This folder has more than 10240 files, read file fails.\n");
            UDISK_FREE(pNewFile->pszShortName);
            UDISK_FREE(pNewFile->pszFullName);
            UDISK_FREE(pNewFile);
            return HI_FAILURE;
        }

#endif
    }

    return HI_SUCCESS;
}

static HI_S32 UDISK_ScanDir(HI_U32 u32Pid, const HI_CHAR* pszDir,
                            HI_UDISK_SCAN_S* pScanOpt,
                            HI_UDISK_FILELIST_S* pFileList)
{
    HI_CHAR subFilePath[MAX_UDISK_NAME_LEN] = {0};
    HI_U32 subFilePathLen = 0;
    DIR* dir = NULL;
    struct dirent* pEntry = NULL;
    HI_S32 s32Ret = 0;

    dir = opendir(pszDir);

    if (NULL == dir)
    {
        HI_LOGE("null dir");
        return HI_UDISK_ERR_NOTEXIST;
    }

    while (NULL != (pEntry = readdir(dir)))
    {
        if (UDISK_RESERVE_MAX_FILE_NUM <= pFileList->u32FileNum)    //files more than 2048 are ignored.
        {
            HI_LOGE("2048 files have been read. exit reading files.");
            break;
        }

        /* query whether to interrupt getting filelist. */
        s32Ret = UDISK_EventCallback(HI_UDISK_EVENT_QUERY_INTERRUPTFLAG, (HI_VOID*)&u32Pid);

        if ((HI_BOOL)s32Ret)
        {
            HI_LOGD("Stop getting file of pid(%d).", u32Pid);
            (HI_VOID)closedir(dir);
            return HI_FAILURE;
        }

        /* "." */
        if (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_CURDIR) /*lint !e655*/
            {
                (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
            }
        }
        else if (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_UPDIR) /*lint !e655*/
            {
                (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
            }
        }
        else
        {
            subFilePathLen = strlen(pszDir) + strlen("/") + strlen(pEntry->d_name) + 1;

            if (MAX_UDISK_NAME_LEN < subFilePathLen)
            {
                HI_LOGD("<WARNING>%s, %d : path length (%d) is too long\r\n",
                        __FUNCTION__, __LINE__, subFilePathLen);
                continue;
            }

            memset(subFilePath, 0x0, MAX_UDISK_NAME_LEN);
            strncpy(subFilePath, pszDir, sizeof(subFilePath) - 1);
            strncat(subFilePath, "/", sizeof(subFilePath) - strlen(subFilePath) - 1);
            strncat(subFilePath, pEntry->d_name, sizeof(subFilePath) - strlen(subFilePath) - 1);

            if (pEntry->d_type == DT_DIR)   //if  it is directory
            {
                if (pScanOpt->enFilterType & HI_UDISK_FLT_SUBDIR) /*lint !e655*/
                {
                    (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                    pFileList->ppFileList[pFileList->u32FileNum - 1]->u32Type = 0;
                }

                if (HI_TRUE == pScanOpt->bRecursive)
                {
                    (HI_VOID)UDISK_ScanDir(u32Pid, subFilePath, pScanOpt, pFileList);
                }
            }
            else if (pEntry->d_type == DT_REG)  //if it is file
            {
                /*File filter*//* 文件过滤 */
                if (HI_SUCCESS == UDISK_CheckFILTER(pEntry->d_name, pScanOpt->enFilterType, pScanOpt->pFileFilter))
                {
                    (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                    pFileList->ppFileList[pFileList->u32FileNum - 1]->u32Type = 1;
                }
            }
        }
    }

    (HI_VOID)closedir(dir);

    return HI_SUCCESS;
}

static HI_BOOL m_UDisk_NotifyQuit = HI_FALSE;
static pthread_t m_Udisk_CheckThread = HI_NULL;

static HI_VOID* UDISK_CheckThread(const HI_VOID* args)
{
    while (m_UDisk_NotifyQuit != HI_TRUE)
    {
        /*Check whether plugin or plugout*/
        UDISK_CheckDiskPlugInOut();

        /*Check whether partition full*/
        UDISK_CheckPartitionFull();

        sync();
        //UDISK_MonitorDiskRWStatus(); //disable for sumaCA power GPIO problem.

        /*sleep 1s*/
        (HI_VOID)sleep(1);
    }

    return HI_NULL;
}

static HI_S32 UDISK_GetSetPartition(UDISK_PARTITON_OP_E enOp, HI_U32 u32DiskIndex, HI_U32 u32PartitionIndex, UDISK_PartNode_S* pPartition)
{
    HI_U32 disk_index, part_index;
    UDISK_DiskNode_S* pDiskNode = HI_NULL;
    HI_UDISK_DISKINFO_S* pDiskInfo = HI_NULL;;
    UDISK_PartNode_S* pPartNode = HI_NULL;
    //HI_UDISK_PARTITIONINFO_S* pPartitionInfo = HI_NULL;

    if (HI_NULL == pPartition)
    {
        return HI_FAILURE;
    }

    UDISK_Lock();

    // Find disk
    disk_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pDiskNode, &m_UDisk_DiskListHead, disk_node)
    {
        if (disk_index++ == u32DiskIndex)
        {
            pDiskInfo = &(pDiskNode->DiskInfo);
            break;
        }
    }

    if (pDiskInfo == HI_NULL)
    {
        // disk not exist!
        UDISK_UnLock();
        return HI_UDISK_ERR_NOTEXIST;
    }

    // disk found
    part_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pPartNode, &(pDiskNode->part_list_head), part_node)
    {
        if (part_index++ == u32PartitionIndex)
        {
            // partition found
            if (UDISK_PARTITION_OP_GET == enOp)
            {
                //pPartitionInfo = &pPartition->PartInfo;
                memcpy(pPartition, pPartNode, sizeof(UDISK_PartNode_S));
                //(HI_VOID)UDISK_StatFs(pPartitionInfo->szMntPath, &pPartitionInfo->u64TotalSize, &pPartitionInfo->u64AvailSize);
            }
            else
            {
                memcpy(pPartNode, pPartition, sizeof(UDISK_PartNode_S));
            }

            UDISK_UnLock();
            return HI_SUCCESS;
        }
    }

    // partition not exist!
    UDISK_UnLock();
    return HI_FAILURE;
}

HI_S32 UDISK_SetPartitionType(HI_U32 u32DiskIndex, HI_U32 u32PartitionIndex, HI_UDISK_FSTYPE_E enFSType)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UDISK_PartNode_S stPartition;

    s32Ret = UDISK_GetSetPartition(UDISK_PARTITION_OP_GET, u32DiskIndex, u32PartitionIndex, &stPartition);

    if (HI_SUCCESS == s32Ret)
    {
        stPartition.PartInfo.enFSType = enFSType;
        s32Ret = UDISK_GetSetPartition(UDISK_PARTITION_OP_SET, u32DiskIndex, u32PartitionIndex, &stPartition);
    }

    return s32Ret;
}

//HI_S32 HI_UDISK_Init(const HI_CHAR * pUdiskRoot)
HI_S32 HI_UDISK_Init(HI_UDISK_INIT_PARAM_S* pstInitParam)
{
    DIR* dir = NULL;
    pthread_mutexattr_t mutex_attr;

    if (pstInitParam == HI_NULL)
    {
        HI_LOGD("<ERROR>udisk root can't be NULL\n");
        return HI_FAILURE;
    }

#if 0 /* diable himount  */

    if (0 == access("/usr/bin/himount", F_OK))
    {
        s_bUseHimount = HI_TRUE;
    }

#endif
    s_bForceMountNTFS = pstInitParam->bForceMountNTFS;
    s_bUseblkidtool = pstInitParam->bUseblkidtool;
    // umount 1:  disks & parts
    UDISK_UMountDisks((HI_CHAR*)pstInitParam->acUdiskRoot);

    // umount 2: root
    (HI_VOID)UDISK_Umount(pstInitParam->acUdiskRoot);

    /* Check the pDiskRoot whether exist*/
    dir = opendir(pstInitParam->acUdiskRoot);

    if (dir == NULL)
    {
        HI_LOGE("opendir");
        HI_LOGD("%s not exist, so make it!\n", pstInitParam->acUdiskRoot);

        if (mkdir(pstInitParam->acUdiskRoot, 0777) < 0)
        {
            HI_LOGE("mkdir");
            HI_LOGD("<ERROR>make dir %s error\r\n", pstInitParam->acUdiskRoot);
            return HI_FAILURE;
        }

        dir = opendir(pstInitParam->acUdiskRoot);

        if (dir == NULL)
        {
            HI_LOGD("++++ Open dir %s failed\n", pstInitParam->acUdiskRoot);
            return HI_FAILURE;
        }
    }

#if 0
    struct dirent* pEntry = NULL;

    // pDiskRoot指向的目录是否为空
    while (NULL != (pEntry = readdir(dir)))
    {
        if ( (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1)) || (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1)))
        {
            continue ;
        }

        // 目录不为空
        HI_LOGD("<ERROR>%s is not empty!\n", pstInitParam->acUdiskRoot);
        (HI_VOID)closedir(dir);
        return HI_FAILURE;
    }

#endif
    (HI_VOID)closedir(dir);

    /* Mount pUdiskRoot to tmpfs*/
    //YLL: if (UDISK_Mount("nodevex", pstInitParam->acUdiskRoot, "tmpfs", NULL) < 0)
    //{
    //    return HI_FAILURE;
    //}

    /* Init the param*/
    m_UDisk_Root = (HI_CHAR*)UDISK_MALLOC(strlen(pstInitParam->acUdiskRoot) + 1);
    memset(m_UDisk_Root, '\0', strlen(pstInitParam->acUdiskRoot) + 1);
    strncpy(m_UDisk_Root, pstInitParam->acUdiskRoot, strlen(pstInitParam->acUdiskRoot));

    bzero(&m_UDisk_Event, sizeof(m_UDisk_Event));

    /*Init the mutex*/
    (HI_VOID)pthread_mutexattr_init(&mutex_attr);
    (HI_VOID)pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    if (0 != pthread_mutex_init(&m_UDisk_mutex, &mutex_attr ))
    {
        HI_LOGD("<ERROR>UDISK check mutex created failed\r\n");
        return HI_FAILURE;
    }

    /* Create the task for check the event*/
    m_UDisk_NotifyQuit = HI_FALSE;

    if (0 != pthread_create(&m_Udisk_CheckThread, NULL, (HI_VOID * (*)(HI_VOID*))UDISK_CheckThread, NULL))
    {
        HI_LOGD("<ERROR>UDISK check thread created failed\r\n");
        return HI_FAILURE;
    }

    (void)gettimeofday(&m_UDiskStartTime, NULL);
    strncpy(acMountParam, pstInitParam->acMountParam, HI_UDISK_PARAM_MAX_LEN);
    acMountParam[HI_UDISK_PARAM_MAX_LEN - 1] = '\0';
    return HI_SUCCESS;
}

HI_VOID HI_UDISK_UmountAllDisk(HI_VOID)
{
    UDISK_UmontAllDisk();
    s_bUmountAll = HI_TRUE;
}

HI_S32 HI_UDISK_Deinit(HI_VOID)
{
    /*Exit the task*/
    m_UDisk_NotifyQuit = HI_TRUE;

    if (HI_NULL != (HI_U32)m_Udisk_CheckThread)
    {
        (HI_VOID)pthread_join(m_Udisk_CheckThread, NULL);
        m_Udisk_CheckThread = HI_NULL;
    }

    UDISK_UmontAllDisk();

    (HI_VOID)UDISK_Umount(m_UDisk_Root);

    if (HI_NULL != m_UDisk_Root)
    {
        UDISK_FREE(m_UDisk_Root);
        m_UDisk_Root = HI_NULL;
    }

    (HI_VOID)pthread_mutex_destroy(&m_UDisk_mutex);
    //sem_destroy(&m_UDisk_sem);

    return HI_SUCCESS;
}

HI_CHAR* HI_UDISK_GetRoot(HI_VOID)
{
    return m_UDisk_Root;
}

HI_S32 HI_UDISK_GetDiskNum(HI_U32* pu32DiskNum)
{
    HI_U32 u32DiskNum = 0;
    struct list_head* pos;

    UDISK_Lock();
    list_for_each(pos, &m_UDisk_DiskListHead)
    {
        u32DiskNum++;
    }
    UDISK_UnLock();

    *pu32DiskNum = u32DiskNum;
    return HI_SUCCESS;
}

HI_S32 HI_UDISK_GetDiskInfo(HI_U32 u32DiskIndex, HI_UDISK_DISKINFO_S* pDiskInfo)
{
    HI_U32 disk_index;
    UDISK_DiskNode_S* pos = HI_NULL;

    UDISK_Lock();

    disk_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pos, &m_UDisk_DiskListHead, disk_node)
    {
        if (disk_index++ == u32DiskIndex)
        {
            // disk found!
            memcpy(pDiskInfo, &pos->DiskInfo, sizeof(HI_UDISK_DISKINFO_S));
            UDISK_UnLock();
            return HI_SUCCESS;
        }
    }

    // disk not exist!
    UDISK_UnLock();
    return HI_FAILURE;
}

HI_S32 HI_UDISK_GetPartitionNum(HI_U32 u32DiskIndex, HI_U32* pu32PartitionNum)
{
    HI_U32 disk_index, part_index;
    UDISK_DiskNode_S* pDiskNode = HI_NULL;
    HI_UDISK_DISKINFO_S* pDiskInfo = HI_NULL;;
    struct list_head* tmp = HI_NULL;

    UDISK_Lock();

    // 找disk
    disk_index = 0;

    /*lint -e666 -e26 -e10 -e48 -e1013 -e40 -e64 -e830 -e746 -e413 -e1055*/
    list_for_each_entry(pDiskNode, &m_UDisk_DiskListHead, disk_node)
    {
        if (disk_index++ == u32DiskIndex)
        {
            pDiskInfo = &(pDiskNode->DiskInfo);
            break;
        }
    }

    if (pDiskInfo == HI_NULL)
    {
        // disk not exist!
        UDISK_UnLock();
        return HI_UDISK_ERR_NOTEXIST;
    }

    // disk found
    part_index = 0;
    list_for_each(tmp, &(pDiskNode->part_list_head))
    {
        part_index++;
    }

    *pu32PartitionNum = part_index;

    UDISK_UnLock();
    return HI_SUCCESS;
}

HI_S32 HI_UDISK_GetPartitionInfo(HI_U32 u32DiskIndex, HI_U32 u32PartitionIndex, HI_UDISK_PARTITIONINFO_S* pPartitionInfo)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UDISK_PartNode_S stPartition;

    s32Ret = UDISK_GetSetPartition(UDISK_PARTITION_OP_GET, u32DiskIndex, u32PartitionIndex, &stPartition);

    if (HI_SUCCESS == s32Ret)
    {
        memcpy(pPartitionInfo, &stPartition.PartInfo, sizeof(HI_UDISK_PARTITIONINFO_S));
    }

    return s32Ret;
}

HI_S32 HI_UDISK_FormatPartition(HI_U32 u32DiskIndex, HI_U32 u32PartitionIndex, HI_UDISK_FSTYPE_E enFSType)
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_UDISK_PARTITIONINFO_S PartInfo;
    HI_CHAR string_format[UDISK_MAX_MOUNT_STR] = {0};
    HI_CHAR string_umount[UDISK_MAX_MOUNT_STR] = {0};
    HI_CHAR string_mount[UDISK_MAX_MOUNT_STR] = {0};

    if (HI_UDISK_GetPartitionInfo(u32DiskIndex, u32PartitionIndex, &PartInfo) != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    snprintf(string_umount, sizeof(string_umount), "umount -l %s;sync", PartInfo.szMntPath);

    if (-1 == system(string_umount))
    {
        return HI_FAILURE;
    }

    switch (enFSType)
    {
        case HI_UDISK_FSTYPE_FAT:
        case HI_UDISK_FSTYPE_FAT32:
            snprintf(string_format, sizeof(string_format), "mkfs.vfat %s", PartInfo.szDevPath);

            if (s_bUseHimount)
            {
                snprintf(string_mount, sizeof(string_mount), "himount %s %s", PartInfo.szDevPath, PartInfo.szMntPath);
            }
            else
            {
                snprintf(string_mount, sizeof(string_mount), "mount -t vfat %s %s %s", acMountParam, PartInfo.szDevPath, PartInfo.szMntPath);
            }

            break;

        case HI_UDISK_FSTYPE_EXT2:
            snprintf(string_format, sizeof(string_format), "mkfs.ext2 %s", PartInfo.szDevPath);
            snprintf(string_mount, sizeof(string_mount), "mount %s %s", PartInfo.szDevPath, PartInfo.szMntPath);
            break;

        case HI_UDISK_FSTYPE_EXT3:
            snprintf(string_format, sizeof(string_format), "mkfs.ext3 %s", PartInfo.szDevPath);
            snprintf(string_mount, sizeof(string_mount), "mount %s %s", PartInfo.szDevPath, PartInfo.szMntPath);
            break;

        case HI_UDISK_FSTYPE_NTFS:
            snprintf(string_format, sizeof(string_format), "mkntfs -F %s", PartInfo.szDevPath);

            if (!s_bForceMountNTFS)
            {
                snprintf(string_mount, sizeof(string_mount) , "mount -t tntfs %s %s %s", acMountParam, PartInfo.szDevPath, PartInfo.szMntPath);
            }
            else
            {
                snprintf(string_mount, sizeof(string_mount) , "mount -t tntfs -o force %s %s", PartInfo.szDevPath, PartInfo.szMntPath);
            }

            break;

        default:
            break;
    }

    if (-1 != system(string_format))
    {
        if (-1 != system(string_mount))
        {
            UDISK_DOFUNC(UDISK_SetPartitionType(u32DiskIndex, u32PartitionIndex, enFSType));
            s32Ret = HI_SUCCESS;
        }
    }

    return s32Ret;
}

/* filelist generated dynamically, without the need for locks */
HI_S32 HI_UDISK_GetFileList(HI_U32 u32Pid, HI_CHAR* pszDir,
                            HI_UDISK_SCAN_S* pScanOpt,
                            HI_UDISK_FILELIST_S* pFileList)
{
    HI_S32 ret;

    if ( (pszDir == HI_NULL) || (pScanOpt == HI_NULL) || (pFileList == HI_NULL) )
    {
        return HI_UDISK_ERR_NULL;
    }

    /* Allocate UDISK_RESERVE_MAX_FILE_NUM file nodes one-time, next realloc the memory*/
    s_ReallocMem = HI_FALSE;
    pFileList->ppFileList = (HI_UDISK_FILEINFO_S**)UDISK_MALLOC(sizeof(HI_UDISK_FILEINFO_S*) * UDISK_RESERVE_MAX_FILE_NUM);

    if (HI_NULL == pFileList->ppFileList)
    {
        return HI_FAILURE;
    }

    bzero(pFileList->ppFileList, sizeof(HI_UDISK_FILEINFO_S*) * UDISK_RESERVE_MAX_FILE_NUM);
    pFileList->u32FileNum = 0;

    ret = UDISK_ScanDir(u32Pid, pszDir, pScanOpt, pFileList);

    if (ret != HI_SUCCESS)
    {
        (HI_VOID)HI_UDISK_FreeFileList(pFileList);
    }

    return ret;
}

HI_S32 HI_UDISK_FreeFileList(HI_UDISK_FILELIST_S* pFileList)
{
    HI_U32 i;

    for (i = 0; i < pFileList->u32FileNum; i++)
    {
        if (pFileList->u32FileNum < UDISK_RESERVE_MAX_FILE_NUM)
        {
            if (pFileList->ppFileList[i] != HI_NULL)
            {
                if (pFileList->ppFileList[i]->pszShortName != HI_NULL)
                {
                    UDISK_FREE(pFileList->ppFileList[i]->pszShortName);
                    pFileList->ppFileList[i]->pszShortName = HI_NULL;
                }

                if (pFileList->ppFileList[i]->pszFullName != HI_NULL)
                {
                    UDISK_FREE(pFileList->ppFileList[i]->pszFullName);
                    pFileList->ppFileList[i]->pszFullName = HI_NULL;
                }
            }
        }

        UDISK_FREE(pFileList->ppFileList[i]);
        pFileList->ppFileList[i] = NULL;
    }

    UDISK_FREE(pFileList->ppFileList);
    pFileList->ppFileList = NULL;

    pFileList->u32FileNum = 0;

    return HI_SUCCESS;
}

HI_S32 HI_UDISK_RegisterEventCallback(HI_UDISK_EVENT_E enEvent,
                                      HI_UDISK_EVENT_CALLBACK pCallbackFunc,
                                      HI_VOID* pPrivate)
{
    if (enEvent >= HI_UDISK_EVENT_BUTT)
    {
        return HI_UDISK_ERR_INVALIDPARA;
    }

    UDISK_Lock();
    m_UDisk_Event.cb[enEvent] = pCallbackFunc;
    m_UDisk_Event.pPrivate[enEvent] = pPrivate;
    UDISK_UnLock();

    return HI_SUCCESS;
}

HI_S32 HI_UDISK_UnRegisterEventCallback(HI_UDISK_EVENT_E enEvent)
{
    if (enEvent >= HI_UDISK_EVENT_BUTT)
    {
        return HI_UDISK_ERR_INVALIDPARA;
    }

    UDISK_Lock();
    m_UDisk_Event.cb[enEvent] = HI_NULL;
    m_UDisk_Event.pPrivate[enEvent] = HI_NULL;
    UDISK_UnLock();

    return HI_SUCCESS;
}

HI_S32 HI_UDISK_SetFullReport(HI_U32 u32DiskIndex, HI_U32 u32PartitionIndex, HI_BOOL bReport, HI_U64 u64WaterLine)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UDISK_PartNode_S stPartition;

    s32Ret = UDISK_GetSetPartition(UDISK_PARTITION_OP_GET, u32DiskIndex, u32PartitionIndex, &stPartition);

    if (HI_SUCCESS == s32Ret)
    {
        stPartition.bFullReport = bReport;
        stPartition.u64WaterLine = u64WaterLine;
        s32Ret = UDISK_GetSetPartition(UDISK_PARTITION_OP_SET, u32DiskIndex, u32PartitionIndex, &stPartition);
    }

    return s32Ret;
}

HI_S32 UDISK_ScanDirForFileNum(const HI_CHAR* pszDir, HI_UDISK_SCAN_S* pScanOpt, HI_U32* pu32FileNum)
{
    DIR* dir = NULL;
    struct dirent* pEntry = NULL;
    HI_CHAR subFilePath[MAX_UDISK_NAME_LEN] = {0};
    HI_U32 subFilePathLen = 0;

    HI_LOGD("UDISK_ScanDirForFileNum");

    dir = opendir(pszDir);

    if (NULL == dir)
    {
        HI_LOGE("file dir not exist.");
        return HI_UDISK_ERR_NOTEXIST;
    }

    while (NULL != (pEntry = readdir(dir)))
    {
        if (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_CURDIR) /*lint !e655*/
            {
                (*pu32FileNum)++;
            }
        }
        else if (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_UPDIR) /*lint !e655*/
            {
                (*pu32FileNum)++;
            }
        }
        else
        {
            subFilePathLen = strlen(pszDir) + strlen("/") + strlen(pEntry->d_name) + 1;

            if (MAX_UDISK_NAME_LEN < subFilePathLen)
            {
                HI_LOGD("<WARNING>%s, %d : path length (%d) is too long\r\n",
                        __FUNCTION__, __LINE__, subFilePathLen);
                continue;
            }

            memset(subFilePath, 0x0, MAX_UDISK_NAME_LEN);
            strncpy(subFilePath, pszDir, sizeof(subFilePath) - 1);
            strncat(subFilePath, "/", sizeof(subFilePath) - strlen(subFilePath) - 1);
            strncat(subFilePath, pEntry->d_name, sizeof(subFilePath) - strlen(subFilePath) - 1);

            if (pEntry->d_type == DT_DIR) //if it is directiory
            {
                if (pScanOpt->enFilterType & HI_UDISK_FLT_SUBDIR) /*lint !e655*/
                {
                    (*pu32FileNum)++;
                }

                if (HI_TRUE == pScanOpt->bRecursive) //if it is file
                {
                    (HI_VOID)UDISK_ScanDirForFileNum(subFilePath, pScanOpt, pu32FileNum);
                }
            }
            else if (pEntry->d_type == DT_REG)
            {
                /*File filter*//* 文件过滤 */
                if (HI_SUCCESS == UDISK_CheckFILTER(pEntry->d_name, pScanOpt->enFilterType, pScanOpt->pFileFilter))
                {
                    (*pu32FileNum)++;
                }
            }
        }
    }

    (HI_VOID)closedir(dir);

    return HI_SUCCESS;
}

HI_S32 HI_UDISK_GetFileNum(HI_CHAR* pszDir,
                           HI_UDISK_SCAN_S*      pScanOpt,
                           HI_U32* pu32FileNum)
{
    HI_S32 ret;

    if ( (pszDir == HI_NULL) || (pScanOpt == HI_NULL) || (pu32FileNum == HI_NULL) )
    {
        return HI_UDISK_ERR_NULL;
    }

    *pu32FileNum = 0;

    ret = UDISK_ScanDirForFileNum(pszDir, pScanOpt, pu32FileNum);

    return ret;
}

static HI_S32 UDISK_ScanDirWithIndex(HI_U32 u32Pid, const HI_CHAR* pszDir,
                                     HI_UDISK_SCAN_S* pScanOpt,
                                     HI_U32 u32StartIndex, HI_U32 u32FileCount,
                                     HI_UDISK_FILELIST_S* pFileList)
{
    HI_CHAR subFilePath[MAX_UDISK_NAME_LEN] = {0};
    HI_U32 subFilePathLen = 0;
    DIR* dir = NULL;
    struct dirent* pEntry = NULL;
    HI_U32 u32FileIndex = 0;
    HI_U32 u32MaxIndex = u32StartIndex + u32FileCount - 1;
    HI_S32 s32Ret = 0;

    dir = opendir(pszDir);

    if (NULL == dir)
    {
        HI_LOGE("null dir");
        return HI_UDISK_ERR_NOTEXIST;
    }

    while ((NULL != (pEntry = readdir(dir))) && (pFileList->u32FileNum < u32FileCount))
    {
        if (UDISK_RESERVE_MAX_FILE_NUM <= pFileList->u32FileNum)    //files more than 2048 are ignored.
        {
            HI_LOGE("2048 files have been read. exit reading files.");
            break;
        }

        /* check whether to interrupt geting filelist. */
        s32Ret = UDISK_EventCallback(HI_UDISK_EVENT_QUERY_INTERRUPTFLAG, (HI_VOID*)&u32Pid);

        if ((HI_BOOL)s32Ret)
        {
            HI_LOGD("Stop getting file.");
            (HI_VOID)closedir(dir);
            return HI_FAILURE;
        }

        /* "." */
        if (0 == strncmp(pEntry->d_name, ".", strlen(".") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_CURDIR) /*lint !e655*/
            {
                if ((u32FileIndex >= u32StartIndex) && (u32FileIndex <= u32MaxIndex))
                {
                    (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                }

                u32FileIndex++;
            }
        }
        else if (0 == strncmp(pEntry->d_name, "..", strlen("..") + 1))
        {
            if (pScanOpt->enFilterType & HI_UDISK_FLT_UPDIR) /*lint !e655*/
            {
                if ((u32FileIndex >= u32StartIndex) && (u32FileIndex <= u32MaxIndex))
                {
                    (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                }

                u32FileIndex++;
            }
        }
        else
        {
            subFilePathLen = strlen(pszDir) + strlen("/") + strlen(pEntry->d_name) + 1;

            if (MAX_UDISK_NAME_LEN < subFilePathLen)
            {
                HI_LOGD("<WARNING>%s, %d : path length (%d) is too long\r\n",
                        __FUNCTION__, __LINE__, subFilePathLen);
                continue;
            }

            memset(subFilePath, 0x0, MAX_UDISK_NAME_LEN);
            strncpy(subFilePath, pszDir, sizeof(subFilePath) - 1);
            strncat(subFilePath, "/", sizeof(subFilePath) - strlen(subFilePath) - 1);
            strncat(subFilePath, pEntry->d_name, sizeof(subFilePath) - strlen(subFilePath) - 1);

            if (pEntry->d_type == DT_DIR)
            {
                if (pScanOpt->enFilterType & HI_UDISK_FLT_SUBDIR) /*lint !e655*/
                {
                    if ((u32FileIndex >= u32StartIndex) && (u32FileIndex <= u32MaxIndex))
                    {
                        (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                        pFileList->ppFileList[pFileList->u32FileNum - 1]->u32Type = 0;
                    }

                    u32FileIndex++;
                }

                if (HI_TRUE == pScanOpt->bRecursive)
                {
                    (HI_VOID)UDISK_ScanDirWithIndex(u32Pid, subFilePath, pScanOpt, u32FileIndex, u32FileCount, pFileList);
                }
            }
            else if (pEntry->d_type == DT_REG)
            {
                /*File filter*//* 文件过滤 */
                if (HI_SUCCESS == UDISK_CheckFILTER(pEntry->d_name, pScanOpt->enFilterType, pScanOpt->pFileFilter))
                {
                    if ((u32FileIndex >= u32StartIndex) && (u32FileIndex <= u32MaxIndex))
                    {
                        (HI_VOID)UDISK_AddFileToList(pszDir, pEntry->d_name, pFileList);
                        pFileList->ppFileList[pFileList->u32FileNum - 1]->u32Type = 1;
                    }

                    u32FileIndex++;
                }
            }
        }
    }

    (HI_VOID)closedir(dir);

    return HI_SUCCESS;
}

HI_S32 HI_UDISK_GetFileListWithIndex(HI_U32 u32Pid, HI_CHAR* pszDir,
                                     HI_UDISK_SCAN_S*      pScanOpt,
                                     HI_U32 u32StartIndex, HI_U32 u32FileCount,
                                     HI_UDISK_FILELIST_S* pFileList)
{
    HI_LOGD("HI_UDISK_GetFileListWithIndex enter");
    HI_S32 ret = HI_FAILURE;

    if ( (pszDir == HI_NULL) || (pScanOpt == HI_NULL) || (pFileList == HI_NULL) )
    {
        HI_LOGD("HI_UDISK_GetFileList: null ptr!");
        return HI_UDISK_ERR_NULL;
    }

    if (0 == u32FileCount)
    {
        HI_LOGE("u32FileCount input error.");
        return HI_FAILURE;
    }

    /* Allocate UDISK_RESERVE_MAX_FILE_NUM file nodes one-time, next realloc the memory*/
    s_ReallocMem = HI_FALSE;
    pFileList->ppFileList = (HI_UDISK_FILEINFO_S**)UDISK_MALLOC(sizeof(HI_UDISK_FILEINFO_S*) * (u32FileCount));

    if (HI_NULL == pFileList->ppFileList)
    {
        HI_LOGE("pFileList->ppFileList UDISK_MALLOC failed");
        return HI_FAILURE;
    }

    bzero(pFileList->ppFileList, sizeof(HI_UDISK_FILEINFO_S*) * (u32FileCount));
    pFileList->u32FileNum = 0;

    ret = UDISK_ScanDirWithIndex(u32Pid, pszDir, pScanOpt, u32StartIndex, u32FileCount, pFileList);

    if (ret != HI_SUCCESS)
    {
        HI_LOGE("UDISK_ScanDir failed");
        (HI_VOID)HI_UDISK_FreeFileList(pFileList);
    }

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
