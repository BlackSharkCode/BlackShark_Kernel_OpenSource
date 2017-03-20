#ifndef __HAWKEYE_LIST_H__
#define __HAWKEYE_LIST_H__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>
/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_NUMCMP
        for compare number val
    params:id-> the member to check
            val-> the member val
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_NUMCMP(id, val)     (id == val || val < 0)

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_NUMCMP
        for compare number val
    params:id-> the member to check
            val-> the member val
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_CLIENT_FREE(pclt)\
    do{\
        if(pclt){\
            if(pclt->private_data){\
                kzfree(pclt->private_data);\
                pclt->private_data = NULL;\
            }\
            kzfree(pclt); \
        }\
    }while(0)\

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_STRCMP
        for compare string val
    params:id-> the member to check
            val-> the member val
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_STRCMP(id, val)     (0 == strncmp(id,val,strlen(id)))

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_INIT
        for init list head & lock
    params:plst-> the list for current ops
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_INIT(plst)\
    ({\
        spin_lock_init(&((plst)->spin_lock));\
        INIT_LIST_HEAD(&((plst)->head));\
        (plst)->count = 0;\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_UNINIT
        for uninit list head & lock
    params:plst-> the list for current ops
           type-> the item type of plst
           mfree-> the item free method
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_UNINIT(plst, type,mfree)\
    ({\
        HAWKEYE_LST_CLEAR(plst,type,mfree);\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_CLEAR
        for clear all time of current list
    params:plst-> the list for current ops
           type-> the item type of plst
           mfree-> the itme free method
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_CLEAR(plst,type,mfree)\
    ({\
        unsigned long f = 0;\
        struct list_head *pos = NULL;\
        struct list_head *postmp = NULL;\
        type *pitem = NULL;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        list_for_each_safe(pos,postmp,&((plst)->head))\
        {\
            pitem = list_entry(pos, type, node);\
            if(pitem)\
            {\
                list_del_init(pos);\
                --(plst)->count;\
                mfree(pitem);\
                break;\
            }\
        }\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LIST_ADD
        for add a new item to plst
    params:plst-> the list for current ops
           pitem-> the new item will be add to list
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LIST_ADD(plst,pitem)\
    ({\
        unsigned long f = 0;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        list_add_tail(&(pitem)->node,&((plst)->head));\
        ++(plst)->count;\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_DEL
        for del the item from plst
    params:plst-> the list for current ops
           type-> the item type of plst
           mb-> the member of type
           val-> the member val of item in the list will be del
           cmp-> the item compare method
           mfree-> the item free method
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_DEL(plst,type,mb,val,cmp,mfree)\
    ({\
        unsigned long f = 0;\
        struct list_head *pos = NULL;\
        struct list_head *postmp = NULL;\
        type *pitem = NULL;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        list_for_each_safe(pos,postmp,&((plst)->head))\
        {\
            pitem = list_entry(pos, type, node);\
            if(pitem && cmp(pitem->mb,val))\
            {\
                list_del_init(pos);\
                --(plst)->count;\
                mfree(pitem);\
                break;\
            }\
        }\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_DEL
        for del the item from plst
    params:plst-> the list for current ops
           type-> the item type of plst
           mb-> the member of type
           val-> the member val of item in the list will be del
           cmp-> the item compare method
           id-> the member of type will be return
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_FIND(plst,type,mb,val,cmp,id)\
    ({\
        int itemid = -1;\
        unsigned long f = 0;\
        struct list_head *pos = NULL;\
        type *pitem = NULL;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        list_for_each(pos,&((plst)->head))\
        {\
            pitem = list_entry(pos, type, node);\
            if(pitem && cmp(pitem->mb,val))\
            {\
                itemid = pitem->id;\
                break;\
            }\
        }\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
        itemid;\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_GET
        for get the item from plst
    params:plst-> the list for current ops
           type-> the item type of plst
           mb-> the member of type
           val-> the member val of item in the list will be del
           cmp-> the item compare method
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_GET(plst,type,mb,val,cmp)\
    ({\
        unsigned long f = 0;\
        struct list_head *pos = NULL;\
        struct list_head *postmp = NULL;\
        type *pitem = NULL;\
        type *pret = NULL;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        list_for_each_safe(pos,postmp,&((plst)->head))\
        {\
            pitem = list_entry(pos, type, node);\
            if(pitem && cmp(pitem->mb,val))\
            {\
                pret= pitem;\
                --(plst)->count;\
                list_del_init(&pitem->node);\
                break;\
            }\
        }\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
        pret;\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_ISEMPTY
        for check if current list is empty
    params:plst-> the list for current ops
    return:0(false)->not empty
----------------------------------------------------------------*/
#define HAWKEYE_LST_ISEMPTY(plst)\
    ({\
        int ret = 0;\
        unsigned long f = 0;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        ret = list_empty_careful(&((plst)->head));\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
        ret;\
    })

/*----------------------------------------------------------------
    funtion:HAWKEYE_LST_COUNT
        for get the item count of current list
    params:plst-> the list for current ops
    return:0->success else errno
----------------------------------------------------------------*/
#define HAWKEYE_LST_COUNT(plst)\
    ({\
        unsigned int ret = 0;\
        unsigned long f = 0;\
        spin_lock_irqsave(&((plst)->spin_lock), f);\
        ret = (plst)->count;\
        spin_unlock_irqrestore(&((plst)->spin_lock), f);\
        ret;\
    })

/*================================================================*/
/*hawkeye list info*/
/*================================================================*/

struct hawkeye_list{
    unsigned int count;
    spinlock_t spin_lock;
    struct list_head head;
};


#endif //_HAWKEYE_LIST_H
