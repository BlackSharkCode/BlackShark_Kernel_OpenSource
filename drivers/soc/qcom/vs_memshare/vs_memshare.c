/* Copyright (c) 2013-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/notifier.h>
#include <soc/qcom/subsystem_restart.h>
#include <soc/qcom/subsystem_notif.h>
#include <soc/qcom/msm_qmi_interface.h>
#include <soc/qcom/scm.h>
#include "vs_memshare.h"
#include "vendor_specific_memory_sharing_v01.h"

#include <soc/qcom/secure_buffer.h>
#include <soc/qcom/ramdump.h>

/* Macros */
#define VS_MEMSHARE_DEV_NAME "vs_memshare"
#define VS_MEMSHARE_CHILD_DEV_NAME "vs_memshare_child"
static unsigned long(attrs);

extern char __initdata boot_command_line[];
static uint32_t oeminfo_boardid = 0;
static char oeminfo_country[2] = {0};
static char oeminfo_carrier[2] = {0};
static char oeminfo_saleschannel[2] = {0};
static char oeminfo_target[2] = {0};

static struct qmi_handle *vs_mem_share_svc_handle;
static void vs_mem_share_svc_recv_msg(struct work_struct *work);
static DECLARE_DELAYED_WORK(vs_work_recv_msg, vs_mem_share_svc_recv_msg);
static struct workqueue_struct *vs_mem_share_svc_workqueue;
static uint64_t bootup_request;
static bool ramdump_event;
static void *vs_memshare_ramdump_dev[MAX_CLIENTS];

struct vs_memshare_client_map {
	char name[20];
	int pid;
};

/* Memshare Driver Structure */
struct vs_memshare_driver {
	struct cdev cdev;
	dev_t devno;
	struct device *memshare_dev;
	struct class *memshare_class;
	struct mutex memshare_mutex;
	struct wakeup_source pa_ws;

	int num_clients;
	struct vs_memshare_client_map *client_map;

	struct device *dev;
	struct mutex mem_share;
	struct mutex mem_free;
	struct work_struct vs_memshare_init_work;
};

struct vs_memshare_child {
	struct device *dev;
};

static struct vs_memshare_driver *vs_memsh_drv;
static struct vs_memshare_child *vs_memsh_child;
static struct vs_mem_blocks vs_memblock[MAX_CLIENTS];
static uint32_t num_clients;
static struct msg_desc vs_mem_share_svc_alloc_generic_req_desc = {
	.max_msg_len = VS_MS_MEM_ALLOC_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_ALLOC_GENERIC_REQ_MSG_V01,
	.ei_array = vs_ms_mem_alloc_generic_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_alloc_generic_resp_desc = {
	.max_msg_len = VS_MS_MEM_ALLOC_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_ALLOC_GENERIC_RESP_MSG_V01,
	.ei_array = vs_ms_mem_alloc_generic_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_free_generic_req_desc = {
	.max_msg_len = VS_MS_MEM_FREE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_FREE_GENERIC_REQ_MSG_V01,
	.ei_array = vs_ms_mem_free_generic_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_free_generic_resp_desc = {
	.max_msg_len = VS_MS_MEM_FREE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_FREE_GENERIC_RESP_MSG_V01,
	.ei_array = vs_ms_mem_free_generic_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_size_query_req_desc = {
	.max_msg_len = VS_MS_MEM_FREE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_QUERY_SIZE_REQ_MSG_V01,
	.ei_array = vs_ms_mem_query_size_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_size_query_resp_desc = {
	.max_msg_len = VS_MS_MEM_FREE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_MEM_QUERY_SIZE_RESP_MSG_V01,
	.ei_array = vs_ms_mem_query_size_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_stat_req_desc = {
	.max_msg_len = VS_MS_FILE_STAT_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_STAT_REQ_MSG_V01,
	.ei_array = vs_ms_file_stat_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_stat_resp_desc = {
	.max_msg_len = VS_MS_FILE_STAT_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_STAT_RESP_MSG_V01,
	.ei_array = vs_ms_file_stat_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_read_req_desc = {
	.max_msg_len = VS_MS_FILE_READ_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_READ_REQ_MSG_V01,
	.ei_array = vs_ms_file_read_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_read_resp_desc = {
	.max_msg_len = VS_MS_FILE_READ_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_READ_RESP_MSG_V01,
	.ei_array = vs_ms_file_read_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_write_req_desc = {
	.max_msg_len = VS_MS_FILE_WRITE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_WRITE_REQ_MSG_V01,
	.ei_array = vs_ms_file_write_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_file_write_resp_desc = {
	.max_msg_len = VS_MS_FILE_WRITE_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_FILE_WRITE_RESP_MSG_V01,
	.ei_array = vs_ms_file_write_resp_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_get_cust_info_req_desc = {
	.max_msg_len = VS_MS_GET_CUST_INFO_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_GET_CUST_INFO_REQ_MSG_V01,
	.ei_array = vs_ms_get_cust_info_req_msg_data_v01_ei,
};

static struct msg_desc vs_mem_share_svc_get_cust_info_resp_desc = {
	.max_msg_len = VS_MS_GET_CUST_INFO_REQ_MAX_MSG_LEN_V01,
	.msg_id = VS_MS_GET_CUST_INFO_RESP_MSG_V01,
	.ei_array = vs_ms_get_cust_info_resp_msg_data_v01_ei,
};

/*
 *  This API creates ramdump dev handlers
 *  for each of the memshare clients.
 *  These dev handlers will be used for
 *  extracting the ramdump for loaned memory
 *  segments.
 */

static int vs_mem_share_configure_ramdump(int client_id, int index)
{
	char client_name[20] = {0};

	if (client_id == VS_MS_CLIENT_HYDRA_RFSA_V01) {
		snprintf(client_name, 20, "HYDRA_%s", "RFSA");
	} else if (client_id == VS_MS_CLIENT_HYDRA_NVEFS_TRACE_V01) {
		snprintf(client_name, 20, "HYDRA_%s", "NVEFS_TRACE");
	} else if (client_id == VS_MS_CLIENT_HYDRA_VOICE_TRACE_V01) {
		snprintf(client_name, 20, "HYDRA_%s", "VOICE_TRACE");
	} else if (client_id == VS_MS_CLIENT_HYDRA_STATS_TRACE_V01) {
		snprintf(client_name, 20, "HYDRA_%s", "STATS_TRACE");
	} else if (client_id == VS_MS_CLIENT_HYDRA_DIAG_V01) {
		snprintf(client_name, 20, "HYDRA_%s", "DIAG");
	} else {
		snprintf(client_name, 20, "HYDRA_%d", client_id);
	}

	vs_memshare_ramdump_dev[index] = create_ramdump_device(client_name,
								NULL);
	if (IS_ERR_OR_NULL(vs_memshare_ramdump_dev[index])) {
		pr_err("vs_memshare: %s: Unable to create memshare ramdump device.\n",
				__func__);
		vs_memshare_ramdump_dev[index] = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int check_client(int client_id, int request)
{
	int i = 0, rc;
	int found = VS_MS_CLIENT_INVALID;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (vs_memblock[i].client_id == client_id) {
			found = i;
			break;
		}
	}
	if ((found == VS_MS_CLIENT_INVALID) && !request) {
		pr_info("vs_memshare: No registered client, adding a new client\n");
		/* Add a new client */
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (vs_memblock[i].client_id == VS_MS_CLIENT_INVALID) {
				vs_memblock[i].client_id = client_id;
				vs_memblock[i].alloted = 0;
				vs_memblock[i].guarantee = 0;
				found = i;

				if (!vs_memblock[i].file_created) {
					rc = vs_mem_share_configure_ramdump(client_id, i);
					if (rc)
						pr_err("In %s, Cannot create ramdump for client: %d\n",
							__func__, client_id);
					else
						vs_memblock[i].file_created = 1;
				}

				break;
			}
		}
	}

	return found;
}

void vs_free_client(int id)
{
	vs_memblock[id].phy_addr = 0;
	vs_memblock[id].virtual_addr = 0;
	vs_memblock[id].alloted = 0;
	vs_memblock[id].guarantee = 0;
	vs_memblock[id].sequence_id = -1;
	vs_memblock[id].memory_type = MEMORY_CMA;

}

void vs_fill_alloc_response(struct vs_ms_mem_alloc_generic_resp_msg_v01 *resp,
						int id, int *flag)
{
	resp->sequence_id_valid = 1;
	resp->sequence_id = vs_memblock[id].sequence_id;
	resp->vs_ms_mem_alloc_addr_info_valid = 1;
	resp->vs_ms_mem_alloc_addr_info.phy_addr = vs_memblock[id].phy_addr;
	resp->vs_ms_mem_alloc_addr_info.num_bytes = vs_memblock[id].size;
	if (!*flag) {
		resp->resp.result = QMI_RESULT_SUCCESS_V01;
		resp->resp.error = QMI_ERR_NONE_V01;
	} else {
		resp->resp.result = QMI_RESULT_FAILURE_V01;
		resp->resp.error = QMI_ERR_NO_MEMORY_V01;
	}

}

void vs_initialize_client(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		vs_memblock[i].alloted = 0;
		vs_memblock[i].size = 0;
		vs_memblock[i].guarantee = 0;
		vs_memblock[i].phy_addr = 0;
		vs_memblock[i].virtual_addr = 0;
		vs_memblock[i].client_id = VS_MS_CLIENT_INVALID;
		vs_memblock[i].sequence_id = -1;
		vs_memblock[i].memory_type = MEMORY_CMA;
		vs_memblock[i].free_memory = 0;
		vs_memblock[i].hyp_mapping = 0;
		vs_memblock[i].file_created = 0;
	}
}

/*
 *  This API initializes the ramdump segments
 *  with the physical address and size of
 *  the memshared clients. Extraction of ramdump
 *  is skipped if memshare client is not alloted
 *  This calls the ramdump api in extracting the
 *  ramdump in elf format.
 */

static int vs_mem_share_do_ramdump(void)
{
	int i = 0, ret;
	char client_name[20] = {0};

	for (i = 0; i < num_clients; i++) {

		struct ramdump_segment *ramdump_segments_tmp = NULL;

		if (i == VS_MS_CLIENT_HYDRA_RFSA_V01) {
			snprintf(client_name, 20, "HYDRA_%s", "RFSA");
		} else if (i == VS_MS_CLIENT_HYDRA_NVEFS_TRACE_V01) {
			snprintf(client_name, 20, "HYDRA_%s", "NVEFS_TRACE");
		} else if (i == VS_MS_CLIENT_HYDRA_VOICE_TRACE_V01) {
			snprintf(client_name, 20, "HYDRA_%s", "VOICE_TRACE");
		} else if (i == VS_MS_CLIENT_HYDRA_STATS_TRACE_V01) {
			snprintf(client_name, 20, "HYDRA_%s", "STATS_TRACE");
		} else if (i == VS_MS_CLIENT_HYDRA_DIAG_V01) {
			snprintf(client_name, 20, "HYDRA_%s", "DIAG");
		} else {
			snprintf(client_name, 20, "HYDRA_%d", i);
		}

		if (!vs_memblock[i].alloted) {
			pr_err("vs_memshare:%s memblock is not alloted\n",
			client_name);
			continue;
		}

		ramdump_segments_tmp = kcalloc(1,
			sizeof(struct ramdump_segment),
			GFP_KERNEL);
		if (!ramdump_segments_tmp)
			return -ENOMEM;

		ramdump_segments_tmp[0].size = vs_memblock[i].size;
		ramdump_segments_tmp[0].address = vs_memblock[i].phy_addr;

		pr_info("vs_memshare: %s:%s client:phy_address = %llx, size = %d\n",
		__func__, client_name,
		(unsigned long long) vs_memblock[i].phy_addr, vs_memblock[i].size);

		ret = do_elf_ramdump(vs_memshare_ramdump_dev[i],
					ramdump_segments_tmp, 1);
		if (ret < 0) {
			pr_err("vs_memshare: Unable to dump: %d\n", ret);
			kfree(ramdump_segments_tmp);
			return ret;
		}
		kfree(ramdump_segments_tmp);
	}
	return 0;
}

static int modem_notifier_cb(struct notifier_block *this, unsigned long code,
					void *_cmd)
{
	int i;
	int ret;
	u32 source_vmlist[2] = {VMID_HLOS, VMID_MSS_MSA};
	int dest_vmids[1] = {VMID_HLOS};
	int dest_perms[1] = {PERM_READ|PERM_WRITE};
	struct notif_data *notifdata = NULL;

	mutex_lock(&vs_memsh_drv->mem_share);

	switch (code) {

	case SUBSYS_BEFORE_SHUTDOWN:
		bootup_request++;
		break;

	case SUBSYS_RAMDUMP_NOTIFICATION:
		ramdump_event = 1;
		break;

	case SUBSYS_BEFORE_POWERUP:
		if (_cmd) {
			notifdata = (struct notif_data *) _cmd;
		} else {
			ramdump_event = 0;
			break;
		}

		if (notifdata->enable_ramdump && ramdump_event) {
			pr_info("vs_memshare: %s, Ramdump collection is enabled\n",
					__func__);
			ret = vs_mem_share_do_ramdump();
			if (ret)
				pr_err("Ramdump collection failed\n");
			ramdump_event = 0;
		}
		break;

	case SUBSYS_AFTER_POWERUP:
		pr_info("vs_memshare: Modem has booted up\n");
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (vs_memblock[i].free_memory > 0 &&
					bootup_request >= 2) {
				vs_memblock[i].free_memory -= 1;
				pr_info("vs_memshare: free_memory count: %d for clinet id: %d\n",
					vs_memblock[i].free_memory,
					vs_memblock[i].client_id);
			}

			if (vs_memblock[i].free_memory == 0) {
				if (!vs_memblock[i].guarantee &&
					vs_memblock[i].alloted) {
					pr_info("vs_memshare: Freeing memory for client id: %d\n",
						vs_memblock[i].client_id);
					ret = hyp_assign_phys(
							vs_memblock[i].phy_addr,
							vs_memblock[i].size,
							source_vmlist,
							2, dest_vmids,
							dest_perms, 1);
					if (ret &&
						vs_memblock[i].hyp_mapping == 1) {
						/*
						 * This is an error case as hyp
						 * mapping was successful
						 * earlier but during unmap
						 * it lead to failure.
						 */
						pr_err("vs_memshare: %s, failed to unmap the region\n",
							__func__);
						vs_memblock[i].hyp_mapping = 1;
					} else {
						vs_memblock[i].hyp_mapping = 0;
					}
					dma_free_attrs(vs_memsh_drv->dev,
						vs_memblock[i].size,
						vs_memblock[i].virtual_addr,
						vs_memblock[i].phy_addr,
						attrs);
					vs_free_client(i);
				}
			}
		}
		bootup_request++;
		break;

	default:
		pr_info("VS Memshare: code: %lu\n", code);
		break;
	}

	mutex_unlock(&vs_memsh_drv->mem_share);
	return NOTIFY_DONE;
}

static struct notifier_block nb = {
	.notifier_call = modem_notifier_cb,
};

static void shared_hyp_mapping(int client_id)
{
	int ret;
	u32 source_vmlist[1] = {VMID_HLOS};
	//u32 source_vmlist[2] = {VMID_HLOS, VMID_MSS_MSA};
	int dest_vmids[2] = {VMID_HLOS, VMID_MSS_MSA};
	int dest_perms[2] = {PERM_READ|PERM_WRITE,
				PERM_READ|PERM_WRITE};

	if (client_id == VS_MS_CLIENT_INVALID) {
		pr_err("vs_memshare: %s, Invalid Client\n", __func__);
		return;
	}

	ret = hyp_assign_phys(vs_memblock[client_id].phy_addr,
			vs_memblock[client_id].size,
			source_vmlist, 1, dest_vmids,
			//source_vmlist, 2, dest_vmids,
			dest_perms, 2);

	if (ret != 0) {
		pr_err("vs_memshare: hyp_assign_phys failed size=%u err=%d\n",
				vs_memblock[client_id].size, ret);
		return;
	}
	vs_memblock[client_id].hyp_mapping = 1;
}

static int handle_alloc_generic_req(void *req_h, void *req, void *conn_h)
{
	struct vs_ms_mem_alloc_generic_req_msg_v01 *alloc_req;
	struct vs_ms_mem_alloc_generic_resp_msg_v01 *alloc_resp;
	int rc, resp = 0;
	int client_id;

	alloc_req = (struct vs_ms_mem_alloc_generic_req_msg_v01 *)req;
	pr_info("vs_memshare: alloc request client id: %d\n", alloc_req->client_id);
	mutex_lock(&vs_memsh_drv->mem_share);
	alloc_resp = kzalloc(sizeof(struct vs_ms_mem_alloc_generic_resp_msg_v01),
					GFP_KERNEL);
	if (!alloc_resp) {
		mutex_unlock(&vs_memsh_drv->mem_share);
		return -ENOMEM;
	}
	alloc_resp->resp.result = QMI_RESULT_FAILURE_V01;
	alloc_resp->resp.error = QMI_ERR_NO_MEMORY_V01;
	client_id = check_client(alloc_req->client_id, CHECK);

	if (client_id >= MAX_CLIENTS) {
		pr_err("vs_memshare: %s client not found, requested client: %d\n",
				__func__, alloc_req->client_id);
		alloc_resp->resp.result = QMI_RESULT_FAILURE_V01;
		alloc_resp->resp.error = QMI_ERR_INVALID_ID_V01;
	} else {
		vs_memblock[client_id].free_memory += 1;
		pr_info("vs_memshare: In %s, free memory count for client id: %d = %d",
			__func__, vs_memblock[client_id].client_id,
				vs_memblock[client_id].free_memory);
		if (!vs_memblock[client_id].alloted) {
			rc = vs_memshare_alloc(vs_memsh_drv->dev, client_id, alloc_req->num_bytes,
						&vs_memblock[client_id]);
			if (rc) {
				pr_err("In %s,Unable to allocate memory for requested client\n",
								__func__);
				resp = 1;
			}
			if (!resp) {
				vs_memblock[client_id].alloted = 1;
				vs_memblock[client_id].size = alloc_req->num_bytes;
			}
		}
		vs_memblock[client_id].sequence_id = alloc_req->sequence_id;

		vs_fill_alloc_response(alloc_resp, client_id, &resp);
		/*
		 * Perform the Hypervisor mapping in order to avoid XPU viloation
		 * to the allocated region for Modem Clients
		 */
		if (!vs_memblock[client_id].hyp_mapping &&
			vs_memblock[client_id].alloted)
			shared_hyp_mapping(client_id);
	}
	mutex_unlock(&vs_memsh_drv->mem_share);
	pr_info("vs_memshare: alloc_resp.num_bytes :%d, alloc_resp.handle :%lx, alloc_resp.mem_req_result :%lx\n",
			  alloc_resp->vs_ms_mem_alloc_addr_info.num_bytes,
			  (unsigned long int)
			  alloc_resp->vs_ms_mem_alloc_addr_info.phy_addr,
			  (unsigned long int)alloc_resp->resp.result);
	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_alloc_generic_resp_desc, alloc_resp,
			sizeof(alloc_resp));

	if (rc < 0)
		pr_err("In %s, Error sending the alloc request: %d\n",
							__func__, rc);
	return rc;
}

static int handle_free_generic_req(void *req_h, void *req, void *conn_h)
{
	struct vs_ms_mem_free_generic_req_msg_v01 *free_req;
	struct vs_ms_mem_free_generic_resp_msg_v01 free_resp;
	int rc;
	int flag = 0;
	uint32_t client_id;

	free_req = (struct vs_ms_mem_free_generic_req_msg_v01 *)req;
	pr_info("vs_memshare: %s: Received Free Request\n", __func__);
	mutex_lock(&vs_memsh_drv->mem_free);
	memset(&free_resp, 0, sizeof(struct vs_ms_mem_free_generic_resp_msg_v01));
	free_resp.resp.error = QMI_ERR_INTERNAL_V01;
	free_resp.resp.result = QMI_ERR_INTERNAL_V01;
	pr_info("vs_memshare: Client id: %d\n", free_req->client_id);
	client_id = check_client(free_req->client_id, FREE);
	if (client_id == VS_MS_CLIENT_INVALID) {
		pr_err("In %s, Invalid client request to free memory\n",
					__func__);
		flag = 1;
	} else if (!vs_memblock[client_id].guarantee &&
					vs_memblock[client_id].alloted) {
		pr_info("In %s: pblk->virtual_addr :%lx, pblk->phy_addr: %lx\n,size: %d",
				__func__,
				(unsigned long int)
				vs_memblock[client_id].virtual_addr,
				(unsigned long int)vs_memblock[client_id].phy_addr,
				vs_memblock[client_id].size);
		dma_free_attrs(vs_memsh_drv->dev, vs_memblock[client_id].size,
			vs_memblock[client_id].virtual_addr,
			vs_memblock[client_id].phy_addr,
			attrs);
		vs_free_client(client_id);
	} else {
		pr_err("In %s, Request came for a guaranteed client cannot free up the memory\n",
						__func__);
	}

	if (flag) {
		free_resp.resp.result = QMI_RESULT_FAILURE_V01;
		free_resp.resp.error = QMI_ERR_INVALID_ID_V01;
	} else {
		free_resp.resp.result = QMI_RESULT_SUCCESS_V01;
		free_resp.resp.error = QMI_ERR_NONE_V01;
	}

	mutex_unlock(&vs_memsh_drv->mem_free);
	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
		&vs_mem_share_svc_free_generic_resp_desc, &free_resp,
		sizeof(free_resp));

	if (rc < 0)
		pr_err("In %s, Error sending the free request: %d\n",
					__func__, rc);

	return rc;
}

static int handle_query_size_req(void *req_h, void *req, void *conn_h)
{
	int rc, client_id;
	struct vs_ms_mem_query_size_req_msg_v01 *query_req;
	struct vs_ms_mem_query_size_resp_msg_v01 *query_resp;

	query_req = (struct vs_ms_mem_query_size_req_msg_v01 *)req;
	mutex_lock(&vs_memsh_drv->mem_share);
	query_resp = kzalloc(sizeof(struct vs_ms_mem_query_size_resp_msg_v01),
					GFP_KERNEL);
	if (!query_resp) {
		mutex_unlock(&vs_memsh_drv->mem_share);
		return -ENOMEM;
	}
	pr_info("vs_memshare: query request client id: %d\n", query_req->client_id);
	client_id = check_client(query_req->client_id, CHECK);

	if (client_id >= MAX_CLIENTS) {
		pr_err("vs_memshare: %s client not found, requested client: %d\n",
				__func__, query_req->client_id);
		query_resp->resp.result = QMI_RESULT_FAILURE_V01;
		query_resp->resp.error = QMI_ERR_INVALID_ID_V01;
	} else {
		if (vs_memblock[client_id].size) {
			query_resp->size_valid = 1;
			query_resp->size = vs_memblock[client_id].size;
		} else {
			query_resp->size_valid = 1;
			query_resp->size = 0;
		}
		query_resp->resp.result = QMI_RESULT_SUCCESS_V01;
		query_resp->resp.error = QMI_ERR_NONE_V01;
	}
	mutex_unlock(&vs_memsh_drv->mem_share);

	pr_info("vs_memshare: query_resp.size :%d, query_resp.mem_req_result :%lx\n",
			  query_resp->size,
			  (unsigned long int)query_resp->resp.result);
	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_size_query_resp_desc, query_resp,
			sizeof(query_resp));

	if (rc < 0)
		pr_err("In %s, Error sending the query request: %d\n",
							__func__, rc);

	return rc;
}

static int handle_file_stat_req(void *req_h, void *req, void *conn_h)
{
	int rc;
	struct kstat stat;
	struct vs_ms_file_stat_req_msg_v01 *stat_req;
	struct vs_ms_file_stat_resp_msg_v01 *stat_resp;

	stat_req = (struct vs_ms_file_stat_req_msg_v01 *)req;
	stat_resp = kzalloc(sizeof(struct vs_ms_file_stat_resp_msg_v01),
					GFP_KERNEL);
	if (!stat_resp) {
		return -ENOMEM;
	}
	pr_info("vs_memshare: stat request file name: %s\n", stat_req->filename);

	rc = vfs_stat(stat_req->filename, &stat);
	if (rc < 0) {
		pr_err("In %s, Error stating the file: %d\n", __func__, rc);
		stat_resp->resp.result = QMI_RESULT_FAILURE_V01;
		stat_resp->resp.error = QMI_ERR_INTERNAL_V01;
	} else {
		stat_resp->resp.result = QMI_RESULT_SUCCESS_V01;
		stat_resp->resp.error = QMI_ERR_NONE_V01;

		if (stat.mode & (S_IRUSR | S_IRGRP | S_IROTH)) {
			stat_resp->flags |= VS_MS_FILE_ACCESS_FLAG_READ_V01;
		}
		if (stat.mode & (S_IWUSR | S_IWGRP | S_IWOTH)) {
			stat_resp->flags |= VS_MS_FILE_ACCESS_FLAG_WRITE_V01;
		}

		if (stat.size > 0) {
			stat_resp->size_valid = 1;
			stat_resp->size = (uint32_t)stat.size;
		}
	}

	pr_info("vs_memshare: stat_resp.flags: 0x%llx, stat_resp.size_valid: %d, stat_resp.size: 0x%x, stat_resp.result: %lx\n",
			stat_resp->flags,
			stat_resp->size_valid,
		  	stat_resp->size,
		  	(unsigned long int)stat_resp->resp.result);

	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_file_stat_resp_desc, stat_resp,
			sizeof(stat_resp));

	if (rc < 0)
		pr_err("In %s, Error sending the file stat response: %d\n", __func__, rc);

	return rc;
}

static int handle_file_read_req(void *req_h, void *req, void *conn_h)
{
	struct vs_ms_file_read_req_msg_v01 *read_req;
	struct vs_ms_file_read_resp_msg_v01 *read_resp;
	int rc, client_id;
	mm_segment_t oldfs = get_fs();
	struct file* filp;
	struct kstat stat;
	uint32_t l;
	loff_t pos;

	read_req = (struct vs_ms_file_read_req_msg_v01 *)req;
	mutex_lock(&vs_memsh_drv->mem_share);
	read_resp = kzalloc(sizeof(struct vs_ms_file_read_resp_msg_v01),
					GFP_KERNEL);
	if (!read_resp) {
		mutex_unlock(&vs_memsh_drv->mem_share);
		return -ENOMEM;
	}
	pr_info("vs_memshare: read request client id: %d, filename: %s, offset 0x%x, size 0x%x\n",
			read_req->client_id,
			read_req->filename,
			read_req->offset,
			read_req->size);

	do {
		client_id = check_client(read_req->client_id, FREE);

		if (client_id >= MAX_CLIENTS) {
			pr_err("vs_memshare: %s client not found, requested client: %d\n",
					__func__, read_req->client_id);
			read_resp->resp.result = QMI_RESULT_FAILURE_V01;
			read_resp->resp.error = QMI_ERR_INVALID_ID_V01;
			break;
		}

		if (!vs_memblock[client_id].alloted) {
			pr_err("vs_memshare: %s client not allocated\n", __func__);
			read_resp->resp.result = QMI_RESULT_FAILURE_V01;
			read_resp->resp.error = QMI_ERR_NO_MEMORY_V01;
			break;
		}

		filp = filp_open(read_req->filename, 0, 0);
		if (IS_ERR(filp))
		{
			pr_err("vs_memshare: %s Error opening file\n", __func__);
			read_resp->resp.result = QMI_RESULT_FAILURE_V01;
			read_resp->resp.error = QMI_ERR_INTERNAL_V01;
			break;
		}

		rc = vfs_stat(read_req->filename, &stat);
		if (rc < 0) {
			pr_err("vs_memshare: %s Error stating the file: %d\n", __func__, rc);
			read_resp->resp.result = QMI_RESULT_FAILURE_V01;
			read_resp->resp.error = QMI_ERR_INTERNAL_V01;
			filp_close(filp, NULL);
			break;
		}

		if ((uint32_t)stat.size <= read_req->offset) {
			pr_info("vs_memshare: %s Error offset 0x%x is larger than size 0x%x\n", __func__, read_req->offset, (uint32_t)stat.size);
			read_resp->data_valid = 1;
		} else {
			pos = (loff_t)read_req->offset;
			if (read_req->offset + read_req->size > (uint32_t)stat.size) {
				pr_info("vs_memshare: %s Error offset 0x%x plus read_req.size 0x%x is larger than size 0x%x\n", __func__,
						read_req->offset, read_req->size, (uint32_t)stat.size);
				l = (uint32_t)stat.size - read_req->offset;
			} else {
				l = read_req->size;
			}
			if (l > vs_memblock[client_id].size) {
				pr_info("vs_memshare: %s Error read length 0x%x is larger than client memory size 0x%x\n", __func__,
						l, vs_memblock[client_id].size);
				l = vs_memblock[client_id].size;
			}

			set_fs(KERNEL_DS);
			if (vfs_read(filp, (char *)vs_memblock[client_id].virtual_addr, l, &pos) != l) {
				set_fs(oldfs);
				pr_err("vs_memshare: %s Error reading file\n", __func__);
				read_resp->resp.result = QMI_RESULT_FAILURE_V01;
				read_resp->resp.error = QMI_ERR_INTERNAL_V01;
			} else {
				set_fs(oldfs);
				read_resp->resp.result = QMI_RESULT_SUCCESS_V01;
				read_resp->resp.error = QMI_ERR_NONE_V01;
				read_resp->data_valid = 1;
				read_resp->data.count = l;
				read_resp->data.buffer = vs_memblock[client_id].phy_addr;
			}
		}
		filp_close(filp, NULL);
	} while (0);
	mutex_unlock(&vs_memsh_drv->mem_share);

	pr_info("vs_memshare: read_resp.data_valid: %d, read_resp.data.count: 0x%x, read_resp.data.buffer: 0x%lx, read_resp.result: %lx\n",
			read_resp->data_valid,
			read_resp->data.count,
			(unsigned long int)read_resp->data.buffer,
		  	(unsigned long int)read_resp->resp.result);

	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_file_read_resp_desc, read_resp,
			sizeof(read_resp));

	if (rc < 0)
		pr_err("vs_memshare: %s Error sending the file read response: %d\n", __func__, rc);

	return rc;
}

static int handle_file_write_req(void *req_h, void *req, void *conn_h)
{
	struct vs_ms_file_write_req_msg_v01 *write_req;
	struct vs_ms_file_write_resp_msg_v01 *write_resp;
	int rc, client_id, flags, i;
	mm_segment_t oldfs = get_fs();
	struct dentry *dentry;
	struct path path;
	struct file* filp;
	struct kstat stat;
	uint32_t l;
	loff_t pos;

	write_req = (struct vs_ms_file_write_req_msg_v01 *)req;
	mutex_lock(&vs_memsh_drv->mem_share);
	write_resp = kzalloc(sizeof(struct vs_ms_file_write_resp_msg_v01),
					GFP_KERNEL);
	if (!write_resp) {
		mutex_unlock(&vs_memsh_drv->mem_share);
		return -ENOMEM;
	}
	pr_info("vs_memshare: write request client id: %d, filename: %s, flags: %llx, offset: 0x%x, size: 0x%x\n",
			write_req->client_id,
			write_req->filename,
			write_req->flags,
			write_req->offset,
			write_req->size);

	do {
		client_id = check_client(write_req->client_id, FREE);

		if (client_id >= MAX_CLIENTS) {
			pr_err("vs_memshare: %s client not found, requested client: %d\n",
					__func__, write_req->client_id);
			write_resp->resp.result = QMI_RESULT_FAILURE_V01;
			write_resp->resp.error = QMI_ERR_INVALID_ID_V01;
			break;
		}

		if (!vs_memblock[client_id].alloted) {
			pr_err("vs_memshare: %s client not allocated\n", __func__);
			write_resp->resp.result = QMI_RESULT_FAILURE_V01;
			write_resp->resp.error = QMI_ERR_NO_MEMORY_V01;
			break;
		}

		rc = vfs_stat(write_req->filename, &stat);
		if (rc == -ENOENT) {
			if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_CREATE_V01) {
				if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_APPEND_V01) {
					flags = O_WRONLY | O_CREAT | O_APPEND;
				} else if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_TRUNC_V01) {
					flags = O_WRONLY | O_CREAT | O_TRUNC;
				} else {
					flags = O_WRONLY | O_CREAT;
				}
			} else {
				pr_err("vs_memshare: %s File not exists\n", __func__);
				write_resp->resp.result = QMI_RESULT_FAILURE_V01;
				write_resp->resp.error = QMI_ERR_INTERNAL_V01;
				break;
			}
		} else if (rc < 0) {
			pr_err("vs_memshare: %s Error stating the file: %d\n", __func__, rc);
			write_resp->resp.result = QMI_RESULT_FAILURE_V01;
			write_resp->resp.error = QMI_ERR_INTERNAL_V01;
			break;
		} else {
			if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_APPEND_V01) {
				flags = O_WRONLY | O_APPEND;
			} else if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_TRUNC_V01) {
				flags = O_WRONLY | O_TRUNC;
			} else {
				flags = O_WRONLY;
			}
		}

		if (flags & O_CREAT) {
			for (i = 1; i < strlen(write_req->filename); i ++) {
				if (write_req->filename[i] == '/' && write_req->filename[i + 1] != 0) {
					write_req->filename[i] = 0;
					if (vfs_stat(write_req->filename, &stat) == -ENOENT) {
						dentry = kern_path_create(AT_FDCWD, write_req->filename, &path, LOOKUP_DIRECTORY);
						rc = vfs_mkdir(path.dentry->d_inode, dentry, 0770);
						if (rc < 0) {
							pr_err("vs_memshare: %s Error creating directory %s: %d\n", __func__, write_req->filename, rc);
							done_path_create(&path, dentry);
							write_req->filename[i] = '/';
							break;
						} else {
							pr_info("vs_memshare: %s Created directory %s\n", __func__, write_req->filename);
							done_path_create(&path, dentry);
						}
					}
					write_req->filename[i] = '/';
				}
			}
			if (i != strlen(write_req->filename)) {
				write_resp->resp.result = QMI_RESULT_FAILURE_V01;
				write_resp->resp.error = QMI_ERR_INTERNAL_V01;
				break;
			}
		}

		filp = filp_open(write_req->filename, flags, 0770);
		if (IS_ERR(filp))
		{
			pr_err("vs_memshare: %s Error opening/creating file: %ld\n", __func__, PTR_ERR(filp));
			write_resp->resp.result = QMI_RESULT_FAILURE_V01;
			write_resp->resp.error = QMI_ERR_INTERNAL_V01;
			break;
		}

		if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_APPEND_V01) {
			pos = filp->f_pos;
		} else if (write_req->flags & VS_MS_FILE_ACCESS_FLAG_TRUNC_V01) {
			pos = (loff_t)0;
		} else {
			pos = (loff_t)write_req->offset;
		}
        l = write_req->size;
		if (l > vs_memblock[client_id].size) {
			pr_info("vs_memshare: %s Error write length 0x%x is larger than client memory size 0x%x\n", __func__,
					l, vs_memblock[client_id].size);
			l = vs_memblock[client_id].size;
		}

		set_fs(KERNEL_DS);
		if (vfs_write(filp, (char *)vs_memblock[client_id].virtual_addr, l, &pos) != l) {
			set_fs(oldfs);
			pr_err("vs_memshare: %s Error writing file\n", __func__);
			write_resp->resp.result = QMI_RESULT_FAILURE_V01;
			write_resp->resp.error = QMI_ERR_INTERNAL_V01;
		} else {
			set_fs(oldfs);
			write_resp->resp.result = QMI_RESULT_SUCCESS_V01;
			write_resp->resp.error = QMI_ERR_NONE_V01;
			write_resp->count_valid = 1;
			write_resp->count = l;
		}

		filp_close(filp, NULL);
	} while (0);
	mutex_unlock(&vs_memsh_drv->mem_share);

	pr_info("vs_memshare: write_resp.count_valid: %d, write_resp.count: 0x%x, write_resp.result: %lx\n",
			write_resp->count_valid,
			write_resp->count,
		  	(unsigned long int)write_resp->resp.result);

	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_file_write_resp_desc, write_resp,
			sizeof(write_resp));

	if (rc < 0)
		pr_err("vs_memshare: %s Error sending the file write response: %d\n", __func__, rc);

	return rc;
}

static int handle_get_cust_info_req(void *req_h, void *req, void *conn_h)
{
	struct vs_ms_get_cust_info_req_msg_v01 *get_req;
	struct vs_ms_get_cust_info_resp_msg_v01 *get_resp;
	int rc;

	get_req = (struct vs_ms_get_cust_info_req_msg_v01 *)req;
	get_resp = kzalloc(sizeof(struct vs_ms_get_cust_info_resp_msg_v01),
					GFP_KERNEL);
	if (!get_resp) {
		return -ENOMEM;
	}

	get_resp->resp.result = QMI_RESULT_SUCCESS_V01;
	get_resp->resp.error = QMI_ERR_NONE_V01;
    if (oeminfo_boardid != 0) {
		get_resp->boardid_valid= 1;
		get_resp->boardid = oeminfo_boardid;
	}
	if (oeminfo_country[0] != 0) {
		get_resp->country_valid= 1;
		get_resp->country[0] = oeminfo_country[0];
		get_resp->country[1] = oeminfo_country[1];
	}
	if (oeminfo_carrier[0] != 0) {
		get_resp->carrier_valid= 1;
		get_resp->carrier[0] = oeminfo_carrier[0];
		get_resp->carrier[1] = oeminfo_carrier[1];
	}
	if (oeminfo_saleschannel[0] != 0) {
		get_resp->saleschannel_valid= 1;
		get_resp->saleschannel[0] = oeminfo_saleschannel[0];
		get_resp->saleschannel[1] = oeminfo_saleschannel[1];
	}
	if (oeminfo_target[0] != 0) {
		get_resp->target_valid= 1;
		get_resp->target[0] = oeminfo_target[0];
		get_resp->target[1] = oeminfo_target[1];
	}

	pr_info("vs_memshare: get_resp.boardid_valid :%d, get_resp.boardid :0x%x\n",
			  get_resp->boardid_valid, get_resp->boardid);
	pr_info("vs_memshare: get_resp.country_valid :%d, get_resp.country :%c%c\n",
			  get_resp->country_valid, get_resp->country[0], get_resp->country[1]);
	pr_info("vs_memshare: get_resp.carrier_valid :%d, get_resp.carrier :%c%c\n",
			  get_resp->carrier_valid, get_resp->carrier[0], get_resp->carrier[1]);
	pr_info("vs_memshare: get_resp.saleschannel_valid :%d, get_resp.saleschannel :%c%c\n",
			  get_resp->saleschannel_valid, get_resp->saleschannel[0], get_resp->saleschannel[1]);
	pr_info("vs_memshare: get_resp.target_valid :%d, get_resp.target :%c%c\n",
			  get_resp->target_valid, get_resp->target[0], get_resp->target[1]);
	rc = qmi_send_resp_from_cb(vs_mem_share_svc_handle, conn_h, req_h,
			&vs_mem_share_svc_get_cust_info_resp_desc, get_resp,
			sizeof(get_resp));

	if (rc < 0)
		pr_err("In %s, Error sending the get response: %d\n",
							__func__, rc);
	return rc;
}


static int vs_mem_share_svc_connect_cb(struct qmi_handle *handle,
			       void *conn_h)
{
	if (vs_mem_share_svc_handle != handle || !conn_h)
		return -EINVAL;

	return 0;
}

static int vs_mem_share_svc_disconnect_cb(struct qmi_handle *handle,
				  void *conn_h)
{
	if (vs_mem_share_svc_handle != handle || !conn_h)
		return -EINVAL;

	return 0;
}

static int vs_mem_share_svc_req_desc_cb(unsigned int msg_id,
				struct msg_desc **req_desc)
{
	int rc;

	pr_info("vs_memshare: In %s\n", __func__);
	switch (msg_id) {
		case VS_MS_MEM_ALLOC_GENERIC_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_alloc_generic_req_desc;
			rc = sizeof(struct vs_ms_mem_alloc_generic_req_msg_v01);
			break;

		case VS_MS_MEM_FREE_GENERIC_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_free_generic_req_desc;
			rc = sizeof(struct vs_ms_mem_free_generic_req_msg_v01);
			break;

		case VS_MS_MEM_QUERY_SIZE_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_size_query_req_desc;
			rc = sizeof(struct vs_ms_mem_query_size_req_msg_v01);
			break;

		case VS_MS_FILE_STAT_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_file_stat_req_desc;
			rc = sizeof(struct vs_ms_file_stat_req_msg_v01);
			break;

		case VS_MS_FILE_READ_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_file_read_req_desc;
			rc = sizeof(struct vs_ms_file_read_req_msg_v01);
			break;

		case VS_MS_FILE_WRITE_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_file_write_req_desc;
			rc = sizeof(struct vs_ms_file_write_req_msg_v01);
			break;

		case VS_MS_GET_CUST_INFO_REQ_MSG_V01:
			*req_desc = &vs_mem_share_svc_get_cust_info_req_desc;
			rc = sizeof(struct vs_ms_get_cust_info_req_msg_v01);
			break;

		default:
			rc = -ENOTSUPP;
			break;
	}
	return rc;
}

static int vs_mem_share_svc_req_cb(struct qmi_handle *handle, void *conn_h,
			void *req_h, unsigned int msg_id, void *req)
{
	int rc;

	pr_info("vs_memshare: In %s\n", __func__);
	if (vs_mem_share_svc_handle != handle || !conn_h)
		return -EINVAL;

	switch (msg_id) {
		case VS_MS_MEM_ALLOC_GENERIC_REQ_MSG_V01:
			rc = handle_alloc_generic_req(req_h, req, conn_h);
			break;

		case VS_MS_MEM_FREE_GENERIC_REQ_MSG_V01:
			rc = handle_free_generic_req(req_h, req, conn_h);
			break;

		case VS_MS_MEM_QUERY_SIZE_REQ_MSG_V01:
			rc = handle_query_size_req(req_h, req, conn_h);
			break;

		case VS_MS_FILE_STAT_REQ_MSG_V01:
			rc = handle_file_stat_req(req_h, req, conn_h);
			break;

		case VS_MS_FILE_READ_REQ_MSG_V01:
			rc = handle_file_read_req(req_h, req, conn_h);
			break;

		case VS_MS_FILE_WRITE_REQ_MSG_V01:
			rc = handle_file_write_req(req_h, req, conn_h);
			break;

		case VS_MS_GET_CUST_INFO_REQ_MSG_V01:
			rc = handle_get_cust_info_req(req_h, req, conn_h);
			break;

		default:
			rc = -ENOTSUPP;
			break;
	}
	return rc;
}

static void vs_mem_share_svc_recv_msg(struct work_struct *work)
{
	int rc;

	pr_info("vs_memshare: In %s\n", __func__);
	do {
		pr_info("%s: Notified about a Receive Event", __func__);
	} while ((rc = qmi_recv_msg(vs_mem_share_svc_handle)) == 0);

	if (rc != -ENOMSG)
		pr_err("%s: Error receiving message\n", __func__);
}

static void vs_qmi_mem_share_svc_ntfy(struct qmi_handle *handle,
		enum qmi_event_type event, void *priv)
{
	pr_info("vs_memshare: In %s\n", __func__);
	switch (event) {
		case QMI_RECV_MSG:
			queue_delayed_work(vs_mem_share_svc_workqueue,
					   &vs_work_recv_msg, 0);
			break;
		default:
			break;
	}
}

static struct qmi_svc_ops_options vs_mem_share_svc_ops_options = {
	.version = 1,
	.service_id = VS_MEM_SHARE_SERVICE_SVC_ID,
	.service_vers = VS_MEM_SHARE_SERVICE_VERS,
	.service_ins = VS_MEM_SHARE_SERVICE_INS_ID,
	.connect_cb = vs_mem_share_svc_connect_cb,
	.disconnect_cb = vs_mem_share_svc_disconnect_cb,
	.req_desc_cb = vs_mem_share_svc_req_desc_cb,
	.req_cb = vs_mem_share_svc_req_cb,
};

int vs_memshare_alloc(struct device *dev,
					int client_id,
					unsigned int block_size,
					struct vs_mem_blocks *pblk)
{

	int ret;

	pr_info("%s: vs_memshare_alloc called", __func__);
	if (!pblk) {
		pr_err("%s: Failed to alloc\n", __func__);
		return -ENOMEM;
	}

	if (client_id == VS_MS_CLIENT_HYDRA_DIAG_V01)
		attrs = DMA_ATTR_NO_KERNEL_MAPPING;
	else
		attrs = 0;

	pblk->virtual_addr = dma_alloc_attrs(dev, block_size,
						&pblk->phy_addr, GFP_KERNEL,
						attrs);
	if (pblk->virtual_addr == NULL) {
		pr_err("allocation failed, %d\n", block_size);
		ret = -ENOMEM;
		return ret;
	}
	pr_info("pblk->phy_addr :%lx, pblk->virtual_addr %lx\n",
		  (unsigned long int)pblk->phy_addr,
		  (unsigned long int)pblk->virtual_addr);
	return 0;
}

static void vs_memshare_init_worker(struct work_struct *work)
{
	int rc;

	vs_mem_share_svc_workqueue =
		create_singlethread_workqueue("vs_mem_share_svc");
	if (!vs_mem_share_svc_workqueue)
		return;

	vs_mem_share_svc_handle = qmi_handle_create(vs_qmi_mem_share_svc_ntfy, NULL);
	if (!vs_mem_share_svc_handle) {
		pr_err("%s: Creating vs_mem_share_svc qmi handle failed\n",
			__func__);
		destroy_workqueue(vs_mem_share_svc_workqueue);
		return;
	}
	rc = qmi_svc_register(vs_mem_share_svc_handle, &vs_mem_share_svc_ops_options);
	if (rc < 0) {
		pr_err("%s: Registering vs mem share svc failed %d\n",
			__func__, rc);
		qmi_handle_destroy(vs_mem_share_svc_handle);
		destroy_workqueue(vs_mem_share_svc_workqueue);
		return;
	}
	pr_info("vs_memshare: vs_memshare_init successful\n");
}

static int vs_memshare_open(struct inode *inode, struct file *file)
{
	int i = 0;
	int pid;
	void *temp;

	mutex_lock(&vs_memsh_drv->memshare_mutex);

	pid = current->tgid;

	for (i = 0; i < vs_memsh_drv->num_clients; i++)
		if (vs_memsh_drv->client_map[i].pid == pid)
				break;

	if (i < vs_memsh_drv->num_clients) {
		pr_err("In %s: client is already opened\n", __func__);
		goto fail;
	}

	vs_memsh_drv->num_clients ++;
	temp = krealloc(vs_memsh_drv->client_map,
				(vs_memsh_drv->num_clients) * sizeof(struct
			 	vs_memshare_client_map), GFP_KERNEL);
	if (!temp) {
		pr_err("In %s: No memory for client_map\n", __func__);
		vs_memsh_drv->num_clients --;
		goto fail;
	} else
		vs_memsh_drv->client_map = temp;

	vs_memsh_drv->client_map[i].pid = pid;
	strlcpy(vs_memsh_drv->client_map[i].name, current->comm, 20);

	pr_info("In %s: Add client[%d] pid %d, name %s\n", __func__, i,
		vs_memsh_drv->client_map[i].pid, vs_memsh_drv->client_map[i].name);

	mutex_unlock(&vs_memsh_drv->memshare_mutex);
	return 0;

fail:
	mutex_unlock(&vs_memsh_drv->memshare_mutex);
	return -ENOMEM;
}

static int vs_memshare_release(struct inode *inode, struct file *file)
{
	int i = 0;
	int pid;

	mutex_lock(&vs_memsh_drv->memshare_mutex);

	pid = current->tgid;

	for (i = 0; i < vs_memsh_drv->num_clients; i++)
		if (vs_memsh_drv->client_map[i].pid == pid)
				break;

	if (i == vs_memsh_drv->num_clients) {
		pr_err("In %s: client pid %d is not found\n", __func__, pid);
		mutex_unlock(&vs_memsh_drv->memshare_mutex);
		return -ENOENT;
	}

	pr_info("In %s: Remove client[%d] pid %d, name %s\n", __func__, i,
		vs_memsh_drv->client_map[i].pid, vs_memsh_drv->client_map[i].name);

	vs_memsh_drv->client_map[i].pid = 0;
	vs_memsh_drv->client_map[i].name[0] = '\0';
	vs_memsh_drv->num_clients --;

	mutex_unlock(&vs_memsh_drv->memshare_mutex);
	return 0;
}

static ssize_t vs_memshare_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	int err = 0, ret = 0;
	char __user *payload_buf = NULL;
	int payload_len = 0;
	int client_id;

	typedef struct
	{
		uint32_t client_id;
		uint32_t num_bytes;
		uint32_t offset;
	} partition_info_type;
	partition_info_type partition_info;

	mutex_lock(&vs_memsh_drv->memshare_mutex);

	if (count < sizeof(partition_info_type)) {
		pr_err("In %s, client is sending short data, len: %d\n",
		       __func__, (int)count);
		ret = -EBADMSG;
		goto exit;
	}

	err = copy_from_user((&partition_info), buf, sizeof(partition_info_type));
	if (err) {
		pr_err_ratelimited("In %s, unable to copy partition_info from userspace, err: %d\n",
				   __func__, err);
		ret = -EIO;
		goto exit;
	}

	//pr_info("client_id %d, num_bytes 0x%x, offset 0x%x\n",
	//	partition_info.client_id, partition_info.num_bytes, partition_info.offset);

	payload_buf = buf + sizeof(partition_info_type);
	payload_len = count - sizeof(partition_info_type);

	client_id = check_client(partition_info.client_id, FREE);
	if (client_id >= MAX_CLIENTS) {
		pr_err("vs_memshare: %s client not found, requested client: %d\n",
				__func__, partition_info.client_id);
		ret = -EIO;
		goto exit;
	}

	if (partition_info.offset + payload_len < partition_info.num_bytes) {
		pr_err("In %s, client is sending short data, len: %d\n",
		       __func__, (int)count);
		ret = -EBADMSG;
		goto exit;
	} else if (vs_memblock[client_id].size < partition_info.offset)
	{
		pr_err("In %s, offset overflows the end of memblock: %d\n",
		       __func__, (int)partition_info.offset);
		ret = -EBADMSG;
		goto exit;
	} else if (vs_memblock[client_id].size == partition_info.offset)
	{
		pr_info("In %s, offset reaches the end of memblock: %d\n",
		       __func__, (int)partition_info.offset);
		ret = 0;
		goto exit;
	} else if (vs_memblock[client_id].size < (partition_info.offset + partition_info.num_bytes))
	{
		pr_info("In %s, offset and num_bytes overflows the end of memblock, use new num_bytes: %d\n",
		       __func__, (int)(vs_memblock[client_id].size - partition_info.offset));
		partition_info.num_bytes = vs_memblock[client_id].size - partition_info.offset;
	}

	err = copy_to_user(payload_buf, vs_memblock[client_id].virtual_addr + partition_info.offset, partition_info.num_bytes);
	if (err) {
		pr_err_ratelimited("In %s, unable to copy data from virt_addr to userspace\n", __func__);
		ret = -EIO;
		goto exit;
	}

	ret = partition_info.num_bytes;

exit:
	mutex_unlock(&vs_memsh_drv->memshare_mutex);
	return ret;
}

static ssize_t vs_memshare_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	return 0;
}

static const struct file_operations vs_memshare_fops = {
	.owner = THIS_MODULE,
	.open = vs_memshare_open,
	.release = vs_memshare_release,
	.read = vs_memshare_read,
	.write = vs_memshare_write,
};

static int vs_memshare_init_add_device(void)
{
	int r = 0;

	mutex_init(&vs_memsh_drv->memshare_mutex);
	wakeup_source_init(&vs_memsh_drv->pa_ws, VS_MEMSHARE_DEV_NAME);

	cdev_init(&vs_memsh_drv->cdev, &vs_memshare_fops);
	vs_memsh_drv->cdev.owner = THIS_MODULE;

	r = cdev_add(&vs_memsh_drv->cdev, vs_memsh_drv->devno, 1);
	if (IS_ERR_VALUE((long)r)) {
		pr_err("%s: cdev_add() failed for vs_memshare_dev ret:%i\n", __func__, r);
		return r;
	}

	vs_memsh_drv->memshare_dev =
		device_create(vs_memsh_drv->memshare_class,
			      NULL,
			      vs_memsh_drv->devno,
			      NULL,
			      VS_MEMSHARE_DEV_NAME);

	if (IS_ERR_OR_NULL(vs_memsh_drv->memshare_dev)) {
		pr_err("%s: device_create() failed for vs_memshare_dev\n", __func__);
		r = -ENOMEM;
		cdev_del(&vs_memsh_drv->cdev);
		wakeup_source_trash(&vs_memsh_drv->pa_ws);
		return r;
	}

	return r;
}

static void vs_memshare_core_deinit(void)
{
	if (vs_memsh_drv->memshare_dev) {
		cdev_del(&vs_memsh_drv->cdev);
		device_destroy(vs_memsh_drv->memshare_class,
			MKDEV(MAJOR(vs_memsh_drv->devno), 0));
		kfree(vs_memsh_drv->memshare_dev);

		if (!IS_ERR_OR_NULL(vs_memsh_drv->memshare_class))
			class_destroy(vs_memsh_drv->memshare_class);

		unregister_chrdev_region(MAJOR(vs_memsh_drv->devno), 1);
	}
}

static int vs_memshare_alloc_chrdev_region(void)
{
	dev_t devno;
	struct class *memshare_class;

	int r = alloc_chrdev_region(&devno,
			       0,
			       1,
			       VS_MEMSHARE_DEV_NAME);
	if (IS_ERR_VALUE((long)r)) {
		pr_err("%s: alloc_chrdev_region() failed ret:%i\n",
			__func__, r);
		return r;
	}

	vs_memsh_drv->devno = devno;

	memshare_class = class_create(THIS_MODULE, VS_MEMSHARE_DEV_NAME);
	if (IS_ERR(memshare_class)) {
		pr_err("%s: class_create() failed ENOMEM\n", __func__);
		r = -ENOMEM;
		unregister_chrdev_region(MAJOR(devno), 1);
		return r;
	}

	vs_memsh_drv->memshare_class = memshare_class;

	return 0;
}

static int vs_memshare_device_init(void)
{
	int ret;

	ret = vs_memshare_alloc_chrdev_region();
	if (ret) {
		pr_err("%s: vs_memshare_alloc_chrdev_region() failed ret:%i\n",
			__func__, ret);
		return ret;
	}

	ret = vs_memshare_init_add_device();
	if (ret < 0) {
		pr_err("add device failed ret=%d\n", ret);
		goto error_destroy;
	}
	return 0;

error_destroy:
	vs_memshare_core_deinit();
	return ret;
}

static int vs_memshare_child_probe(struct platform_device *pdev)
{
	int rc;
	uint32_t size, client_id;
	struct vs_memshare_child *drv;

	drv = devm_kzalloc(&pdev->dev, sizeof(struct vs_memshare_child),
							GFP_KERNEL);

	if (!drv) {
		pr_err("Unable to allocate memory to driver\n");
		return -ENOMEM;
	}

	drv->dev = &pdev->dev;
	vs_memsh_child = drv;
	platform_set_drvdata(pdev, vs_memsh_child);

	rc = of_property_read_u32(pdev->dev.of_node, "vs,peripheral-size",
						&size);
	if (rc) {
		pr_err("In %s, Error reading size of clients, rc: %d\n",
				__func__, rc);
		return rc;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "vs,client-id",
						&client_id);
	if (rc) {
		pr_err("In %s, Error reading client id, rc: %d\n",
				__func__, rc);
		return rc;
	}

	vs_memblock[num_clients].guarantee = of_property_read_bool(
							pdev->dev.of_node,
							"vs,allocate-boot-time");

	vs_memblock[num_clients].size = size;
	vs_memblock[num_clients].client_id = client_id;

  /*
   *	Memshare allocation for guaranteed clients
   */
	if (vs_memblock[num_clients].guarantee) {
		rc = vs_memshare_alloc(vs_memsh_child->dev,
				client_id,
				vs_memblock[num_clients].size,
				&vs_memblock[num_clients]);
		if (rc) {
			pr_err("In %s, Unable to allocate memory for guaranteed clients, rc: %d\n",
							__func__, rc);
			return rc;
		}
		vs_memblock[num_clients].alloted = 1;
	}

	/*
	 *  call for creating ramdump dev handlers for
	 *  memshare clients
	 */

	if (!vs_memblock[num_clients].file_created) {
		rc = vs_mem_share_configure_ramdump(client_id, num_clients);
		if (rc)
			pr_err("In %s, cannot collect dumps for client id: %d\n",
					__func__,
					vs_memblock[num_clients].client_id);
		else
			vs_memblock[num_clients].file_created = 1;
	}

	num_clients++;

	return 0;
}

static int vs_memshare_probe(struct platform_device *pdev)
{
	int rc;
	struct vs_memshare_driver *drv;

	drv = devm_kzalloc(&pdev->dev, sizeof(struct vs_memshare_driver),
							GFP_KERNEL);
	if (!drv) {
		pr_err("Unable to allocate memory to driver\n");
		return -ENOMEM;
	}

	/* Memory allocation has been done successfully */
	mutex_init(&drv->mem_free);
	mutex_init(&drv->mem_share);

	INIT_WORK(&drv->vs_memshare_init_work, vs_memshare_init_worker);
	schedule_work(&drv->vs_memshare_init_work);

	drv->dev = &pdev->dev;
	vs_memsh_drv = drv;
	platform_set_drvdata(pdev, vs_memsh_drv);
	vs_initialize_client();
	num_clients = 0;

	rc = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);

	if (rc) {
		pr_err("In %s, error populating the devices\n", __func__);
		return rc;
	}

	subsys_notif_register_notifier("modem", &nb);

	rc = vs_memshare_device_init();
	if (rc) {
		pr_err("%s: device tree init failed\n", __func__);
		return rc;
	}
	
	pr_info("In %s, VS Memshare probe success\n", __func__);

	return 0;
}

static int vs_memshare_remove(struct platform_device *pdev)
{
	if (!vs_memsh_drv)
		return 0;

	qmi_svc_unregister(vs_mem_share_svc_handle);
	flush_workqueue(vs_mem_share_svc_workqueue);
	qmi_handle_destroy(vs_mem_share_svc_handle);
	destroy_workqueue(vs_mem_share_svc_workqueue);

	return 0;
}

static int vs_memshare_child_remove(struct platform_device *pdev)
{
	if (!vs_memsh_child)
		return 0;

	return 0;
}

static struct of_device_id vs_memshare_match_table[] = {
	{
		.compatible = "vs,memshare",
	},
	{}
};

static struct of_device_id vs_memshare_match_table1[] = {
	{
		.compatible = "vs,memshare-peripheral",
	},
	{}
};


static struct platform_driver vs_memshare_pdriver = {
	.probe          = vs_memshare_probe,
	.remove         = vs_memshare_remove,
	.driver = {
		.name   = VS_MEMSHARE_DEV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = vs_memshare_match_table,
	},
};

static struct platform_driver vs_memshare_pchild = {
	.probe          = vs_memshare_child_probe,
	.remove         = vs_memshare_child_remove,
	.driver = {
		.name   = VS_MEMSHARE_CHILD_DEV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = vs_memshare_match_table1,
	},
};

module_platform_driver(vs_memshare_pdriver);
module_platform_driver(vs_memshare_pchild);

static int __init vs_memshare_init(void)
{
	char *p;

	pr_info("vs_memshare: cmdline %s\n", boot_command_line);

	p = strstr(boot_command_line, "androidboot.hw_boardid=");
	if (p) {
		oeminfo_boardid = (uint32_t)simple_strtol(p + strlen("androidboot.hw_boardid="), NULL, 16);
		pr_info("vs_memshare: oeminfo_boardid 0x%x\n", oeminfo_boardid);
	}

	p = strstr(boot_command_line, "androidboot.country=");
	if (p) {
		oeminfo_country[0] = *(p + strlen("androidboot.country="));
		oeminfo_country[1] = *(p + strlen("androidboot.country=") + 1);
		pr_info("vs_memshare: oeminfo_country %c%c\n", oeminfo_country[0], oeminfo_country[1]);
	}

	p = strstr(boot_command_line, "androidboot.carrier=");
	if (p) {
		oeminfo_carrier[0] = *(p + strlen("androidboot.carrier="));
		oeminfo_carrier[1] = *(p + strlen("androidboot.carrier=") + 1);
		pr_info("vs_memshare: oeminfo_carrier %c%c\n", oeminfo_carrier[0], oeminfo_carrier[1]);
	}

	p = strstr(boot_command_line, "androidboot.saleschannel=");
	if (p) {
		oeminfo_saleschannel[0] = *(p + strlen("androidboot.saleschannel="));
		oeminfo_saleschannel[1] = *(p + strlen("androidboot.saleschannel=") + 1);
		pr_info("vs_memshare: oeminfo_saleschannel %c%c\n", oeminfo_saleschannel[0], oeminfo_saleschannel[1]);
	}

	p = strstr(boot_command_line, "androidboot.target=");
	if (p) {
		oeminfo_target[0] = *(p + strlen("androidboot.target="));
		oeminfo_target[1] = *(p + strlen("androidboot.target=") + 1);
		pr_info("vs_memshare: oeminfo_target %c%c\n", oeminfo_target[0], oeminfo_target[1]);
	}

	return 0;
}

module_init(vs_memshare_init);

MODULE_DESCRIPTION("VS Mem Share QMI Service Driver");
MODULE_LICENSE("GPL v2");
