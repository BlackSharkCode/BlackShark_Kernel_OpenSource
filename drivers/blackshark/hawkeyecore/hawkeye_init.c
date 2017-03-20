#include "hawkeye_bms.h"
#include "hawkeye_bms_fs.h"

static int __init hawkeye_core_init(void)
{
    libhawkeye_bugfs_init(HAWKEYE_FS_SIZE);
    libhawkeye_bms_init();

    HAWKEYE_LOG_INFO("====hawkeye core init====");
    return 0;
}

static void __exit hawkeye_core_uninit(void)
{
    libhawkeye_bms_uninit();
    HAWKEYE_LOG_INFO("====hawkeye core uninit====");
    return;
}


MODULE_LICENSE("GPL");
module_init(hawkeye_core_init);
module_exit(hawkeye_core_uninit);