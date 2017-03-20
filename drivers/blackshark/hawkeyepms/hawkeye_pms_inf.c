#include "hawkeye_pms_inf.h"

struct hawkeye_pms_operations g_heyepms_ops = {
    //hawkeye pms config ops
    NULL,//int (*config_set)(int,struct hawkeye_config*);
    NULL,//int (*config_get)(int,struct hawkeye_config*);

    //hawkeye fun ops
    NULL,//int (*fun_start)(u32*);
    NULL,//int (*fun_stop)(u32*);
    NULL,//int (*fun_empty)(void);

    //hawkeye svr ops
    NULL,//int (*fun_count)(void);
    NULL,//int (*fun_count_by_id)(u16);
    NULL,//int (*fun_info_by_idx)(int,struct hawkeye_func*);
    NULL,//int (*fun_info_by_id)(int,struct hawkeye_func*,u16);
    NULL,//int (*fun_clr)(void);
};
EXPORT_SYMBOL(g_heyepms_ops);

/*================================================================*/
/*hawkeye pms interface */
/*================================================================*/
/*----------------------------------------------------------------
    funtion:hawkeye_pms_start_func
        for recoder fun start time
    params:idfun->fun id
    return:id else errno
----------------------------------------------------------------*/
int hawkeye_pms_start_func(u32* func_id)
{
    int ret = 0;
    if(g_heyepms_ops.fun_start){
        ret = g_heyepms_ops.fun_start(func_id);
    }
    return ret;

}
EXPORT_SYMBOL(hawkeye_pms_start_func);

/*----------------------------------------------------------------
    funtion:hawkeye_pms_stop_func
        for recoder fun stop time
    params:id->hawkeye_pms_start_func return value
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_stop_func(u32* func_id)
{
    int ret = 0;
    if(g_heyepms_ops.fun_stop){
        ret = g_heyepms_ops.fun_stop(func_id);
    }
    return ret;

}
EXPORT_SYMBOL(hawkeye_pms_stop_func);


/*----------------------------------------------------------------
    funtion:hawkeye_pms_empty
        for check if has oldest page
    params:none
    return:true->empty else not
----------------------------------------------------------------*/
int hawkeye_pms_empty(void)
{
    int ret = 0;
    if(g_heyepms_ops.fun_empty){
        ret = g_heyepms_ops.fun_empty();
    }
    return ret;
}


/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_total_count_by_funcid
        for get total num of fun idfun
    params:idfun->fun id
    return:num else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_total_count_by_funcid(u16 func_id)
{
    int ret = 0;
    if(g_heyepms_ops.fun_count_by_id){
        ret = g_heyepms_ops.fun_count_by_id(func_id);
    }
    return ret;
}


/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_func_by_funcid
        for get specify fun by idfun & index
    params:idx->index of fun will get
           pfuns->return  the fun will get
           idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_func_by_funcid(int idx,struct hawkeye_func *pfuns,u16 idfun)
{
    int ret = 0;
    if(g_heyepms_ops.fun_info_by_id){
        ret = g_heyepms_ops.fun_info_by_id(idx,pfuns,idfun);
    }
    return ret;
}

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_total_count
        for get total num fun
    params:idfun->fun id
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_total_count(void)
{
    int ret = 0;
    if(g_heyepms_ops.fun_count){
        ret = g_heyepms_ops.fun_count();
    }
    return ret;

}

/*----------------------------------------------------------------
    funtion:hawkeye_pms_get_func_by_index
        for get specify fun by index
    params:idx->index of fun will get
           pfuns->return  the fun will get
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_get_func_by_index(int idx,struct hawkeye_func *pfuns)
{
    int ret = 0;
    if(g_heyepms_ops.fun_info_by_idx){
        ret = g_heyepms_ops.fun_info_by_idx(idx,pfuns);
    }
    return ret;

}

/*----------------------------------------------------------------
    funtion:hawkeye_pms_clr_all
        for clear current page
    params:none
    return:0->success else errno
----------------------------------------------------------------*/
int hawkeye_pms_clr_all(void)
{
    int ret = 0;
    if(g_heyepms_ops.fun_clr){
        ret = g_heyepms_ops.fun_clr();
    }
    return ret;
}


