#ifndef __HAWKEYE_PRIV_H__
#define __HAWKEYE_PRIV_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ctype.h>

#ifndef ASSERT
#define ASSERT BUG_ON
#endif//ASSERT


//common log interface
#define HAWKEYE_LOG_ERR(x,...)                      printk(KERN_ERR "[%s %d]"x,__func__,__LINE__,##__VA_ARGS__)
//#define HAWKEYE_LOG_INFO(x,...)                     printk(KERN_INFO "[%s %d]"x,__func__,__LINE__,##__VA_ARGS__)
#define HAWKEYE_LOG_INFO(x,...)

#ifdef CONFIG_HAWKEYE_DBG
#define HAWKEYE_LOG_DEBUG(x,...)                    printk(KERN_DEBUG "[%s %d]"x,__func__,__LINE__,##__VA_ARGS__)
#else
#define HAWKEYE_LOG_DEBUG(x,...)
#endif //CONFIG_HAWKEYE_DBG

#define HAWKEYE_MIN(a,b)                            ((a)>(b)?(b):(a))
#define HAWKEYE_MAX(a,b)                            ((a)<(b)?(b):(a))

#endif //_HAWKEYE_PRIV_H
