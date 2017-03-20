#include "hawkeye_bms_fs.h"
#include "hawkeye_bms_msg.h"

extern struct hawkeye_list g_heyeclient;
extern struct hawkeye_bms_operations g_heyebms_ops;
extern int hawkeye_get_client_id(void);

static int g_defclientfd = 0;
static struct hawkeye_bug_server g_heye_bugsrv = {0};

static inline int __hawkeye_create_msg(int clientfd, u32 errid,int msgtype)
{
    int msgid = -1;
    int clientid = -1;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if( clientfd < 0 || HAWKEYE_VERIFY_CLIENTFD(clientfd)){
        HAWKEYE_LOG_ERR("invalid client fd=%d\n", clientfd);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.check the level of msg*/
    /*================================================================*/
    if(!libhawkeye_config_check(msgtype,errid)){
        return -EPERM;
    }
    /*================================================================*/
    /*3.check client if still exist*/
    /*================================================================*/
    clientid = HAWKEYE_CONVERT_CLIENTFD2ID(clientfd);
    if(clientid != HAWKEYE_LST_FIND(&g_heyeclient,struct hawkeye_bug_client,client_id,clientid,HAWKEYE_LST_NUMCMP,client_id)){
        HAWKEYE_LOG_ERR("client fd=%d no exist\n",clientfd);
        return -EINVAL;
    }
    /*================================================================*/
    /*4.create a new msg and return msg id*/
    /*================================================================*/
    msgid = libhawkeye_create_msg(clientid,errid,msgtype);
    if(msgid < 0){
        HAWKEYE_LOG_ERR("client fd=%d,errid=%d no empty msg\n",errid,clientfd);
        return -EINVAL;
    }
    HAWKEYE_LOG_DEBUG("msg start client_fd=%d, errid=%d,msg_id=%d\n", clientfd,errid,msgid);
    return msgid;
}

static inline void __hawkeye_on_time(unsigned long data)
{
    /*================================================================*/
    /*1.gc msg every one hour*/
    /*================================================================*/
    HAWKEYE_LOG_DEBUG("gc msg on timer\n");
    /*================================================================*/
    /*2.gc msg */
    /*================================================================*/
    libhawkeye_msg_gc(MSGTYPE_EVENT);
    libhawkeye_msg_gc(MSGTYPE_BUG);
    /*================================================================*/
    /*3.restart timer */
    /*================================================================*/
    mod_timer(&g_heye_bugsrv.msg_gctimer, jiffies + HAWKEYE_MAX_GCTIMER);
    return ;
}

int libhawkeye_bms_dbg_callback(void)
{
    HAWKEYE_LOG_DEBUG("client list count=%d\n",HAWKEYE_LST_COUNT(&g_heyeclient));
    return 0;
}

int libhawkeye_bms_default_client(void)
{
    int clientfd = -1;
    struct hawkeye_bug_client *pclient = NULL;

    /*================================================================*/
    /*1.create a new client with default name and NULL private info*/
    /*================================================================*/
    if(in_interrupt()){
        pclient = kzalloc(sizeof(struct hawkeye_bug_client), GFP_ATOMIC);
    }else{
        pclient = kzalloc(sizeof(struct hawkeye_bug_client), GFP_KERNEL);
    }
    if(!pclient){
        HAWKEYE_LOG_ERR("client malloc failed\n");
        return -ENOMEM;
    }
    pclient->client_id = hawkeye_get_client_id();
    pclient->private_data = NULL;
    snprintf(pclient->client_name,HAWKEYE_MAX_CLIENT_NAME_LEN, HAWKEYE_DEFAULT_NAME,pclient->client_id);

    /*================================================================*/
    /*2.add new client to client list and return this client fd*/
    /*================================================================*/
    HAWKEYE_LIST_ADD(&g_heyeclient,pclient);

    clientfd = HAWKEYE_CONVERT_CLIENTID2FD(pclient->client_id);
    HAWKEYE_LOG_DEBUG("client %s register id=%d, fd=%d,total_cout=%d\n",
        pclient->client_name,pclient->client_id,clientfd,HAWKEYE_LST_COUNT(&g_heyeclient));
    return clientfd;

}

int libhawkeye_bms_register_client(const char *clientname, const char *privatedata)
{
    int clientfd = -1;
    int clientid = -1;
    struct hawkeye_bug_client *pclient = NULL;
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.check params*/
    /*================================================================*/
    if(!clientname){
        HAWKEYE_LOG_ERR("invalid client name\n");
        return -EINVAL;
    }

    /*================================================================*/
    /*3.check if have registered client with same name, if so, return the fd*/
    /*================================================================*/
    clientid = HAWKEYE_LST_FIND(&g_heyeclient,struct hawkeye_bug_client,client_name, clientname, HAWKEYE_LST_STRCMP, client_id);
    if(clientid >= 0){
        clientfd = HAWKEYE_CONVERT_CLIENTID2FD(clientid);
        HAWKEYE_LOG_DEBUG("client %s already register and id =%d, fd=%d\n",clientname,clientid,clientfd);
        return clientfd;
    }

    /*================================================================*/
    /*4.create new client info*/
    /*================================================================*/
    if(in_interrupt()){
        pclient = (struct hawkeye_bug_client*)kzalloc(sizeof(struct hawkeye_bug_client), GFP_ATOMIC);
    }else{
        pclient = (struct hawkeye_bug_client*)kzalloc(sizeof(struct hawkeye_bug_client), GFP_KERNEL);
    }
    if(!pclient){
        HAWKEYE_LOG_ERR("client %s malloc failed\n",clientname);
        return -ENOMEM;
    }
    pclient->client_id = hawkeye_get_client_id();
    strncpy(pclient->client_name, clientname, HAWKEYE_MAX_CLIENT_NAME_LEN);
    if(privatedata){
        if(in_interrupt()){
            pclient->private_data = kstrdup(privatedata, GFP_ATOMIC);
        }else{
            pclient->private_data = kstrdup(privatedata, GFP_KERNEL);
        }
        
        if(!pclient->private_data){
            HAWKEYE_LOG_ERR("kstrdup private_data failed\n");
        }
    }

    /*================================================================*/
    /*5.add to the client list and return this client fd*/
    /*================================================================*/
    HAWKEYE_LIST_ADD(&g_heyeclient,pclient);

    clientfd = HAWKEYE_CONVERT_CLIENTID2FD(pclient->client_id);
    HAWKEYE_LOG_DEBUG("client %s register id=%d, fd=%d,total_cout=%d\n",
        pclient->client_name,pclient->client_id,clientfd,HAWKEYE_LST_COUNT(&g_heyeclient));

    return clientfd;
}

int libhawkeye_bms_unregister_client(int clientfd)
{
    int clientid = -1;
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.check params*/
    /*================================================================*/
    if( clientfd < 0 || HAWKEYE_VERIFY_CLIENTFD(clientfd)){
        HAWKEYE_LOG_ERR("invalid client fd=%d\n", clientfd);
        return -EINVAL;
    }
    /*================================================================*/
    /*3.delete current client from client lst*/
    /*================================================================*/
    clientid = HAWKEYE_CONVERT_CLIENTFD2ID(clientfd);
    HAWKEYE_LST_DEL(&g_heyeclient,struct hawkeye_bug_client,client_id,clientid,HAWKEYE_LST_NUMCMP,HAWKEYE_CLIENT_FREE);

    /*================================================================*/
    /*4.clean current client msg*/
    /*================================================================*/
    libhawkeye_msg_clean(clientid);
    HAWKEYE_LOG_DEBUG("client fd=%d unregistertotal_cout=%d\n",clientfd,HAWKEYE_LST_COUNT(&g_heyeclient));
    return 0;
}

static int __hawkeye_bms_default_register(void)
{
    g_defclientfd = libhawkeye_bms_register_client("default",NULL);
    HAWKEYE_LOG_INFO("reigster default client fd=0x%x\n",g_defclientfd);
    return g_defclientfd;
}

static int __hawkeye_bms_default_unregister(void)
{
    HAWKEYE_LOG_INFO("unreigster default client fd=0x%x\n",g_defclientfd);
    if(g_defclientfd > 0 ){
        libhawkeye_bms_unregister_client(g_defclientfd);
        g_defclientfd = 0;
    }
    return 0;
}

int libhawkeye_bms_msg_start(int clientfd, u32 errid)
{
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.create bug msg*/
    /*================================================================*/
    return __hawkeye_create_msg(clientfd,errid,MSGTYPE_BUG);
}


int libhawkeye_bms_msg_stop(int msgid)
{
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.now lunch the msg*/
    /*================================================================*/
    libhawkeye_launch_msg(msgid);
    return 0;
}


int libhawkeye_bms_msg_record(int msgid,const char *fmt, ...)
{
    int ret = -1;
    int len = 0;
    va_list args;
    char logbuf[HAWKEYE_MAX_BUGMSG_BUFF_LEN] = {0};
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.formate msg*/
    /*================================================================*/
    va_start(args, fmt);
    len = vsnprintf(logbuf, HAWKEYE_MAX_BUGMSG_BUFF_LEN, fmt, args);
    va_end(args);

    /*================================================================*/
    /*3.write msg to mem if need*/
    /*================================================================*/
    if(len > 0){
        ret = libhawkeye_load_msg(msgid, logbuf, len);
    }else{
        HAWKEYE_LOG_ERR("vsnprintf failed %d\n",len);
    }
    HAWKEYE_LOG_DEBUG("libhawkeye_load_msg msg_id =%d, write len =%d, actual write size =%d\n", msgid, len, ret);
    return 0;
}

int libhawkeye_bms_msg_copy(int msgid, const char *buff, int size)
{
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*1.write msg to mem if need*/
    /*================================================================*/
    return libhawkeye_load_msg(msgid, buff, size);
}

int libhawkeye_ems_notify(int clientfd, u32 eventid, const char *buff, int size)
{
    int ret = -EINVAL;
    int msgid = 0;
    /*================================================================*/
    /*1.check if init*/
    /*================================================================*/
    if(!g_heye_bugsrv.binit){
        HAWKEYE_LOG_ERR("uninit\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.create event msg*/
    /*================================================================*/
    ret = __hawkeye_create_msg(clientfd,eventid,MSGTYPE_EVENT);
    if(ret < 0 ){
        if(ret != (-EPERM)){
            HAWKEYE_LOG_ERR("__hawkeye_create_msg failed eventid=0X%X,msgid=%d\n",eventid,msgid);
        }
        return ret;
    }
    msgid = ret;
    /*================================================================*/
    /*3.handle event buf if need*/
    /*================================================================*/
    if(buff && size > 0){
        //event only support short msg
        size = HAWKEYE_MIN(size,HAWKEYE_MAX_BUGMSG_BUFF_LEN);
        ret = libhawkeye_load_msg(msgid, buff, size);
        if(ret < 0){
            HAWKEYE_LOG_ERR("libhawkeye_load_msg failed eventid=0X%X,msgid=%d\n",eventid,msgid);
            return ret;
        }
    }
    /*================================================================*/
    /*4.notify event*/
    /*================================================================*/
    ret = libhawkeye_launch_msg(msgid);
    if(ret < 0){
        HAWKEYE_LOG_ERR("libhawkeye_load_msg failed eventid=0X%X,msgid=%d\n",eventid,msgid);
        return ret;
    }
    return 0;
}

int libhawkeye_ems_get(struct hawkeye_event *pevent)
{
    int ret = 0;
    struct hawkeye_param_capturemsg cpmsg;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(!pevent){
        HAWKEYE_LOG_ERR("invalid params\n");
        return -EINVAL;
    }
    /*================================================================*/
    /*2.get ready event msg for process */
    /*================================================================*/
    ret = libhawkeye_caputre_msg(MSGTYPE_EVENT,&cpmsg);
    if(ret < 0){
        HAWKEYE_LOG_DEBUG("libhawkeye_caputre_msg failed retd=%d\n",ret);
        return ret;
    }

    pevent->clt_id = cpmsg.msg.clt_id;
    pevent->msg_id = cpmsg.msg.msg_id;
    pevent->event_id = cpmsg.msg.err_no;
    pevent->cur_tid = cpmsg.msg.cur_tid;
    pevent->data_len = HAWKEYE_MIN(cpmsg.msg.data_len,HAWKEYE_MAX_BUGMSG_BUFF_LEN);
    pevent->tm_create = cpmsg.msg.tm_create;
    pevent->tm_kernel = cpmsg.msg.tm_kernel;
    memcpy(pevent->data,cpmsg.msg.msg_data,pevent->data_len);
    
    /*================================================================*/
    /*3.after process ,the msg goto idle */
    /*================================================================*/
    ret = libhawkeye_release_msg(MSGTYPE_EVENT,cpmsg.msg.msg_id);
    return ret;
}


int libhawkeye_bms_check_by_clientid(int clientid)
{
    return HAWKEYE_LST_FIND(&g_heyeclient,struct hawkeye_bug_client,client_id, clientid, HAWKEYE_LST_NUMCMP, client_id);
}

int libhawkeye_bms_check_by_clientfd(int clientfd)
{
    int clientid = -1;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if( clientfd < 0 || HAWKEYE_VERIFY_CLIENTFD(clientfd)){
        HAWKEYE_LOG_ERR("invalid client fd=%d\n", clientfd);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.check client lst*/
    /*================================================================*/
    clientid = HAWKEYE_CONVERT_CLIENTFD2ID(clientfd);
    return HAWKEYE_LST_FIND(&g_heyeclient,struct hawkeye_bug_client,client_id, clientid, HAWKEYE_LST_NUMCMP, client_id);
}

int libhawkeye_bms_update_client(int clientfd,const char *clientname,const char *private_data)
{
    int clientid = -1;
    struct hawkeye_bug_client *pclient = NULL;
    /*================================================================*/
    /*1.check params*/
    /*================================================================*/
    if(clientfd < 0 || HAWKEYE_VERIFY_CLIENTFD(clientfd)){
        HAWKEYE_LOG_ERR("invalid client fd=%d\n", clientfd);
        return -EINVAL;
    }
    /*================================================================*/
    /*2.if the name already exist,and delete the default added before*/
    /*================================================================*/
    pclient = HAWKEYE_LST_GET(&g_heyeclient,struct hawkeye_bug_client,client_name, clientname, HAWKEYE_LST_STRCMP);
    if(pclient){
        HAWKEYE_LOG_DEBUG("as the client name %s already register, delete the default %d added before\n", clientname,clientfd);
        /*2.1 delete the default added before*/
        HAWKEYE_LST_DEL(&g_heyeclient,
                        struct hawkeye_bug_client,
                        client_id,
                        HAWKEYE_CONVERT_CLIENTFD2ID(clientfd),
                        HAWKEYE_LST_NUMCMP,
                        HAWKEYE_CLIENT_FREE);
        /*2.2 get the client fd of the find client*/
        clientfd = HAWKEYE_CONVERT_CLIENTID2FD(pclient->client_id);
    }else{
        /*================================================================*/
        /*3. update the default client added before and return this client fd*/
        /*================================================================*/
        clientid = HAWKEYE_CONVERT_CLIENTFD2ID(clientfd);
        /*3.1.get the client by client id*/
        pclient= HAWKEYE_LST_GET(&g_heyeclient,struct hawkeye_bug_client,client_id,clientid,HAWKEYE_LST_NUMCMP);
        if(!pclient){
            HAWKEYE_LOG_ERR("invalid client fd=%d\n", clientfd);
            return -EINVAL;
        }
        /*3.2.update client name*/
        strncpy(pclient->client_name, clientname,HAWKEYE_MAX_CLIENT_NAME_LEN);
        if(pclient->private_data){
            HAWKEYE_LOG_ERR("should not got here\n");
        }
    }

    /*2.3|3.3 update private data*/
    if(private_data){
        if(in_interrupt()){
           pclient->private_data = kstrdup(private_data, GFP_ATOMIC);
        }else{
           pclient->private_data = kstrdup(private_data, GFP_KERNEL);
        }
        if(!pclient->private_data){
            HAWKEYE_LOG_ERR("kstrdup private_data failed\n");
        }
    }
    /*2.4|3.4 return the current find client fd*/
    HAWKEYE_LIST_ADD(&g_heyeclient,pclient);

    HAWKEYE_LOG_DEBUG("client %s register id=%d, fd=%d,total_cout=%d\n",
        pclient->client_name,pclient->client_id,clientfd,HAWKEYE_LST_COUNT(&g_heyeclient));
    return clientfd;
}

int libhawkeye_bms_get_default_clientfd(void)
{
    return g_defclientfd;
}

static int __hawkeye_bms_inf_init(void)
{
    g_heyebms_ops.config_set = libhawkeye_config_set;
    g_heyebms_ops.config_get = libhawkeye_config_get;

    g_heyebms_ops.default_client = libhawkeye_bms_default_client;
    g_heyebms_ops.update_client = libhawkeye_bms_update_client;
    g_heyebms_ops.register_client = libhawkeye_bms_register_client;
    g_heyebms_ops.unregister_client = libhawkeye_bms_unregister_client;

    g_heyebms_ops.msg_start = libhawkeye_bms_msg_start;
    g_heyebms_ops.msg_copy = libhawkeye_bms_msg_copy;
    g_heyebms_ops.msg_stop = libhawkeye_bms_msg_stop;
    g_heyebms_ops.msg_empty = libhawkeye_msg_empty;

    g_heyebms_ops.ems_notify = libhawkeye_ems_notify;
    g_heyebms_ops.ems_get = libhawkeye_ems_get;

    g_heyebms_ops.bugfs_share = libhawkeye_bugfs_share;
    g_heyebms_ops.msg_capture = libhawkeye_caputre_msg;
    g_heyebms_ops.msg_release = libhawkeye_release_msg;

    HAWKEYE_LOG_INFO("bms inf init\n");
    return 0;
}

static int __hawkeye_bms_inf_uninit(void)
{
    g_heyebms_ops.config_set = NULL;
    g_heyebms_ops.config_get = NULL;

    g_heyebms_ops.default_client = NULL;
    g_heyebms_ops.update_client = NULL;
    g_heyebms_ops.register_client = NULL;
    g_heyebms_ops.unregister_client = NULL;

    g_heyebms_ops.msg_start = NULL;
    g_heyebms_ops.msg_copy = NULL;
    g_heyebms_ops.msg_stop = NULL;
    g_heyebms_ops.msg_empty = NULL;

    g_heyebms_ops.ems_notify = NULL;
    g_heyebms_ops.ems_get = NULL;

    g_heyebms_ops.bugfs_share = NULL;
    g_heyebms_ops.msg_capture = NULL;
    g_heyebms_ops.msg_release = NULL;

    HAWKEYE_LOG_INFO("bms inf init\n");
    return 0;

}

int libhawkeye_bms_init(void)
{
    /*================================================================*/
    /*1.init bug manger info*/
    /*================================================================*/
    memset(&g_heye_bugsrv, 0, sizeof(struct hawkeye_bug_server));
    init_timer(&g_heye_bugsrv.msg_gctimer);

    g_heye_bugsrv.msg_gctimer.function = __hawkeye_on_time;
    g_heye_bugsrv.msg_gctimer.data = (unsigned long)&g_heye_bugsrv;
    g_heye_bugsrv.msg_gctimer.expires = jiffies + HAWKEYE_MAX_GCTIMER;
    add_timer(&g_heye_bugsrv.msg_gctimer);

    /*================================================================*/
    /*3.init bug msg manger*/
    /*================================================================*/
    libhawkeye_init_msgmgr();
    libhawkeye_config_init();

    /*================================================================*/
    /*4.init bms inf & default client*/
    /*================================================================*/
    __hawkeye_bms_inf_init();
    g_heye_bugsrv.binit = 1;

    __hawkeye_bms_default_register();
    return 0;
}

int libhawkeye_bms_uninit(void)
{
    g_heye_bugsrv.binit = 0;
    del_timer(&g_heye_bugsrv.msg_gctimer);
    /*================================================================*/
    /*1.uninit bug inf manger*/
    /*================================================================*/
    __hawkeye_bms_inf_uninit();
    __hawkeye_bms_default_unregister();
    
    /*================================================================*/
    /*2.uninit bug msg manger*/
    /*================================================================*/
    libhawkeye_uninit_msgmgr();

    /*================================================================*/
    /*2.uninit client list*/
    /*================================================================*/
    
    return 0;
}

