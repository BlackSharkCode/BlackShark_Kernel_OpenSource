#ifndef DSI_IRIS2P_API_H
#define DSI_IRIS2P_API_H
#include "dsi_panel.h"

extern void iris_info_init(struct dsi_panel *panel);
extern void iris_pre_lightup(struct dsi_panel *panel);
extern void iris_lightup(struct dsi_panel *panel);

extern void iris2p_register_fs(void);
extern void iris2p_unregister_fs(void);
extern void iris_cmd_kickoff_proc(void);
extern void iris_drm_device_handle_get(struct drm_device *drm_dev);
extern u32 iris_hdr_enable_get(void);
extern void iris_firmware_download_cont_splash(struct dsi_panel *panel);
extern int get_iris_status(void);
extern int notify_iris_esd_exit(void);
extern void iris_lightdown(struct dsi_panel *panel);
#endif



