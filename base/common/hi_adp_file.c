/**********************************************************************************
*             Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_file.c
* Description:the interface of file function adpter for all module
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-10   q63946  NULL         Create this file.
* 1.2       2006-05-15   t41030     NULL         delete(D) some function..
***********************************************************************************/

/*
1.2       2006-05-15   t41030     NULL         modify this file.

D fopen  -> fopen
D fclose -> fclose
D HI_FWrite    -> fwrite
D HI_Fread  -> fread
D HI_File_S -> FILE

*/

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "hi_type.h"

#if HI_OS_TYPE==HI_OS_WIN32
//#include "hi_head.h"
#include <windows.h>
#include <WINBASE.H>
#include <direct.h>
#include <io.h>

//#define  O_RDWR  _O_RDWR
//#define  O_CREAT _O_CREAT

#elif HI_OS_TYPE==HI_OS_LINUX

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mntent.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
int vscanf(const char *format, va_list ap);
int vfscanf(FILE *stream, const char *format, va_list ap);

//#include "hi_adp_file.h"
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif

//#include "hi_head.h"

#include "common/hi_adp_file.h"
#include "common/hi_adp_thread.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

HI_S32 HI_Access( const HI_CHAR *dirname ,HI_S32 mode)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return access(dirname, mode);
#elif HI_OS_TYPE == HI_OS_WIN32
     return _access(dirname, mode);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_ChDir( const HI_CHAR *dirname )
{
#if HI_OS_TYPE == HI_OS_LINUX
    return chdir(dirname);
#elif HI_OS_TYPE == HI_OS_WIN32
     return _chdir(dirname);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_MkDir( const HI_CHAR *dirname ,HI_MODE_T mode)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return mkdir(dirname,mode);
#elif HI_OS_TYPE == HI_OS_WIN32
     mode = mode;
     return _mkdir(dirname);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Rename(const HI_CHAR *oldpath, const HI_CHAR *newpath)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return rename(oldpath, newpath);
#elif HI_OS_TYPE == HI_OS_WIN32
    return rename(oldpath, newpath);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}
/*
HI_S32 HI_FStat(const HI_S32 fd, HI_STAT_S *buf)
{
    HI_S32 result;
#if HI_OS_TYPE == HI_OS_LINUX
       struct stat tempstat;
       result = fstat(fd,&tempstat);
       if(0 == result)
       {
            memset(buf,0,sizeof(HI_STAT_S));
            memcpy(&buf->st_dev,&tempstat.st_dev,sizeof(tempstat.st_dev));

            buf->st_ino = tempstat.st_ino;
            buf->st_mode = tempstat.st_mode;
            buf->st_nlink = tempstat.st_nlink;
            buf->st_uid = tempstat.st_uid;
            buf->st_gid = tempstat.st_gid;
            memcpy(&buf->st_rdev,&tempstat.st_rdev,sizeof(tempstat.st_rdev));

            buf->st_size = tempstat.st_size;
            buf->st_blksize = tempstat.st_blksize;
            buf->st_blocks = tempstat.st_blocks;
#if  defined (HI_LINUX_SUPPORT_UCLIBC) || defined(ANDROID_VERSION)
            buf->st_atime  = tempstat.st_atime;
            buf->st_atimensec = 0;
            buf->st_mtime  = tempstat.st_mtime;
            buf->st_mtimensec = 0;
            buf->st_ctime  = tempstat.st_ctime;
            buf->st_ctimensec = 0;
#else
            buf->st_atime  = tempstat.st_atim.tv_sec;
            buf->st_atimensec = tempstat.st_atim.tv_nsec;
            buf->st_mtime  = tempstat.st_mtim.tv_sec;
            buf->st_mtimensec = tempstat.st_mtim.tv_nsec;
            buf->st_ctime  = tempstat.st_ctim.tv_sec;
            buf->st_ctimensec = tempstat.st_ctim.tv_nsec;
#endif
        }
        return result;
#elif HI_OS_TYPE == HI_OS_WIN32
    struct _stat tempstat;
    result =  _fstat(fd,&tempstat);
    if(0 == result)
       {
            memset(buf,0,sizeof(HI_STAT_S));
            buf->st_dev = tempstat.st_dev;
            buf->st_ino = tempstat.st_ino;
            buf->st_mode = tempstat.st_mode;
            buf->st_nlink = tempstat.st_nlink;
            buf->st_uid = tempstat.st_uid;
            buf->st_gid = tempstat.st_gid;
            buf->st_rdev = tempstat.st_rdev;
            buf->st_size = tempstat.st_size;
            buf->st_atime = tempstat.st_atime;
            buf->st_mtime = tempstat.st_mtime;
            buf->st_ctime = tempstat.st_ctime;
        }
       return result;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}
*/
/***************************************************************************
*                            printf ¥Ú”°–≈œ¢
****************************************************************************/

/* Write formatted output to STREAM. */
HI_S32 HI_Fprintf (FILE *stream, const HI_CHAR *format, ...)
{
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );
    dwBufLen = vfprintf((FILE *)stream, format, marker);
    va_end(marker);

    return dwBufLen;
}

/* Read formatted input from STREAM. */
HI_S32 HI_Fscanf (FILE * stream,const HI_CHAR * format, ...)
{
#if HI_OS_TYPE == HI_OS_LINUX
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );
    dwBufLen = vfscanf( (FILE *)stream,format, marker);
    va_end(marker);
    return dwBufLen;
 #elif HI_OS_TYPE == HI_OS_WIN32
        return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Open(const HI_CHAR *pathname, HI_S32 flags, HI_MODE_T mode)
{
 #if HI_OS_TYPE == HI_OS_LINUX
     return open(pathname,flags,mode);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _open(pathname,flags,mode);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Creat(const HI_CHAR *pathname, HI_MODE_T mode)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return creat(pathname,mode);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _creat(pathname,(HI_S32)mode);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_SSIZE_T HI_Read(HI_S32 fd, HI_VOID *buf, HI_SIZE_T count)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return read(fd,buf,count);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _read(fd,buf,count);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_SSIZE_T HI_Write(HI_S32 fd, const HI_VOID *buf, HI_SIZE_T count)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return write(fd,buf,count);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _write(fd,buf,count);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

 HI_S32 HI_Close(HI_S32 fd)
{
 #if HI_OS_TYPE == HI_OS_LINUX
     return close(fd);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _close(fd);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}
HI_S32 HI_CloseHandle(HI_VOID *handle)
{
 #if HI_OS_TYPE == HI_OS_LINUX
     return close((HI_S32)handle);
#elif HI_OS_TYPE == HI_OS_WIN32
     return CloseHandle((HANDLE)handle)==0?(-1):0;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_OFF_T HI_LSeek(HI_S32 fildes, HI_OFF_T offset, HI_S32 whence)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return lseek(fildes,offset,whence);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _lseek(fildes,offset,whence);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Ioctl(HI_S32 fd, HI_S32 request,HI_VOID *lpInBuffer
                                              ,HI_U32  nInBufferSize,
                                              HI_VOID * lpOutBuffer,
                                              HI_U32 nOutBufferSize,
                                              HI_U32 *lpBytesReturned)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return ioctl(fd,request,lpInBuffer);
#elif HI_OS_TYPE == HI_OS_WIN32
    HI_S32 result;
    result = DeviceIoControl((HANDLE)fd,(HI_U32)request,lpInBuffer,nInBufferSize,lpOutBuffer,nOutBufferSize,lpBytesReturned,0);
    if(result==0)
    {
        return HI_ERR_OSCALL_ERROR;
    }
    else
    {
        return HI_SUCCESS;
    }
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
