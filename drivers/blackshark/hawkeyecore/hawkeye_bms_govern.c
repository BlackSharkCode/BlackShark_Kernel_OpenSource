#include <linux/delay.h>
#include <linux/kthread.h>
#include "hawkeye_bms_msg.h"
#include "hawkeye_bms_govern.h"

//======================================//
struct hawkeye_govern_info{
    int govern_id;
    int consume_rate_high;
    int consume_rate_low;
    int water_line_high;
    int water_line_low;
    int increase_size;
    int step_vaild_time;
    int sampling_time;
};


struct hawkeye_govern_state{
    int lock_mode;
    int current_state;
    int target_state;
    int expire_time;
    struct mutex lock;
};

static struct task_struct *g_govern_task = NULL;
static struct hawkeye_govern_state gstate;
static struct hawkeye_govern_info gmap[] ={
    {//rate - 2msg/s
        .govern_id = HAWKEYE_BMS_GOV_2PS,
        .consume_rate_high = 2,
        .consume_rate_low = -999,
        .water_line_high = 128,
        .water_line_low = 64,
        .increase_size = 16,
        .step_vaild_time = 32,
        .sampling_time = 32,
    },
    {//rate - 4msg/s
        .govern_id = HAWKEYE_BMS_GOV_4PS,
        .consume_rate_high = 6,
        .consume_rate_low = 2,
        .water_line_high = 128,
        .water_line_low = 64,
        .increase_size = 32,
        .step_vaild_time = 16,
        .sampling_time = 16,
    },
    {//rate - 8msg/s
        .govern_id = HAWKEYE_BMS_GOV_8PS,
        .consume_rate_high = 10,
        .consume_rate_low = 6,
        .water_line_high = 128,
        .water_line_low = 64,
        .increase_size = 64,
        .step_vaild_time = 8,
        .sampling_time = 8,
    },
    {//rate - 16msg/s
        .govern_id = HAWKEYE_BMS_GOV_16PS,
        .consume_rate_high = 24,
        .consume_rate_low = 10,
        .water_line_high = 256,
        .water_line_low = 128,
        .increase_size = 128,
        .step_vaild_time = 4,
        .sampling_time = 4,
    },
    {//rate - 32msg/s
        .govern_id = HAWKEYE_BMS_GOV_32PS,
        .consume_rate_high = 999,
        .consume_rate_low = 24,
        .water_line_high = 512,
        .water_line_low = 128,
        .increase_size = 256,
        .step_vaild_time = 2,
        .sampling_time = 1,
    },
};

//======================================//

//======================================//
static int __hawkeye_govern_thread(void *arg)
{
    int i = 0;
    int rate = 0;
    int sleeptm = 0;
    int new_msg_count = 0;
    int gc_msg_count = 0;
    int idle_msg_count = 0;
    int toatl_msg_count = 0;

    while(!kthread_should_stop()){
        gc_msg_count = 0;
        new_msg_count = 0;

        mutex_lock(&gstate.lock);
        if(!gstate.lock_mode){
            rate = libhawkeye_msg_get_rate();
            for(i = 0; i < HAWKEYE_BMS_GOV_MAX; ++i){
                if(rate >= gmap[i].consume_rate_low && rate < gmap[i].consume_rate_high){
                    break;
                }
            }
            if(unlikely(i == HAWKEYE_BMS_GOV_MAX)){
                HAWKEYE_LOG_ERR("crate=%d invalid\n",rate);
                set_current_state(TASK_INTERRUPTIBLE);
                mutex_unlock(&gstate.lock);
                sleeptm = 5;//no match gov, sleep 5 seconds
                goto lbsleep;
            }
            if(gmap[i].govern_id == gstate.target_state){
                gstate.expire_time += gmap[gstate.current_state].sampling_time;
            }else{
                gstate.target_state = gmap[i].govern_id;
                gstate.expire_time = 0;
            }
            if(gstate.expire_time >= gmap[gstate.target_state].step_vaild_time){
                if(gstate.target_state != gstate.current_state){
                    HAWKEYE_LOG_INFO("####expirt_time:%d starte: %d ==>> %d\n",
                        gstate.expire_time,gstate.current_state,gstate.target_state);
                    gstate.current_state = gstate.target_state;
                }else{
                    gstate.expire_time = 0;
                }
            }
        }

        idle_msg_count = libhawkeye_get_msg_idlecount();
        toatl_msg_count = libhawkeye_get_msg_totalcount();
        sleeptm = gmap[gstate.current_state].sampling_time;
        if(idle_msg_count <= gmap[gstate.current_state].water_line_low){
            new_msg_count = HAWKEYE_MIN(gmap[gstate.current_state].increase_size, 
                HAWKEYE_MAX((HAWKEYE_MSG_MAX_COUNT - toatl_msg_count), 0));
        }
        if(idle_msg_count >= gmap[gstate.current_state].water_line_high){
            gc_msg_count = idle_msg_count - gmap[gstate.current_state].water_line_high;
        }
        
        set_current_state(TASK_INTERRUPTIBLE);
        mutex_unlock(&gstate.lock);

        HAWKEYE_LOG_DEBUG("#### totalcount=%d,idlecount=%d,newcount=%d,gcount=%d\n",toatl_msg_count,idle_msg_count,new_msg_count,gc_msg_count);
        if(new_msg_count > 0){
            libhawkeye_alloc_msgs(new_msg_count);
        }

        if(gc_msg_count > 0){
            libhawkeye_free_msgs(gc_msg_count);
        }

lbsleep:
        HAWKEYE_LOG_DEBUG("#### sleep=%d\n",sleeptm);
        schedule_timeout_interruptible(HZ*sleeptm);
        set_current_state(TASK_RUNNING);
        HAWKEYE_LOG_DEBUG("#### wake up\n");
    }

    return 0;
}

//======================================//
int libhawkeye_govern_init(void)
{
    int state = HAWKEYE_BMS_GOV_32PS;
    struct sched_param param = {
        .sched_priority = MAX_USER_RT_PRIO/2,
    };
    /*================================================================*/
    /*1.init govern state*/
    /*================================================================*/
    mutex_init(&gstate.lock);
    mutex_lock(&gstate.lock);
    gstate.expire_time = 0;
    gstate.lock_mode = 0;
    gstate.target_state = state;
    gstate.current_state = state;
    mutex_unlock(&gstate.lock);
    /*================================================================*/
    /*2.alloc max idle msg of current state*/
    /*================================================================*/
    libhawkeye_alloc_msgs(gmap[state].water_line_high);
    /*================================================================*/
    /*3.create govern task for handle idle msg*/
    /*================================================================*/
    g_govern_task = kthread_create(__hawkeye_govern_thread, NULL, "hawkeye_govern");
    if (IS_ERR(g_govern_task)){
        HAWKEYE_LOG_ERR("hawkeye pms thread create failed");
        return PTR_ERR(g_govern_task);
    }
    sched_setscheduler(g_govern_task, SCHED_FIFO, &param);
    wake_up_process(g_govern_task);
    return 0;

}

void libhawkeye_govern_unint(void)
{
    /*================================================================*/
    /*1. stop govern task task */
    /*================================================================*/
    kthread_stop(g_govern_task);
    return ;
}

void libhawkeye_govern_wakeup(void)
{
    HAWKEYE_LOG_DEBUG("#### wake up govern task\n");
    wake_up_process(g_govern_task);
    return ;
}

int libhawkeye_govern_lock(int state)
{
    int ret = 0;
    if(state <= HAWKEYE_BMS_GOV_MAX && 
        state >= HAWKEYE_BMS_GOV_2PS){
        mutex_lock(&gstate.lock);
        gstate.lock_mode = 1;
        gstate.current_state = state;
        mutex_unlock(&gstate.lock);
        wake_up_process(g_govern_task);
    }else{
        ret = -EINVAL;
    }
    HAWKEYE_LOG_DEBUG("lock state=%d\n",state);
    return ret;
}

int libhawkeye_govern_unlock(void)
{
    mutex_lock(&gstate.lock);
    gstate.lock_mode = 0;
    mutex_unlock(&gstate.lock);
    wake_up_process(g_govern_task);
    HAWKEYE_LOG_DEBUG("unlock\n");
    return 0;
}

