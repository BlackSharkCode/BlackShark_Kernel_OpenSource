#ifndef __HAWKEYE_H__
#define __HAWKEYE_H__

/*================================================================*/
/*hawkeye common params*/
/*================================================================*/
//max client name len
#define HAWKEYE_MAX_CLIENT_NAME_LEN            (16)
//max client extra info len
#define HAWKEYE_MAX_CLIENT_PRIV_LEN            (128)
//max msg buffer len
#define HAWKEYE_MAX_BUGMSG_BUFF_LEN            (128)
//hawkeye max configs
#define HAWKEYE_MAX_CONFIG                     (1024)
//hawkeye min configs
#define HAWKEYE_MIN_CONFIG                     (512)
/*================================================================*/
/*hawkeye_command params for bug report clienta */
/*================================================================*/
/*----------------------------------------------------------------
    hawkeye_param_updateclient:
    command param of HAWKEYE_UPDATE_CLIENT
    client_name: client name & not null
    private_string: extra client info
----------------------------------------------------------------*/
struct hawkeye_param_updateclient{
    char client_name[HAWKEYE_MAX_CLIENT_NAME_LEN];
    char private_string[HAWKEYE_MAX_CLIENT_PRIV_LEN];
};
/*----------------------------------------------------------------
    hawkeye_param_startmsg:
    command param of HAWKEYE_START_MSG
    err_no: errno of the msg & not null
----------------------------------------------------------------*/
struct hawkeye_param_startmsg{
    unsigned int err_no;
};
/*----------------------------------------------------------------
    hawkeye_param_stopmsg:
    command param of HAWKEYE_STOP_MSG
    msg_id: value of HAWKEYE_START_MSG return;
----------------------------------------------------------------*/
struct hawkeye_param_stopmsg{
    int msg_id;
};
/*----------------------------------------------------------------
    hawkeye_param_printmsg:
    command param of HAWKEYE_PRINT_MSG
    msg_id: value of HAWKEYE_START_MSG return;
    msg_len: strlen of msg info & not lager than HAWKEYE_MAX_BUGMSG_BUFF_LEN
    msg_str: msg info
----------------------------------------------------------------*/
struct hawkeye_param_printmsg{
    int msg_id;
    int msg_len;
    char msg_str[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};

/*================================================================*/
/*hawkeye_command params for junk server */
/*================================================================*/
enum hawkeye_msg_type{
    MSGTYPE_BUG = 0,
    MSGTYPE_EVENT,
};

enum hawkeye_msg_dttype{
    MSGDATATYPE_SHORT = 0,
    MSGDATATYPE_LONG,
};

struct hawkeye_msg{
    int clt_id;//define client id
    int msg_id;//define msg id
    int cur_tid;//thread id
    int msg_type;//0:bug 1:event
    int page_id;//share file system page number
    int err_no;//define notify error number
    int data_len;//record msg_data used size
    int data_type;//0:short read from msg_data; 1:long read from page_id
    unsigned long long tm_create;//msg create time from boot in milsecs
    unsigned long long tm_kernel;//kernel time in secs
    char msg_data[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};

/*----------------------------------------------------------------
    command param of HAWKEYE_MEM_SHARE
    mm_fd: return share mem fd for mmap & umap
    mm_sz: return share mem size
----------------------------------------------------------------*/
struct hawkeye_param_share{
    int mm_fd;
    int mm_sz;
};
/*----------------------------------------------------------------
    command param of HAWKEYE_MSG_CAPTURE
    msg_id: return this msg id
    page_id: return this msg first page id
    err_no:return this msg errno
----------------------------------------------------------------*/
struct hawkeye_param_capturemsg{
    struct hawkeye_msg msg;
};
/*----------------------------------------------------------------
    command param of HAWKEYE_MSG_RELEASE
    msg_id: value of HAWKEYE_MSG_RELEASE return;
----------------------------------------------------------------*/
struct hawkeye_param_releasemsg{
    int msg_id;
};

/*================================================================*/
/*hawkeye_command params for hawkeye pms */
/*================================================================*/
struct hawkeye_func{
    unsigned short fun_id;
    unsigned long long cur_time;
    unsigned char  cur_type;
    unsigned char  cpu_id;
};
/*----------------------------------------------------------------
    command param of HAWKEYE_START_FUNC
    fun_id: fun id;
----------------------------------------------------------------*/
struct hawkeye_param_func_start{
    unsigned int func_id;
};

/*----------------------------------------------------------------
    command param of HAWKEYE_STOP_FUNC
    fun_id: fun id;
----------------------------------------------------------------*/
struct hawkeye_param_func_stop{
    unsigned int func_id;
};

/*----------------------------------------------------------------
    command param of HAWKEYE_NUM_FUN
    fun_id:     fun id;
    flags:      flags = 0,ignore func_id, and will get total func count of current page,else get the specify func_id func count
----------------------------------------------------------------*/
struct hawkeye_param_get_func_count{
    unsigned short func_flgs;
    unsigned short func_id;
};
/*----------------------------------------------------------------
    command param of HAWKEYE_NUM_FUN
    fun_id:     fun id;
    func_flgs:  func_flgs = 0,ignore func_id, and will get func by idx of current page,else get the specify func_id func by idx
    func_idx:   the idx to get,0< idx < count
----------------------------------------------------------------*/
struct hawkeye_param_get_func{
    unsigned short func_idx;
    unsigned short func_flgs;
    unsigned short func_id;
    struct hawkeye_func func;
};
/*----------------------------------------------------------------
    command param of HAWKEYE_NUM_FUN
    clr_flgs: clear flags
----------------------------------------------------------------*/
struct hawkeye_param_clr_func{
    unsigned short clr_flgs;
};

/*----------------------------------------------------------------
    hawkeye_event info
----------------------------------------------------------------*/
struct hawkeye_event{
    int clt_id;//define client id
    int msg_id;
    int cur_tid;//thread id
    int data_len;//record msg_data used size
    unsigned int event_id;
    unsigned long long tm_create;//msg create time from boot in milsecs
    unsigned long long tm_kernel;//kernel time in secs
    char data[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};
/*----------------------------------------------------------------
    command param of HAWKEYE_NOTIFY_EVENT
    event_id: event id
----------------------------------------------------------------*/
struct hawkeye_param_notify_event{
    unsigned int event_id;
    int data_len;
    char data[HAWKEYE_MAX_BUGMSG_BUFF_LEN];
};

/*----------------------------------------------------------------
    command param of HAWKEYE_GET_EVENT
    event_id: event id
----------------------------------------------------------------*/
struct hawkeye_param_get_event{
    struct hawkeye_event event;
};

/*----------------------------------------------------------------
    command param of HAWKEYE config
    event_id: event id
----------------------------------------------------------------*/
struct hawkeye_config{
    int cfg_start;//cur start mode of config
    int cfg_count;//cur count will config,less than HAWKEYE_MIN_CONFIG
    char cfg_pro[HAWKEYE_MIN_CONFIG];//config values
};

struct hawkeye_param_confg{
    struct hawkeye_config cfg;
};

/*================================================================*/
/*hawkeye_magic char*/
/*================================================================*/
#ifndef HAWKEYE_MAGIC_NUMBER
#define HAWKEYE_MAGIC_NUMBER               'E'
#endif //HAWKEYE_MAGIC_NUMBER

/*================================================================*/
/*hawkeye_command for hawkeye bms */
/*================================================================*/
#define HAWKEYE_UPDATE_CLIENT              _IOWR(HAWKEYE_MAGIC_NUMBER, 0, struct hawkeye_param_updateclient)             
#define HAWKEYE_START_MSG                  _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_param_startmsg)      
#define HAWKEYE_STOP_MSG                   _IOWR(HAWKEYE_MAGIC_NUMBER, 2, struct hawkeye_param_stopmsg) 
#define HAWKEYE_PRINT_MSG                  _IOWR(HAWKEYE_MAGIC_NUMBER, 3, struct hawkeye_param_printmsg)
#define HAWKEYE_NOTIFY_EVENT               _IOWR(HAWKEYE_MAGIC_NUMBER, 4, struct hawkeye_param_notify_event)
#define HAWKEYE_SET_BMSCFG                 _IOWR(HAWKEYE_MAGIC_NUMBER, 5, struct hawkeye_param_confg)
#define HAWKEYE_GET_BMSCFG                 _IOWR(HAWKEYE_MAGIC_NUMBER, 6, struct hawkeye_param_confg)
#define HAWKEYE_SET_EMSCFG                 _IOWR(HAWKEYE_MAGIC_NUMBER, 7, struct hawkeye_param_confg)
#define HAWKEYE_GET_EMSCFG                 _IOWR(HAWKEYE_MAGIC_NUMBER, 8, struct hawkeye_param_confg)

#define HAWKEYE_MSG_CAPTURE                _IOWR(HAWKEYE_MAGIC_NUMBER, 0, struct hawkeye_param_capturemsg)
#define HAWKEYE_MSG_RELEASE                _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_param_releasemsg)
#define HAWKEYE_MEM_SHARE                  _IOWR(HAWKEYE_MAGIC_NUMBER, 2, struct hawkeye_param_share)
#define HAWKEYE_GET_EVENT                  _IOWR(HAWKEYE_MAGIC_NUMBER, 4, struct hawkeye_param_get_event)
/*================================================================*/
/*hawkeye_command for hawkeye pms */
/*================================================================*/
#define HAWKEYE_START_FUNC                  _IOWR(HAWKEYE_MAGIC_NUMBER, 0, struct hawkeye_param_func_start)
#define HAWKEYE_STOP_FUNC                   _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_param_func_stop)

#define HAWKEYE_GET_FUNC_COUNT              _IOWR(HAWKEYE_MAGIC_NUMBER, 0, struct hawkeye_param_get_func_count)
#define HAWKEYE_GET_FUNC_INFO               _IOWR(HAWKEYE_MAGIC_NUMBER, 1, struct hawkeye_param_get_func)
#define HAWKEYE_CLR_FUNC_INFO               _IOWR(HAWKEYE_MAGIC_NUMBER, 2, struct hawkeye_param_clr_func)

#endif /*__HAWKEYE_H__*/
