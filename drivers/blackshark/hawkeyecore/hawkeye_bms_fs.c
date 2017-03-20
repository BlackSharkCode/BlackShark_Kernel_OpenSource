#include "hawkeye_bms.h"
#include "hawkeye_bms_fs.h"

#define HAWKEYE_ION_NAME   "HEYE_ION"
#define HAWKEYE_BUGFS_SB_MAGIC_NUMBER           (0xDEAD)
#define HAWKEYE_BUGFS_CB_MAGIC_NUMBER           (0x55AA)
#define HAWKEYE_BUGFS_PAGE_SIZE                 (512) //bug file system data area page size = 512Bytes

enum page_state{
    PAGE_USED_BIT = 0,
};

enum encrypt_config{
    ENCRYPT_NULL = 0,
    ENCRYPT_MD5,
};

struct hawkeye_bugfs_superblock{
    u16 page_size;
    u16 page_count;
    u16 page_usable_size;
    u16 sb_magic_number;
    u8  data_area[0];
};

struct hawkeye_bugfs_controlblock{
    u8  used_flag;
    u8  page_id;
    u8  next_id;
    u8  encrypt;
    u16 used_size;
    u16 cb_magic_number;
    u8  data_filed[0];
};

struct hawkeye_bugfs_ion{
    int mmsz;
    struct ion_client *iclient;
    struct ion_handle *ihandle;
};

static struct hawkeye_bugfs_superblock *sb = NULL;
static struct hawkeye_bugfs_ion sbion;
//static spinlock_t sblock = SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(sblock);
static int hawkeye_bugfs_create(void *memory_addr, int memory_size)
{
    int page_nr;
    int index;
    struct hawkeye_bugfs_controlblock *cb_ptr;

    if(!memory_addr || memory_size <= 0){
        HAWKEYE_LOG_ERR("Invalid param memaddr=%p, size=%d\n",memory_addr,memory_size);
        return -EINVAL;
    }

    /*make sure memory address 4Bytes align*/
    //memory_addr = memory_addr & 0xFFFFFFFC;

    page_nr = (memory_size - sizeof(struct hawkeye_bugfs_superblock))/HAWKEYE_BUGFS_PAGE_SIZE;
    if(page_nr){
        sb = (struct hawkeye_bugfs_superblock *)memory_addr;
        sb->page_size = HAWKEYE_BUGFS_PAGE_SIZE;
        sb->page_count = page_nr;
        sb->page_usable_size = HAWKEYE_BUGFS_PAGE_SIZE - sizeof(struct hawkeye_bugfs_controlblock);
    }else{
        HAWKEYE_LOG_ERR("memaddr=%p, size=%d no more space\n",memory_addr,memory_size);
        return -EINVAL;
    }

    for(index = 0; index < page_nr; index++){
        cb_ptr = (struct hawkeye_bugfs_controlblock *)&sb->data_area[index * HAWKEYE_BUGFS_PAGE_SIZE];
        cb_ptr->used_flag = 0;
        cb_ptr->page_id = index + 1;
        cb_ptr->next_id = 0;
        cb_ptr->encrypt = ENCRYPT_NULL;
        cb_ptr->used_size = 0;
        cb_ptr->cb_magic_number = HAWKEYE_BUGFS_CB_MAGIC_NUMBER;
    }
    sb->sb_magic_number = HAWKEYE_BUGFS_SB_MAGIC_NUMBER;
    HAWKEYE_LOG_DEBUG("memaddr=%p, size=%d, page_num=%d\n",memory_addr,memory_size,page_nr);
    return 0;
}


static inline struct hawkeye_bugfs_controlblock *find_page(int page_id)
{
    struct hawkeye_bugfs_controlblock *cb_prev_ptr = NULL;
    struct hawkeye_bugfs_controlblock *cb_next_ptr = NULL;
    u8 *data_area = NULL;
    int index = 0;
    unsigned long flgs = 0;

    if(page_id != 0){//page id >0 - find old page to write, =0 - malloc new page to write
        do{
            cb_prev_ptr = (struct hawkeye_bugfs_controlblock *)&sb->data_area[(page_id - 1) * HAWKEYE_BUGFS_PAGE_SIZE];
            page_id = cb_prev_ptr->next_id;
            if(index++ == sb->page_count){//avoid dead loop
                HAWKEYE_LOG_ERR("dead search page id %d \n",page_id);
                ASSERT(1);
                return NULL;
            }
        }while(page_id);//find the last page

        if(cb_prev_ptr->used_size < sb->page_usable_size){//still have space to write
            HAWKEYE_LOG_DEBUG("page id %d  used_size[%d] < page_usable_size[%d]\n",page_id,cb_prev_ptr->used_size,sb->page_usable_size);
            return cb_prev_ptr;
        }
    }

    /*malloc a new page*/
    data_area = &sb->data_area[0];
    spin_lock_irqsave(&sblock, flgs);
    for(index = 0; index < sb->page_count; index++, data_area += HAWKEYE_BUGFS_PAGE_SIZE){//alloc new page to write
        cb_next_ptr = (struct hawkeye_bugfs_controlblock *)data_area;
        if(!(cb_next_ptr->used_flag & 1<<PAGE_USED_BIT)){
            cb_next_ptr->used_flag |= 1<<PAGE_USED_BIT;
            break;
        }
    }
    spin_unlock_irqrestore(&sblock, flgs);

    if(index == sb->page_count){//there no page to alloc
        HAWKEYE_LOG_ERR("no page to alloc, count=%d\n",sb->page_count);
        cb_next_ptr = NULL;
    }else if(cb_prev_ptr){//link new page to prev page if needed
        cb_prev_ptr->next_id = cb_next_ptr->page_id;
    }

    return cb_next_ptr;
}

int libhawkeye_bugfs_write(int *page_id, const char *buff, int size)
{
    struct hawkeye_bugfs_controlblock *cb;
    int first_page_id = 0;
    int write_size = 0;
    int total_write_size = 0;

    if(!buff || !size){
        HAWKEYE_LOG_ERR("Invalid param buff=%p, size=%d\n",buff,size);
        return -EINVAL;
    }

    if(page_id)
        first_page_id = *page_id;

    if(first_page_id < 0 || first_page_id > sb->page_count){
        HAWKEYE_LOG_ERR("first_page_id= %d invalid\n",first_page_id);
        return -EINVAL;
    }

    do{
        /*get a free page to use*/
        cb = find_page(first_page_id);
        if(!cb){
            HAWKEYE_LOG_ERR("first_page_id= %d find failed\n",first_page_id);
            return total_write_size;
        }
        if(!first_page_id)
            *page_id = first_page_id = cb->page_id;
        /*calculate actual write size*/
        write_size = sb->page_usable_size - cb->used_size;
        if(size < write_size)
            write_size = size;

        /*write data filed*/
        memcpy(&cb->data_filed[cb->used_size], buff + total_write_size, write_size);

        /*update control block*/
        cb->used_size += write_size;
        total_write_size += write_size;
        size -= write_size;

    }while(size);//continue to write if not complete
    HAWKEYE_LOG_DEBUG("page_id=%d write %s size=%d\n",first_page_id,buff,total_write_size);
    return total_write_size;
}

int libhawkeye_bugfs_read(int page_id, char *buff, int size, int offset)
{
    int index = 0;
    int seek_flag = 1;
    int read_size = 0;
    int read_pos = 0;
    int total_read_size = 0;
    struct hawkeye_bugfs_controlblock *cb_ptr = NULL;

    if(!buff || !size){
        HAWKEYE_LOG_ERR("Invalid param buff=%p, size=%d\n",buff,size);
        return -EINVAL;
    }

    if(page_id <= 0 || page_id > sb->page_count){
        HAWKEYE_LOG_ERR("Invalid page_id= %d\n",page_id);
        return -EINVAL;
    }

    do{
        cb_ptr = (struct hawkeye_bugfs_controlblock *)&sb->data_area[(page_id - 1) * HAWKEYE_BUGFS_PAGE_SIZE];
        page_id = cb_ptr->next_id;
        read_size = cb_ptr->used_size;
        if(seek_flag){//seek the 1st read page
            offset -= read_size;
            if(offset >= 0)//not this page continue
                continue;
            seek_flag = 0;
            offset += read_size;
            read_size -= offset;//convert actual read size != page used size
        }
        read_pos = cb_ptr->used_size - read_size;

        if(size < read_size){
            read_size = size;
        }

        memcpy(buff + total_read_size, &cb_ptr->data_filed[read_pos], read_size);
        total_read_size += read_size;
        size -= read_size;

        if(index++ == sb->page_count){//avoid dead loop
            HAWKEYE_LOG_ERR("dead search page id %d \n",page_id);
            ASSERT(1);
            break;
        }
    }while(page_id && (size > 0));//find the last page
    HAWKEYE_LOG_DEBUG("page_id=%d size=%d\n",page_id,size);
    return total_read_size;
}

int libhawkeye_bugfs_gc(int page_id)
{
    int index = 0;
    struct hawkeye_bugfs_controlblock *cb_ptr = NULL;

    if(page_id <= 0 || page_id > sb->page_count){
        HAWKEYE_LOG_ERR("Invalid page_id= %d\n",page_id);
        return -EINVAL;
    }

    do{
        cb_ptr = (struct hawkeye_bugfs_controlblock *)&sb->data_area[(page_id - 1) * HAWKEYE_BUGFS_PAGE_SIZE];
        page_id = cb_ptr->next_id;
        /*file system garbage collection*/
        cb_ptr->next_id = 0;
        cb_ptr->encrypt = ENCRYPT_NULL;
        cb_ptr->used_size = 0;
        cb_ptr->used_flag = 0;
        if(index++ == sb->page_count){//avoid dead loop
            HAWKEYE_LOG_ERR("dead search page id %d \n",page_id);
            ASSERT(1);
            break;
        }
    }while(page_id);//find the last page

    return 0;
}

int libhawkeye_bugfs_get_file_size(int page_id)
{
    int index = 0;
    int total_file_size = 0;
    struct hawkeye_bugfs_controlblock *cb_ptr = NULL;

    if(page_id <= 0 || page_id > sb->page_count){
        HAWKEYE_LOG_ERR("Invalid page_id= %d\n",page_id);
        return -EINVAL;
    }

    do{
        cb_ptr = (struct hawkeye_bugfs_controlblock *)&sb->data_area[(page_id - 1) * HAWKEYE_BUGFS_PAGE_SIZE];
        page_id = cb_ptr->next_id;
        /*file system garbage collection*/
        total_file_size += cb_ptr->used_size;
        if(index++ == sb->page_count){//avoid dead loop
            HAWKEYE_LOG_ERR("dead search page id %d \n",page_id);
            ASSERT(1);
            break;
        }
    }while(page_id);//find the last page

    return total_file_size;
}

int libhawkeye_bugfs_init(int bugfs_size)
{
    void *paddr = NULL;
    struct ion_client *iclient = NULL;
    struct ion_handle *ihandle = NULL;

    iclient = msm_ion_client_create(HAWKEYE_ION_NAME);
    if(IS_ERR_OR_NULL(iclient)){
        HAWKEYE_LOG_ERR("msm_ion_client_create failed\n");
        return -ENOMEM;
    }
    ihandle = ion_alloc(iclient,bugfs_size,SZ_4K,ION_HEAP(ION_SYSTEM_HEAP_ID), 0);
    if (IS_ERR_OR_NULL(ihandle)){
        HAWKEYE_LOG_ERR("ion_alloc failed\n");
        return -ENOMEM;
    }
    paddr = ion_map_kernel(iclient,ihandle);
    if (IS_ERR_OR_NULL(paddr)) {
        HAWKEYE_LOG_ERR("ion_map_kernel failed\n");
        return -EINVAL;
    }

    HAWKEYE_LOG_INFO("ic=%p,ih=%p,addr=%p,len=%d\n",iclient,ihandle,paddr,bugfs_size);
    sbion.iclient = iclient;
    sbion.ihandle = ihandle;
    sbion.mmsz = bugfs_size;
    return hawkeye_bugfs_create(paddr, bugfs_size);
}

int libhawkeye_bugfs_monitor(void)
{
    return 0;
}

void libhawkeye_bugfs_uninit()
{
    struct ion_client *iclient = NULL;
    struct ion_handle *ihandle = NULL;

    if(!sbion.iclient || !sbion.ihandle){
        HAWKEYE_LOG_ERR("iclient failed\n");
        return;
    }

    iclient = sbion.iclient;
    ihandle = sbion.ihandle;

    sbion.iclient = NULL;
    sbion.ihandle = NULL;

    ion_unmap_kernel(iclient,ihandle);
    ion_free(iclient,ihandle);
    ion_client_destroy(iclient);
    return ;
}

int libhawkeye_bugfs_share(int *psz)
{
    int fd = -1;
    struct ion_client *iclient = NULL;
    struct ion_handle *ihandle = NULL;

    if(!sbion.iclient || !sbion.ihandle){
        HAWKEYE_LOG_ERR("iclient failed\n");
        return -EINVAL;
    }
    iclient = sbion.iclient;
    ihandle = sbion.ihandle;

    fd = ion_share_dma_buf_fd(iclient, ihandle);
    if(fd < 0)
    {
        printk(KERN_ERR "heye_ioctl ion_share_dma_buf_fd failed\n");
        return -EINVAL;
    }

    if(psz){
       *psz = sbion.mmsz;
    }

    return fd;
}


