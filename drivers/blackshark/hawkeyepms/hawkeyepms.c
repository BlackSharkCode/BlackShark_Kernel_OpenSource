#include "hawkeye_pms_inf.h"
#include "hawkeye_pms_port.h"

static int __init hawkeye_pms_mode_init(void)
{
    hawkeye_pms_register();
#ifdef CONFIG_HAWKEYE_DBG
    //hawkeye_pms_dbg(1);
#endif //CONFIG_HAWKEYE_DBG
    HAWKEYE_LOG_INFO("====hawkeye pms init====");
    return 0;
}

static void __exit hawkeye_pms_mode_uninit(void)
{
    hawkeye_pms_unregister();
    HAWKEYE_LOG_INFO("====hawkeye pms uninit====");
    return;
}

MODULE_LICENSE("GPL");
fs_initcall(hawkeye_pms_mode_init);
module_exit(hawkeye_pms_mode_uninit);

