#include "hawkeye_pms.h"

#define HAWKEYE_LARGE_FIFO_SIZE     (256)
#define HAWKEYE_NORMAL_FIFO_SIZE    (128)
#define HAWKEYE_MOD(v,m)            ((v)&(m-1))// m must be 2^n


static DEFINE_PER_CPU(struct hawkeye_fifo *, g_hfifo);
static struct hawkeye_profile_stat *g_pstat = NULL;
static struct task_struct *g_pms_task = NULL;
static atomic_t dynamic_id = ATOMIC_INIT(0);

extern int hawkeye_pms_wakeup(void);

static struct hawkeye_fifo* hawkeye_pms_fifo_create(int fifo_size)
{
    int ret = -1;
    struct hawkeye_fifo* pfifo = NULL;
    /*================================================================*/
    /*1.check params --- fifo_size:max num of item for fifo*/
    /*================================================================*/
    if(fifo_size < 0){
        HAWKEYE_LOG_ERR("params failed");
        return NULL;
    }
    /*================================================================*/
    /*2.alloc fifo*/
    /*================================================================*/
    pfifo = kzalloc(sizeof(struct hawkeye_fifo),GFP_KERNEL);
    if(NULL == pfifo){
        HAWKEYE_LOG_ERR("malloc failed");
        return NULL;
    }
    /*================================================================*/
    /*3.init &return fifo*/
    /*================================================================*/
    ret = HAWKEYE_FIFO_INIT(pfifo,fifo_size*sizeof(struct hawkeye_profile_msg));
    if(ret < 0){
        HAWKEYE_LOG_ERR("fifo malloc failed");
        kfree(pfifo);
        return NULL;
    }

    return pfifo;
}

static void hawkeye_pms_fifo_destroy(struct hawkeye_fifo* pfifo)
{
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(NULL == pfifo ){
        HAWKEYE_LOG_ERR("params failed");
        return ;
    }
    /*================================================================*/
    /*2.uninit & free fifo*/
    /*================================================================*/
    HAWKEYE_FIFO_UNINIT(pfifo);
    kzfree(pfifo);
    return ;
}

static int hawkeye_pms_send_msg(struct hawkeye_fifo* pfifo, struct hawkeye_profile_msg *msg)
{
    u32 fsz = 0;//free space of fifo
    /*================================================================*/
    /*2.write msg to fifo*/
    /*================================================================*/
    fsz = HAWKEYE_FREE_SIZE(pfifo);
    if(fsz < sizeof(struct hawkeye_profile_msg)){
        HAWKEYE_LOG_ERR("free size=%d,not enough space of fifo, write failed\n",fsz);
        return -ENOMEM;
    }
    HAWKEYE_FIFO_WRITE(pfifo, msg, sizeof(struct hawkeye_profile_msg));
    /*================================================================*/
    /*4. wakup pms task */
    /*================================================================*/
#ifndef CONFIG_HAWKEYE_HP
    wake_up_process(g_pms_task);
#endif //CONFIG_HAWKEYE_HP
    return 0;
}


static int hawkeye_pms_recieve_msg(struct hawkeye_fifo* pfifo, struct hawkeye_profile_msg *msg)
{
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(NULL == pfifo || !msg){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.read msg from fifo*/
    /*================================================================*/
    return HAWKEYE_FIFO_READ(pfifo, msg, sizeof(struct hawkeye_profile_msg));
}

static bool hawkeye_pms_check_msg(struct hawkeye_fifo *pfifo)
{
    return (HAWKEYE_USED_SIZE(pfifo) > 0 ? true : false);
}

static int hawkeye_pms_get_msg_count(struct hawkeye_fifo *pfifo)
{
    return (int)(HAWKEYE_USED_SIZE(pfifo)/sizeof(struct hawkeye_profile_msg));
}

static inline int hawkeye_pms_free_page(int pageid)
{
    int ret = pageid;
    struct hawkeye_profile_page *pp = NULL;

    /*================================================================*/
    /*2.get oldest page*/
    /*================================================================*/
    pp = &g_pstat->pages[pageid];

    /*================================================================*/
    /*3.free page*/
    /*================================================================*/
    memset(pp,0, sizeof(struct hawkeye_profile_page));

    /*================================================================*/
    /*4.update oldest page id*/
    /*================================================================*/
    ret = HAWKEYE_MOD(pageid + 1,HAWKEYE_PAGE_SIZE);
    HAWKEYE_LOG_DEBUG("free page id=%d ok\n",pageid);
    return ret;
}

static int hawkeye_pms_alloc_page(void)
{
    int i = 0;
    int current_id = 0;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat if init*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    HAWKEYE_LOG_DEBUG("current id=%d, oldest_id=%d\n",g_pstat->current_id,g_pstat->oldest_id);
    /*================================================================*/
    /*2.check if there any free page*/
    /*================================================================*/
    current_id = HAWKEYE_MOD(g_pstat->current_id + 1,HAWKEYE_PAGE_SIZE);
    if(unlikely(current_id == g_pstat->oldest_id)){
        HAWKEYE_LOG_ERR("no more page erro,current id[%d]=oldest id\n",g_pstat->current_id);
        return -ENOMEM;
    }
    pp = &g_pstat->pages[current_id];
    pp->flag = HAWKEYE_PMS_PAGE_USED;
    /*================================================================*/
    /*3.init hash of current page*/
    /*================================================================*/
    for(i = 0;i < HAWKEYE_MAXKEY; ++i){
        INIT_HLIST_HEAD(&pp->hash_tbl[i]);
    }
    g_pstat->current_id = current_id;

    HAWKEYE_LOG_DEBUG("new page id=%d, new hash\n",current_id);
    return 0;
}

static struct hawkeye_profile* hawkeye_pms_alloc_profile(void)
{
    int ret = 0;
    struct hawkeye_profile *pf = NULL;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat if init*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("not init failed");
        return NULL;
    }
    /*================================================================*/
    /*2.get a empty profile from current page, if current page is full ,then get it from next page*/
    /*================================================================*/
    pp = &g_pstat->pages[g_pstat->current_id];
    if(pp->index == HAWKEYE_PROFILES_PER_PAGE){

        /*update page flage*/
        pp->flag = HAWKEYE_PMS_PAGE_DIRTY;

        /*2.1 current page is full,notify junk */
        HAWKEYE_LOG_DEBUG("wake up junk svr");
        //wake_up_interruptible_all(&g_pstat->waitq);
        hawkeye_pms_wakeup();

        /*2.2 init new page if need */
        ret = hawkeye_pms_alloc_page();
        if(ret < 0){
            HAWKEYE_LOG_ERR("alloc page failed");
            return NULL;
        }

        /*2.3 get the new page */
        pp = &g_pstat->pages[g_pstat->current_id];
    }
    /*================================================================*/
    /*3.get a profile from the page and update the index of this page */
    /*================================================================*/
    pf = &pp->records[pp->index];
    pp->index++;

    HAWKEYE_LOG_ERR("curren_id=%d, oldest id=%d,index=%d\n",g_pstat->current_id,g_pstat->oldest_id,pp->index);
    return pf;

}

static int hawkeye_pms_hash_profile(struct hawkeye_profile* pf)
{
    uint32_t hh = 0;
    struct hlist_head * phead = NULL;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(NULL == pf){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.check pstat is inited*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    pp = &g_pstat->pages[g_pstat->current_id];
    /*================================================================*/
    /*3.add current profile to current page hash*/
    /*================================================================*/
    INIT_HLIST_NODE(&pf->node);
    hh = hash_32(pf->msg.func.fun_id, HAWKEYE_KEYSIZE);
    phead = &pp->hash_tbl[hh];
    hlist_add_head(&pf->node, phead);

    HAWKEYE_LOG_DEBUG("hash func_id=%d,h=%d,phead=%p\n",pf->msg.func.fun_id,hh,phead);
    return 0;

}

static int hawkeye_pms_init_profiles(void)
{
    int i = 0;
    struct hawkeye_profile_stat *pstat = NULL;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.alloc mem*/
    /*================================================================*/
    pstat = kzalloc(sizeof(struct hawkeye_profile_stat),GFP_KERNEL);
    if(!pstat){
        HAWKEYE_LOG_ERR("stat malloc failed");
        return -EINVAL;
    }
    memset(pstat, 0, sizeof(struct hawkeye_profile_stat));
    g_pstat = pstat;
    /*================================================================*/
    /*2.init current page hash*/
    /*================================================================*/
    pp = &g_pstat->pages[g_pstat->current_id];
    pp->flag = HAWKEYE_PMS_PAGE_USED;
    for(i = 0;i < HAWKEYE_MAXKEY; ++i){
        INIT_HLIST_HEAD(&pp->hash_tbl[i]);
    }
    HAWKEYE_LOG_ERR("init ok\n");
    return 0;
}

static inline bool hawkeye_pms_task_continue(void)
{
    bool task_continue = true;
    int cpu;
    u32 seq_ids[NR_CPUS] = {0};

repeat:
    if(unlikely(kthread_should_stop())) {
        task_continue = false;
        goto out;
    }
    /*================================================================*/
    /*1.check if have msg for handle*/
    /*================================================================*/
    for_each_possible_cpu(cpu){
        if(hawkeye_pms_check_msg(per_cpu(g_hfifo, cpu))) {
            set_current_state(TASK_RUNNING);
            HAWKEYE_LOG_DEBUG("cpu %d fifo not empty task state to TASK_RUNNING\n", cpu);
            goto out;
        }
        seq_ids[cpu] = per_cpu(g_hfifo, cpu)->seq_id;
    }

    set_current_state(TASK_INTERRUPTIBLE);
    HAWKEYE_LOG_DEBUG("no fifo data state to  TASK_INTERRUPTIBLE\n");
    smp_mb();
    /*================================================================*/
    /*2.check if have msg for handle again by seqid*/
    /*================================================================*/
    for_each_possible_cpu(cpu){
        if(unlikely(seq_ids[cpu] != per_cpu(g_hfifo, cpu)->seq_id)){
            set_current_state(TASK_RUNNING);
            HAWKEYE_LOG_DEBUG("cpu %d fifo have new data task state to TASK_RUNNING\n", cpu);
            goto repeat;
        }
    }
#ifdef CONFIG_HAWKEYE_HP
    schedule_timeout(HZ);
    HAWKEYE_LOG_DEBUG("hawkeye_pms_task_continue timeout repeat\n");
#else
    schedule();
    HAWKEYE_LOG_DEBUG("hawkeye_pms_task_continue repeat\n");
#endif //CONFIG_HAWKEYE_HP

    goto repeat;

out:
    HAWKEYE_LOG_DEBUG("hawkeye_pms_task_continue out\n");
    return task_continue;
}

static int hawkeye_pms_thread(void *arg)
{
    int cpu, earliest_msg_id;
    unsigned long compare_mask;
    unsigned long total_msg_count;
    unsigned long msg_count[NR_CPUS];
    struct hawkeye_profile *profile;
    struct hawkeye_profile_msg msg[NR_CPUS];

    while(hawkeye_pms_task_continue())
    {
        total_msg_count = 0;
        compare_mask = 0;
        earliest_msg_id = -1;
        profile = NULL;
        memset(msg_count, 0, sizeof(u32)*NR_CPUS);
        memset(msg, 0, sizeof(struct hawkeye_profile_msg)*NR_CPUS);
        /*================================================================*/
        /*1.get every fifo and total msg num*/
        /*================================================================*/
        for_each_possible_cpu(cpu){
            msg_count[cpu] = hawkeye_pms_get_msg_count(per_cpu(g_hfifo, cpu));
            total_msg_count += msg_count[cpu];
        }
        HAWKEYE_LOG_DEBUG("total msg count=%lu\n",total_msg_count);
        while(total_msg_count){
            earliest_msg_id = -1;
            /*================================================================*/
            /*2.get every cpu msg from fifo if have */
            /*================================================================*/
            for_each_possible_cpu(cpu){
                if(msg_count[cpu] && !test_bit(cpu, &compare_mask)){
                    hawkeye_pms_recieve_msg(per_cpu(g_hfifo, cpu), &msg[cpu]);
                    msg_count[cpu]--;
                    set_bit(cpu, &compare_mask);
                }
            }
            HAWKEYE_LOG_DEBUG("compare_mask=%lu\n",compare_mask);
            /*================================================================*/
            /*3.get the oldest cpu msg  */
            /*================================================================*/
            for_each_set_bit(cpu, &compare_mask, NR_CPUS){
                if(earliest_msg_id < 0){
                    earliest_msg_id = cpu;
                    continue;
                }
                if(time_after64(msg[earliest_msg_id].func.cur_time, msg[cpu].func.cur_time)){
                    earliest_msg_id = cpu;
                    continue;
                }
            }
            HAWKEYE_LOG_DEBUG("earliest_msg_id=%d\n",earliest_msg_id);
            /*================================================================*/
            /*4.alloc profile and hash the msg  */
            /*================================================================*/
            profile = hawkeye_pms_alloc_profile();
            if(profile){
                memcpy(&(profile->msg), &msg[earliest_msg_id], sizeof(struct hawkeye_profile_msg));
                hawkeye_pms_hash_profile(profile);
            }else{
                HAWKEYE_LOG_ERR("hawkeye_pms_alloc_profile failed, shouldn't happen\n"); 
            }
            /*================================================================*/
            /*5.free msg info  */
            /*================================================================*/
            total_msg_count--;
            clear_bit(earliest_msg_id, &compare_mask);
        }
    }

    HAWKEYE_LOG_DEBUG("hawkeye pms thread exited\n");
    return 0;
}

int libhawkeye_pms_dbg_callback(void)
{
    int cpu = 0;
    for_each_possible_cpu(cpu){
        HAWKEYE_LOG_DEBUG("fifo on cpu %d count %d\n",cpu,hawkeye_pms_get_msg_count(per_cpu(g_hfifo, cpu))); 
    }
    return 0;
}


int libhawkeye_pms_uninit(void)
{
    int cpu = 0;
    /*================================================================*/
    /*1. free fifo of every cpu */
    /*================================================================*/
    for_each_possible_cpu(cpu){
    if(per_cpu(g_hfifo, cpu))
        hawkeye_pms_fifo_destroy(per_cpu(g_hfifo, cpu));
    }

    /*================================================================*/
    /*2. stop pms task */
    /*================================================================*/
    kthread_stop(g_pms_task);

    /*================================================================*/
    /*3. free mem */
    /*================================================================*/
    kfree(g_pstat);
    g_pstat = NULL;
    return 0;
}


int libhawkeye_pms_start_func(u32 *func_id)
{
    struct hawkeye_profile_msg msg;
    unsigned long flags;

    local_irq_save(flags);
    msg.func.fun_id = (u16)(*func_id);
    msg.func.cur_time = sched_clock();
    msg.func.cur_type = HAWKEYE_PMS_MSG_START;
    msg.func.cpu_id = (u8)smp_processor_id();
    msg.dynamic_id =  (u16)atomic_inc_return(&dynamic_id);
    hawkeye_pms_send_msg(per_cpu(g_hfifo, msg.func.cpu_id),&msg);
    *func_id = ((msg.dynamic_id<<16)|(msg.func.fun_id&0xFFFF));
    local_irq_restore(flags);

    HAWKEYE_LOG_DEBUG("func_id=%d\n", *func_id);
    return 0;
}

int libhawkeye_pms_stop_func(u32 *func_id)
{
    struct hawkeye_profile_msg msg;
    unsigned long flags;

    local_irq_save(flags);
    msg.func.fun_id = (u16)((*func_id) & 0xFFFF);
    msg.func.cur_time = sched_clock();
    msg.func.cur_type = HAWKEYE_PMS_MSG_STOP;
    msg.func.cpu_id = (u8)smp_processor_id();
    msg.dynamic_id =  (u16)(((*func_id)>>16) & 0xFFFF);
    hawkeye_pms_send_msg(per_cpu(g_hfifo, msg.func.cpu_id),&msg);
    local_irq_restore(flags);

    HAWKEYE_LOG_DEBUG("func_id=%d\n", *func_id);
    return 0;
}

int libhawkeye_pms_get_total_count(void)
{
    int num = 0;
    int count = 0;
    int pageid = 0;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat is inited*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("params failed\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*3.get total num from oldest page hash*/
    /*================================================================*/
    pageid = g_pstat->oldest_id;
    for(;count < HAWKEYE_PAGE_SIZE; count++){
        pp = &g_pstat->pages[pageid];
        if(pp->flag != HAWKEYE_PMS_PAGE_DIRTY)
            break;

        num += HAWKEYE_PROFILES_PER_PAGE;
        pageid = HAWKEYE_MOD(pageid + 1, HAWKEYE_PAGE_SIZE);
    }
    HAWKEYE_LOG_DEBUG("get total num=%d\n",num);
    return num;
}

int libhawkeye_pms_get_total_count_by_funcid(u16 func_id)
{
    int num = 0;
    int pageid = 0;
    int count = 0;
    struct hlist_head *phead = NULL;
    struct hawkeye_profile *pf = NULL;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat is inited*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*3.get func_id num from oldest page hash*/
    /*================================================================*/
    pageid = g_pstat->oldest_id;
    for(;count < HAWKEYE_PAGE_SIZE; count++){
        pp = &g_pstat->pages[pageid];
        if(pp->flag != HAWKEYE_PMS_PAGE_DIRTY)
            break;

        phead = &pp->hash_tbl[hash_32(func_id, HAWKEYE_KEYSIZE)];
        hlist_for_each_entry(pf, phead, node) {
            if(pf->msg.func.fun_id == func_id)
                num++;
        }
        pageid = HAWKEYE_MOD(pageid + 1, HAWKEYE_PAGE_SIZE);
    }
    HAWKEYE_LOG_DEBUG("get total num=%d by idfun=%d\n",num,func_id);
    return num;
}

int libhawkeye_pms_get_func_by_index(int idx, struct hawkeye_func *pfuns)
{
    int pageid = 0;
    int count = 0;
    int pos = idx;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat is inited*/
    /*================================================================*/
    if(!g_pstat || !pfuns){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get idx fun from current page hash*/
    /*================================================================*/
    pageid = g_pstat->oldest_id;
    for(;count < HAWKEYE_PAGE_SIZE; count++){
        pp = &g_pstat->pages[pageid];
        pageid = HAWKEYE_MOD(pageid + 1,HAWKEYE_PAGE_SIZE);
        if(pp->flag != HAWKEYE_PMS_PAGE_DIRTY)
            break;

        pos -= HAWKEYE_PROFILES_PER_PAGE;
        if(pos > 0)
            continue;

        pos += HAWKEYE_PROFILES_PER_PAGE;
        memcpy(pfuns,&pp->records[pos].msg.func,sizeof(struct hawkeye_func));
        break;
    }
    HAWKEYE_LOG_DEBUG("get idfun=%d by idx=%d,page id=%d,pos=%d by \n",pfuns->fun_id,idx,pageid,pos);
    return 0;
}

int libhawkeye_pms_get_func_by_funcid(int idx,struct hawkeye_func *pfuns,u16 func_id)
{
    int pageid = 0;
    int count = 0;
    int pos = idx;
    int hh = 0;
    struct hlist_head *phead = NULL;
    struct hawkeye_profile *pf = NULL;
    struct hawkeye_profile_page *pp = NULL;

    /*================================================================*/
    /*1.check pstat is inited*/
    /*================================================================*/
    if(!g_pstat || !pfuns){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get func_id fun from current page hash*/
    /*================================================================*/
    pageid = g_pstat->oldest_id;
    for(;count < HAWKEYE_PAGE_SIZE; count++){
        pp = &g_pstat->pages[pageid];
        if(pp->flag != HAWKEYE_PMS_PAGE_DIRTY)
            break;

        hh = hash_32(func_id, HAWKEYE_KEYSIZE);
        phead = &pp->hash_tbl[hh];
        HAWKEYE_LOG_DEBUG("get hash func_id=%d,h=%d,phead=%p\n",func_id,hh,phead);

        hlist_for_each_entry(pf, phead, node) {
            if(pf->msg.func.fun_id != func_id)
               continue;

            if(0 == pos){
                memcpy(pfuns,&pf->msg.func,sizeof(struct hawkeye_func));
                HAWKEYE_LOG_DEBUG("get idfun=%d by idx=%d,page id=%d\n",func_id,idx,pageid);
                return 0;
            }
            --pos;
        }
        pageid = HAWKEYE_MOD(pageid + 1,HAWKEYE_PAGE_SIZE);
    }
    HAWKEYE_LOG_DEBUG("get idfun=%d by idx=%d failed\n",func_id,idx);
    return -ENOENT;
}

int libhawkeye_pms_clr_all(void)
{
    int pageid = g_pstat->oldest_id;
    int count = 0;
    struct hawkeye_profile_page *pp = NULL;
    /*================================================================*/
    /*1.check pstat is inited */
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("params failed");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.free oldest page */
    /*================================================================*/
    for(;count < HAWKEYE_PAGE_SIZE; count++){
        pp = &g_pstat->pages[pageid];
        if(pp->flag != HAWKEYE_PMS_PAGE_DIRTY)
            break;

        pageid = hawkeye_pms_free_page(pageid);
        HAWKEYE_LOG_DEBUG("pms page %d finish gc", g_pstat->oldest_id);
        g_pstat->oldest_id = pageid;
    }
    return 0;
}

int libhawkeye_pms_empty(void)
{
    /*================================================================*/
    /*1.check if bug server already init,if not return erro*/
    /*================================================================*/
    if(!g_pstat){
        HAWKEYE_LOG_ERR("bug manger not init\n");
        return -1;
    }
    /*================================================================*/
    /*2.check oldest page ready*/
    /*================================================================*/
    HAWKEYE_LOG_DEBUG("oldest id=%d, current id=%d\n",g_pstat->oldest_id,g_pstat->current_id);
    if(g_pstat->oldest_id == g_pstat->current_id){
        return 1;//empty
    }
    return 0;//not empty
}

extern struct hawkeye_pms_operations g_heyepms_ops;
static int hawkeye_pms_inf_init(void)
{
    g_heyepms_ops.fun_start = libhawkeye_pms_start_func;
    g_heyepms_ops.fun_stop = libhawkeye_pms_stop_func;
    g_heyepms_ops.fun_empty = libhawkeye_pms_empty;

    g_heyepms_ops.fun_count = libhawkeye_pms_get_total_count;
    g_heyepms_ops.fun_count_by_id = libhawkeye_pms_get_total_count_by_funcid;
    g_heyepms_ops.fun_info_by_idx = libhawkeye_pms_get_func_by_index;
    g_heyepms_ops.fun_info_by_id = libhawkeye_pms_get_func_by_funcid;
    g_heyepms_ops.fun_clr = libhawkeye_pms_clr_all;

    HAWKEYE_LOG_INFO("pms inf init\n");
    return 0;
}

int libhawkeye_pms_init(void)
{
    int cpu;
    struct hawkeye_fifo *fptr = NULL;
    struct sched_param param = {
        .sched_priority = MAX_USER_RT_PRIO/2,
    };

    /*================================================================*/
    /*1.init global profile manager stat*/
    /*================================================================*/
    hawkeye_pms_init_profiles();
    hawkeye_pms_inf_init();
    /*================================================================*/
    /*2. create fifo on each cpu */
    /*================================================================*/
    for_each_possible_cpu(cpu){
        if(cpu == 0 || cpu == 1)
            fptr = hawkeye_pms_fifo_create(HAWKEYE_LARGE_FIFO_SIZE);
        else
            fptr = hawkeye_pms_fifo_create(HAWKEYE_NORMAL_FIFO_SIZE);

        if(!fptr){
            HAWKEYE_LOG_ERR("hawkeye pms create cpu %d's fifo failed", cpu);
            goto err_out;
        }
        HAWKEYE_LOG_DEBUG("create cpu %d fifo %p\n",cpu,fptr);
        per_cpu(g_hfifo, cpu) = fptr;
    }

    /*================================================================*/
    /*3. create pms task for handle fifo */
    /*================================================================*/
    g_pms_task = kthread_create(hawkeye_pms_thread, NULL, "hawkeye_pms");
    if (IS_ERR(g_pms_task)){
        HAWKEYE_LOG_ERR("hawkeye pms thread create failed");
        kfree(g_pstat);
        g_pstat = NULL;
        return PTR_ERR(g_pms_task);
    }
    sched_setscheduler(g_pms_task, SCHED_FIFO, &param);
    wake_up_process(g_pms_task);
    HAWKEYE_LOG_DEBUG("create pms task %p success\n",g_pms_task);
    return 0;

err_out:
    for_each_possible_cpu(cpu){
        if(per_cpu(g_hfifo, cpu))
            hawkeye_pms_fifo_destroy(per_cpu(g_hfifo, cpu));
    }
    kthread_stop(g_pms_task);
    kfree(g_pstat);
    g_pstat = NULL;

    return 0;

}