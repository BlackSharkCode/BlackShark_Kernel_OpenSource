#ifndef __HAWKEYE_BMS_COMPAT_PORT_
#define __HAWKEYE_BMS_COMPAT_PORT_

#include "hawkeye_bms_inf.h"

#ifdef CONFIG_COMPAT
long hawkeye_bms_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
long hawkeye_ems_clt_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
#define hawkeye_bms_clt_compat_ioctl NULL
#define hawkeye_ems_clt_compat_ioctl NULL
#endif

#endif//__HAWKEYE_BMS_COMPAT_PORT_