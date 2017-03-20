#ifndef __SHARK_UTIL_H__
#define __SHARK_UTIL_H__

static struct workqueue_struct *shark_pm_monitor_wq = NULL;

#define MAX_PID 65536
#define WAKELOCK_SIZE 2048
#define NUM_BUSY_THREAD_CHECK 5
struct _shark_kernel_power_monitor {
        struct delayed_work dwork;
        unsigned long  *prev_proc_time;
        int *curr_proc_delta;
        int *curr_proc_pid;
        char *curr_active_wakelock;
        struct task_struct **task_ptr_array;
        struct kernel_cpustat curr_cpustat;
        struct kernel_cpustat prev_cpustat;
        unsigned long cpustat_time;
        int top_loading_pid[NUM_BUSY_THREAD_CHECK];
        spinlock_t lock;
};
extern ssize_t pm_show_wakelocks(char *buf, bool show_active);
extern void shark_get_rpmh_master_stats(void);
extern void shark_get_rpmstats(void);
#endif
