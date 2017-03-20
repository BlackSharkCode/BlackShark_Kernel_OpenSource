#include <linux/blkdev.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/hawkeye/hawkeye_pub.h>
#include "hawkeye_bms_config.h"
#include "hawkeye_bms_priv.h"
#include "hawkeye_bms_msg.h"

struct hawkeye_inerconfg{
    rwlock_t lock;
    char cfg_pro[HAWKEYE_MAX_CONFIG];
};

static struct hawkeye_inerconfg g_hawkeybmscfg;
static struct hawkeye_inerconfg g_hawkeyemscfg;

int libhawkeye_config_set(int type,struct hawkeye_config *pcfg)
{
    struct hawkeye_inerconfg *pinercfg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!pcfg){
        HAWKEYE_LOG_ERR("invalid params,pcfg=%p\n",pcfg);
        return -EINVAL;
    }
    if(pcfg->cfg_start < 0 || pcfg->cfg_count > HAWKEYE_MIN_CONFIG){
        HAWKEYE_LOG_ERR("invalid params,cfg_start=%d,cfg_count=%d\n",pcfg->cfg_start,pcfg->cfg_count);
        return -EINVAL;
    }
    if(MSGTYPE_EVENT == type){
        pinercfg = &g_hawkeyemscfg;
    }else if(MSGTYPE_BUG == type){
        pinercfg = &g_hawkeybmscfg;
    }else{
        HAWKEYE_LOG_ERR("invalid params,type=%d\n",type);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.update configs*/
    /*================================================================*/
    pcfg->cfg_count = HAWKEYE_MIN(pcfg->cfg_count,HAWKEYE_MAX_CONFIG - pcfg->cfg_start);
    write_lock(&pinercfg->lock);
    memcpy(&pinercfg->cfg_pro[pcfg->cfg_start],&pcfg->cfg_pro,pcfg->cfg_count);
    write_unlock(&pinercfg->lock);
    return 0;
}

int libhawkeye_config_get(int type,struct hawkeye_config *pcfg)
{
    struct hawkeye_inerconfg *pinercfg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!pcfg){
        HAWKEYE_LOG_ERR("invalid params,pcfg=%p\n",pcfg);
        return -EINVAL;
    }
    if(pcfg->cfg_start < 0 || pcfg->cfg_count > HAWKEYE_MIN_CONFIG){
        HAWKEYE_LOG_ERR("invalid params,cfg_start=%d,cfg_count=%d\n",pcfg->cfg_start,pcfg->cfg_count);
        return -EINVAL;
    }
    if(MSGTYPE_EVENT == type){
        pinercfg = &g_hawkeyemscfg;
    }else if(MSGTYPE_BUG == type){
        pinercfg = &g_hawkeybmscfg;
    }else{
        HAWKEYE_LOG_ERR("invalid params,type=%d\n",type);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.capture configs*/
    /*================================================================*/
    pcfg->cfg_count = HAWKEYE_MIN(pcfg->cfg_count,HAWKEYE_MAX_CONFIG - pcfg->cfg_start);
    read_lock(&pinercfg->lock);
    memcpy(&pcfg->cfg_pro,&pinercfg->cfg_pro[pcfg->cfg_start],pcfg->cfg_count);
    read_unlock(&pinercfg->lock);
    return 0;
}

int libhawkeye_config_set_pro(int type,int mode,char pro)
{
    struct hawkeye_inerconfg *pinercfg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!HAWKEYE_MODE_VALID(mode)||!HAWKEYE_PRO_VALID(pro)){
        HAWKEYE_LOG_ERR("invalid params,mode=%d,pro=%d\n",mode,pro);
        return -EINVAL;
    }
    if(MSGTYPE_EVENT == type){
        pinercfg = &g_hawkeyemscfg;
    }else if(MSGTYPE_BUG == type){
        pinercfg = &g_hawkeybmscfg;
    }else{
        HAWKEYE_LOG_ERR("invalid params,type=%d\n",type);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.update specify mode pro*/
    /*================================================================*/
    write_lock(&pinercfg->lock);
    pinercfg->cfg_pro[mode] = pro;
    write_unlock(&pinercfg->lock);
    return 0;
}

char libhawkeye_config_get_pro(int type,int mode)
{
    char pro = 0;
    struct hawkeye_inerconfg *pinercfg = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!HAWKEYE_MODE_VALID(mode)){
        HAWKEYE_LOG_ERR("invalid params,mode=%d\n",mode);
        return -EINVAL;
    }
    if(MSGTYPE_EVENT == type){
        pinercfg = &g_hawkeyemscfg;
    }else if(MSGTYPE_BUG == type){
        pinercfg = &g_hawkeybmscfg;
    }else{
        HAWKEYE_LOG_ERR("invalid params,type=%d\n",type);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get specify mode pro*/
    /*================================================================*/
    read_lock(&pinercfg->lock);
    pro = pinercfg->cfg_pro[mode];
    read_unlock(&pinercfg->lock);
    return pro;
}

int libhawkeye_config_init(void)
{
    int i = 0;
    /*================================================================*/
    /*1.init rw locks*/
    /*================================================================*/
    rwlock_init(&g_hawkeybmscfg.lock);
    rwlock_init(&g_hawkeyemscfg.lock);
    
    /*================================================================*/
    /*2.init default pro is ignore*/
    /*================================================================*/
    write_lock(&g_hawkeybmscfg.lock);
    for( i = 0; i < HAWKEYE_MAX_CONFIG; ++i){
        g_hawkeybmscfg.cfg_pro[i] = ERRNOPRO_IGNORE;
    }
    write_unlock(&g_hawkeybmscfg.lock);

    write_lock(&g_hawkeyemscfg.lock);
    for( i = 0; i < HAWKEYE_MAX_CONFIG; ++i){
        g_hawkeyemscfg.cfg_pro[i] = ERRNOPRO_IGNORE;
    }
    write_unlock(&g_hawkeyemscfg.lock);
    return 0;
}

int libhawkeye_config_check(int type,u32 errid)
{
    int pro = libhawkeye_config_get_pro(type,HAWKEYE_ERRNO_MOD(errid));
    if(HAWKEYE_PRO_VALID(pro) && HAWKEYE_ERRNO_PRO(errid) <= pro){
        return 1;//the errid is enable
    }
    return 0;//the errid is disable
}

