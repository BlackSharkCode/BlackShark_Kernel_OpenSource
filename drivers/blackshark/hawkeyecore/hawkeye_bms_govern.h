#ifndef __HAWKEYE_GOVERN_H__
#define __HAWKEYE_GOVERN_H__
#include "hawkeye_bms.h"

enum{
    HAWKEYE_BMS_GOV_2PS = 0,// 2msg per second
    HAWKEYE_BMS_GOV_4PS,// 4msg per second
    HAWKEYE_BMS_GOV_8PS,// 8msg per second
    HAWKEYE_BMS_GOV_16PS,// 16msg per second
    HAWKEYE_BMS_GOV_32PS,// 32msg per second
    HAWKEYE_BMS_GOV_MAX,
};

/*----------------------------------------------------------------
    funtion:libhawkeye_govern_init
        for govern init
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_govern_init(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_govern_unint
        for govern uninit
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
void libhawkeye_govern_unint(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_govern_lock
        for govern lock in state,eg. HAWKEYE_BMS_GOV_2PS
    params:HAWKEYE_BMS_GOV_*
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_govern_lock(int state);

/*----------------------------------------------------------------
    funtion:libhawkeye_govern_unlock
        for unlock state of govern
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int libhawkeye_govern_unlock(void);

/*----------------------------------------------------------------
    funtion:libhawkeye_govern_wakeup
        for wake up govern to handle idle msg
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
void libhawkeye_govern_wakeup(void);


#endif //_HAWKEYE_GOVERN_H