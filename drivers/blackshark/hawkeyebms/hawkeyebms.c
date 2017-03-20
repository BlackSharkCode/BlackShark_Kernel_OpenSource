#include "hawkeye_bms_inf.h"
#include "hawkeye_bms_port.h"

static int __init hawkeye_bms_mode_init(void)
{
    hawkeye_client_mgr_init();
    hawkeye_bms_register();
    hawkeye_ems_register();

#ifdef CONFIG_HAWKEYE_DBG
    //hawkeye_bms_dbg(1);
#endif //CONFIG_HAWKEYE_DBG
    HAWKEYE_LOG_INFO("====hawkeye bms init====");
    return 0;
}

static void __exit hawkeye_bms_mode_uninit(void)
{
    hawkeye_ems_unregister();
    hawkeye_bms_unregister();
    hawkeye_client_mgr_uninit();

    HAWKEYE_LOG_INFO("====hawkeye bms uninit====");
    return;
}


MODULE_LICENSE("GPL");
fs_initcall(hawkeye_bms_mode_init);
module_exit(hawkeye_bms_mode_uninit);

