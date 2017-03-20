/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
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

#ifndef VS_MS_SERVICE_01_H
#define VS_MS_SERVICE_01_H

#include <soc/qcom/msm_qmi_interface.h>

#define VS_MS_MEM_ALLOC_REQ_MAX_MSG_LEN_V01 255
#define VS_MS_MEM_FREE_REQ_MAX_MSG_LEN_V01 255
#define VS_MS_FILE_STAT_REQ_MAX_MSG_LEN_V01 300
#define VS_MS_FILE_READ_REQ_MAX_MSG_LEN_V01 300
#define VS_MS_FILE_WRITE_REQ_MAX_MSG_LEN_V01 300
#define VS_MS_GET_CUST_INFO_REQ_MAX_MSG_LEN_V01 300

#define MAX_FILE_PATH_V01 255

struct vs_ms_mem_alloc_addr_info_type_v01 {
	uint64_t phy_addr;
	uint32_t num_bytes;
};

enum vs_ms_client_id_v01 {
	/*To force a 32 bit signed enum.  Do not change or use*/
	VS_MS_CLIENT_ID_MIN_ENUM_VAL_V01 = -2147483647,
	/*  Request from HYDRA RFSA Client    */
	VS_MS_CLIENT_HYDRA_RFSA_V01 = 0,
	/*  Request from HYDRA NVEFS TRACE Client    */
	VS_MS_CLIENT_HYDRA_NVEFS_TRACE_V01 = 1,
	/*  Request from HYDRA VOICE TRACE Client    */
	VS_MS_CLIENT_HYDRA_VOICE_TRACE_V01 = 2,
	/*  Request from HYDRA STATS TRACE Client    */
	VS_MS_CLIENT_HYDRA_STATS_TRACE_V01 = 3,
	/*  Request from DIAG Client    */
	VS_MS_CLIENT_HYDRA_DIAG_V01 = 4,
	/* Invalid Client */
	VS_MS_CLIENT_INVALID = 1000,
	/* To force a 32 bit signed enum.  Do not change or use */
	VS_MS_CLIENT_ID_MAX_ENUM_VAL_V01 = 2147483647
};

enum vs_ms_mem_block_align_enum_v01 {
	/* To force a 32 bit signed enum.  Do not change or use
	*/
	VS_MS_MEM_BLOCK_ALIGN_ENUM_MIN_ENUM_VAL_V01 = -2147483647,
	/* Align allocated memory by 2 bytes  */
	VS_MS_MEM_BLOCK_ALIGN_2_V01 = 0,
	/* Align allocated memory by 4 bytes  */
	VS_MS_MEM_BLOCK_ALIGN_4_V01 = 1,
	/**<  Align allocated memory by 8 bytes */
	VS_DHMS_MEM_BLOCK_ALIGN_8_V01 = 2,
	/**<  Align allocated memory by 16 bytes */
	VS_MS_MEM_BLOCK_ALIGN_16_V01 = 3,
	/**<  Align allocated memory by 32 bytes */
	VS_MS_MEM_BLOCK_ALIGN_32_V01 = 4,
	/**<  Align allocated memory by 64 bytes */
	VS_MS_MEM_BLOCK_ALIGN_64_V01 = 5,
	/**<  Align allocated memory by 128 bytes */
	VS_MS_MEM_BLOCK_ALIGN_128_V01 = 6,
	/**<  Align allocated memory by 256 bytes */
	VS_MS_MEM_BLOCK_ALIGN_256_V01 = 7,
	/**<  Align allocated memory by 512 bytes */
	VS_MS_MEM_BLOCK_ALIGN_512_V01 = 8,
	/**<  Align allocated memory by 1024 bytes */
	VS_MS_MEM_BLOCK_ALIGN_1K_V01 = 9,
	/**<  Align allocated memory by 2048 bytes */
	VS_MS_MEM_BLOCK_ALIGN_2K_V01 = 10,
	/**<  Align allocated memory by 4096 bytes */
	VS_MS_MEM_BLOCK_ALIGN_4K_V01 = 11,
	VS_MS_MEM_BLOCK_ALIGN_ENUM_MAX_ENUM_VAL_V01 = 2147483647
	/* To force a 32 bit signed enum.  Do not change or use
	*/
};

typedef uint64_t vs_ms_file_access_flag_mask_v01;
#define VS_MS_FILE_ACCESS_FLAG_READ_V01 ((vs_ms_file_access_flag_mask_v01)0x01ull) 
#define VS_MS_FILE_ACCESS_FLAG_WRITE_V01 ((vs_ms_file_access_flag_mask_v01)0x02ull) 
#define VS_MS_FILE_ACCESS_FLAG_APPEND_V01 ((vs_ms_file_access_flag_mask_v01)0x04ull) 
#define VS_MS_FILE_ACCESS_FLAG_CREATE_V01 ((vs_ms_file_access_flag_mask_v01)0x08ull) 
#define VS_MS_FILE_ACCESS_FLAG_TRUNC_V01 ((vs_ms_file_access_flag_mask_v01)0x10ull) 

struct vs_ms_file_content_type_v01 {

	uint32_t count;

	uint64_t buffer;
};

struct vs_ms_mem_alloc_generic_req_msg_v01 {

	/* Mandatory */
	uint32_t num_bytes;

	/* Mandatory */
	enum vs_ms_client_id_v01 client_id;

	/* Mandatory */
	uint32_t sequence_id;

	/* Optional */
	uint8_t alloc_contiguous_valid;  /**< Must be set to true if alloc_contiguous is being passed */
	uint8_t alloc_contiguous;

	/* Optional */
	uint8_t block_alignment_valid;  /**< Must be set to true if block_alignment is being passed */
	enum vs_ms_mem_block_align_enum_v01 block_alignment;
}; /* Message */

struct vs_ms_mem_alloc_generic_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

	/* Optional */
	uint8_t sequence_id_valid;  /**< Must be set to true if sequence_id is being passed */
	uint32_t sequence_id;

	/* Optional */
	uint8_t vs_ms_mem_alloc_addr_info_valid;	/**< Must be set to true if vs_ms_mem_alloc_addr_info is being passed */
	struct vs_ms_mem_alloc_addr_info_type_v01 vs_ms_mem_alloc_addr_info;
};/* Message */

struct vs_ms_mem_free_generic_req_msg_v01 {

	/* Mandatory */
	struct vs_ms_mem_alloc_addr_info_type_v01 vs_ms_mem_alloc_addr_info;

	/* Optional */
	uint8_t client_id_valid;	/**< Must be set to true if client_id is being passed */
	enum vs_ms_client_id_v01 client_id;
}; /* Message */

struct vs_ms_mem_free_generic_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;
}; /* Message */

struct vs_ms_mem_query_size_req_msg_v01 {

	/* Mandatory */
	enum vs_ms_client_id_v01 client_id;
}; /* Message */

struct vs_ms_mem_query_size_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

	/* Optional */
	uint8_t size_valid;  /**< Must be set to true if size is being passed */
	uint32_t size;
}; /* Message */

struct vs_ms_file_stat_req_msg_v01 {

	/* Mandatory */
	char filename[MAX_FILE_PATH_V01 + 1];
}; /* Message */

struct vs_ms_file_stat_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

	/* Mandatory */
	vs_ms_file_access_flag_mask_v01 flags;

	/* Optional */
	uint8_t size_valid;  /**< Must be set to true if size is being passed */
	uint32_t size;
}; /* Message */

struct vs_ms_file_read_req_msg_v01 {

	/* Mandatory */
	enum vs_ms_client_id_v01 client_id;

	/* Mandatory */
	char filename[MAX_FILE_PATH_V01 + 1];

	/* Mandatory */
	uint32_t offset;

	/* Mandatory */
	uint32_t size;
}; /* Message */

struct vs_ms_file_read_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

	/* Optional */
	uint8_t data_valid;  /**< Must be set to true if data is being passed */
	struct vs_ms_file_content_type_v01 data;
}; /* Message */

struct vs_ms_file_write_req_msg_v01 {

	/* Mandatory */
	enum vs_ms_client_id_v01 client_id;

	/* Mandatory */
	char filename[MAX_FILE_PATH_V01 + 1];

	/* Mandatory */
	vs_ms_file_access_flag_mask_v01 flags;

	/* Mandatory */
	uint32_t offset;

	/* Mandatory */
	uint32_t size;
}; /* Message */

struct vs_ms_file_write_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

	/* Optional */
	uint8_t count_valid;  /**< Must be set to true if data is being passed */
	uint32_t count;
}; /* Message */

struct vs_ms_get_cust_info_req_msg_v01 {

	char __placeholder;
}; /* Message */

struct vs_ms_get_cust_info_resp_msg_v01 {

	/* Mandatory */
	struct qmi_response_type_v01 resp;

    /* Optional */
	uint8_t boardid_valid;  /**< Must be set to true if boardid is being passed */
	uint32_t boardid;

	/* Optional */
	uint8_t country_valid;  /**< Must be set to true if country is being passed */
	char country[2];

	/* Optional */
	uint8_t carrier_valid;  /**< Must be set to true if carrier is being passed */
	char carrier[2];

	/* Optional */
	uint8_t saleschannel_valid;  /**< Must be set to true if saleschannel is being passed */
	char saleschannel[2];

	/* Optional */
	uint8_t target_valid;  /**< Must be set to true if target is being passed */
	char target[2];
}; /* Message */


extern struct elem_info vs_ms_mem_alloc_generic_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_mem_alloc_generic_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_mem_free_generic_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_mem_free_generic_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_mem_query_size_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_mem_query_size_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_stat_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_stat_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_read_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_read_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_write_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_file_write_resp_msg_data_v01_ei[];
extern struct elem_info vs_ms_get_cust_info_req_msg_data_v01_ei[];
extern struct elem_info vs_ms_get_cust_info_resp_msg_data_v01_ei[];

/*Service Message Definition*/
#define VS_MS_MEM_ALLOC_GENERIC_REQ_MSG_V01 0x0020
#define VS_MS_MEM_ALLOC_GENERIC_RESP_MSG_V01 0x0020
#define VS_MS_MEM_FREE_GENERIC_REQ_MSG_V01 0x0021
#define VS_MS_MEM_FREE_GENERIC_RESP_MSG_V01 0x0021
#define VS_MS_MEM_QUERY_SIZE_REQ_MSG_V01 0x0022
#define VS_MS_MEM_QUERY_SIZE_RESP_MSG_V01 0x0022
#define VS_MS_FILE_STAT_REQ_MSG_V01 0x0030
#define VS_MS_FILE_STAT_RESP_MSG_V01 0x0030
#define VS_MS_FILE_READ_REQ_MSG_V01 0x0031
#define VS_MS_FILE_READ_RESP_MSG_V01 0x0031
#define VS_MS_FILE_WRITE_REQ_MSG_V01 0x0032
#define VS_MS_FILE_WRITE_RESP_MSG_V01 0x0032
#define VS_MS_GET_CUST_INFO_REQ_MSG_V01 0x0040
#define VS_MS_GET_CUST_INFO_RESP_MSG_V01 0x0040

#endif
