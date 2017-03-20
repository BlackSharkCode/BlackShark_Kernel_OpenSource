#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/atomic.h>
#include <linux/leds.h>
#include <asm/uaccess.h>
#include <linux/hw_dev_dec.h>

//for color control
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/pid.h>

#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/jiffies.h>

#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/time.h>
// end for color control

//////////////////////////////////////
// Marco
//////////////////////////////////////
#define DEBUG

#define AW9120_LED_NAME		"aw9120_led"

#define     RSTR       	 0x00
#define     GCR      	 0x01

#define     LER1   	     0x50
#define     LER2   	     0x51
#define     LCR   	     0x52
#define     PMR 	     0x53
#define     RMR  	     0x54
#define     CTRS1  	     0x55
#define     CTRS2  	     0x56
#define     IMAX1    	 0x57
#define     IMAX2    	 0x58
#define     IMAX3    	 0x59
#define     IMAX4    	 0x5a
#define     IMAX5    	 0x5b
#define     TIER     	 0x5c
#define     INTVEC   	 0x5d
#define     LISR2     	 0x5e
#define     SADDR	     0x5f

#define     PCR      	 0x60
#define     CMDR     	 0x61
#define     RA       	 0x62
#define     RB       	 0x63
#define     RC       	 0x64
#define     RD       	 0x65
#define     R1       	 0x66
#define     R2       	 0x67
#define     R3       	 0x68
#define     R4       	 0x69
#define     R5       	 0x6a
#define     R6       	 0x6b
#define     R7       	 0x6c
#define     R8       	 0x6d
#define     GRPMSK1  	 0x6e
#define     GRPMSK2  	 0x6f

#define     TCR      	 0x70
#define     TAR      	 0x71
#define     TDR      	 0x72
#define     TDATA      	 0x73
#define     TANA     	 0x74
#define     TKST     	 0x75
#define     FEXT   	     0x76
#define     WP       	 0x7d
#define     WADDR    	 0x7e
#define     WDATA    	 0x7f

#define     DIMMING_LOGE    "loge"
#define     DIMMING_LOG10   "log10"
#define     DIMMING_LINEAR  "linear"
#define     PWM_FREQ_245HZ  245
#define     PWM_FREQ_122HZ  122

#define     EACH_MODE_NAME_MAX_LEN  32
#define     EACH_MODE_BUF_LEN   (256 * 7 + EACH_MODE_NAME_MAX_LEN)
#define     EACH_ASM_CODE_LEN   6
#define     ROM_CODE_MAX 255

#define     INITCMD 		0x00
#define     CLOSECMD 		0xbf00
#define     IMAXCMD35 		0x1111
#define     IMAX2CMD35 		0x1110
#define     IMAXCMD70 		0x2222
#define     IMAX2CMD70 		0x2220
#define     IMAXCMD105 		0x3333
#define     IMAX2CMD105 	0x3330
#define     IMAXCMD140 		0x4444
#define     IMAX2CMD140 	0x4440
#define     IMAXCMD175		0x5555
#define     IMAX2CMD175		0x5550
#define		REGMSK1			0xac00
#define		REGMSK2			0xa000
#define		REGMSK3			0xaf00
#define		REGMSK4			0xa600
#define		REGMSK5			0xaa00
#define		GREENMSK1		0xad00
#define		GREENMSK2		0xa100
#define		GREENMSK3		0xa400
#define		GREENMSK4		0xa700
#define		GREENMSK5		0xab00
#define		BLUEMSK1		0xae00
#define		BLUEMSK2		0xa200
#define		BLUEMSK3		0xa500
#define		BLUEMSK4		0xa300
#define		BLUEMSK5		0xa900


#ifdef pr_debug
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG"[%s:%d]"fmt"",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

//////////////////////////////////////
// client
//////////////////////////////////////
enum max_current_type{
    max_current_0_0ma = 0,
    max_current_3_5ma = 1,
    max_current_7_0ma = 2,
    max_current_10_5ma = 3,
    max_current_14_0ma = 4,
    max_current_17_5ma = 5,
    max_current_21_0ma = 6,
    max_current_24_5ma = 7,
    max_type
};

struct led_mode_list {
    struct list_head list;
    char *led_mode_name;
    int TWO_BYTES_mode_code_count;
    unsigned int *mode_code;
};

struct aw9120_platform_data {
    struct led_classdev cdev;
	struct work_struct brightness_work;
	enum max_current_type max_current;
 	int id;
	struct aw9120_led *led;
};

struct aw9120_led{
	struct i2c_client *i2c_client;
    int pwm_freq;
    const char	*control_mode;
    struct aw9120_platform_data *led_array;
	struct mutex lock;
	int num_leds;
    struct pinctrl *aw9120ctrl;
    struct pinctrl_state *aw9120_pdn_high;
    struct pinctrl_state *aw9120_pdn_low;
    struct led_mode_list led_mode_list_head;
    int led_work_mode_count;
};

struct aw9120_led *led_light_dev;

int led_code_len = 7;
int led_breath_code[ROM_CODE_MAX] = {
	0xbf00,0x9f05,0xfffa,0x3c7d,0xdffa,0x3cbb,0x2,
};
int led_all_on_code[ROM_CODE_MAX] = {
    0xbf00,0xbfff,0x3400,
};
int led_all_off_code[ROM_CODE_MAX] = {
    0xbf00,0x3400,
};
int led_mmitest_code[ROM_CODE_MAX] = {
	0xbf00,0xacff,0xa0ff,0xafff,0xa6ff,0xaaff,0x3c50,0xac00,0xa000,0xaf00,0xa600,0xaa00,0xadff,0xa1ff,0xa4ff,0xa7ff,
	0xabff,0x3c50,0xad00,0xa100,0xa400,0xa700,0xab00,0xaeff,0xa2ff,0xa5ff,0xa3ff,0xa9ff,0x3c50,0xae00,0xa200,0xa500,
	0xa300,0xa900,0x3400,
};
int led_agingtest_code[ROM_CODE_MAX] = {
	0xbf00,0x48fb,0xacff,0xa0ff,0xafff,0xa6ff,0xaaff,0x3c40,0xac00,0xa000,0xaf00,0xa600,0xaa00,0xadff,0xa1ff,0xa4ff,
	0xa7ff,0xabff,0x3c40,0xad00,0xa100,0xa400,0xa700,0xab00,0xaeff,0xa2ff,0xa5ff,0xa3ff,0xa9ff,0x3c40,0xae00,0xa200,
	0xa500,0xa300,0xa900,0xacff,0xa0ff,0xafff,0xa6ff,0xaaff,0xadff,0xa1ff,0xa4ff,0xa7ff,0xabff,0x3c40,0xad00,0xa100,
	0xa400,0xa700,0xab00,0xaeff,0xa2ff,0xa5ff,0xa3ff,0xa9ff,0x3c40,0xac00,0xa000,0xaf00,0xa600,0xaa00,0xadff,0xa1ff,
	0xa4ff,0xa7ff,0xabff,0x3c40,0xbfff,0x3c40,0xbf00,0x3400,
};

static int table_cali[15][256];

//for color control

#define X_HALF_VALUE_LEN 361

#define X_INT_VALUE_LEN 22

#define Y_INT_VALUE_LEN 22

#define EASE_IN_X_INT_VALUE_LEN 28

#define EASE_IN_Y_INT_VALUE_LEN 28

#define EASE_OUT_X_INT_VALUE_LEN 28

#define EASE_OUT_Y_INT_VALUE_LEN 28


static int x_half_value[] = {0,166,333,500,666,833,1000,1166,1333,1499,1666,1833,1999,2166,2333,2500,2666,2833,3000,3166,3333,3500,3666,3833,3999,4166,4333,4500,4666,4833,5000,5166,5333,5500,5666,5833,6000,6166,6333,6500,6666,6833,7000,7166,7333,7500,7666,7833,8000,8166,8333,8500,8666,8833,9000,9166,9333,9500,9666,9833,10000,9833,9666,9500,9333,9166,9000,8833,8666,8500,8333,8166,7999,7833,7666,7500,7333,7166,7000,6833,6666,6500,6333,6166,6000,5833,5666,5499,5333,5166,5000,4833,4666,4500,4333,4166,3999,3833,3666,3500,3333,3166,2999,2833,2666,2500,2333,2166,2000,1833,1666,1499,1333,1166,1000,833,666,499,333,166,0,166,333,499,666,833,999,1166,1333,1500,1666,1833,2000,2166,2333,2500,2666,2833,2999,3166,3333,3499,3666,3833,4000,4166,4333,4500,4666,4833,5000,5166,5333,5499,5666,5833,5999,6166,6333,6500,6666,6833,7000,7166,7333,7500,7666,7833,7999,8166,8333,8499,8666,8833,9000,9166,9333,9500,9666,9833,10000,9833,9666,9500,9333,9166,9000,8833,8666,8499,8333,8166,7999,7833,7666,7500,7333,7166,7000,6833,6666,6500,6333,6166,5999,5833,5666,5499,5333,5166,5000,4833,4666,4500,4333,4166,4000,3833,3666,3499,3333,3166,2999,2833,2666,2500,2333,2166,2000,1833,1666,1500,1333,1166,999,833,666,499,333,166,0,166,333,500,666,833,999,1166,1333,1500,1666,1833,1999,2166,2333,2500,2666,2833,3000,3166,3333,3499,3666,3833,4000,4166,4333,4499,4666,4833,5000,5166,5333,5500,5666,5833,5999,6166,6333,6500,6666,6833,6999,7166,7333,7500,7666,7833,8000,8166,8333,8499,8666,8833,9000,9166,9333,9499,9666,9833,10000,9833,9666,9499,9333,9166,9000,8833,8666,8499,8333,8166,8000,7833,7666,7500,7333,7166,6999,6833,6666,6500,6333,6166,5999,5833,5666,5500,5333,5166,5000,4833,4666,4499,4333,4166,4000,3833,3666,3499,3333,3166,3000,2833,2666,2500,2333,2166,1999,1833,1666,1500,1333,1166,999,833,666,500,333,166,0};
/*
//ease
static int x_int[] = {0,22735,44189,83984,120849,156250,191650,228515,268310,312500,362548,419921,486083,562500,605011,650634,699554,751953,808013,867919,931854,1000000};

static int y_int[] = {0,11666,27709,71679,129418,198437,276245,360351,448266,537500,625561,709960,788208,857812,888595,916284,940567,961132,977670,989868,997415,1000000};

//ease-in
static int ease_in_x_int[] = {0,39821,80444,121734,163554,205770,248247,290847,333437,375880,418042,500976,581157,657500,728920,794335,824452,852661,878826,902812,924484,943706,960342,974257,985316,993383,998323,1000000};

static int ease_in_y_int[] = {0,2868,11230,24719,42968,65612,92285,122619,156250,192810,231933,316406,406738,500000,593261,683593,726745,768066,807189,843750,877380,907714,934387,957031,975280,988769,997131,1000000};

//ease-out
static int ease_out_x_int[] = {0,1676,6616,14683,25742,39657,56293,75515,97187,121173,147338,175547,205664,271079,342500,418842,499023,581958,624119,666562,709152,751752,794229,836445,878266,919555,960178,1000000};

static int ease_out_y_int[] = {0,2868,11230,24719,42968,65612,92285,122619,156250,192810,231933,273254,316406,406738,500000,593261,683593,768066,807189,843750,877380,907714,934387,957031,975280,988769,997131,1000000};
*/
#define PERIOD (10 * HZ / 1000) // 10ms, HZ = 100,




static struct task_struct *task_time;

static long time_count = 0;

#define  LIGHT_SCENE_OFF 0x0000000

#define  LIGHT_SCENE_RING 0x1000000
#define  LIGHT_SCENE_RING_MODE_ONE 0x1000001
#define  PERIOD_SCENE_RING_MODE_ONE 60*1000//1200

#define  LIGHT_SCENE_NOTIFICATION 0x2000000
#define  LIGHT_SCENE_NOTIFICATION_MODE_ONE 0x2000001
#define  PERIOD_SCENE_NOTIFICATION_MODE_ONE 2330   // 3000 / 2500

#define  LIGHT_SCENE_BOOT 0x3000000
#define  LIGHT_SCENE_BOOT_MODE_ONE 0x3000001
#define  PERIOD_SCENE_BOOT_MODE_ONE  10700  //300 + 100 + 10000 +300

#define  LIGHT_SCENE_GAME 0x4000000
#define  LIGHT_SCENE_GAME_MODE_LOADING 0x4000001
#define  PERIOD_SCENE_GAME_MODE_LOADING 2000
#define  PERIOD_SCENE_GAME_MODE_LOADING_STOP 2*60*1000

#define  LIGHT_SCENE_GAME_MODE_NORMAL 0x4000002
#define  PERIOD_SCENE_GAME_MODE_NORMAL 2000

#define  LIGHT_SCENE_GAME_MODE_SPECIAL 0x4000003
#define  PERIOD_SCENE_GAME_MODE_SPECIAL 1700

#define  LIGHT_SCENE_GAME_MODE_VICTOR 0x4000004
#define  PERIOD_SCENE_GAME_MODE_VICTOR 960


#define  LED_LIGHT_NUM 15

unsigned short led_light_param[LED_LIGHT_NUM];

struct light_hsv_param{
    int id;
    int hue;
    int saturation;
    int value;
};

struct light_mode_param{
    struct light_hsv_param light_hsv_param_logo;
    struct light_hsv_param light_hsv_param_left_up;
    struct light_hsv_param light_hsv_param_left_down;
    struct light_hsv_param light_hsv_param_right_up;
    struct light_hsv_param light_hsv_param_right_down;
};

struct light_mode_param *light_mode_param_dev;

static int light_mode_data = LIGHT_SCENE_OFF;

static int music_volume = 0;
static int light_mode_debug = 0;
static int light_mode_debug_h = 0;
static int light_mode_debug_s = 0;
static int light_mode_debug_v = 0;
static int light_mode_debug_fresh = 0;
static int light_mode_debug_r_g_b = 0;

static int display_r = 0;
static int display_g = 0;
static int display_b = 0;
static int display_peroid = 500;
static int display_mode_flag = 1;//0:in setting ;1 in normal mode
static int display_mode_scene = 0xde00;

#define    DISPLAY_MODE_FLAG_SETTING    0
#define    DISPLAY_MODE_FLAG_NORMAL    1

#define    LOGO_MODE_RED    0x0A
#define    LOGO_MODE_GREEN    0x0B
#define    LOGO_MODE_BLUE    0x09

#define    CORNER1_MODE_RED    0x0C
#define    CORNER1_MODE_GREEN    0x0D
#define    CORNER1_MODE_BLUE    0x0E

#define    CORNER2_MODE_RED    0x00
#define    CORNER2_MODE_GREEN    0x01
#define    CORNER2_MODE_BLUE    0x02

#define    CORNER3_MODE_RED    0x0F
#define    CORNER3_MODE_GREEN    0x04
#define    CORNER3_MODE_BLUE    0x05

#define    CORNER4_MODE_RED    0x06
#define    CORNER4_MODE_GREEN    0x07
#define    CORNER4_MODE_BLUE    0x03

#define    ALL_MODE_LED    0x1F

#define LIGHT_INDEX_LOGO   5
#define LIGHT_INDEX_LEFT_UP   1
#define LIGHT_INDEX_LEFT_DOWN   4
#define LIGHT_INDEX_RIGHT_UP  2
#define LIGHT_INDEX_RIGHT_DOWN  3
//end for color control

static unsigned int fw_update_flag = 0;

//////////////////////////////////////////////////////////////////////////////////////////
// string operation fun
//////////////////////////////////////////////////////////////////////////////////////////
static int get_char_count(unsigned char ch, unsigned char* dest_buf, unsigned int dest_buf_max_len)
{
    int i = 0, count = 0;

    if(NULL == dest_buf)
    {
        pr_err("para is NULL\n");
    }
    for(i = 0; i < dest_buf_max_len; i++)
    {
        if(ch == *dest_buf)
        {
            count++;
        }
        if('\0' ==  *dest_buf)
        {
            return -EINVAL;
        }
        dest_buf++;
    }

    return count;
}

// for color control
//////////////////////////////////////////////////////////////////////////////////////////

unsigned int i2c_write_reg_off(struct aw9120_led *dev )
{
    int ret = 0;
    u8 wdbuf[3] = {0};
    unsigned int  reg_data = CLOSECMD;//trun all light off
    unsigned char addr = CMDR;

    struct i2c_msg msgs[] = {
        {
            .addr    = dev->i2c_client->addr,
            .flags   = 0,
            .len     = 3,
            .buf     = wdbuf,
        },
    };
    if (light_mode_debug)
    {
        printk("bslight_tag i2c_write_reg_off begin \n");
    }

    if (NULL == dev->i2c_client) {
        pr_err("msg aw9120_i2c_client is NULL\n");
        return -1;
    }

    wdbuf[0] = addr;
    wdbuf[2] = (unsigned char)(reg_data & 0x00ff);
    wdbuf[1] = (unsigned char)((reg_data & 0xff00)>>8);

    ret = i2c_transfer(dev->i2c_client->adapter, msgs, 1);
    if (ret < 0)
        pr_err("bslight_tag msg i2c read error: %d\n", ret);

    if (light_mode_debug)
    {
        printk("bslight_tag i2c_write_reg_off end\n");
    }
    return ret;
}

// i2c write and write
//////////////////////////////////////////////////////////////////////////////////////////
unsigned int i2c_write_reg_for_light(struct aw9120_led *dev, unsigned char addr,const unsigned short *reg_data)
{
    int ret=0;
    u8 wdbuf[45] = {0};

    struct i2c_msg msgs[] = {
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +3 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    =wdbuf +6 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +9 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +12 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +15 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +18 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +21 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +24 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +27 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +30 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +33 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +36 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +39 ,
        },
        {
            .addr    = dev->i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf +42 ,
        },
    };

    if (light_mode_debug)
    {
        printk("bslight_tag i2c_write_reg_for_light begin \n");
    }

    if(NULL == dev->i2c_client) {
        pr_err("bslight_tag msg aw9120_i2c_client is NULL\n");
        return -1;
    }

    wdbuf[0] = addr;

    wdbuf[2] = (unsigned char)(reg_data[0] & 0x00ff);
    wdbuf[1] = (unsigned char)((reg_data[0] & 0xff00)>>8);

    wdbuf[3] = addr;
    wdbuf[5] = (unsigned char)(reg_data[1] & 0x00ff);
    wdbuf[4] = (unsigned char)((reg_data[1] & 0xff00)>>8);

    wdbuf[6] = addr;
    wdbuf[8] = (unsigned char)(reg_data[2] & 0x00ff);
    wdbuf[7] = (unsigned char)((reg_data[2] & 0xff00)>>8);

    wdbuf[9] = addr;
    wdbuf[11] = (unsigned char)(reg_data[3] & 0x00ff);
    wdbuf[10] = (unsigned char)((reg_data[3] & 0xff00)>>8);

    wdbuf[12] = addr;
    wdbuf[14] = (unsigned char)(reg_data[4] & 0x00ff);
    wdbuf[13] = (unsigned char)((reg_data[4] & 0xff00)>>8);

    wdbuf[15] = addr;
    wdbuf[17] = (unsigned char)(reg_data[5] & 0x00ff);
    wdbuf[16] = (unsigned char)((reg_data[5] & 0xff00)>>8);

    wdbuf[18] = addr;
    wdbuf[20] = (unsigned char)(reg_data[6] & 0x00ff);
    wdbuf[19] = (unsigned char)((reg_data[6] & 0xff00)>>8);

    wdbuf[21] = addr;
    wdbuf[23] = (unsigned char)(reg_data[7] & 0x00ff);
    wdbuf[22] = (unsigned char)((reg_data[7] & 0xff00)>>8);

    wdbuf[24] = addr;
    wdbuf[26] = (unsigned char)(reg_data[8] & 0x00ff);
    wdbuf[25] = (unsigned char)((reg_data[8] & 0xff00)>>8);

    wdbuf[27] = addr;
    wdbuf[29] = (unsigned char)(reg_data[9] & 0x00ff);
    wdbuf[28] = (unsigned char)((reg_data[9] & 0xff00)>>8);

    wdbuf[30] = addr;
    wdbuf[32] = (unsigned char)(reg_data[10] & 0x00ff);
    wdbuf[31] = (unsigned char)((reg_data[10] & 0xff00)>>8);

    wdbuf[33] = addr;
    wdbuf[35] = (unsigned char)(reg_data[11] & 0x00ff);
    wdbuf[34] = (unsigned char)((reg_data[11] & 0xff00)>>8);

    wdbuf[36] = addr;
    wdbuf[38] = (unsigned char)(reg_data[12] & 0x00ff);
    wdbuf[37] = (unsigned char)((reg_data[12] & 0xff00)>>8);

    wdbuf[39] = addr;
    wdbuf[41] = (unsigned char)(reg_data[13] & 0x00ff);
    wdbuf[40] = (unsigned char)((reg_data[13] & 0xff00)>>8);

    wdbuf[42] = addr;
    wdbuf[44] = (unsigned char)(reg_data[14] & 0x00ff);
    wdbuf[43] = (unsigned char)((reg_data[14] & 0xff00)>>8);

    if(light_mode_data != LIGHT_SCENE_OFF)
    {
        ret = i2c_transfer(dev->i2c_client->adapter, msgs, 15);
    }

    if (ret < 0)
        pr_err("bslight_tag msg i2c read error: %d\n", ret);

    if(task_time && light_mode_data == LIGHT_SCENE_OFF){
        printk("bslight_tag i2c_write_reg_for_light light_mode_data =%x \n",light_mode_data);
        i2c_write_reg_off(led_light_dev);
    }

    if (light_mode_debug)
    {
        printk("bslight_tag i2c_write_reg_for_light end \n");
    }
    return ret;
}
//end for color control

//////////////////////////////////////////////////////////////////////////////////////////
// i2c write and read
//////////////////////////////////////////////////////////////////////////////////////////
unsigned int i2c_write_reg(struct aw9120_led *dev, unsigned char addr, unsigned int reg_data)
{
	int ret;
	u8 wdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= dev->i2c_client->addr,
			.flags	= 0,
			.len	= 3,
			.buf	= wdbuf,
		},
	};

	if(NULL == dev->i2c_client) {
		pr_err("msg aw9120_i2c_client is NULL\n");
		return -1;
	}

	wdbuf[0] = addr;
	wdbuf[2] = (unsigned char)(reg_data & 0x00ff);
	wdbuf[1] = (unsigned char)((reg_data & 0xff00)>>8);

	ret = i2c_transfer(dev->i2c_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg i2c read error: %d\n", ret);

	return ret;
}


unsigned int i2c_read_reg(struct aw9120_led *dev, unsigned char addr)
{
	int ret;
	u8 rdbuf[512] = {0};
	unsigned int getdata;

	struct i2c_msg msgs[] = {
		{
			.addr	= dev->i2c_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.addr	= dev->i2c_client->addr,
			.flags	= I2C_M_RD,
			.len	= 2,
			.buf	= rdbuf,
		},
	};

	if(NULL == dev->i2c_client) {
		pr_err("msg aw9120_i2c_client is NULL\n");
		return -1;
	}

	rdbuf[0] = addr;

	ret = i2c_transfer(dev->i2c_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg i2c read error: %d\n", ret);

	getdata=rdbuf[0] & 0x00ff;
	getdata<<= 8;
	getdata |=rdbuf[1];

	return getdata;
}

//for color control

int thr_fn_time_stop(void)
{
    printk("bslight_tag thr_fn_time_stop begin \n");

    i2c_write_reg_off(led_light_dev);

    light_mode_data = LIGHT_SCENE_OFF;

    time_count = 0;
    display_r = 0;
    display_g = 0;
    display_b = 0;

    display_mode_scene = 0xde00;

    printk("bslight_tag thr_fn_time_stop \n");

    if(task_time)
    {
        kthread_stop(task_time);
        task_time = NULL;
    }

    printk("bslight_tag thr_fn_time_stop end \n");

   return 0;
}
/*
static int get_interpolation_value(int time,int first_value,int last_value) {

    int start_index = 0;
    int end_index = X_INT_VALUE_LEN - 1;
    int mid_index = 0;
    int x_range = 0;
    int tInRange = 0;
    int  fraction = 0;
    int start_y = 0;
    int end_y = 0;
    int result =0;

    time = time * 1000;//time is ms

    while (end_index - start_index > 1){

         mid_index = (start_index + end_index) / 2;
         if (time < x_int[mid_index]){
              end_index = mid_index;
         } else {
              start_index = mid_index;
         }
    }

    x_range = x_int[end_index] - x_int[start_index];

    if (x_range == 0) {
        return y_int[start_index];
    }

    tInRange = time - x_int[start_index];
    fraction = tInRange * 10000 / x_range;

    start_y = y_int[start_index];
    end_y = y_int[end_index];
    if (light_mode_debug)
    {
        printk("bslight_tag fraction = %d, tInRange = %d, x_range = %d, start_index = %d, end_index = %d,\n", fraction,tInRange,x_range,start_index,end_index);
    }
    fraction = start_y + (fraction * (end_y - start_y))/10000;

    result = first_value +  ((fraction  * (last_value - first_value)))/1000000;
    if (light_mode_debug)
    {
         printk("bslight_tag time = %d, result = %d, end_y = %d, start_y = %d, fraction = %d, first_value = %d\n", time,result,end_y,start_y,fraction,first_value);
    }
    return result;
}

static int get_ease_in_interpolation_value(int time,int first_value,int last_value) {

    int start_index = 0;
    int end_index = EASE_IN_X_INT_VALUE_LEN - 1;
    int mid_index = 0;
    int x_range = 0;
    int tInRange = 0;
    int  fraction = 0;
    int start_y = 0;
    int end_y = 0;
    int result =0;

    time = time * 1000;//time is ms

    while (end_index - start_index > 1){

         mid_index = (start_index + end_index) / 2;
         if (time < ease_in_x_int[mid_index]){
              end_index = mid_index;
         } else {
              start_index = mid_index;
         }
    }

    x_range = ease_in_x_int[end_index] - ease_in_x_int[start_index];

    if (x_range == 0) {
        return ease_in_y_int[start_index];
    }

    tInRange = time - ease_in_x_int[start_index];
    fraction = tInRange * 10000 / x_range;

    start_y = ease_in_y_int[start_index];
    end_y = ease_in_y_int[end_index];
    if (light_mode_debug)
    {
        printk("bslight_tag fraction = %d, tInRange = %d, x_range = %d, start_index = %d, end_index = %d,\n", fraction,tInRange,x_range,start_index,end_index);
    }
    fraction = start_y + (fraction * (end_y - start_y))/10000;

    result = first_value +  ((fraction  * (last_value - first_value)))/1000000;
    if (light_mode_debug)
    {
         printk("bslight_tag time = %d, result = %d, end_y = %d, start_y = %d, fraction = %d, first_value = %d\n", time,result,end_y,start_y,fraction,first_value);
    }
    return result;
}

static int get_ease_out_interpolation_value(int time,int first_value,int last_value) {

    int start_index = 0;
    int end_index = EASE_OUT_X_INT_VALUE_LEN - 1;
    int mid_index = 0;
    int x_range = 0;
    int tInRange = 0;
    int  fraction = 0;
    int start_y = 0;
    int end_y = 0;
    int result =0;

    time = time * 1000;//time is ms

    while (end_index - start_index > 1){

         mid_index = (start_index + end_index) / 2;
         if (time < ease_out_x_int[mid_index]){
              end_index = mid_index;
         } else {
              start_index = mid_index;
         }
    }

    x_range = ease_out_x_int[end_index] - ease_out_x_int[start_index];

    if (x_range == 0) {
        return ease_out_y_int[start_index];
    }

    tInRange = time - ease_out_x_int[start_index];
    fraction = tInRange * 10000 / x_range;

    start_y = ease_out_y_int[start_index];
    end_y = ease_out_y_int[end_index];
    if (light_mode_debug)
    {
        printk("bslight_tag fraction = %d, tInRange = %d, x_range = %d, start_index = %d, end_index = %d,\n", fraction,tInRange,x_range,start_index,end_index);
    }
    fraction = start_y + (fraction * (end_y - start_y))/10000;

    result = first_value +  ((fraction  * (last_value - first_value)))/1000000;
    if (light_mode_debug)
    {
         printk("bslight_tag time = %d, result = %d, end_y = %d, start_y = %d, fraction = %d, first_value = %d\n", time,result,end_y,start_y,fraction,first_value);
    }
    return result;
}
*/
unsigned short set_pwmi(unsigned short ch, unsigned char im){
    return 0xA000|(ch<<8)|im;
}

int mcu_light_control(const short *data)
{
    struct aw9120_led *led_data = NULL;
    led_data = led_light_dev;

    i2c_write_reg_for_light(led_data, CMDR,data);

    return 0;
}

int hsv2rgb(struct light_mode_param *mode_param) {

    int h = 0;
    int s = 0;
    int v = 0;
    int c = 0;
    int x = 0;
    int m = 0;
    int r = 0, g = 0, b = 0;
    int index = 1;

    while (index <= 5){
        if(index == mode_param->light_hsv_param_logo.id){
            h = mode_param->light_hsv_param_logo.hue;
            s = mode_param->light_hsv_param_logo.saturation;
            v = mode_param->light_hsv_param_logo.value;
        } else if(index == mode_param->light_hsv_param_left_up.id){
            h = mode_param->light_hsv_param_left_up.hue;
            s = mode_param->light_hsv_param_left_up.saturation;
            v = mode_param->light_hsv_param_left_up.value;
        } else if(index == mode_param->light_hsv_param_left_down.id){
            h = mode_param->light_hsv_param_left_down.hue;
            s = mode_param->light_hsv_param_left_down.saturation;
            v = mode_param->light_hsv_param_left_down.value;
        }else if(index == mode_param->light_hsv_param_right_down.id){
            h = mode_param->light_hsv_param_right_down.hue;
            s = mode_param->light_hsv_param_right_down.saturation;
            v = mode_param->light_hsv_param_right_down.value;
        }else if(index == mode_param->light_hsv_param_right_up.id){
            h = mode_param->light_hsv_param_right_up.hue;
            s = mode_param->light_hsv_param_right_up.saturation;
            v = mode_param->light_hsv_param_right_up.value;
        }

        c = v * s;
        x = c * x_half_value[h]/10000;
        m = v * 100 - c;

        if (light_mode_debug)
        {
            printk("bslight_tag hsv2rgb h = %d, s = %d, v = %d, c = %d, x = %d, m = %d\n", h,s,v,c,x,m);
        }

        if (h >= 0 && h < 60) {
          r = c;
          g = x;
          b = 0;
        } else if (h >= 60 && h < 120) {
          r = x;
          g = c;
          b = 0;
        } else if (h >= 120 && h < 180) {
          r = 0;
          g = c;
          b = x;
        } else if (h >= 180 && h < 240) {
          r = 0;
          g = x;
          b = c;
        } else if (h >= 240 && h < 300) {
          r = x;
          g = 0;
          b = c;
        } else if (h >= 300 && h <= 360) {
          r = c;
          g = 0;
          b = x;
        }
        r= ((r + m) * 255+5000)/10000;
        g= ((g + m) * 255+5000)/10000;
        b= ((b + m) * 255+5000)/10000;

        if (light_mode_debug_r_g_b)
        {
            printk("bslight_tag hsv2rgb r = %d,g = %d,b = %d,index = %d\n",r,g,b,index);
        }

        if (index != LIGHT_INDEX_LOGO ){
            r = 0;//display_r;
            g = 0;//display_g;
            b = 0;//display_b;
        }

        switch(index){
            case LIGHT_INDEX_LEFT_UP:
               led_light_param[0] = set_pwmi(CORNER1_MODE_RED,(unsigned char)r);
               led_light_param[1] = set_pwmi(CORNER1_MODE_GREEN,(unsigned char)g);
               led_light_param[2] = set_pwmi(CORNER1_MODE_BLUE,(unsigned char)b);
            break;
           case LIGHT_INDEX_RIGHT_UP:
               led_light_param[3] = set_pwmi(CORNER2_MODE_RED,(unsigned char)r);
               led_light_param[4] = set_pwmi(CORNER2_MODE_GREEN,(unsigned char)g);
               led_light_param[5] = set_pwmi(CORNER2_MODE_BLUE,(unsigned char)b);
            break;
            case LIGHT_INDEX_RIGHT_DOWN:
               led_light_param[6] = set_pwmi(CORNER3_MODE_RED,(unsigned char)r);
               led_light_param[7] = set_pwmi(CORNER3_MODE_GREEN,(unsigned char)g);
               led_light_param[8] = set_pwmi(CORNER3_MODE_BLUE,(unsigned char)b);
            break;
            case LIGHT_INDEX_LEFT_DOWN:
               led_light_param[9] = set_pwmi(CORNER4_MODE_RED,(unsigned char)r);
               led_light_param[10] = set_pwmi(CORNER4_MODE_GREEN,(unsigned char)g);
               led_light_param[11] = set_pwmi(CORNER4_MODE_BLUE,(unsigned char)b);
            break;
            case LIGHT_INDEX_LOGO:
               led_light_param[12] = set_pwmi(LOGO_MODE_RED,(unsigned char)r);
               led_light_param[13] = set_pwmi(LOGO_MODE_GREEN,(unsigned char)g);
               led_light_param[14] = set_pwmi(LOGO_MODE_BLUE,(unsigned char)b);
            break;
            default:
            break;
        }
        index++;
    }
    return 0;
}

int get_h_value(int mode,int index,int time){
    int h_value = 0 ;
    int h_time = time;
    switch(mode){
       case LIGHT_SCENE_BOOT_MODE_ONE:
           h_time = h_time % 4000;
           if(h_time >=0 && h_time <2000)
               h_value = 120 + 46 * h_time /2000 ;
           else if(h_time >=2000 && h_time <4000)
               h_value = 166 - 46 * (h_time - 2000)/2000 ;
           else
               h_value = 120;
       break;
       case LIGHT_SCENE_NOTIFICATION_MODE_ONE:
           h_value = 120;
       break;
       case LIGHT_SCENE_RING_MODE_ONE:
           h_time = h_time % 880;
           if(h_time >=0 && h_time <440)
               h_value = 120 + 46 * h_time /440 ;
           else if(h_time >=440 && h_time <880)
               h_value = 166 - 46 * (h_time - 440)/440 ;
           else
               h_value = 120;
       break;
       break;
       case LIGHT_SCENE_GAME_MODE_LOADING:
       case LIGHT_SCENE_GAME_MODE_NORMAL:
       case LIGHT_SCENE_GAME_MODE_SPECIAL:
       case LIGHT_SCENE_GAME_MODE_VICTOR:
           if(display_r == 0 && display_g == 255 && display_b ==  194){
               h_value = 166;
           }else if(display_r == 255 && display_g == 100 && display_b ==  91){
               h_value = 339;
           }else{
               h_value = 0;
           }
       break;
       default:
           h_value = 0;
       break;
    }


    if((time > PERIOD_SCENE_GAME_MODE_VICTOR && mode == LIGHT_SCENE_GAME_MODE_VICTOR)
                 || (time > PERIOD_SCENE_GAME_MODE_SPECIAL && mode == LIGHT_SCENE_GAME_MODE_SPECIAL)
                 || (time > PERIOD_SCENE_GAME_MODE_NORMAL && mode == LIGHT_SCENE_GAME_MODE_NORMAL)
                 || (time > PERIOD_SCENE_NOTIFICATION_MODE_ONE && mode == LIGHT_SCENE_NOTIFICATION_MODE_ONE)
                 || (time > PERIOD_SCENE_GAME_MODE_LOADING_STOP && mode == LIGHT_SCENE_GAME_MODE_LOADING)
                 || (time > PERIOD_SCENE_RING_MODE_ONE && mode == LIGHT_SCENE_RING_MODE_ONE)
                 ){
        h_value = 0;
    }

    if (light_mode_debug_h)
    {
        printk("bslight_tag h_value  = %d ,index = %d, time = %d, mode = %x  \n",h_value,index,time,mode);
    }
    return h_value;
}

int get_s_value(int mode,int index,int time){
    int s_value = 100 ;
    int s_time = time;
    switch(mode){
       case LIGHT_SCENE_BOOT_MODE_ONE:
           s_time = s_time % PERIOD_SCENE_BOOT_MODE_ONE ;
       break;
       case LIGHT_SCENE_NOTIFICATION_MODE_ONE:
       break;
       case LIGHT_SCENE_RING_MODE_ONE:
       break;
       case LIGHT_SCENE_GAME_MODE_LOADING:
       case LIGHT_SCENE_GAME_MODE_NORMAL:
       case LIGHT_SCENE_GAME_MODE_SPECIAL:
       case LIGHT_SCENE_GAME_MODE_VICTOR:
       break;
       default:
           s_value = 0;
       break;
    }

    if((time > PERIOD_SCENE_GAME_MODE_VICTOR && mode == LIGHT_SCENE_GAME_MODE_VICTOR)
                 || (time > PERIOD_SCENE_GAME_MODE_SPECIAL && mode == LIGHT_SCENE_GAME_MODE_SPECIAL)
                 || (time > PERIOD_SCENE_GAME_MODE_NORMAL && mode == LIGHT_SCENE_GAME_MODE_NORMAL)
                 || (time > PERIOD_SCENE_NOTIFICATION_MODE_ONE && mode == LIGHT_SCENE_NOTIFICATION_MODE_ONE)
                 || (time > PERIOD_SCENE_GAME_MODE_LOADING_STOP && mode == LIGHT_SCENE_GAME_MODE_LOADING)
                 || (time > PERIOD_SCENE_RING_MODE_ONE && mode == LIGHT_SCENE_RING_MODE_ONE)
                 ){
        s_value = 0;
    }

    if (light_mode_debug_s)
    {
        printk("bslight_tag s_value  = %d ,index = %d, time = %d, mode = %x  \n",s_value,index,time,mode);
    }
    return s_value;
}

int get_v_value(int mode,int index,int time){

    int v_value = 0 ;
    int v_time = time;

    switch(mode){
       case LIGHT_SCENE_BOOT_MODE_ONE:
           v_time = v_time % 4000;

           if(v_time >= 0 && v_time < 1000){
               v_value = v_time/10;
           }else if(v_time >= 1000 && v_time < 2000){
               v_value =100 - (v_time -1000)/10;
           }else if(v_time >= 2000 && v_time < 3000){
               v_value =(v_time -2000)/10;
           }else if(v_time >= 3000 && v_time < 4000){
               v_value =100 - (v_time -3000)/10;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_NOTIFICATION_MODE_ONE:
           v_time = v_time % PERIOD_SCENE_NOTIFICATION_MODE_ONE;
           if((v_time >= 0 && v_time < 330 )|| (v_time >= 1000 && v_time < 1330 )){
               v_time = v_time % 1000;
               v_value = v_time * 100 / 330;
           }else if(v_time >= 330 && v_time < 660 ){
               v_value = 100 - (v_time -330) * 100 / 330;
           }else if(v_time >= 660 && v_time < 1000){
               v_value =0;
           }else if(v_time >= 1330 && v_time < 2330){
               v_value = 100 - (v_time -1330) / 10;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_RING_MODE_ONE:
           v_time = v_time % 1000;
           if ((v_time >= 0 && v_time < 220) || (v_time >= 440 && v_time < 660) ){
               v_time = v_time % 440;
               v_value = 100 - v_time * 100 /220;
           }else if ((v_time >= 220 && v_time < 440)|| (v_time >= 660 && v_time < 880) ){
               v_time = (v_time - 220) % 440;
               v_value = v_time * 100 /220;
           }else if (v_time >= 880 && v_time < 1000 ){
               v_value = 100 - (v_time -880) * 100 /120;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_GAME_MODE_LOADING:
           v_time = v_time % PERIOD_SCENE_GAME_MODE_LOADING;
           if (v_time >= 0 && v_time < 1000){
               v_value =100 - v_time/10;
           }else if (v_time >= 1000 && v_time < 2000){
               v_value =(v_time -1000)/10;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_GAME_MODE_NORMAL:
           v_time = v_time % PERIOD_SCENE_GAME_MODE_NORMAL;
           if (v_time >= 0 && v_time < 250){
               v_value = v_time * 2/5;
           }else if (v_time >= 250 && v_time < 1500){
               v_value = 100;
           }else if (v_time >= 1500 && v_time < 2000){
               v_value =(v_time -1500) /5;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_GAME_MODE_SPECIAL:
           v_time = v_time % PERIOD_SCENE_GAME_MODE_SPECIAL;
           if ((v_time >= 0 && v_time < 125) || (v_time >= 375 && v_time < 500) || (v_time >= 750 && v_time < 875)){
               v_time = v_time % 125;
               v_value =100 - v_time * 100/125;
           }else if ((v_time >= 125 && v_time < 250) || (v_time >= 500 && v_time < 625) || (v_time >= 875 && v_time < 1000)){
               v_time = v_time % 125;
               v_value = v_time * 100/125; ;
           }else if ((v_time >= 250 && v_time < 375) || (v_time >= 625 && v_time < 750)){
               v_value = 100;
           }else if (v_time >= 1000 && v_time < 1700){
               v_value = 100 - (v_time - 1000) * 100/700; ;
           }else{
               v_value = 0 ;
           }
       break;
       case LIGHT_SCENE_GAME_MODE_VICTOR:
           v_time = v_time % PERIOD_SCENE_GAME_MODE_VICTOR;
           if ((v_time >= 0 && v_time < 160) || (v_time >= 320 && v_time < 480) || (v_time >= 640 && v_time < 800)){
               v_time = v_time % 160;
               v_value = v_time * 100/160;
           }else if ((v_time >= 160 && v_time < 320) || (v_time >= 480 && v_time < 640) || (v_time >= 800 && v_time < 960)){
               v_time = v_time % 160;
               v_value = 100 - v_time * 100/160; ;
           }else{
               v_value = 0 ;
           }
       break;
       default:
          v_value = 0 ;
       break;
    }

    if((time > PERIOD_SCENE_GAME_MODE_VICTOR && mode == LIGHT_SCENE_GAME_MODE_VICTOR)
                 || (time > PERIOD_SCENE_GAME_MODE_SPECIAL && mode == LIGHT_SCENE_GAME_MODE_SPECIAL)
                 || (time > PERIOD_SCENE_GAME_MODE_NORMAL && mode == LIGHT_SCENE_GAME_MODE_NORMAL)
                 || (time > PERIOD_SCENE_NOTIFICATION_MODE_ONE && mode == LIGHT_SCENE_NOTIFICATION_MODE_ONE)
                 || (time > PERIOD_SCENE_GAME_MODE_LOADING_STOP && mode == LIGHT_SCENE_GAME_MODE_LOADING)
                 || (time > PERIOD_SCENE_RING_MODE_ONE && mode == LIGHT_SCENE_RING_MODE_ONE)
                 ){
        v_value = 0;
    }
    if (light_mode_debug_v)
    {
        printk("bslight_tag v_value  = %d ,index = %d, time = %d, mode = %x  \n",v_value,index,time,mode);
    }
    return v_value;
}
int set_color(int mode)
{
    if (light_mode_debug_fresh)
    {
        printk("bslight_tag set_color  start [ %lu  ]  \n",jiffies);
    }

    if (light_mode_data == LIGHT_SCENE_OFF)
    {
        return 0;
    }

    light_mode_param_dev->light_hsv_param_logo.id = 5;
    light_mode_param_dev->light_hsv_param_logo.hue = get_h_value(mode,5,time_count);
    light_mode_param_dev->light_hsv_param_logo.saturation =get_s_value(mode,5,time_count);
    light_mode_param_dev->light_hsv_param_logo.value = get_v_value(mode,5,time_count);

    light_mode_param_dev->light_hsv_param_left_up.id = 1;
    light_mode_param_dev->light_hsv_param_left_up.hue = get_h_value(mode,1,time_count);
    light_mode_param_dev->light_hsv_param_left_up.saturation =get_s_value(mode,1,time_count);
    light_mode_param_dev->light_hsv_param_left_up.value = get_v_value(mode,1,time_count);

    light_mode_param_dev->light_hsv_param_left_down.id = 4;
    light_mode_param_dev->light_hsv_param_left_down.hue =get_h_value(mode,4,time_count);
    light_mode_param_dev->light_hsv_param_left_down.saturation =get_s_value(mode,4,time_count);
    light_mode_param_dev->light_hsv_param_left_down.value = get_v_value(mode,4,time_count);

    light_mode_param_dev->light_hsv_param_right_up.id = 2;
    light_mode_param_dev->light_hsv_param_right_up.hue = get_h_value(mode,2,time_count);
    light_mode_param_dev->light_hsv_param_right_up.saturation =get_s_value(mode,2,time_count);
    light_mode_param_dev->light_hsv_param_right_up.value = get_v_value(mode,2,time_count);

    light_mode_param_dev->light_hsv_param_right_down.id = 3;
    light_mode_param_dev->light_hsv_param_right_down.hue =get_h_value(mode,3,time_count);
    light_mode_param_dev->light_hsv_param_right_down.saturation =get_s_value(mode,3,time_count);
    light_mode_param_dev->light_hsv_param_right_down.value = get_v_value(mode,3,time_count);

    hsv2rgb(light_mode_param_dev);



    mcu_light_control(led_light_param);

    if (light_mode_debug_fresh)
    {
        printk("bslight_tag set_color  end [ %lu  ]  \n",jiffies);
    }
    return 0;
}

int thr_fn_time(void *arg)
{
    if (light_mode_debug)
    {
        printk("bslight_tag HZ : %d\n", HZ);
        printk("bslight_tag light_mode_data =  %d\n", light_mode_data);
    }

    while (!kthread_should_stop())
    {
        set_current_state(TASK_UNINTERRUPTIBLE);

        schedule_timeout(PERIOD);
        if (light_mode_debug)
        {
            printk("bslight_tag time_count =  %ld,light_mode_data = %x\n", time_count,light_mode_data);
        }
        if((light_mode_data == LIGHT_SCENE_NOTIFICATION_MODE_ONE && time_count >= PERIOD_SCENE_NOTIFICATION_MODE_ONE)
                    ||(time_count >= PERIOD_SCENE_GAME_MODE_VICTOR && light_mode_data == LIGHT_SCENE_GAME_MODE_VICTOR)
                    ||(time_count >= PERIOD_SCENE_GAME_MODE_SPECIAL && light_mode_data == LIGHT_SCENE_GAME_MODE_SPECIAL)
                    ||(time_count >= PERIOD_SCENE_GAME_MODE_NORMAL && light_mode_data == LIGHT_SCENE_GAME_MODE_NORMAL)
                    ||(time_count >= PERIOD_SCENE_BOOT_MODE_ONE && light_mode_data == LIGHT_SCENE_BOOT_MODE_ONE)
                    ||(time_count >= PERIOD_SCENE_GAME_MODE_LOADING_STOP && light_mode_data == LIGHT_SCENE_GAME_MODE_LOADING)
                    ||(time_count >= PERIOD_SCENE_RING_MODE_ONE && light_mode_data == LIGHT_SCENE_RING_MODE_ONE)
                    )
        {
            thr_fn_time_stop();
        } else {
            set_color(light_mode_data);
        }
        time_count = time_count + 10;
    }
   return 0;
}

//end for color control

//////////////////////////////////////////////////////////////////////////////////////////
// PDN power control
//////////////////////////////////////////////////////////////////////////////////////////

int aw9120_pinctrl_init(struct aw9120_led *data)
{
	int ret = 0;
    pr_debug("enter devm_pinctrl_get\n");
	data->aw9120ctrl = devm_pinctrl_get(&data->i2c_client->dev);
	if (IS_ERR(data->aw9120ctrl)) {
		dev_err(&data->i2c_client->dev, "Cannot find aw9120 pinctrl!\n");
		ret = PTR_ERR(data->aw9120ctrl);
	}
	data->aw9120_pdn_high = pinctrl_lookup_state(data->aw9120ctrl, "awinic_pin_active");
	if (IS_ERR(data->aw9120_pdn_high)) {
		ret = PTR_ERR(data->aw9120_pdn_high);
		dev_err(&data->i2c_client->dev, "pinctrl err, aw9120_pdn_high\n");
	}
	data->aw9120_pdn_low = pinctrl_lookup_state(data->aw9120ctrl, "awinic_pin_suspend");
	if (IS_ERR(data->aw9120_pdn_low)) {
		ret = PTR_ERR(data->aw9120_pdn_low);
		dev_err(&data->i2c_client->dev, "pinctrl err, aw9120_pdn_low\n");
	}

	return ret;
}

static void aw9120_config_init(struct aw9120_led *led_data)
{
    unsigned int reg,i;
    mutex_lock(&led_data->lock);
	//Disable LED Module
	reg = i2c_read_reg(led_data, GCR);
	reg &= 0xFFFE;
	i2c_write_reg(led_data, GCR, reg); 		// GCR-Disable LED Module
    //set dimming mode
	reg = i2c_read_reg(led_data, LCR);
	reg &= 0xFFF8;
	if(!strcmp(led_data->control_mode, DIMMING_LOGE))
	{
	    reg |= 0x0;
	}
	else if(!strcmp(led_data->control_mode, DIMMING_LOG10))
	{
        reg |= 0x1;
	}
	else if(!strcmp(led_data->control_mode, DIMMING_LINEAR))
	{
        reg |= 0x2;
	}
	else
	{
        pr_err("error parse dimming mode\n");
	}
	//set PWM freq
	if(PWM_FREQ_245HZ == led_data->pwm_freq)
	{
        reg |= 0x0;
	}
	else if(PWM_FREQ_122HZ == led_data->pwm_freq)
	{
        reg |= 0x4;
	}
	else
	{
        pr_err("error parse pwm freq\n");
	}
	i2c_write_reg(led_data, LCR, reg);
	//LED Config
	for(i = 0; i < led_data->num_leds; i++)
	{
        reg = i2c_read_reg(led_data, IMAX1 + (led_data->led_array[i].id>>2));
        reg |= led_data->led_array[i].max_current<<((led_data->led_array[i].id%4)<<2);
        i2c_write_reg(led_data, IMAX1 + (led_data->led_array[i].id>>2), reg);     // IMAX-LED1~LED20 Current
        reg = i2c_read_reg(led_data, LER1 + (led_data->led_array[i].id/12));
        reg |= 1 << (led_data->led_array[i].id%12);
        i2c_write_reg(led_data, LER1 + (led_data->led_array[i].id/12),reg);   // LED1~LED20 Enable
    }
	i2c_write_reg(led_data, CTRS1,0x0000); 	// CTRS1-LED1~LED12: sram Control
	i2c_write_reg(led_data, CTRS2,0x0000); 	// CTRS2-LED13~LED20: sram Control
	//Enable LED Module
	reg |= 0x0001;
	i2c_write_reg(led_data, GCR,reg); 		// GCR-Enable LED Module
    mutex_unlock(&led_data->lock);
}
static void aw9120_hw_on(struct aw9120_led *led_data)
{
	pinctrl_select_state(led_data->aw9120ctrl, led_data->aw9120_pdn_low);
	msleep(5);
	pinctrl_select_state(led_data->aw9120ctrl, led_data->aw9120_pdn_high);
	msleep(5);
}

static void aw9120_write_mode(struct aw9120_led *led_data, int *code, int code_length)
{
	unsigned int i = 0, reg = 0;

    if(NULL == code)
    {
        pr_err("aw9120_write_mode para is error\n");
        return;
    }

    mutex_lock(&led_data->lock);
    //Disable LED Module
    reg = i2c_read_reg(led_data, GCR);
    reg &= 0xFFFE;
    i2c_write_reg(led_data, GCR, reg);

    reg |= 0x0001;
    i2c_write_reg(led_data, GCR,reg);                   // GCR-Enable LED Module
	i2c_write_reg(led_data, CTRS1,0x0000); 	// CTRS1-LED1~LED12: sram Control
	i2c_write_reg(led_data, CTRS2,0x0000); 	// CTRS2-LED13~LED20: sram Control

	// LED SRAM Hold Mode
	i2c_write_reg(led_data, PMR,0x0000);		// PMR-Load SRAM with I2C
	i2c_write_reg(led_data, RMR,0x0000);		// RMR-Hold Mode

	// Load LED SRAM
	i2c_write_reg(led_data, WADDR,0x0000);	// WADDR-SRAM Load Addr
	for(i=0; i < code_length; i++)
	{
		i2c_write_reg(led_data, WDATA,code[i]);
	}

	// LED SRAM Run
	i2c_write_reg(led_data, SADDR,0x0000);	// SADDR-SRAM Run Start Addr:0
	i2c_write_reg(led_data, PMR,0x0001);		// PMR-Reload and Excute SRAM
	i2c_write_reg(led_data, RMR,0x0002);		// RMR-Run

	mutex_unlock(&led_data->lock);
}

/*
static void aw9120_hw_off(struct aw9120_led *led_data)
{
	pr_debug("enter");
	pinctrl_select_state(led_data->aw9120ctrl, led_data->aw9120_pdn_low);
	msleep(5);
	pr_debug("out");
}
*/

//////////////////////////////////////////////////////////////////////////////////////////
// aw9120 led
//////////////////////////////////////////////////////////////////////////////////////////
void aw9120_led_off(struct aw9120_led *led_data)
{
	aw9120_write_mode(led_data, led_all_off_code, sizeof(led_all_off_code)/sizeof(led_all_off_code[0]));
}

void aw9120_led_on(struct aw9120_led *led_data)
{
	aw9120_write_mode(led_data, led_all_on_code, sizeof(led_all_on_code)/sizeof(led_all_on_code[0]));
}

void aw9120_led_breath(struct aw9120_led *led_data)
{
	aw9120_write_mode(led_data, led_breath_code, sizeof(led_breath_code)/sizeof(led_breath_code[0]));
}

void aw9120_led_agingtest(struct aw9120_led *led_data)
{
	aw9120_write_mode(led_data, led_agingtest_code, sizeof(led_agingtest_code)/sizeof(led_agingtest_code[0]));
}
//////////////////////////////////////////////////////////////////////////////////////////
// adb shell
//////////////////////////////////////////////////////////////////////////////////////////
static int fw_update(struct aw9120_led *led_data);
static ssize_t aw9120_show_debug(struct device* dev,struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_debug(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_fw_download(struct device* dev,struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_fw_download(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_calibration(struct device* dev,struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_calibration(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_get_reg(struct device* dev,struct device_attribute *attr, char* buf);
static ssize_t aw9120_write_reg(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_modestate(struct device* dev,struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_modestate(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_fw_flag(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_fw_flag(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_mcu_control(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_mcu_control(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_mmi_test(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_mmi_test(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw9120_show_mmi_cali(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t aw9120_store_mmi_cali(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);

//for color control
static ssize_t bslight_get_light_mode(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t bslight_set_light_mode(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t bslight_get_light_mode_debug(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t bslight_set_light_mode_debug(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t bslight_get_light_mode_music(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t bslight_set_light_mode_music(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t bslight_get_light_mode_display(struct device* dev, struct device_attribute *attr, char* buf);
static ssize_t bslight_set_light_mode_display(struct device* dev, struct device_attribute *attr,const char* buf, size_t len);
//end for color control

static DEVICE_ATTR(debug, 0664, aw9120_show_debug, aw9120_store_debug);
static DEVICE_ATTR(fw_download, 0664, aw9120_show_fw_download, aw9120_store_fw_download);
static DEVICE_ATTR(calibration, 0664, aw9120_show_calibration, aw9120_store_calibration);
static DEVICE_ATTR(reg, 0664, aw9120_get_reg, aw9120_write_reg);
static DEVICE_ATTR(modestate, 0664, aw9120_show_modestate, aw9120_store_modestate);
static DEVICE_ATTR(fw_flag, 0664, aw9120_show_fw_flag, aw9120_store_fw_flag);
static DEVICE_ATTR(mcu_control, 0664, aw9120_show_mcu_control, aw9120_store_mcu_control);
static DEVICE_ATTR(mmi_test, 0664, aw9120_show_mmi_test, aw9120_store_mmi_test);
static DEVICE_ATTR(mmi_cali, 0664, aw9120_show_mmi_cali, aw9120_store_mmi_cali);

//for color control
static DEVICE_ATTR(light_mode, 0664, bslight_get_light_mode, bslight_set_light_mode);
static DEVICE_ATTR(bs_light_mode_debug, 0664, bslight_get_light_mode_debug, bslight_set_light_mode_debug);
static DEVICE_ATTR(bs_light_mode_music, 0664, bslight_get_light_mode_music, bslight_set_light_mode_music);
static DEVICE_ATTR(bs_light_mode_display, 0664, bslight_get_light_mode_display, bslight_set_light_mode_display);
//end for color control

static ssize_t aw9120_show_modestate(struct device* dev,struct device_attribute *attr, char* buf)
{
    struct aw9120_led *led_data = NULL;
    struct list_head *pos, *n;
    struct led_mode_list *p;
    int len = 0, i = 0;

	led_data = dev_get_drvdata(dev);

    if(fw_update(led_data) < 0)
    {
        pr_err("Load firmware error\n");
        return -EINVAL;
    }

    list_for_each_safe(pos, n, &led_data->led_mode_list_head.list)
    {
        p = list_entry(pos, struct led_mode_list, list);
        len += snprintf(buf+len, PAGE_SIZE-len, "%s:\n", p->led_mode_name);
        for(i = 0; i < p->TWO_BYTES_mode_code_count; i++)
        {
            len += snprintf(buf+len, PAGE_SIZE-len, "0x%X,", p->mode_code[i]);
        }
        buf[len] = '\n';
        len += 1;
    }

    return len;
}

static ssize_t aw9120_store_modestate(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
    struct aw9120_led *led_data = NULL;
    unsigned char databuf[EACH_MODE_NAME_MAX_LEN] = {0};
    struct list_head *pos = NULL, *n = NULL;
    struct led_mode_list *p = NULL;

    if(len >= EACH_MODE_NAME_MAX_LEN)
    {
        pr_err("echo:para's length is no right, max len is 32Byte\n");
        return -EINVAL;
    }
    led_data = dev_get_drvdata(dev);
    if(fw_update(led_data) < 0)
    {
        pr_err("Load firmware error\n");
        return -EINVAL;
    }

    memcpy(databuf, buf, len);
    list_for_each_safe(pos, n, &led_data->led_mode_list_head.list)
    {
        p = list_entry(pos, struct led_mode_list, list);
        if(p->led_mode_name && (!strncmp(p->led_mode_name, databuf, len - 1)))
        {
            aw9120_write_mode(led_data, p->mode_code, p->TWO_BYTES_mode_code_count);
        }
        else
        {
            pr_info("led_mode_name=%s, echo buf=%s, len=%d\n", p->led_mode_name, buf, (int)len);
        }
    }

    return len;
}

static ssize_t aw9120_show_debug(struct device* dev,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "aw9120_led_off(void)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 > debug\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "aw9120_led_on(void)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 1 > debug\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "aw9120_led_breath(void)\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "echo 2 > debug\n");

	return len;
}

static ssize_t aw9120_store_debug(struct device* dev, struct device_attribute *attr,
				const char* buf, size_t len)
{
	unsigned int databuf[1];
    struct aw9120_led *led_data = NULL;

	led_data = dev_get_drvdata(dev);
    if(fw_update(led_data) < 0)
    {
        pr_err("aw9120_store_debug Load firmware error\n");
        //return -EINVAL;
    }

	sscanf(buf,"%d",&databuf[0]);
	if(databuf[0] == 0) {				// OFF
		aw9120_led_off(led_data);
	} else if(databuf[0] == 1){			// ON
		aw9120_led_on(led_data);
	} else if(databuf[0] == 2){			// Breath
		aw9120_led_breath(led_data);
	} else {
		aw9120_led_off(led_data);
	}

	return len;
}

static ssize_t aw9120_show_fw_download(struct device* dev,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "shark aw9120 fw_download not can used\n");

	return len;
}

static ssize_t aw9120_store_fw_download(struct device* dev, struct device_attribute *attr,
				const char* buf, size_t len)
{
	unsigned int databuf = 0;
	unsigned int data_fw[256] = {0};
	int i= 0;
	unsigned char hex_ch[4] = {0x00};
	ssize_t ret = -EINVAL;
	struct aw9120_led *led_data = NULL;
	led_data = dev_get_drvdata(dev);
	
	for(i=0; i<(len/4); i++){
		memset(hex_ch, 0x00, 4);
		memcpy(hex_ch, buf + 4 * i, 4);
		ret = sscanf(hex_ch,"%04x",&databuf);
		if(ret!=1)
			pr_err("shart aw9120 read fw_download failed ret = %ld\n",ret);

		data_fw[i] = databuf;
		pr_debug("shark aw9120 fw_download from hal data[%d] = 0x%x\n",i,databuf);
	}
	aw9120_write_mode(led_data, data_fw, len/4);

	return len;
}

static ssize_t aw9120_show_calibration(struct device* dev,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "shark aw9120 fw_download not can used\n");

	return len;
}

static ssize_t aw9120_store_calibration(struct device* dev, struct device_attribute *attr,
				const char* buf, size_t len)
{
	int i= 0;
	static int j  = 0;
	char hex_ch[2];
	ssize_t ret = -EINVAL;
	struct aw9120_led *led_data = NULL;

	led_data = dev_get_drvdata(dev);
	for(i=0; i<256; i++){
		memset(hex_ch, 0, 2);
		memcpy(hex_ch, buf + 2*i , 2);
		ret = sscanf(hex_ch,"%04x",&table_cali[j][i]);
		if(ret!=1)
			pr_err("shart aw9120 read fw_download failed ret = %ld\n",ret);

		//printk("shark aw9120 calibration from hal table_cali[%d][%d] = %x\n",j,i,table_cali[j][i]);
	}

	j++;
	if(j>14)
		j = 0;
	return len;
}

static ssize_t aw9120_get_reg(struct device* dev,struct device_attribute *attr, char* buf)
{
	unsigned int reg_val[1];
	struct aw9120_led *led_data = NULL;
	ssize_t len = 0;
	unsigned char i;

	led_data = dev_get_drvdata(dev);
	for(i=1;i<0x7F;i++) {
		reg_val[0] = i2c_read_reg(led_data, i);
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%2X = 0x%4X, \n", i,reg_val[0]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;
}

static ssize_t aw9120_write_reg(struct device* dev, struct device_attribute *attr,
				const char* buf, size_t len)
{
	unsigned int databuf[2];
	struct aw9120_led *led_data = NULL;

	led_data = dev_get_drvdata(dev);
	if(2 == sscanf(buf,"%x %x",&databuf[0], &databuf[1])) {
		i2c_write_reg(led_data, (u8)databuf[0],databuf[1]);
	}
	return len;
}

static ssize_t aw9120_show_fw_flag(struct device* dev,struct device_attribute *attr, char* buf)
{
    return sprintf(buf,"%d\n", fw_update_flag);
}

static ssize_t aw9120_store_fw_flag(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
    sscanf(buf, "%d", &fw_update_flag);

    return len;
}

static ssize_t aw9120_show_mmi_test(struct device* dev,struct device_attribute *attr, char* buf)
{
	int reg_value= 0;
	int state = 0;
	struct aw9120_led *led_data = NULL;
	
	led_data = dev_get_drvdata(dev);
	//aw9120_hw_on(led_data);
	//aw9120_config_init(led_data);

	reg_value = i2c_read_reg(led_data, 0x00);				//read chip ID
	pr_info("aw9120 chip ID = 0x%4x\n", reg_value);
	if (reg_value == 0xb223)
		state = 1;

	aw9120_write_mode(led_data, led_mmitest_code, sizeof(led_mmitest_code)/sizeof(led_mmitest_code[0]));

	return sprintf(buf,"%d\n", state);
}

static ssize_t aw9120_store_mmi_test(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
	unsigned int databuf[1];
    struct aw9120_led *led_data = NULL;

	led_data = dev_get_drvdata(dev);
	sscanf(buf,"%d",&databuf[0]);

	if(databuf[0] == 0) {
		aw9120_led_off(led_data);
	} else if(databuf[0] == 3){
		i2c_write_reg(led_data, IMAX1,IMAXCMD105);
		i2c_write_reg(led_data, IMAX2,IMAXCMD105);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD105);
		i2c_write_reg(led_data, IMAX4,IMAXCMD105);
		aw9120_led_agingtest(led_data);
	} else if(databuf[0] == 4){
		i2c_write_reg(led_data, IMAX1,IMAXCMD140);
		i2c_write_reg(led_data, IMAX2,IMAXCMD140);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD140);
		i2c_write_reg(led_data, IMAX4,IMAXCMD140);
		aw9120_led_agingtest(led_data);
	} else if(databuf[0] == 5){
		i2c_write_reg(led_data, IMAX1,IMAXCMD175);
		i2c_write_reg(led_data, IMAX2,IMAXCMD175);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD175);
		i2c_write_reg(led_data, IMAX4,IMAXCMD175);
		aw9120_led_agingtest(led_data);
	} else {
		aw9120_led_off(led_data);
	}

    return len;
}

static ssize_t aw9120_show_mmi_cali(struct device* dev,struct device_attribute *attr, char* buf)
{
    return sprintf(buf,"%d\n", fw_update_flag);
}

static ssize_t aw9120_store_mmi_cali(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
	unsigned int databuf = 0;
	unsigned int reg = 0;
	ssize_t ret = -EINVAL;
	struct aw9120_led *led_data = NULL;
	led_data = dev_get_drvdata(dev);
	
	ret = sscanf(buf,"%08x",&databuf);
	if(ret!=1)
		pr_err("shart aw9120 read mmi_cali failed ret = %ld\n",ret);
	pr_debug("shark aw9120 mmi_cali data  = 0x%x\n",databuf);

	if(databuf == INITCMD){
		i2c_write_reg(led_data, RSTR,0x55aa);
		//Disable LED Module
		reg = i2c_read_reg(led_data, GCR);
		reg &= 0xFFFE;
		i2c_write_reg(led_data, GCR, reg);
		reg |= 0x0001;
		i2c_write_reg(led_data, GCR,reg);					// GCR-Enable LED Module
		i2c_write_reg(led_data, LER1,0x0eff);
		i2c_write_reg(led_data, LER2,0x000f);
		i2c_write_reg(led_data, CTRS1,0x0EFF);	// CTRS1-LED1~LED12: mcu Control
		i2c_write_reg(led_data, CTRS2,0x000F);	// CTRS2-LED13~LED20: mcu Control
		i2c_write_reg(led_data, CMDR,CLOSECMD);

		return len;
	} else {
		//current
	switch(databuf&0xff){
		case 1:
			i2c_write_reg(led_data, IMAX1,IMAXCMD35);
			i2c_write_reg(led_data, IMAX2,IMAXCMD35);
			i2c_write_reg(led_data, IMAX3,IMAX2CMD35);
			i2c_write_reg(led_data, IMAX4,IMAXCMD35);
			pr_info("AW9120 shark mmi_cali set current 3.5mA\n");
			break;
		case 2:
			i2c_write_reg(led_data, IMAX1,IMAXCMD70);
			i2c_write_reg(led_data, IMAX2,IMAXCMD70);
			i2c_write_reg(led_data, IMAX3,IMAX2CMD70);
			i2c_write_reg(led_data, IMAX4,IMAXCMD70);
			pr_info("AW9120 shark mmi_cali set current 7mA\n");
			break;
		case 3:
			i2c_write_reg(led_data, IMAX1,IMAXCMD105);
			i2c_write_reg(led_data, IMAX2,IMAXCMD105);
			i2c_write_reg(led_data, IMAX3,IMAX2CMD105);
			i2c_write_reg(led_data, IMAX4,IMAXCMD105);
			pr_info("AW9120 shark mmi_cali set current 10.5mA\n");
			break;
		case 4:
			i2c_write_reg(led_data, IMAX1,IMAXCMD140);
			i2c_write_reg(led_data, IMAX2,IMAXCMD140);
			i2c_write_reg(led_data, IMAX3,IMAX2CMD140);
			i2c_write_reg(led_data, IMAX4,IMAXCMD140);
			pr_info("AW9120 shark mmi_cali set current 14mA\n");
			break;
		case 5:
			i2c_write_reg(led_data, IMAX1,IMAXCMD175);
			i2c_write_reg(led_data, IMAX2,IMAXCMD175);
			i2c_write_reg(led_data, IMAX3,IMAX2CMD175);
			i2c_write_reg(led_data, IMAX4,IMAXCMD175);
			pr_info("AW9120 shark mmi_cali set current 17.5mA\n");
			break;
		default:
			pr_info("AW9120 shark mmi_cali not set current.\n");
			break;
		}

		//rgb
	switch((databuf>>20)&0xff){
		case 0:					//all close
			i2c_write_reg(led_data, CMDR,CLOSECMD);
			break;
		case 1:					//red
			i2c_write_reg(led_data, CMDR,CLOSECMD);
			i2c_write_reg(led_data, CMDR,REGMSK1|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,REGMSK2|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,REGMSK3|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,REGMSK4|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,REGMSK5|((databuf>>8)&0xff));
			break;
		case 2:					//green
			i2c_write_reg(led_data, CMDR,CLOSECMD);
			i2c_write_reg(led_data, CMDR,GREENMSK1|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,GREENMSK2|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,GREENMSK3|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,GREENMSK4|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,GREENMSK5|((databuf>>8)&0xff));
			break;
		case 3:					//blue
			i2c_write_reg(led_data, CMDR,CLOSECMD);
			i2c_write_reg(led_data, CMDR,BLUEMSK1|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,BLUEMSK2|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,BLUEMSK3|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,BLUEMSK4|((databuf>>8)&0xff));
			i2c_write_reg(led_data, CMDR,BLUEMSK5|((databuf>>8)&0xff));
			break;
		case 4:					//test
			//i2c_write_reg(led_data, CMDR,CLOSECMD);
			switch((databuf>>16)&0x0f){
				case 0:
					i2c_write_reg(led_data, CMDR,REGMSK1|((databuf>>8)&0xff));
					break;
				case 1:
					i2c_write_reg(led_data, CMDR,REGMSK2|((databuf>>8)&0xff));
					break;
				case 2:
					i2c_write_reg(led_data, CMDR,REGMSK3|((databuf>>8)&0xff));
					break;
				case 3:
					i2c_write_reg(led_data, CMDR,REGMSK4|((databuf>>8)&0xff));
					break;
				case 4:
					i2c_write_reg(led_data, CMDR,REGMSK5|((databuf>>8)&0xff));
					break;
				case 5:
					i2c_write_reg(led_data, CMDR,GREENMSK1|((databuf>>8)&0xff));
					break;
				case 6:
					i2c_write_reg(led_data, CMDR,GREENMSK2|((databuf>>8)&0xff));
					break;
				case 7:
					i2c_write_reg(led_data, CMDR,GREENMSK3|((databuf>>8)&0xff));
					break;
				case 8:
					i2c_write_reg(led_data, CMDR,GREENMSK4|((databuf>>8)&0xff));
					break;
				case 9:
					i2c_write_reg(led_data, CMDR,GREENMSK5|((databuf>>8)&0xff));
					break;
				case 10:
					i2c_write_reg(led_data, CMDR,BLUEMSK1|((databuf>>8)&0xff));
					break;
				case 11:
					i2c_write_reg(led_data, CMDR,BLUEMSK2|((databuf>>8)&0xff));
					break;
				case 12:
					i2c_write_reg(led_data, CMDR,BLUEMSK3|((databuf>>8)&0xff));
					break;
				case 13:
					i2c_write_reg(led_data, CMDR,BLUEMSK4|((databuf>>8)&0xff));
					break;
				case 14:
					i2c_write_reg(led_data, CMDR,BLUEMSK5|((databuf>>8)&0xff));
					break;
				default:
					pr_info("AW9120 shark mmi_cali test  error!\n");
					break;
			}
			break;
		default:
			pr_info("AW9120 shark mmi_cali rgb  error!\n");
			break;
		}
	}

	return len;
}

#if 0
int aw_nvram_read(char *filename, char *buf)
{
    struct file *fp;
    //ssize_t ret;
    int retLen = 0;
    loff_t pos;

    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(filename, O_RDONLY, 0);
    if(IS_ERR(fp)) {
        pr_info("[AW9120][nvram_read] : failed to open!!\n");
        return -1;
    }
	pos =0;
	vfs_read(fp,buf, 1020, &pos);
    filp_close(fp, NULL);
    set_fs(old_fs);

    return retLen;
}
#endif

static ssize_t aw9120_show_mcu_control(struct device* dev,struct device_attribute *attr, char* buf)
{
#if 0
	int ret = 0;
	static char buff[1024] = {0};
	int i=0;
	unsigned int databuf = 0;
	unsigned int data_fw[255] = {0};
	struct aw9120_led *led_data = NULL;

	ret = aw_nvram_read("/data/misc/rgb/21.ucf",buff);
    if(ret == -1)
    {
        pr_info("AW9120 NO UCF FILE,use default UCF\n");
       // return 0;
    }

	led_data = dev_get_drvdata(dev);
	for(i=0; i<255; i++){
		ret = sscanf(buff+4*i,"%04x",&databuf);
		if(ret!=1)
			pr_err("AW9120 kernel ret = %d\n",ret);

		data_fw[i] = databuf;
		pr_info("AW9120 kernel data i%d = 0x%x\n",i,databuf);
	}
	aw9120_write_mode(led_data, data_fw, 255);

    return sprintf(buf,"%d\n", fw_update_flag);
#endif

	ssize_t len = 0;
	len += snprintf(buf+len, PAGE_SIZE-len, "init: echo 0 > mcu_control\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "set current 1-5: echo 5 > mcu_control\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "command: echo command > mcu_control\n");

	return len;
}

static ssize_t aw9120_store_mcu_control(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
	unsigned int reg = 0;
	int data = 0;
	struct aw9120_led *led_data = NULL;
	led_data = dev_get_drvdata(dev);

	sscanf(buf, "%x", &data);
	pr_info("AW9120 shark data = %x\n",data);

	if(data == INITCMD){
		i2c_write_reg(led_data, RSTR,0x55aa);
		//Disable LED Module
		reg = i2c_read_reg(led_data, GCR);
		reg &= 0xFFFE;
		i2c_write_reg(led_data, GCR, reg);
		reg |= 0x0001;
		i2c_write_reg(led_data, GCR,reg);					// GCR-Enable LED Module
		i2c_write_reg(led_data, LER1,0x0eff);
		i2c_write_reg(led_data, LER2,0x000f);
		i2c_write_reg(led_data, CTRS1,0x0EFF); 	// CTRS1-LED1~LED12: mcu Control
		i2c_write_reg(led_data, CTRS2,0x000F); 	// CTRS2-LED13~LED20: mcu Control
		i2c_write_reg(led_data, CMDR,CLOSECMD);
		pr_info("AW9120 shark mcu_control CLOSECMD\n");

		return len;
	}

	switch(data){
	case 1:
		i2c_write_reg(led_data, IMAX1,IMAXCMD35);
		i2c_write_reg(led_data, IMAX2,IMAXCMD35);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD35);
		i2c_write_reg(led_data, IMAX4,IMAXCMD35);
		pr_info("AW9120 shark mcu_control set current 3.5mA\n");
		break;
	case 2:
		i2c_write_reg(led_data, IMAX1,IMAXCMD70);
		i2c_write_reg(led_data, IMAX2,IMAXCMD70);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD70);
		i2c_write_reg(led_data, IMAX4,IMAXCMD70);
		pr_info("AW9120 shark mcu_control set current 7.0mA\n");
		break;
	case 3:
		i2c_write_reg(led_data, IMAX1,IMAXCMD105);
		i2c_write_reg(led_data, IMAX2,IMAXCMD105);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD105);
		i2c_write_reg(led_data, IMAX4,IMAXCMD105);
		pr_info("AW9120 shark mcu_control set current 10.5mA\n");
		break;
	case 4:
		i2c_write_reg(led_data, IMAX1,IMAXCMD140);
		i2c_write_reg(led_data, IMAX2,IMAXCMD140);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD140);
		i2c_write_reg(led_data, IMAX4,IMAXCMD140);
		pr_info("AW9120 shark mcu_control set current 14mA\n");
		break;
	case 5:
		i2c_write_reg(led_data, IMAX1,IMAXCMD175);
		i2c_write_reg(led_data, IMAX2,IMAXCMD175);
		i2c_write_reg(led_data, IMAX3,IMAX2CMD175);
		i2c_write_reg(led_data, IMAX4,IMAXCMD175);
		pr_info("AW9120 shark mcu_control set current 17.5mA\n");
		break;
	default:
		pr_info("AW9120 shark mcu_control not set current.\n");
		break;
	}

	i2c_write_reg(led_data, CMDR,data);

	//i2c_write_reg(led_data, CMDR,0xbf00);
	//i2c_write_reg(led_data, CMDR,0x9f0f);
	//i2c_write_reg(led_data, CMDR,0xffff);
    return len;
}

//for color control
static ssize_t bslight_get_light_mode(struct device* dev, struct device_attribute *attr, char* buf)
{
    printk("bslight_tag bslight_get_light_mode light_mode_data = 0x%x\n",light_mode_data);
    return sprintf(buf,"%08x\n", light_mode_data);
}

static ssize_t bslight_set_light_mode(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
    unsigned reg = 0;
    int data = 0;
    int i=0;
    unsigned int databuf = 0;
    unsigned int data_fw[256] = {0};
    unsigned char hex_ch[8] = {0x00};
    struct aw9120_led *led_data = NULL;
    led_data = dev_get_drvdata(dev);

    for(i=0; i<(len/8); i++){
        memset(hex_ch, 0x00, 8);
        memcpy(hex_ch, buf + 8 * i, 8);
        sscanf(hex_ch,"%x",&databuf);

        data_fw[i] = databuf;
        if (light_mode_debug)
        {
            printk("bslight_tag display params  data[%d] = %x,len=%d\n",i,databuf,(int)len);
        }
    }
    data = data_fw[0];//mode
    display_r = data_fw[1];// color r
    display_g = data_fw[2];// color g
    display_b = data_fw[3];// color b

    printk("bslight_tag bslight_set_light_mode begin data = %x,display_r = %d,display_g = %d,display_b = %d, light_mode_data = %d\n",data,display_r,display_g,display_b,light_mode_data);

    if(data == LIGHT_SCENE_OFF)
    {
        if(light_mode_data != LIGHT_SCENE_OFF){
            thr_fn_time_stop();
        }else{
             light_mode_data = LIGHT_SCENE_OFF;
             return reg;
        }
    }else{

        light_mode_data = data;
        if (light_mode_debug)
        {
            printk("bslight_tag bslight_set_light_mode begin light_mode_data = %x\n",light_mode_data);
        }
        time_count =0;
        if(1){//data == 0xbf00
            i2c_write_reg(led_data, RSTR,0x55aa);
            //Disable LED Module
            reg = i2c_read_reg(led_data, GCR);
            reg &= 0xFFFE;
            i2c_write_reg(led_data, GCR, reg);
            reg |= 0x0001;
            i2c_write_reg(led_data, GCR,reg);// GCR-Enable LED Module
            i2c_write_reg(led_data, LER1,0x0eff);
            i2c_write_reg(led_data, LER2,0x000f);
            i2c_write_reg(led_data, CTRS1,0x0EFF); // CTRS1-LED1~LED12: mcu Control
            i2c_write_reg(led_data, CTRS2,0x000F); // CTRS2-LED13~LED20: mcu Control
            i2c_write_reg(led_data, IMAX1,0x2222);
            i2c_write_reg(led_data, IMAX2,0x2222);
            i2c_write_reg(led_data, IMAX3,0x2220);
            i2c_write_reg(led_data, IMAX4,0x2222);
            i2c_write_reg(led_data, CMDR,data);
            pr_info("bslight_tag mcu bf00\n");
        }

        task_time = kthread_run(thr_fn_time, NULL, "time_thread_%d", light_mode_data);

        if (IS_ERR(task_time)){
            printk("bslight_tag create thread task_time failed \n");
        }else{
            printk("bslight_tag success create thread task_time\n");
        }
    }

    return len;
}
static ssize_t bslight_get_light_mode_debug(struct device* dev, struct device_attribute *attr, char* buf)
{
    return sprintf(buf,"%x\n", light_mode_debug | light_mode_debug_fresh);
}

static ssize_t bslight_set_light_mode_debug(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
    int data = 0;
    struct aw9120_led *led_data = NULL;
    struct timespec now;
    struct tm ts;

    led_data = dev_get_drvdata(dev);
    now = current_kernel_time();
    time_to_tm(now.tv_sec, 0, &ts);
    ts.tm_year += 1970;
    ts.tm_mon += 1;

    printk("bslight_tag shark light_mode_debug begin %04ld-%02d-%02d %02d:%02d:%02d",ts.tm_year, ts.tm_mon, ts.tm_mday,ts.tm_hour, ts.tm_min, ts.tm_sec);
    printk("bslight_tag bslight_set_light_mode_debug  jiffies = [ %lu  ]  \n",jiffies);
    printk("bslight_tag bslight_set_light_mode_debug  tv_nsec = [ %lu  ]  \n",now.tv_nsec);
    printk("bslight_tag bslight_set_light_mode_debug  tv_sec = [ %lu  ]  \n",now.tv_sec);

    sscanf(buf, "%x", &data);
    if(data == 1){
        light_mode_debug = data;
    }else if (data == 2){
        light_mode_debug_fresh = data;
    }else if (data == 3){
        light_mode_debug_h = data;
    }else if (data == 4){
        light_mode_debug_s = data;
    }else if (data == 5){
        light_mode_debug_v = data;
    }else if (data == 6){
        light_mode_debug_r_g_b = data;
    }else{
        light_mode_debug = 0;
        light_mode_debug_fresh = 0;
        light_mode_debug_h = 0;
        light_mode_debug_s = 0;
        light_mode_debug_v = 0;
        light_mode_debug_r_g_b = 0;
    }

    printk("bslight_tag shark light_mode_debug begin data = %x\n",data);

    return len;
}
static ssize_t bslight_get_light_mode_music(struct device* dev, struct device_attribute *attr, char* buf)
{
     return sprintf(buf,"%x\n", music_volume);
}

static ssize_t bslight_set_light_mode_music(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{

    int data = 0;
    struct aw9120_led *led_data = NULL;
    led_data = dev_get_drvdata(dev);

    if(light_mode_data == LIGHT_SCENE_OFF)
    {
        if (light_mode_debug)
        {
            printk("bslight_tag light_mode_music error , mode if off ");
        }

         music_volume = 0;
         return -1;
    }

    sscanf(buf, "%x", &data);
    if (light_mode_debug)
    {
        printk("bslight_tag bslight_set_light_mode_music begin data = %x\n",data);
    }

    music_volume = data;

    return len;
}

static ssize_t bslight_get_light_mode_display(struct device* dev, struct device_attribute *attr, char* buf)
{
    return 0;
}
static ssize_t bslight_set_light_mode_display(struct device* dev, struct device_attribute *attr,const char* buf, size_t len)
{
    unsigned int databuf = 0;
    unsigned int data_fw[256] = {0};
    int i= 0;
    unsigned char hex_ch[4] = {0x00};
    ssize_t ret = -EINVAL;
    struct aw9120_led *led_data = NULL;
    led_data = dev_get_drvdata(dev);

    for(i=0; i<(len/4); i++){
        memset(hex_ch, 0x00, 4);
        memcpy(hex_ch, buf + 4 * i, 4);
        ret = sscanf(hex_ch,"%04x",&databuf);
        if(ret!=1)
            pr_err("bslight_tag display params failed ret = %ld\n",ret);

        data_fw[i] = databuf;
        if (light_mode_debug)
        {
            printk("bslight_tag display params _displayfrom hal data[%d] = 0x%x\n",i,databuf);
        }
    }
    display_peroid = data_fw[0];//peroid
    display_r = data_fw[1];// color r
    display_g = data_fw[2];// color g
    display_b = data_fw[3];// color b
    display_mode_flag = data_fw[4];// flag for normal mode or setting,0 setting ;1 normal;
    display_mode_scene = data_fw[5];

    if (light_mode_debug)
    {
        printk("bslight_tag display params _displayfrom hal display_peroid = %d, display_r = %d,display_g = %d,display_b = %d,display_mode_flag = %d,display_mode_scene = %d\n ",display_peroid,display_r,display_g,display_b,display_mode_flag,display_mode_scene);
    }
    return len;
}

static ssize_t bslight_set_boot_mode(void)
{
    unsigned reg = 0;
    int data = LIGHT_SCENE_BOOT_MODE_ONE ;
    struct aw9120_led *led_data = NULL;
    led_data = led_light_dev;
    if (light_mode_debug)
    {
        printk("bslight_tag bslight_set_boot_mode begin data = %x\n",data);
    }

    light_mode_data = data;

    time_count =0;

    {
        i2c_write_reg(led_data, RSTR,0x55aa);
        //Disable LED Module
        reg = i2c_read_reg(led_data, GCR);
        reg &= 0xFFFE;
        i2c_write_reg(led_data, GCR, reg);
        reg |= 0x0001;
        i2c_write_reg(led_data, GCR,reg);// GCR-Enable LED Module
        i2c_write_reg(led_data, LER1,0x0eff);
        i2c_write_reg(led_data, LER2,0x000f);
        i2c_write_reg(led_data, CTRS1,0x0EFF); // CTRS1-LED1~LED12: mcu Control
        i2c_write_reg(led_data, CTRS2,0x000F); // CTRS2-LED13~LED20: mcu Control
        i2c_write_reg(led_data, IMAX1,0x2222);
        i2c_write_reg(led_data, IMAX2,0x2222);
        i2c_write_reg(led_data, IMAX3,0x2220);
        i2c_write_reg(led_data, IMAX4,0x2222);
        i2c_write_reg(led_data, CMDR,data);
        pr_info("bslight_tag boot mode & mcu \n");
    }

    task_time = kthread_run(thr_fn_time, NULL, "time_thread_%d", light_mode_data);
    if (IS_ERR(task_time)){
        printk("bslight_tag create thread task_time for boot mode  failed \n");
    }else{
        printk("bslight_tag success create thread task_time for boot mode \n");
    }

    return 0;
}
//end for color control

static int aw9120_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_debug);
    if(err < 0)
    {
        pr_err("create debug attr error\n");
        goto debug_attr_error;
    }
	err = device_create_file(dev, &dev_attr_reg);
	if(err < 0)
	{
	    pr_err("create reg attr error\n");
        goto reg_attr_error;
	}
    err = device_create_file(dev, &dev_attr_modestate);
    if(err < 0)
    {
	    pr_err("create modestate attr error\n");
        goto modestate_attr_error;
    }
    err = device_create_file(dev, &dev_attr_fw_flag);
    if(err < 0)
    {
	    pr_err("create fw_flag attr error\n");
        goto fw_flag_attr_error;
    }
	err = device_create_file(dev, &dev_attr_fw_download);
    if(err < 0)
    {
        pr_err("create fw_download attr error\n");
        goto fw_download_attr_error;
    }
	err = device_create_file(dev, &dev_attr_calibration);
    if(err < 0)
    {
        pr_err("create calibration attr error\n");
        goto fw_download_attr_error;
    }
	err = device_create_file(dev, &dev_attr_mcu_control);
    if(err < 0)
    {
        pr_err("create mcu_control attr error\n");
        goto fw_download_attr_error;
    }
    //for color control
    err = device_create_file(dev, &dev_attr_light_mode);
    if(err < 0)
    {
        pr_err("create light_mode attr error\n");
        goto fw_download_attr_error;
    }
    err = device_create_file(dev, &dev_attr_bs_light_mode_debug);
    if(err < 0)
    {
        pr_err("create bs_light_mode_debug attr error\n");
        goto fw_download_attr_error;
    }
    err = device_create_file(dev, &dev_attr_bs_light_mode_music);
    if(err < 0)
    {
        pr_err("create dev_attr_bs_light_mode_music attr error\n");
        goto fw_download_attr_error;
    }
    err = device_create_file(dev, &dev_attr_bs_light_mode_display);
    if(err < 0)
    {
        pr_err("create bs_light_mode_display attr error\n");
        goto fw_download_attr_error;
    }
    //end for color control
	err = device_create_file(dev, &dev_attr_mmi_test);
    if(err < 0)
    {
        pr_err("create mcu_control attr error\n");
        goto fw_download_attr_error;
    }
	err = device_create_file(dev, &dev_attr_mmi_cali);
    if(err < 0)
    {
        pr_err("create mcu_control attr error\n");
        goto fw_download_attr_error;
    }



    return 0;
	
fw_download_attr_error:
    device_remove_file(dev, &dev_attr_fw_flag);
fw_flag_attr_error:
    device_remove_file(dev, &dev_attr_modestate);
modestate_attr_error:
    device_remove_file(dev, &dev_attr_reg);
reg_attr_error:
	device_remove_file(dev, &dev_attr_debug);
debug_attr_error:

	return err;
}

static void aw9120_brightness_work(struct work_struct *work)
{
    struct aw9120_platform_data *pdata = NULL;
    pdata = container_of(work, struct aw9120_platform_data, brightness_work);
    pr_debug("aw9120_brightness_work, brightness = %d, id=%d\n", pdata->cdev.brightness, pdata->id);

    if(pdata->cdev.brightness > pdata->cdev.max_brightness)
    {
        pdata->cdev.brightness = pdata->cdev.max_brightness;
    }

    mutex_lock(&pdata->led->lock);
	// LED SRAM Hold Mode
	i2c_write_reg(pdata->led, PMR,0x0000);		// PMR-Load SRAM with I2C
	i2c_write_reg(pdata->led, RMR,0x0000);		// RMR-Hold Mode

	// Load LED SRAM
	i2c_write_reg(pdata->led, WADDR,0x0000);	// WADDR-SRAM Load Addr

	i2c_write_reg(pdata->led, WDATA,(0xA000|(pdata->id<<8)|pdata->cdev.brightness));
    i2c_write_reg(pdata->led, WDATA,0x3400);

	// LED SRAM Run
	i2c_write_reg(pdata->led, SADDR,0x0000);	// SADDR-SRAM Run Start Addr:0
	i2c_write_reg(pdata->led, PMR,0x0001);		// PMR-Reload and Excute SRAM
	i2c_write_reg(pdata->led, RMR,0x0002);		// RMR-Run

    mutex_unlock(&pdata->led->lock);
}

static void aw9120_set_brightness(struct led_classdev *cdev,
			     enum led_brightness brightness)
{
	struct aw9120_platform_data *pdata = container_of(cdev, struct aw9120_platform_data, cdev);

	pdata->cdev.brightness = brightness;

	schedule_work(&pdata->brightness_work);
}

static void aw9120_led_parse_max_current(struct aw9120_platform_data * pdata, unsigned int led_current)
{
    if(NULL == pdata)
    {
        return;
    }

    switch(led_current)
    {
        case 0:
            pdata->max_current = max_current_0_0ma;
            break;
        case 3:
            pdata->max_current = max_current_3_5ma;
            break;
        case 7:
            pdata->max_current = max_current_7_0ma;
            break;
        case 10:
            pdata->max_current = max_current_10_5ma;
            break;
        case 14:
            pdata->max_current = max_current_14_0ma;
            break;
        case 17:
            pdata->max_current = max_current_17_5ma;
            break;
        case 21:
            pdata->max_current = max_current_21_0ma;
            break;
        case 24:
            pdata->max_current = max_current_24_5ma;
            break;
        default:
            pr_err("error current para,set default value(0mA)!\n");
            pdata->max_current = max_current_0_0ma;
            break;
    }
}

static int aw9120_led_parse_child_node(struct aw9120_led *led_data, struct device_node *node)
{
    int rc = -1, parsed_leds = 0,i = 0;
    unsigned int max_current_temp = 0;
    struct device_node *temp1,*temp2;
    struct aw9120_platform_data * pdata = NULL;

    if((NULL == led_data)||(NULL == node))
    {
        pr_err("error para!\n");
        return -EINVAL;
    }
    rc = of_property_read_u32(node, "aw9120,pwm-freq",
        &led_data->pwm_freq);
    if(rc < 0)
    {
        dev_err(&led_data->i2c_client->dev,
            "Failure reading led pwm-freq, rc = %d\n", rc);
        goto err;
    }
    rc = of_property_read_string(node, "aw9120,dimming-mode", &led_data->control_mode);
    if(rc < 0)
    {
        dev_err(&led_data->i2c_client->dev,
            "Failure reading led control mode, rc = %d\n", rc);
        goto err;
    }
    //get led number
    for_each_child_of_node(node, temp1)
    {
        led_data->num_leds += of_get_available_child_count(temp1);
    }
    if(!led_data->num_leds)
    {
        dev_err(&led_data->i2c_client->dev,
            "Get led number error\n");
        rc = -1;
        goto err;
    }
    else
    {
        pr_err("AW9120 led number:%d\n", led_data->num_leds);
    }
    //malloc memory for every led config
    led_data->led_array = devm_kzalloc(&led_data->i2c_client->dev,
				sizeof(struct aw9120_platform_data) * led_data->num_leds,
				GFP_KERNEL);
    if(!led_data)
    {
        pr_err("failed to allocate memory for led_array\n");
        rc = -ENOMEM;
        goto err;
    }
    for_each_child_of_node(node, temp1)
    {
        for_each_child_of_node(temp1, temp2)
        {
            pdata = &led_data->led_array[parsed_leds];
            pdata->led = led_data;
            rc = of_property_read_string(temp2, "aw9120,name",
    			&pdata->cdev.name);
    	    if(rc < 0)
    	    {
                dev_err(&led_data->i2c_client->dev,
                    "Failure reading led name, rc = %d\n", rc);
                    goto free_pdata;
    	    }
            rc = of_property_read_u32(temp2, "aw9120,id",
                &pdata->id);
    	    if(rc < 0)
    	    {
                dev_err(&led_data->i2c_client->dev,
                    "Failure reading led id, rc = %d\n", rc);
                    goto free_pdata;
    	    }
            rc = of_property_read_u32(temp2, "aw9120,max-brightness",
                &pdata->cdev.max_brightness);
    	    if(rc < 0)
    	    {
                dev_err(&led_data->i2c_client->dev,
                    "Failure reading led max-brightness, rc = %d\n", rc);
                    goto free_pdata;
    	    }
            rc = of_property_read_u32(temp2, "aw9120,max-current",
                &max_current_temp);
    	    if(rc < 0)
    	    {
                dev_err(&led_data->i2c_client->dev,
                    "Failure reading led max-current, rc = %d\n", rc);
                    goto free_pdata;
    	    }
            aw9120_led_parse_max_current(pdata, max_current_temp);

    	    INIT_WORK(&pdata->brightness_work, aw9120_brightness_work);
            pdata->cdev.brightness_set = aw9120_set_brightness;

            rc = led_classdev_register(&led_data->i2c_client->dev, &pdata->cdev);
            if(rc < 0)
            {
                dev_err(&led_data->i2c_client->dev,
                "unable to register led %d,rc=%d\n",
                    pdata->id, rc);
                goto classdev_unregister;
            }
            parsed_leds++;
        }
    }
    return 0;
classdev_unregister:
    for(i = 0; i < parsed_leds; i++)
    {
        led_classdev_unregister(&led_data->led_array[i].cdev);
        cancel_work_sync(&led_data->led_array[i].brightness_work);
    }
free_pdata:
    devm_kfree(&led_data->i2c_client->dev, led_data->led_array);
err:
    return rc;
}

static int parse_each_led_mode(unsigned char *str, struct aw9120_led *led_data, struct led_mode_list *mode_list_node)
{
    unsigned char *startIndex = NULL, *endIndex = NULL, *str_temp = NULL;
    unsigned char temp_buf[EACH_ASM_CODE_LEN] = {0};
    int led_mode_name_len = 0, i = 0;

    if(NULL == str)
    {
        pr_err("error para\n");
        return -1;
    }
    pr_debug("str:%s\n", str);
    str_temp = str;
    startIndex = strchr(str_temp, '[');
    led_mode_name_len = startIndex - str_temp;
    if(led_mode_name_len > EACH_MODE_NAME_MAX_LEN)
    {
        pr_err("led mode name is over buf\n");
        return -ENOMEM;
    }
    mode_list_node->led_mode_name = devm_kzalloc(&led_data->i2c_client->dev, led_mode_name_len + 1, GFP_KERNEL);
    if(!mode_list_node->led_mode_name)
    {
        pr_err("failed to allocate memory for led_mode_name\n");
        return -ENOMEM;
    }
    memcpy(mode_list_node->led_mode_name, str_temp, led_mode_name_len);
    mode_list_node->led_mode_name[led_mode_name_len] = '\0';

    endIndex = strchr(str_temp, ']');
    if(startIndex >= 0 && endIndex >= 0 && endIndex >= startIndex + 1 && endIndex <= startIndex + EACH_ASM_CODE_LEN + 1)
    {
        memcpy(temp_buf, startIndex + 1, endIndex - startIndex - 1);
        sscanf(temp_buf, "%d", &mode_list_node->TWO_BYTES_mode_code_count);
        pr_debug("mode_code_count:%d\n", mode_list_node->TWO_BYTES_mode_code_count);
        if((mode_list_node->TWO_BYTES_mode_code_count < 1) || (mode_list_node->TWO_BYTES_mode_code_count > 255))
        {
            pr_err("error mode_code_count:%d\n", mode_list_node->TWO_BYTES_mode_code_count);
            return -1;
        }
        mode_list_node->mode_code = devm_kzalloc(&led_data->i2c_client->dev,
            mode_list_node->TWO_BYTES_mode_code_count * sizeof(unsigned int), GFP_KERNEL);
        if(!mode_list_node->mode_code)
        {
            pr_err("failed to allocate memory for mode_code\n");
            return -ENOMEM;
        }
        for(i = 0; i < mode_list_node->TWO_BYTES_mode_code_count; i++)
        {
            if(0 == i)
            {
                startIndex = strchr(str_temp, '=') + 1;
                endIndex = strchr(str_temp, ',');
            }
            else if(mode_list_node->TWO_BYTES_mode_code_count - 1 == i)
            {
                startIndex = endIndex + 1;
                endIndex = strchr(startIndex, '.');
            }
            else
            {
                startIndex = endIndex + 1;
                endIndex = strchr(startIndex, ',');
            }

            if((endIndex <= startIndex + EACH_ASM_CODE_LEN + 1)&&(NULL != startIndex)&&(NULL != endIndex))
            {
                memcpy(temp_buf, startIndex, endIndex - startIndex);
            }
            else
            {
                pr_err("ASM code length is no right, endIndex=%lu, startIndex=%lu, i = %d\n",
                    (unsigned long)endIndex, (unsigned long)startIndex, i);
                return -1;
            }
            sscanf(temp_buf, "%x,", &mode_list_node->mode_code[i]);
            pr_debug("str=0x%X", mode_list_node->mode_code[i]);
            memset(temp_buf, 0, EACH_ASM_CODE_LEN);
        }
    }
    else
    {
        pr_err("mode_code_count parse error\n");
        return -1;
    }

    return 0;
}

static void aw9120_free_mode_list(struct aw9120_led *led_data)
{
    struct list_head *pos = NULL, *n = NULL;
    struct led_mode_list *p = NULL;
    pr_info("enter aw9120_free_mode_list\n");

    list_for_each_safe(pos, n, &led_data->led_mode_list_head.list)
    {
        list_del(pos);
        p = list_entry(pos, struct led_mode_list, list);
        if(p->led_mode_name)
        {
            devm_kfree(&led_data->i2c_client->dev, p->led_mode_name);
        }

        if(p->mode_code)
        {
            devm_kfree(&led_data->i2c_client->dev, p->mode_code);
        }
        if(p)
        {
            devm_kfree(&led_data->i2c_client->dev, p);
        }
    }
}

static int parse_led_workmode_from_fw(const struct firmware *firmware, struct aw9120_led *led_data)
{
    unsigned char *p = NULL, *dest_ch = NULL;
    int line_len = 0, parse_fw_size = 0, ret = 0, i = 0;
    unsigned char each_mode_temp_buf[EACH_MODE_BUF_LEN] = {0};
    struct led_mode_list *led_mode_list_temp = NULL;

    p = devm_kzalloc(&led_data->i2c_client->dev, firmware->size, GFP_KERNEL);
    if(!p)
    {
        pr_err("malloc firmware temp buffer error\n");
        return -ENOMEM;
    }
    memcpy(p, firmware->data, firmware->size);

    if(get_char_count('=', p, firmware->size) != (led_data->led_work_mode_count = get_char_count('.', p, firmware->size)))
    {
        pr_err("error fw\n");
        ret = -EBADF;
        goto fw_check_error;
    }
    pr_info("led_data->led_work_mode_count = %d\n", led_data->led_work_mode_count);
    for(i = 0; i < led_data->led_work_mode_count; i++)
    {
        dest_ch = strchr(p + parse_fw_size, '\n');

        if(NULL == dest_ch)
        {
            if(('.' == p[firmware->size - 1])&&(led_data->led_work_mode_count - 1 == i))
            {
                line_len = firmware->size - parse_fw_size;
            }
            else
            {
                pr_err("parse_fw_error, chr=%c, size=%d, count=%d, i=%d\n",
                    p[firmware->size - 1], (int)firmware->size, led_data->led_work_mode_count, i);
                ret = -EBADF;
                goto parse_fw_error;
            }
        }
        else
        {
            line_len = dest_ch - (p + parse_fw_size) + 1;
        }
        memcpy(each_mode_temp_buf, p + parse_fw_size, line_len);

        led_mode_list_temp = devm_kzalloc(&led_data->i2c_client->dev, sizeof(struct led_mode_list), GFP_KERNEL);
        if(!led_mode_list_temp)
        {
            pr_err("failed to allocate memory for led_mode_list_temp\n");
            ret = -ENOMEM;
            goto parse_fw_error;
        }
        ret = parse_each_led_mode(each_mode_temp_buf, led_data, led_mode_list_temp);
        if(ret < 0)
        {
            devm_kfree(&led_data->i2c_client->dev, led_mode_list_temp);
            goto parse_fw_error;
        }
        list_add_tail(&led_mode_list_temp->list, &led_data->led_mode_list_head.list);
        parse_fw_size += line_len;

        dest_ch = NULL;
        line_len = 0;
        memset(each_mode_temp_buf, 0, EACH_MODE_BUF_LEN);
    }

    devm_kfree(&led_data->i2c_client->dev, p);
    return 0;

parse_fw_error:
    aw9120_free_mode_list(led_data);
fw_check_error:
    devm_kfree(&led_data->i2c_client->dev, p);
    return ret;
}

static int fw_update(struct aw9120_led *led_data)
{
    int ret = 0;
    const struct firmware *firmware;

    if(fw_update_flag)
    {
        return ret;
    }

    ret = request_firmware(&firmware, "AW9120.ucf", &led_data->i2c_client->dev);
    if (ret < 0){
         pr_err("Firmware request failed, ret = %d\n", ret);
         goto request_firmware_error;
    }
    ret = parse_led_workmode_from_fw(firmware, led_data);
    release_firmware(firmware);
    if(ret < 0)
    {
        pr_err("parse_led_workmode_from_fw failed\n");
        goto request_firmware_error;
    }
    fw_update_flag = 1;
request_firmware_error:
    return ret;
}

static int aw9120_led_remove(struct i2c_client *client)
{

    struct aw9120_led *led_data = NULL;
    int i, parsed_leds;

	pr_info("aw9120_led_remove start!");
    led_data = i2c_get_clientdata(client);
    if(!led_data)
    {
        return 0;
    }
    aw9120_free_mode_list(led_data);
    parsed_leds = led_data->num_leds;

    for(i = 0; i < parsed_leds; i++)
    {
        led_classdev_unregister(&led_data->led_array[i].cdev);
        cancel_work_sync(&led_data->led_array[i].brightness_work);
    }
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_fw_flag);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_modestate);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_reg);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_debug);
	device_remove_file(&led_data->i2c_client->dev, &dev_attr_fw_download);
	device_remove_file(&led_data->i2c_client->dev, &dev_attr_calibration);
	device_remove_file(&led_data->i2c_client->dev, &dev_attr_mcu_control);
	device_remove_file(&led_data->i2c_client->dev, &dev_attr_mmi_test);
	device_remove_file(&led_data->i2c_client->dev, &dev_attr_mmi_cali);
    //for color control
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_light_mode);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_bs_light_mode_debug);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_bs_light_mode_music);
    device_remove_file(&led_data->i2c_client->dev, &dev_attr_bs_light_mode_display);
    //end for color control

    devm_kfree(&client->dev, led_data->led_array);
    led_data->led_array = NULL;
    devm_kfree(&client->dev, led_data);
    led_data = NULL;

    return 0;
}

static int aw9120_led_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i, ret, count = 0, reg_value = 0;
    struct aw9120_led *led_data = NULL;

	pr_debug("start!\n");
	if(NULL == client->dev.of_node)
	{
        return -EINVAL;
	}

    led_data = devm_kzalloc(&client->dev, sizeof(struct aw9120_led), GFP_KERNEL);
    if (!led_data) {
        pr_err("failed to allocate memory for led_data\n");
        ret = -ENOMEM;
        goto dev_kzalloc_error;
    }
    led_data->i2c_client = client;
    mutex_init(&led_data->lock);
    INIT_LIST_HEAD(&led_data->led_mode_list_head.list);

	//for color control
    led_light_dev = led_data;

    light_mode_param_dev = devm_kzalloc(&client->dev,sizeof(struct light_mode_param), GFP_KERNEL);
    if (!light_mode_param_dev) {
        pr_err("failed to allocate memory for led_data\n");
        ret = -ENOMEM;
        goto dev_kzalloc_error;
    }
	//end for color control

    ret = aw9120_led_parse_child_node(led_data, client->dev.of_node);
    if(ret < 0)
    {
        pr_err("failed to parse_child_node.\n");
        goto parse_child_node;
    }
	ret = aw9120_pinctrl_init(led_data);
	if (ret != 0) {
		pr_err("failed to init aw9120 pinctrl.\n");
		ret = -EIO;
		goto parse_pinctrl_error;
	} else {
		pr_err("Success to init aw9120 pinctrl.\n");
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ret = -ENODEV;
		goto parse_pinctrl_error;
	}

	pr_info(": i2c addr=%x\n", client->addr);
	
	aw9120_hw_on(led_data);
	dev_set_drvdata(&client->dev, led_data);
    i2c_set_clientdata(led_data->i2c_client, led_data);
	for(count = 0; count < 5; count++){
		reg_value = i2c_read_reg(led_data, 0x00);				//read chip ID
		pr_info("aw9120 chip ID = 0x%4x\n", reg_value);
		if (reg_value == 0xb223)
			break;
		msleep(5);
		if(count == 4) {
			pr_err("msg read id error\n");
			ret = -ENODEV;
			goto exit_id_check_error;
		}
	}
    aw9120_config_init(led_data);
	ret = aw9120_create_sysfs(led_data->i2c_client);
	if(ret < 0)
	{
        goto exit_id_check_error;
	}

	ret = sysfs_create_link(NULL, &led_data->i2c_client->dev.kobj, "rgb");
	if (ret){
		pr_err("Error %d creating rgb link\n",ret);
	}

#ifdef CONFIG_HW_DEV_DCT
		set_hw_dev_flag(DEV_PERIPHIAL_RGB);
#endif

    bslight_set_boot_mode();//show boot mode

	pr_debug("exit!\n");
    return 0;

exit_id_check_error:
	i2c_set_clientdata(led_data->i2c_client, NULL);
parse_pinctrl_error:
    for(i = 0; i < led_data->num_leds; i++)
    {
        led_classdev_unregister(&led_data->led_array[i].cdev);
        cancel_work_sync(&led_data->led_array[i].brightness_work);
    }
    devm_kfree(&led_data->i2c_client->dev, led_data->led_array);
parse_child_node:
    devm_kfree(&client->dev, led_data);
    led_data = NULL;
dev_kzalloc_error:
    return ret;
}


#ifdef CONFIG_PM_SLEEP
static int aw9120_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw9120_led *led_data = i2c_get_clientdata(client);

	pinctrl_select_state(led_data->aw9120ctrl, led_data->aw9120_pdn_low);

	return 0;
}

static int aw9120_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw9120_led *led_data = i2c_get_clientdata(client);

	pinctrl_select_state(led_data->aw9120ctrl, led_data->aw9120_pdn_high);
	msleep(5);
	aw9120_config_init(led_data);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(aw9120_pm, aw9120_suspend, aw9120_resume);

static const struct i2c_device_id aw9120_i2c_id[] = {
	{ AW9120_LED_NAME, 0 },{ }
};
MODULE_DEVICE_TABLE(i2c, aw9120_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id aw9120_i2c_of_match[] = {
	{.compatible = "awinic,aw9120_led"},
	{},
};
#endif

static struct i2c_driver aw9120_led_driver = {
	.probe		= aw9120_led_probe,
	.remove		= aw9120_led_remove,
	.id_table	= aw9120_i2c_id,
	.driver	= {
		.name	= AW9120_LED_NAME,
		.pm	= &aw9120_pm,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw9120_i2c_of_match,
#endif
	},
};

static int __init aw9120_led_init(void)
{
	int ret;
	pr_debug("Enter\n");
	
	ret = i2c_add_driver(&aw9120_led_driver);
	if (ret) {
		pr_err("Unable to register driver (%d)\n", ret);
		return ret;
	}

	pr_debug("Exit\n");

	return ret;
}

static void __exit aw9120_led_exit(void)
{
	pr_debug("Enter\n");
	i2c_del_driver(&aw9120_led_driver);
	pr_debug("Exit\n");

    //for color control
    if(task_time){
        kthread_stop(task_time);
        task_time = NULL;
    }

    if (light_mode_param_dev) {
        printk("bslight_tag kfree, light_mode_param_dev \n");
        kfree(light_mode_param_dev);
        light_mode_param_dev=NULL;
    }
	//end for color control

    printk(KERN_ALERT "Goodbye, cruel world\n");

}

module_init(aw9120_led_init);
module_exit(aw9120_led_exit);

MODULE_AUTHOR("<blackshark>");
MODULE_DESCRIPTION("AWINIC AW9120 LED Driver");
MODULE_LICENSE("GPL v2");
