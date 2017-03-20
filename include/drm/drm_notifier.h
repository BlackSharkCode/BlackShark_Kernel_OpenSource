#ifndef _DRM_NOTIFIER_H_
#define _DRM_NOTIFIER_H_

/**
 * This minimalistic include file is intended for touch panel that to receive
 * blank/unblank event.
 */

#define	DRM_EARLY_EVENT_BLANK 	0x01
#define	DRM_EVENT_BLANK		0x02
#define	DRM_R_EARLY_EVENT_BLANK 0x03

#define DRM_BLANK_UNBLANK	1
#define DRM_BLANK_NORMAL	2
#define DRM_BLANK_POWERDOWN	3
#define DRM_BLANK_LCDOFF	4


struct drm_notify_data {
	bool is_primary;
	void *data;
};

extern int drm_register_client(struct notifier_block *nb);
extern int drm_unregister_client(struct notifier_block *nb);
extern int drm_notifier_call_chain(unsigned long val, void *v);

#endif
