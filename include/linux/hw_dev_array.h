#ifndef _HW_DEV_ARRAY_H_
#define _HW_DEV_ARRAY_H_
#include "hw_dev_dec.h"

typedef struct {
    int devices_id;
    char* devices_name;
}hw_dec_struct;

static hw_dec_struct hw_dec_device_array[] =
{
    { DEV_AUDIO_CODEC, "codec"},
    { DEV_PERIPHIAL_CAMERA_MAIN,"camera_main" },
    { DEV_PERIPHIAL_CAMERA_REARAUX,"camera_rearaux" },
    { DEV_PERIPHIAL_CAMERA_SLAVE,"camera_slave" },
    { DEV_PERIPHIAL_CAMERA_DSP,"camera_dsp" },
    { DEV_PERIPHIAL_CHARGER_MASTER, "charge_master"},
    { DEV_PERIPHIAL_CHARGER_SLAVE, "charge_slave"},
    { DEV_PERIPHIAL_BATTERY,"battery" },
    { DEV_PERIPHIAL_USB_SWITCH,"usb_switch"},
    { DEV_PERIPHIAL_LCD_DSP,"lcd_dsp"},
    { DEV_PERIPHIAL_RGB,"rgb"},
    { DEV_PERIPHIAL_TP, "tp"},
    { DEV_CONNECTIVITY_WIFI,"wifi" },
    { DEV_PERIPHIAL_MAX,"NULL" },
};

#endif
