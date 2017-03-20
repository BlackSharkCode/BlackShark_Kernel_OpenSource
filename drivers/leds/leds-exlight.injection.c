#include "leds-exlight.injection.h"

#define DEBUG_LOG                       false
#define DEBUG_ANIMATE_FRAME_LOG         false
#define TAG "exlight@kernel"
#define elog(format, arg...) printk(KERN_INFO"%s ERROR: " format, TAG, ##arg)
#define ilog(format, arg...) printk(KERN_INFO"%s: " format, TAG, ##arg)
#define dlog(format, arg...) if (DEBUG_LOG) printk(KERN_INFO"%s: " format, TAG, ##arg)
#define dflog(format, arg...) if (DEBUG_ANIMATE_FRAME_LOG) printk(KERN_INFO"%s: " format, TAG, ##arg)

static int g_animate_abort;
static int g_animate_frame_dur = 1000 / 60;

/** ************************ private function ************************ **/
static void led_enable(void *ptr) {
#ifdef EX_LIGHT_AW2015
    struct aw2015_led *led_data = (struct aw2015_led *)ptr;

    u8 val = 0;
    /* enable aw2015 if disabled */
    aw2015_read(led_data, AW2015_REG_GCR, &val);
    if (!(val & AW2015_LED_CHIP_ENABLE_MASK)) {
        aw2015_write(led_data, AW2015_REG_GCR, AW2015_LED_CHIP_ENABLE_MASK | AW2015_LED_CHARGE_DISABLE_MASK);
    }
    aw2015_write(led_data, AW2015_REG_LCFG1, AW2015_LED_ON_MODE_MASK);
    aw2015_write(led_data, AW2015_REG_LCFG2, AW2015_LED_ON_MODE_MASK);
    aw2015_write(led_data, AW2015_REG_LCFG3, AW2015_LED_ON_MODE_MASK);
    aw2015_write(led_data, AW2015_REG_IMAX, led_data->pdata->imax);
    aw2015_write(led_data, AW2015_REG_PWM1, AW2015_LED_ON_PWM_MASK);
    aw2015_write(led_data, AW2015_REG_PWM2, AW2015_LED_ON_PWM_MASK);
    aw2015_write(led_data, AW2015_REG_PWM3, AW2015_LED_ON_PWM_MASK);
    aw2015_write(led_data, AW2015_REG_LEDEN, AW2015_LED_ALL_MASK);
#endif
}

static void led_disable(void *ptr) {
#ifdef EX_LIGHT_AW2015
    struct aw2015_led *led_data = (struct aw2015_led *)ptr;

    aw2015_write(led_data, AW2015_REG_PWM1, 0);
    aw2015_write(led_data, AW2015_REG_PWM2, 0);
    aw2015_write(led_data, AW2015_REG_PWM3, 0);
    aw2015_write(led_data, AW2015_REG_ILED1, 0);
    aw2015_write(led_data, AW2015_REG_ILED2, 0);
    aw2015_write(led_data, AW2015_REG_ILED3, 0);
#endif
}

static void led_light_on(void *ptr, char r, char g, char b) {
#ifdef EX_LIGHT_AW2015
    struct aw2015_led *dev_led_data = (struct aw2015_led *)ptr;
    aw2015_write(dev_led_data, AW2015_REG_ILED2, r);    //red
    aw2015_write(dev_led_data, AW2015_REG_ILED1, g);    //green
    aw2015_write(dev_led_data, AW2015_REG_ILED3, b);    //blue
#endif
}


/**
 *    .- (1/1000)x        (0<=x<1000)
 * y =|
 *    `- -(1/1000)x+2     (1000<=x<2000)
 */
static void effect_func_slow_and_quiet(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_1000;
    elapse = elapse % 2000;
    if (elapse >= 0 && elapse < 1000) {
        brightness_1000 = elapse;
    } else if (elapse >= 1000 && elapse <= 2000) {
        brightness_1000 = -elapse + 2000;
    }

    *r = (brightness_1000 * (*r)) / 1000;
    *g = (brightness_1000 * (*g)) / 1000;
    *b = (brightness_1000 * (*b)) / 1000;
}

/**
 *     .- (1/330)x              (0<=x<330)
 * y = |  -(1/330)x + 2         (330<x<=660)
 *     `- 0                     (660<x<=1000)
 */
static void effect_func_harmony_and_rhythm(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_330 ;
    elapse = elapse % 1000;
    if (elapse >= 0 && elapse < 330) {
        brightness_330 = elapse;
    } else if (elapse >= 330 && elapse <= 660) {
        brightness_330 = -elapse + 660;
    } else if (elapse > 660 && elapse <= 1000) {
        brightness_330 = 0;
    }
    *r = (brightness_330 * (*r)) / 330;
    *g = (brightness_330 * (*g)) / 330;
    *b = (brightness_330 * (*b)) / 330;
}

/**
 *     .- (1/160)x              (0<=x<160)
 * y = |  -(1/160)x + 2         (160<x<=320)
 *     `- 0                     (320<x<=420)
 */
static void effect_func_free_and_jump(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_160 = 0;
    elapse = elapse % 420;
    if (elapse >= 0 && elapse < 160) {
        brightness_160 = elapse;
    } else if (elapse >= 160 && elapse <= 320) {
        brightness_160 = -elapse + 320;
    } else if (elapse > 320 && elapse <= 420) {
        brightness_160 = 0;
    }
    *r = (brightness_160 * (*r)) / 160;
    *g = (brightness_160 * (*g)) / 160;
    *b = (brightness_160 * (*b)) / 160;
}

/**
 *    .- (1/200)x        (0<=x<200)
 * y =|
 *    `- -(1/200)x+2     (200<=x<400)
 */
static void effect_func_quickness_and_spirituality(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_200;
    elapse = elapse % 400;
    if (elapse >= 0 && elapse < 200) {
        brightness_200 = elapse;
    } else if (elapse >= 200 && elapse <= 400) {
        brightness_200 = -elapse + 400;
    }

    *r = (brightness_200 * (*r)) / 200;
    *g = (brightness_200 * (*g)) / 200;
    *b = (brightness_200 * (*b)) / 200;
}

/**
 * y = x
 */
static void effect_func_music(uint32_t elapse, char *r, char *g, char *b) {
    *r = *r;
    *g = *g;
    *b = *b;
}

/**
 *    .- (1/120)x                               (0<=x<120)
 *    |  1                                      (120<=x<420)
 *    |  -(x-420)/180+1                         (420<=x<600)
 *    |  (x-600)/140                            (600<=x<740)
 *    |  -(x-740)/140+1                         (740<=x<880)
 *    |  (x-880)/140                            (880<=x<1020)
 *    |  -(x-1020)/140+1                        (1020<=x<1160)
 * y =|  (x-1160)/140                           (1160<=x<1300)
 *    |  -(x-1300)/140+1                        (1300<=x<1440)
 *    |  (x-1440)/140                           (1440<=x<1580)
 *    |  -(x-1580)/140+1                        (1580<=x<1720)
 *    |  (x-1720)/140                           (1720<=x<1860)
 *    |  1                                      (1860<=x<2160)
 *    `- -(x-2160)/400+1                        (2160<=x<2560)
 */
static void effect_func_shark_space(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div;

    elapse = elapse % 2560;
    if (0 <= elapse && elapse < 120) {
        div = 120;
        brightness_div = elapse;
    } else if (120 <= elapse && elapse < 420) {
        div = 1;
        brightness_div = 1;
    } else if (420 <= elapse && elapse < 600) {
        div = 180;
        brightness_div = -elapse + 600;
    } else if (600 <= elapse && elapse < 740) {
        div = 140;
        brightness_div = elapse - 600;
    } else if (740 <= elapse && elapse < 880) {
        div = 140;
        brightness_div = -elapse + 880;
    } else if (880 <= elapse && elapse < 1020) {
        div = 140;
        brightness_div = elapse - 880;
    } else if (1020 <= elapse && elapse < 1160) {
        div = 140;
        brightness_div = -elapse + 1160;
    } else if (1160 <= elapse && elapse < 1300) {
        div = 140;
        brightness_div = elapse - 1160;
    } else if (1300 <= elapse && elapse < 1440) {
        div = 140;
        brightness_div = -elapse + 1440;
    } else if (1440 <= elapse && elapse < 1580) {
        div = 140;
        brightness_div = elapse - 1440;
    } else if (1580 <= elapse && elapse < 1720) {
        div = 140;
        brightness_div = -elapse + 1720;
    } else if (1720 <= elapse && elapse < 1860) {
        div = 140;
        brightness_div = elapse - 1720;
    } else if (1860 <= elapse && elapse < 2160) {
        div = 1;
        brightness_div = 1;
    } else if (2160 <= elapse && elapse <= 2560) {
        div = 400;
        brightness_div = -elapse + 2560;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}

/**
 *    .- (1/80)x            (0<=x<80)
 * y =|  1                  (80<=x<160)
 *    `- -(x-160)/80+1      (160<=x<240)
 */
static void effect_func_game_pad(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div;

    elapse = elapse % 240;
    if (0 <= elapse && elapse < 80) {
        div = 80;
        brightness_div = elapse;
    } else if (80 <= elapse && elapse < 160) {
        div = 1;
        brightness_div = 1;
    } else if (160 <= elapse && elapse < 240) {
        div = 80;
        brightness_div = -elapse + 240;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}

/**
 *    .- -(1/1000)x+1       (0<=x<1000)
 * y =|
 *    `- (1/1000)x        (1000<=x<2000)
 */
static void effect_func_tencent_tmgp_sgame_loading(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div = 1000;

    elapse = elapse % 2000;
    if (0 <= elapse && elapse < 1000) {
        brightness_div = -elapse + 1000;
    } else if (1000 <= elapse && elapse < 2000) {
        brightness_div = elapse;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}

/**
 *    .- (1/250)x           (0<=x<250)
 * y =|  1                  (250<=x<1500)
 *    `- -(x-1500)/500+1      (1500<=x<2000)
 */
static void effect_func_tencent_tmgp_sgame_kill(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div;

    elapse = elapse % 2000;
    if (0 <= elapse && elapse < 250) {
        div = 250;
        brightness_div = elapse;
    } else if (250 <= elapse && elapse < 1500) {
        div = 1;
        brightness_div = 1;
    } else if (1500 <= elapse && elapse < 2000) {
        div = 500;
        brightness_div = -elapse + 2000;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}

/**
 *    .- -(1/125)x              (0<=x<125)
 *    |  (x-125)/125            (125<=x<250)
 *    |  1                      (250<=x<375)
 *    |  -(x-375)/125+1         (375<=x<500)
 * y =|  (x-500)/125            (500<=x<625)
 *    |  1                      (625<=x<750)
 *    |  -(x-750)/125+1         (750<=x<875)
 *    |  (x-875)/125            (875<=x<1000)
 *    `- -(x-1000)/700+1        (1000<=x<1700)
 */
static void effect_func_tencent_tmgp_sgame_special_kill(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div;

    elapse = elapse % 1700;
    if (0 <= elapse && elapse < 125) {
        div = 125;
        brightness_div = -elapse;
    } else if (125 <= elapse && elapse < 250) {
        div = 125;
        brightness_div = elapse - 125;
    } else if (250 <= elapse && elapse < 375) {
        div = 1;
        brightness_div = 1;
    } else if (375 <= elapse && elapse < 500) {
        div = 125;
        brightness_div = -elapse + 500;
    } else if (500 <= elapse && elapse < 625) {
        div = 125;
        brightness_div = elapse - 500;
    } else if (625 <= elapse && elapse < 750) {
        div = 1;
        brightness_div = 1;
    } else if (750 <= elapse && elapse < 875) {
        div = 125;
        brightness_div = -elapse + 875;
    } else if (875 <= elapse && elapse < 1000) {
        div = 125;
        brightness_div = elapse - 875;
    } else if (1000 <= elapse && elapse < 1700) {
        div = 700;
        brightness_div = -elapse + 1700;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}

/**
 *    .- (1/160)x               (0<=x<160)
 *    |  -(x-160)/160+1         (160<=x<320)
 *    |  (x-320)/160            (320<=x<480)
 *    |  -(x-480)/160+1         (480<=x<640)
 *    |  (x-640)/160            (640<=x<800)
 *    `- -(x-800)/160+1        (800<=x<960)
 */
static void effect_func_tencent_tmgp_sgame_victory(uint32_t elapse, char *r, char *g, char *b) {
    uint32_t brightness_div;
    int div = 160;

    elapse = elapse % 960;
    if (0 <= elapse && elapse < 160) {
        brightness_div = elapse;
    } else if (160 <= elapse && elapse < 320) {
        brightness_div = -elapse + 320;
    } else if (320 <= elapse && elapse < 480) {
        brightness_div = elapse - 320;
    } else if (480 <= elapse && elapse < 640) {
        brightness_div = -elapse + 640;
    } else if (640 <= elapse && elapse < 800) {
        brightness_div = elapse - 640;
    } else if (800 <= elapse && elapse < 960) {
        brightness_div = -elapse + 960;
    }

    *r = (brightness_div * (*r)) / div;
    *g = (brightness_div * (*g)) / div;
    *b = (brightness_div * (*b)) / div;
}


static void str_append(char **str, char *apdstr) {
    int len = strlen(apdstr);
    int i = 0;
    for (; i < len; i++) {
        *((*str)++) = *(apdstr + i);
    }
}

static void dump_exlight_data(struct exlight_data *p_exlight_data) {
    char buf[64] = {0};
    char dump_info[1024] = {0};
    char *pstr = dump_info;
    struct exlight_led_data *ptr;
    struct exlight_led_play_data *ptr1;

    str_append(&pstr, " exlight_data:");
    sprintf(buf, " cmd: %d", p_exlight_data->cmd); str_append(&pstr, buf);

    dlog("exlight_data[%p]:\n", p_exlight_data);
    dlog(" cmd: %d\n", p_exlight_data->cmd);

    ptr = p_exlight_data->p_led_data_head;
    while (ptr != 0) {
        str_append(&pstr, " exlight_led_data:");
        sprintf(buf, " led_id: %d", ptr->led_id); str_append(&pstr, buf);

        dlog("  exlight_led_data[%p]:\n", ptr);
        dlog("    led_id: %d\n", ptr->led_id);
        dlog("    p_next: %p\n", ptr->p_next);

        ptr1 = ptr->p_led_play_data_head;
        while (ptr1 != 0) {
            str_append(&pstr, " exlight_led_play_data:");
            sprintf(buf, " start_time: %d", ptr1->start_time); str_append(&pstr, buf);
            sprintf(buf, " end_time: %d", ptr1->end_time); str_append(&pstr, buf);
            sprintf(buf, " r:%d g:%d b:%d", ptr1->r, ptr1->g, ptr1->b); str_append(&pstr, buf);
            sprintf(buf, " effect_func_id: %d", ptr1->effect_func_id); str_append(&pstr, buf);

            dlog("    exlight_led_play_data[%p]:\n", ptr1);
            dlog("      start_time: %d\n", ptr1->start_time);
            dlog("      end_time: %d\n", ptr1->end_time);
            dlog("      r:%d g:%d b:%d\n", ptr1->r, ptr1->g, ptr1->b);
            dlog("      effect_func_id: %d\n", ptr1->effect_func_id);
            dlog("      p_next: %p\n", ptr1->p_next);
            ptr1 = ptr1->p_next;
        }
        ptr = ptr->p_next;
    }

    ilog("dump_info: %s", dump_info);
}

static void free_exlight_data(struct exlight_data *data) {
    struct exlight_led_data *p_led_data;
    struct exlight_led_data *p_free_led_data;
    struct exlight_led_play_data *p_led_play_data;
    struct exlight_led_play_data *p_free_led_play_data;

    p_led_data = data->p_led_data_head;
    while (p_led_data != 0) {
        p_led_play_data = p_led_data->p_led_play_data_head;
        while (p_led_play_data != 0) {
            p_free_led_play_data = p_led_play_data;
            p_led_play_data = p_led_play_data->p_next;
            dlog("free led_play_data[%p]\n", p_free_led_play_data);
            kfree(p_free_led_play_data);
        }
        p_free_led_data = p_led_data;
        p_led_data = p_led_data->p_next;
        dlog("free led_data[%p]\n", p_free_led_data);
        kfree(p_free_led_data);
    }
    dlog("free data[%p]\n", data);
    kfree(data);
}

static void get_exlight_data_from_user(struct exlight_data **data, const char *buf, size_t len) {
    int i;
    void __user *user_buf;
    unsigned long p_data = 0;
    struct exlight_data *_data;

    struct exlight_led_data **pp_led_data;
    struct exlight_led_data *p_led_data;

    struct exlight_led_play_data **pp_led_play_data;
    struct exlight_led_play_data *p_led_play_data;

    // convert buf to unsigned long ptr
    for (i = len - 1; i >= 0; i--) {
        p_data |= ((unsigned long) buf[i] & 0xFF) << i * 8;
    }

    // alloc mem in kernel space
    _data = (struct exlight_data *) kzalloc(sizeof(struct exlight_data), GFP_KERNEL);
    if (!_data) {
        elog("exlight_control_set kzalloc failed, _data: %lu\n", (unsigned long) _data);
        kfree(_data);
        return;
    }

    // recover exlight_data from user space
    user_buf = (void __user *)p_data;
    if (copy_from_user(_data, user_buf, sizeof(struct exlight_data))) {
        elog("copy_from_user failed of exlight_data[%p]\n", user_buf);
    }

    pp_led_data = &_data->p_led_data_head;
    while (*pp_led_data != 0) {
        p_led_data = (struct exlight_led_data *) kzalloc(sizeof(struct exlight_led_data), GFP_KERNEL);
        if (copy_from_user(p_led_data, *pp_led_data, sizeof(struct exlight_led_data))) {
            elog("copy_from_user failed of exlight_led_data[%p]\n", *pp_led_data);
            goto loop_end;
        }

        pp_led_play_data = &p_led_data->p_led_play_data_head;
        while (*pp_led_play_data != 0) {
            p_led_play_data = (struct exlight_led_play_data *) kzalloc(sizeof(struct exlight_led_play_data), GFP_KERNEL);
            if (copy_from_user(p_led_play_data, *pp_led_play_data, sizeof(struct exlight_led_play_data))) {
                elog("copy_from_user failed of exlight_led_play_data[%p]\n", *pp_led_play_data);
                goto loop_end;
            }

            // set effect func entry for convenience
            switch (p_led_play_data->effect_func_id){
                case EFFECT_FUNC_SLOW_AND_QUIET:
                    p_led_play_data->effect_func_entry = effect_func_slow_and_quiet;
                    break;
                case EFFECT_FUNC_HARMONY_AND_RHYTHM:
                    p_led_play_data->effect_func_entry = effect_func_harmony_and_rhythm;
                    break;
                case EFFECT_FUNC_FREE_AND_JUMP:
                    p_led_play_data->effect_func_entry = effect_func_free_and_jump;
                    break;
                case EFFECT_FUNC_QUICKNESS_AND_SPIRITUALITY:
                    p_led_play_data->effect_func_entry = effect_func_quickness_and_spirituality;
                    break;
                case EFFECT_FUNC_MUSIC:
                    p_led_play_data->effect_func_entry = effect_func_music;
                    break;
                case EFFECT_FUNC_SHARK_MODE:
                    p_led_play_data->effect_func_entry = effect_func_shark_space;
                    break;
                case EFFECT_FUNC_GAME_PAD:
                    p_led_play_data->effect_func_entry = effect_func_game_pad;
                    break;
                case EFFECT_FUNC_TENCENT_TMGP_SGAME_LOADING:
                    p_led_play_data->effect_func_entry = effect_func_tencent_tmgp_sgame_loading;
                    break;
                case EFFECT_FUNC_TENCENT_TMGP_SGAME_KILL:
                    p_led_play_data->effect_func_entry = effect_func_tencent_tmgp_sgame_kill;
                    break;
                case EFFECT_FUNC_TENCENT_TMGP_SGAME_SPECIAL_KILL:
                    p_led_play_data->effect_func_entry = effect_func_tencent_tmgp_sgame_special_kill;
                    break;
                case EFFECT_FUNC_TENCENT_TMGP_SGAME_VICTORY:
                    p_led_play_data->effect_func_entry = effect_func_tencent_tmgp_sgame_victory;
                    break;
            }

            *pp_led_play_data = p_led_play_data;
            pp_led_play_data = &p_led_play_data->p_next;
        }

        *pp_led_data = p_led_data;
        pp_led_data = &p_led_data->p_next;
    }
    loop_end:

    if (1 || DEBUG_LOG) dump_exlight_data(_data);
    *data = _data;
}

static uint32_t get_current_time_mills(void) {
    struct timeval tv;
    do_gettimeofday(&tv);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void do_animate(struct aw2015_led *dev_led_data, struct exlight_data *data) {
    int is_animation_over;
    uint32_t start_time;
    uint32_t current_time;
    uint32_t elapse;
    char r, g, b;

    int to_sleep_dur;
    uint32_t frame_start_time = -1;

    struct exlight_led_data *p_exlight_led_data;
    struct exlight_led_play_data *p_exlight_led_play_data;

    led_enable(dev_led_data);
    start_time = get_current_time_mills();
    is_animation_over = 0;
    g_animate_abort = 0;

    while (!is_animation_over) {
        is_animation_over = 1;
        do { // loop for each leds
            p_exlight_led_data = data->p_led_data_head;
            do { // loop for each effect of one led
                p_exlight_led_play_data = p_exlight_led_data->p_led_play_data_head;
                current_time = get_current_time_mills();
                elapse = current_time - start_time;
                if (elapse >= p_exlight_led_play_data->start_time && elapse <= p_exlight_led_play_data->end_time) {
                    r = p_exlight_led_play_data->r;
                    g = p_exlight_led_play_data->g;
                    b = p_exlight_led_play_data->b;
                    p_exlight_led_play_data->effect_func_entry(elapse - p_exlight_led_play_data->start_time, &r, &g, &b);
                    dflog("led[%d] light on [r:%d g:%d b:%d] at elapse time %d ms [end_time: %d]\n", p_exlight_led_data->led_id, r, g, b,
                           (int) (elapse - p_exlight_led_play_data->start_time), (int) p_exlight_led_play_data->end_time);

                    led_light_on(dev_led_data, r, g, b);

                    if (frame_start_time == -1) {
                        to_sleep_dur = 0;
                    } else {
                        to_sleep_dur = g_animate_frame_dur - (get_current_time_mills() - frame_start_time);
                    }

                    dflog("to_sleep_dur: %d ms\n", to_sleep_dur);
                    if (to_sleep_dur > 0) {
                        msleep(to_sleep_dur);
                    }
                    frame_start_time = get_current_time_mills();

                    is_animation_over = 0;
                }
                if (g_animate_abort) {
                    ilog("do_animate g_animate_abort: %d\n", g_animate_abort);
                    goto animate_abort;
                }
            } while (p_exlight_led_play_data->p_next != 0);
        } while (p_exlight_led_data->p_next != 0);
    }

animate_abort:
    ilog("led_disable, g_animate_abort: %d\n", g_animate_abort);
    led_disable(dev_led_data);
}

static void do_music_animate(struct aw2015_led *dev_led_data, struct exlight_data *data) {
    struct exlight_led_data *p_exlight_led_data;
    struct exlight_led_play_data *p_exlight_led_play_data;

    led_enable(dev_led_data);

    p_exlight_led_data = data->p_led_data_head;
    p_exlight_led_play_data = p_exlight_led_data->p_led_play_data_head;

    led_light_on(dev_led_data, p_exlight_led_play_data->r, p_exlight_led_play_data->g, p_exlight_led_play_data->b);
}


/** ************************ DEVICE_ATTR define ************************ **/
static ssize_t exlight_data_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return snprintf(buf, PAGE_SIZE, "aw2015_led_time_show \n");
}

static ssize_t exlight_data_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t len) {
    struct exlight_data *data = 0;
    struct aw2015_led *dev_led_data = dev_get_drvdata(dev);

    ilog("exlight_data_set\n");

    // exlight_data is malloced here.
    get_exlight_data_from_user(&data, buf, len);

    // do led animation
    if (data->cmd == CMD_PLAY_LIGHT_SCRIPT) {
        do_animate(dev_led_data, data);
    } else if (data->cmd == CMD_PLAY_MUSIC) {
        do_music_animate(dev_led_data, data);
    }

    // important!!! free exlight_data
    free_exlight_data(data);
    return len;
}

static ssize_t exlight_control_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return snprintf(buf, PAGE_SIZE, "g_animate_abort: %d\n", g_animate_abort);
}

static ssize_t exlight_control_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t len) {
    struct aw2015_led *dev_led_data = dev_get_drvdata(dev);

    ilog("exlight_control_set\n");

    g_animate_abort = 1;
    led_disable(dev_led_data);
    return len;
}

static DEVICE_ATTR(exlight_data, 0664, exlight_data_show, exlight_data_set);
static DEVICE_ATTR(exlight_control, 0664, exlight_control_show, exlight_control_set);

/** ************************ public function ************************ **/
#ifdef EX_LIGHT_AW2015
static int exlight_create_sys_node(struct aw2015_led *led_array) {
    int ret = -EINVAL;
    ret = device_create_file(&led_array->client->dev, &dev_attr_exlight_control);
    if (ret < 0) {
        pr_err("create exlight_control attr error\n");
    }

    ret = device_create_file(&led_array->client->dev, &dev_attr_exlight_data);
    if (ret < 0) {
        pr_err("create exlight_data attr error\n");
    }
    return ret;
}

static void exlight_boot_up_light_on(struct aw2015_led *led_data) {
    led_enable(led_data);
    led_light_on(led_data, 0, 255, 0);
}
#endif

#ifdef EX_LIGHT_AW9120_BULLHEAD
static int exlight_create_sys_node(struct aw9120_led *led_data) {
    int ret = -EINVAL;
    ret = device_create_file(&led_data->i2c_client->dev, &dev_attr_exlight_control);
    if (ret < 0) {
        pr_err("create exlight_control attr error\n");
    }

    ret = device_create_file(&led_data->i2c_client->dev, &dev_attr_exlight_data);
    if (ret < 0) {
        pr_err("create exlight_data attr error\n");
    }
    return ret;
}

static void exlight_boot_up_light_on(struct aw9120_led *led_data) {
}
#endif
