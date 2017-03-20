#ifndef __HAWKEYE_BMS_CONFIG__
#define __HAWKEYE_BMS_CONFIG__

/*----------------------------------------------------------------
    funtion:libhawkeye_config_init
        for init BMS & EMS config
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_config_init(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_config_check
        for check point should be point
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_config_check(int type,u32 errid);

/*----------------------------------------------------------------
    funtion:libhawkeye_config_get_pro
        for get specify mode
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
char libhawkeye_config_get_pro(int type,int mode);

/*----------------------------------------------------------------
    funtion:libhawkeye_config_set_pro
        for set specify mode
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_config_set_pro(int type,int mode,char pro);

/*----------------------------------------------------------------
    funtion:libhawkeye_config_set
        for update configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_config_set(int type,struct hawkeye_config *pcfg);

/*----------------------------------------------------------------
    funtion:libhawkeye_config_get
        for get configs
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_config_get(int type,struct hawkeye_config *pcfg);

#endif //_HAWKEYE_BMS_CONFIG_