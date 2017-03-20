#ifndef __HAWKEYE_PMS_COMPAT_PORT_
#define __HAWKEYE_PMS_COMPAT_PORT_

#include "hawkeye_pms_inf.h"

#ifdef CONFIG_COMPAT
long hawkeye_pms_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
#define hawkeye_pms_clt_compat_ioctl NULL
#endif

#endif//__HAWKEYE_BMS_COMPAT_PORT_