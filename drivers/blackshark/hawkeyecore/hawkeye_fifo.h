#ifndef __HAWKEYE_FIFO_H__
#define __HAWKEYE_FIFO_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/hash.h>

/*----------------------------------------------------------------
    funtion:HAWKEYE_FIFO_INIT
        for init fifo
    params:pfifo-> the fifo to init
           sz->max size of fifo
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_FIFO_INIT(pfifo,sz)\
    ({\
        int ret = 0;\
        u32 newsz = (u32)(sz);\
        if(!(pfifo)){\
            HAWKEYE_LOG_ERR("HAWKEYE_FIFO_INIT invalid param\n");\
            ret = -EINVAL;\
        }else{\
            if (!is_power_of_2(newsz)) {\
               newsz = roundup_pow_of_two(newsz);\
            }\
            (pfifo)->seq_id = 0;\
            (pfifo)->read_index = 0;\
            (pfifo)->write_index = 0;\
            (pfifo)->size = newsz;\
            (pfifo)->pdata = kzalloc(newsz,GFP_KERNEL);\
            if(NULL == (pfifo)->pdata){\
                HAWKEYE_LOG_ERR("malloc failed");\
                ret = -ENOMEM;\
            }\
            smp_wmb();\
        }\
        ret;\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_FIFO_UNINIT
        for uninit fifo
    params:pfifo-> the fifo to uninit
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_FIFO_UNINIT(pfifo)\
    ({\
        int ret = 0;\
        void *ptmp = NULL;\
        if(NULL == (pfifo)){\
            HAWKEYE_LOG_ERR("params failed");\
            ret=-EINVAL;\
        }\
        ptmp = (pfifo)->pdata;\
        (pfifo)->pdata = NULL;\
        if(ptmp){\
            kzfree(ptmp);\
        }\
        ret;\
    })


/*----------------------------------------------------------------
    funtion:HAWKEYE_FIFO_WRITE
        for write fifo
    params:pfifo-> the fifo to wirte
           buf&bufsz->wirte buf & size
    return:size
----------------------------------------------------------------*/
#define HAWKEYE_FIFO_WRITE(pfifo,buf,bufsz)\
        ({\
            u32 l;\
            u32 sz = (u32)(bufsz);\
            smp_rmb();\
            sz = min(sz, (pfifo)->size - (pfifo)->write_index + (pfifo)->read_index);\
            l = min(sz, pfifo->size - ((pfifo)->write_index & ((pfifo)->size - 1)));\
            memcpy((pfifo)->pdata + ((pfifo)->write_index & ((pfifo)->size - 1)), buf, l);\
            memcpy((pfifo)->pdata, buf + l, sz - l);\
            (pfifo)->write_index += sz;\
            (pfifo)->seq_id++;\
            smp_wmb();\
            sz;\
            })

/*----------------------------------------------------------------
    funtion:HAWKEYE_FIFO_READ
        for read fifo
    params:pfifo-> the fifo to read
           buf&bufsz->read buf & size
    return:size
----------------------------------------------------------------*/
#define HAWKEYE_FIFO_READ(pfifo,buf,bufsz)\
        ({\
            u32 l;\
            u32 sz = (u32)(bufsz);\
            smp_rmb();\
            sz = min(sz, (pfifo)->write_index - (pfifo)->read_index);\
            l = min(sz, (pfifo)->size - ((pfifo)->read_index & ((pfifo)->size - 1)));\
            memcpy(buf, pfifo->pdata + ((pfifo)->read_index & ((pfifo)->size - 1)), l);\
            memcpy(buf + l, (pfifo)->pdata, sz - l);\
            (pfifo)->read_index += sz;\
            smp_wmb();\
            sz;\
        })


/*----------------------------------------------------------------
    funtion:HAWKEYE_FREE_SIZE
        for get fifo free size
    params:pfifo-> fifo
    return:size
----------------------------------------------------------------*/
#define HAWKEYE_FREE_SIZE(pfifo)\
        ({\
            smp_rmb();\
            ((pfifo)->size - (pfifo)->write_index + (pfifo)->read_index);\
        })


/*----------------------------------------------------------------
    funtion:HAWKEYE_USED_SIZE
        for get fifo data size
    params:pfifo-> fifo
    return:size
----------------------------------------------------------------*/
#define HAWKEYE_USED_SIZE(pfifo)\
        ({\
            smp_rmb();\
            ((pfifo)->write_index - (pfifo)->read_index);\
        })

/*================================================================*/
/*hawkeye fifo info                                               */
/*================================================================*/
struct hawkeye_fifo{
    u32 seq_id;
    u32 write_index;
    u32 read_index;
    u32 size;
    void *pdata;
};


#endif//__HAWKEYE_FIFO_H_
