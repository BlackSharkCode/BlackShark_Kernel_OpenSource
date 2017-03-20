#ifndef __LEDS_EXLIGHT_H_INCLUDED
#define __LEDS_EXLIGHT_H_INCLUDED

#define MAX_PARAM_COUNT                             5

#define CMD_TURN_OFF_LIGHT                          0
#define CMD_PLAY_LIGHT_SCRIPT                       1
#define CMD_PLAY_MUSIC                              2

#define EFFECT_FUNC_SLOW_AND_QUIET                  0
#define EFFECT_FUNC_HARMONY_AND_RHYTHM              1
#define EFFECT_FUNC_FREE_AND_JUMP                   2
#define EFFECT_FUNC_QUICKNESS_AND_SPIRITUALITY      3
#define EFFECT_FUNC_MUSIC                           4
#define EFFECT_FUNC_SHARK_MODE                      5
#define EFFECT_FUNC_GAME_PAD                        6
#define EFFECT_FUNC_TENCENT_TMGP_SGAME_LOADING      7
#define EFFECT_FUNC_TENCENT_TMGP_SGAME_KILL         8
#define EFFECT_FUNC_TENCENT_TMGP_SGAME_SPECIAL_KILL 9
#define EFFECT_FUNC_TENCENT_TMGP_SGAME_VICTORY      10

struct aw2015_led;
struct aw9120_led;

struct exlight_led_play_data {
    uint32_t start_time;
    uint32_t end_time;
    char r, g, b;
    // each effect name maps to a implement function
    int effect_func_id;
    void (*effect_func_entry)(uint32_t elapse, char *r, char *g, char *b);

    struct exlight_led_play_data *p_next;
};

struct exlight_led_data {
    int led_id;
    // This describes each effect section in led effect life cycle
    struct exlight_led_play_data *p_led_play_data_head;

    struct exlight_led_data *p_next;
};

struct exlight_data {
    char cmd;
    // This describes light effect lifecycle of each led
    struct exlight_led_data *p_led_data_head;
};

#ifdef EX_LIGHT_AW2015
static int exlight_create_sys_node(struct aw2015_led *led_array);
static void exlight_boot_up_light_on(struct aw2015_led *led_array);
#endif

#ifdef EX_LIGHT_AW9120_BULLHEAD
static int exlight_create_sys_node(struct aw9120_led *led_data);
static void exlight_boot_up_light_on(struct aw9120_led *led_data);
#endif


#endif    /* __LEDS_EXLIGHT_H_INCLUDED */
