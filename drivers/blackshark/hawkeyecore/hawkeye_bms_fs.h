#ifndef __HAWKEYE_FS_H__
#define __HAWKEYE_FS_H__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>
/*================================================================*/
/*hawkeye share file sytem interface                              */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_init
        for init share fs
    params:bugfs_size->total size of the fs
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_init(int bugfs_size);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_uninit
        for uninit share fs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
void libhawkeye_bugfs_uninit(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_write
        for wite data to share fs
    params:page_id->return current page id
           buff&size->the write data and data len
    return:number of write data else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_write(int *page_id, const char *buff, int size);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_read
        for read data from share fs
    params:page_id->return libhawkeye_bugfs_write
           buff&size->the read data and data len
           offset->the offset of current read
    return:number of read data else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_read(int page_id, char *buff, int size, int offset);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_gc
        for gc page from current page id
    params:page_id->return libhawkeye_bugfs_write
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_gc(int page_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_get_file_size
        for get the total size from current page id
    params:page_id->return libhawkeye_bugfs_write
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_get_file_size(int page_id);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_monitor
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_monitor(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_bugfs_share
        for get share mem fd
    params:psz->return share mem size
    return:share mem fd else errno
----------------------------------------------------------------*/
int libhawkeye_bugfs_share(int *psz);
#endif