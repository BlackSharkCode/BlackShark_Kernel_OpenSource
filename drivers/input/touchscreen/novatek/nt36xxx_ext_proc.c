/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 *
 * $Revision: 15382 $
 * $Date: 2017-08-15 09:19:01 +0800 (Tue, 15 Aug 2017) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */


#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "nt36xxx.h"

#if NVT_TOUCH_EXT_PROC
#define NVT_FW_VERSION "nvt_fw_version"
#define NVT_BASELINE "nvt_baseline"
#define NVT_RAW "nvt_raw"
#define NVT_DIFF "nvt_diff"
#define NVT_XIAOMI_CONFIG_INFO "nvt_xiaomi_config_info"
#define NVT_XIAOMI_LOCKDOWN_INFO "tp_lockdown_info"
#define NVT_BLACKSHARK_COLOR_DATA_RW_EXAMPLE "nvt_blackshark_color_data_rw_example"
#define NVT_DOZE_SWITCH "nvt_doze_switch"
#define NVT_SENSITIVITY "nvt_sensitivity"

#define I2C_TANSFER_LENGTH  64

#define NORMAL_MODE 0x00
#define TEST_MODE_1 0x21
#define TEST_MODE_2 0x22
#define HANDSHAKING_HOST_READY 0xBB

#define XDATA_SECTOR_SIZE   256

#if BOOT_UPDATE_FIRMWARE
extern int32_t Init_BootLoader(void);
extern int32_t Resume_PD(void);
#endif
extern int32_t Sector_Erase(int32_t sector_num);
extern int32_t Fast_Read_Checksum(uint32_t flash_address, int32_t length, uint16_t *checksum);

static uint8_t xdata_tmp[2048] = {0};
static int32_t xdata[2048] = {0};
static int32_t xdata_i[2048] = {0};
static int32_t xdata_q[2048] = {0};

static struct proc_dir_entry *NVT_proc_fw_version_entry;
static struct proc_dir_entry *NVT_proc_baseline_entry;
static struct proc_dir_entry *NVT_proc_raw_entry;
static struct proc_dir_entry *NVT_proc_diff_entry;
static struct proc_dir_entry *NVT_proc_xiaomi_config_info_entry;
static struct proc_dir_entry *NVT_proc_xiaomi_lockdown_info_entry;
static struct proc_dir_entry *NVT_proc_blackshark_color_data_rw_example_entry;
static struct proc_dir_entry *NVT_proc_doze_switch_entry;
static struct proc_dir_entry *NVT_proc_sensitivity_entry;

// Xiaomi Config Info.
static uint8_t nvt_xiaomi_conf_info_fw_ver = 0;
static uint8_t nvt_xiaomi_conf_info_fae_id = 0;
static uint64_t nvt_xiaomi_conf_info_reservation = 0;
// Xiaomi Lockdown Info
static uint8_t tp_maker_cg_lamination = 0;
static uint8_t display_maker = 0;
static uint8_t cg_ink_color = 0;
static uint8_t hw_version = 0;
static uint16_t project_id = 0;
static uint8_t cg_maker = 0;
static uint8_t reservation_byte = 0;

/*******************************************************
Description:
	Novatek touchscreen change mode function.

return:
	n.a.
*******************************************************/
void nvt_change_mode(uint8_t mode)
{
	uint8_t buf[8] = {0};

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	//---set mode---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = mode;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

	if (mode == NORMAL_MODE) {
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = HANDSHAKING_HOST_READY;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		msleep(20);
	}
}

/*******************************************************
Description:
	Novatek touchscreen get firmware pipe function.

return:
	Executive outcomes. 0---pipe 0. 1---pipe 1.
*******************************************************/
uint8_t nvt_get_fw_pipe(void)
{
	uint8_t buf[8]= {0};

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	//---read fw status---
	buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
	buf[1] = 0x00;
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

	//NVT_LOG("FW pipe=%d, buf[1]=0x%02X\n", (buf[1]&0x01), buf[1]);

	return (buf[1] & 0x01);
}

/*******************************************************
Description:
	Novatek touchscreen read meta data function.

return:
	n.a.
*******************************************************/
void nvt_read_mdata(uint32_t xdata_addr, uint32_t xdata_btn_addr)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[I2C_TANSFER_LENGTH + 1] = {0};
	uint32_t head_addr = 0;
	int32_t dummy_len = 0;
	int32_t data_len = 0;
	int32_t residual_len = 0;

	//---set xdata sector address & length---
	head_addr = xdata_addr - (xdata_addr % XDATA_SECTOR_SIZE);
	dummy_len = xdata_addr - head_addr;
	data_len = ts->x_num * ts->y_num * 2;
	residual_len = (head_addr + dummy_len + data_len) % XDATA_SECTOR_SIZE;

	//printk("head_addr=0x%05X, dummy_len=0x%05X, data_len=0x%05X, residual_len=0x%05X\n", head_addr, dummy_len, data_len, residual_len);

	//read xdata : step 1
	for (i = 0; i < ((dummy_len + data_len) / XDATA_SECTOR_SIZE); i++) {
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = ((head_addr + XDATA_SECTOR_SIZE * i) >> 16) & 0xFF;
		buf[2] = ((head_addr + XDATA_SECTOR_SIZE * i) >> 8) & 0xFF;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---read xdata by I2C_TANSFER_LENGTH
		for (j = 0; j < (XDATA_SECTOR_SIZE / I2C_TANSFER_LENGTH); j++) {
			//---read data---
			buf[0] = I2C_TANSFER_LENGTH * j;
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, I2C_TANSFER_LENGTH + 1);

			//---copy buf to xdata_tmp---
			for (k = 0; k < I2C_TANSFER_LENGTH; k++) {
				xdata_tmp[XDATA_SECTOR_SIZE * i + I2C_TANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04X\n", buf[k+1], (XDATA_SECTOR_SIZE*i + I2C_TANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (head_addr+XDATA_SECTOR_SIZE*i));
	}

	//read xdata : step2
	if (residual_len != 0) {
		//---change xdata index---
		buf[0] = 0xFF;
		buf[1] = ((xdata_addr + data_len - residual_len) >> 16) & 0xFF;
		buf[2] = ((xdata_addr + data_len - residual_len) >> 8) & 0xFF;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---read xdata by I2C_TANSFER_LENGTH
		for (j = 0; j < (residual_len / I2C_TANSFER_LENGTH + 1); j++) {
			//---read data---
			buf[0] = I2C_TANSFER_LENGTH * j;
			CTP_I2C_READ(ts->client, I2C_FW_Address, buf, I2C_TANSFER_LENGTH + 1);

			//---copy buf to xdata_tmp---
			for (k = 0; k < I2C_TANSFER_LENGTH; k++) {
				xdata_tmp[(dummy_len + data_len - residual_len) + I2C_TANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04x\n", buf[k+1], ((dummy_len+data_len-residual_len) + I2C_TANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (xdata_addr+data_len-residual_len));
	}

	//---remove dummy data and 2bytes-to-1data---
	for (i = 0; i < (data_len / 2); i++) {
		xdata[i] = (int16_t)(xdata_tmp[dummy_len + i * 2] + 256 * xdata_tmp[dummy_len + i * 2 + 1]);
	}

#if TOUCH_KEY_NUM > 0
	//read button xdata : step3
	//---change xdata index---
	buf[0] = 0xFF;
	buf[1] = (xdata_btn_addr >> 16) & 0xFF;
	buf[2] = ((xdata_btn_addr >> 8) & 0xFF);
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	//---read data---
	buf[0] = (xdata_btn_addr & 0xFF);
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, (TOUCH_KEY_NUM * 2 + 1));

	//---2bytes-to-1data---
	for (i = 0; i < TOUCH_KEY_NUM; i++) {
		xdata[ts->x_num * ts->y_num + i] = (int16_t)(buf[1 + i * 2] + 256 * buf[1 + i * 2 + 1]);
	}
#endif

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
}

/*******************************************************
Description:
	Novatek touchscreen read meta data from IQ to rss function.

return:
	n.a.
*******************************************************/
void nvt_read_mdata_rss(uint32_t xdata_i_addr, uint32_t xdata_q_addr, uint32_t xdata_btn_i_addr, uint32_t xdata_btn_q_addr)
{
	int i = 0;

	nvt_read_mdata(xdata_i_addr, xdata_btn_i_addr);
	memcpy(xdata_i, xdata, ((ts->x_num * ts->y_num + TOUCH_KEY_NUM) * sizeof(int32_t)));

	nvt_read_mdata(xdata_q_addr, xdata_btn_q_addr);
	memcpy(xdata_q, xdata, ((ts->x_num * ts->y_num + TOUCH_KEY_NUM) * sizeof(int32_t)));

	for (i = 0; i < (ts->x_num * ts->y_num + TOUCH_KEY_NUM); i++) {
		xdata[i] = (int32_t)int_sqrt((unsigned long)(xdata_i[i] * xdata_i[i]) + (unsigned long)(xdata_q[i] * xdata_q[i]));
	}
}

/*******************************************************
Description:
    Novatek touchscreen get meta data function.

return:
    n.a.
*******************************************************/
void nvt_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num)
{
    *m_x_num = ts->x_num;
    *m_y_num = ts->y_num;
    memcpy(buf, xdata, ((ts->x_num * ts->y_num + TOUCH_KEY_NUM) * sizeof(int32_t)));
}

/*******************************************************
Description:
	Novatek touchscreen firmware version show function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_fw_version_show(struct seq_file *m, void *v)
{
	seq_printf(m, "fw_ver=%d, x_num=%d, y_num=%d, button_num=%d\n", ts->fw_ver, ts->x_num, ts->y_num, ts->max_button_num);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print show
	function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_show(struct seq_file *m, void *v)
{
	int32_t i = 0;
	int32_t j = 0;

	for (i = 0; i < ts->y_num; i++) {
		for (j = 0; j < ts->x_num; j++) {
			seq_printf(m, "%5d, ", xdata[i * ts->x_num + j]);
		}
		seq_puts(m, "\n");
	}

#if TOUCH_KEY_NUM > 0
	for (i = 0; i < TOUCH_KEY_NUM; i++) {
		seq_printf(m, "%5d, ", xdata[ts->x_num * ts->y_num + i]);
	}
	seq_puts(m, "\n");
#endif

	seq_printf(m, "\n\n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print start
	function.

return:
	Executive outcomes. 1---call next function.
	NULL---not call next function and sequence loop
	stop.
*******************************************************/
static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print next
	function.

return:
	Executive outcomes. NULL---no next and call sequence
	stop function.
*******************************************************/
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print stop
	function.

return:
	n.a.
*******************************************************/
static void c_stop(struct seq_file *m, void *v)
{
	return;
}

const struct seq_operations nvt_fw_version_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = c_fw_version_show
};

const struct seq_operations nvt_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = c_show
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_fw_version open
	function.

return:
	n.a.
*******************************************************/
static int32_t nvt_fw_version_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return seq_open(file, &nvt_fw_version_seq_ops);
}

static const struct file_operations nvt_fw_version_fops = {
	.owner = THIS_MODULE,
	.open = nvt_fw_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_baseline open function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_baseline_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	if (nvt_clear_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	nvt_change_mode(TEST_MODE_2);

	if (nvt_check_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (ts->carrier_system) {
		nvt_read_mdata_rss(ts->mmap->BASELINE_ADDR, ts->mmap->BASELINE_Q_ADDR,
				ts->mmap->BASELINE_BTN_ADDR, ts->mmap->BASELINE_BTN_Q_ADDR);
	} else {
		nvt_read_mdata(ts->mmap->BASELINE_ADDR, ts->mmap->BASELINE_BTN_ADDR);
	}

	nvt_change_mode(NORMAL_MODE);

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_baseline_fops = {
	.owner = THIS_MODULE,
	.open = nvt_baseline_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_raw open function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_raw_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	if (nvt_clear_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	nvt_change_mode(TEST_MODE_2);

	if (nvt_check_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (ts->carrier_system) {
		if (nvt_get_fw_pipe() == 0)
			nvt_read_mdata_rss(ts->mmap->RAW_PIPE0_ADDR, ts->mmap->RAW_PIPE0_Q_ADDR,
				ts->mmap->RAW_BTN_PIPE0_ADDR, ts->mmap->RAW_BTN_PIPE0_Q_ADDR);
		else
			nvt_read_mdata_rss(ts->mmap->RAW_PIPE1_ADDR, ts->mmap->RAW_PIPE1_Q_ADDR,
				ts->mmap->RAW_BTN_PIPE1_ADDR, ts->mmap->RAW_BTN_PIPE1_Q_ADDR);
	} else {
		if (nvt_get_fw_pipe() == 0)
			nvt_read_mdata(ts->mmap->RAW_PIPE0_ADDR, ts->mmap->RAW_BTN_PIPE0_ADDR);
		else
			nvt_read_mdata(ts->mmap->RAW_PIPE1_ADDR, ts->mmap->RAW_BTN_PIPE1_ADDR);
	}

	nvt_change_mode(NORMAL_MODE);

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_raw_fops = {
	.owner = THIS_MODULE,
	.open = nvt_raw_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_diff open function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_diff_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	if (nvt_clear_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	nvt_change_mode(TEST_MODE_2);

	if (nvt_check_fw_status()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	if (ts->carrier_system) {
		if (nvt_get_fw_pipe() == 0)
			nvt_read_mdata_rss(ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_PIPE0_Q_ADDR,
				ts->mmap->DIFF_BTN_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_Q_ADDR);
		else
			nvt_read_mdata_rss(ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_PIPE1_Q_ADDR,
				ts->mmap->DIFF_BTN_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_Q_ADDR);
	} else {
		if (nvt_get_fw_pipe() == 0)
			nvt_read_mdata(ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_ADDR);
		else
			nvt_read_mdata(ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_ADDR);
	}

	nvt_change_mode(NORMAL_MODE);

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_diff_fops = {
	.owner = THIS_MODULE,
	.open = nvt_diff_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int nvt_xiaomi_config_info_show(struct seq_file *m, void *v)
{
	seq_printf(m, "FW version/Config version, Debug version: 0x%02X\n", nvt_xiaomi_conf_info_fw_ver);
	seq_printf(m, "FAE ID: 0x%02X\n", nvt_xiaomi_conf_info_fae_id);
	seq_printf(m, "Reservation byte: 0x%012llX\n", nvt_xiaomi_conf_info_reservation);

	return 0;
}

static int32_t nvt_xiaomi_config_info_open(struct inode *inode, struct file *file)
{
	uint8_t buf[16] = {0};

	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	buf[0] = 0x9C;
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 9);

	nvt_xiaomi_conf_info_fw_ver = buf[1];
	nvt_xiaomi_conf_info_fae_id = buf[2];
	nvt_xiaomi_conf_info_reservation = (((uint64_t)buf[3] << 40) | ((uint64_t)buf[4] << 32) | ((uint64_t)buf[5] << 24) | ((uint64_t)buf[6] << 16) | ((uint64_t)buf[7] << 8) | (uint64_t)buf[8]);

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return single_open(file, nvt_xiaomi_config_info_show, NULL);
}

static const struct file_operations nvt_xiaomi_config_info_fops = {
	.owner = THIS_MODULE,
	.open = nvt_xiaomi_config_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int32_t nvt_get_oem_data(uint8_t *data, uint32_t flash_address, int32_t size)
{
	uint8_t buf[64] = {0};
	uint8_t tmp_data[512] = {0};
	int32_t count_256 = 0;
	uint32_t cur_flash_addr = 0;
	uint32_t cur_sram_addr = 0;
	uint16_t checksum_get = 0;
	uint16_t checksum_cal = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t ret = 0;
	int32_t retry = 0;

	NVT_LOG("++\n");

	// maximum 256 bytes each read
	if (size % 256)
		count_256 = size / 256 + 1;
	else
		count_256 = size / 256;

get_oem_data_retry:
	nvt_sw_reset_idle();

	// Step 1: Initial BootLoader
	ret = Init_BootLoader();
	if (ret < 0) {
		NVT_ERR("Init BootLoader failed!\n");
		goto get_oem_data_out;
	}

	// Step 2: Resume PD
	ret = Resume_PD();
	if (ret < 0) {
		NVT_ERR("Resume PD failed!\n");
		goto get_oem_data_out;
	}

	// Step 3: Unlock
	buf[0] = 0x00;
	buf[1] = 0x35;
	ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
	if (ret < 0) {
		NVT_ERR("Unlock failed!\n");
		goto get_oem_data_out;
	}
	msleep(10);

	for (i = 0; i < count_256; i++) {
		cur_flash_addr = flash_address + i * 256;
		// Step 4: Flash Read Command
		buf[0] = 0x00;
		buf[1] = 0x03;
		buf[2] = ((cur_flash_addr >> 16) & 0xFF);
		buf[3] = ((cur_flash_addr >> 8) & 0xFF);
		buf[4] = (cur_flash_addr & 0xFF);
		buf[5] = 0x00;
		buf[6] = 0xFF;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 7);
		if (ret < 0) {
			NVT_ERR("Flash Read Command error!!(%d)\n", ret);
			goto get_oem_data_out;
		}
		msleep(10);
		// Check 0xAA (Read Command)
		buf[0] = 0x00;
		buf[2] = 0x00;
		ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			NVT_ERR("Check 0xAA (Read Command) error!!(%d)\n", ret);
			goto get_oem_data_out;
		}
		if (buf[1] != 0xAA) {
			NVT_ERR("Check 0xAA (Read Command) error!! status=0x%02X\n", buf[1]);
			ret = -1;
			goto get_oem_data_out;
		}
		msleep(10);

		// Step 5: Read Data and Checksum
		for (j = 0; j < ((256 / 32) + 1); j++) {
			cur_sram_addr = ts->mmap->READ_FLASH_CHECKSUM_ADDR + j * 32;
			buf[0] = 0xFF;
			buf[1] = (cur_sram_addr >> 16) & 0xFF;
			buf[2] = (cur_sram_addr  >> 8) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);
			if (ret < 0) {
				NVT_ERR("change I2C buffer index error!!(%d)\n", ret);
				goto get_oem_data_out;
			}

			buf[0] = cur_sram_addr & 0xFF;
			ret = CTP_I2C_READ(ts->client, I2C_BLDR_Address, buf, 33);
			if (ret < 0) {
				NVT_ERR("Read Data and Checksum error!!(%d)\n", ret);
				goto get_oem_data_out;
			}

			memcpy(tmp_data + j * 32, buf + 1, 32);
		}
		// get checksum of the 256 bytes data read
		checksum_get = (uint16_t)((tmp_data[1] << 8) | tmp_data[0]);
		// calculate checksum of of the 256 bytes data read
		checksum_cal = (uint16_t)((cur_flash_addr >> 16) & 0xFF) + (uint16_t)((cur_flash_addr >> 8) & 0xFF) + (cur_flash_addr & 0xFF) + 0x00 + 0xFF;
		for (j = 0; j < 256; j++) {
			checksum_cal += tmp_data[j + 2];
		}
		checksum_cal = 65535 - checksum_cal + 1;
		//NVT_LOG("checksum_get = 0x%04X, checksum_cal = 0x%04X\n", checksum_get, checksum_cal);
		// compare the checksum got and calculated
		if (checksum_get != checksum_cal) {
			if (retry < 3) {
				retry++;
				goto get_oem_data_retry;
			} else {
				NVT_ERR("Checksum not match error! checksum_get=0x%04X, checksum_cal=0x%04X, i=%d\n", checksum_get, checksum_cal, i);
				ret = -2;
				goto get_oem_data_out;
			}
		}

		// Step 6: Remapping (Remove 2 Bytes Checksum)
		if ((i + 1) * 256 > size) {
			memcpy(data + i * 256, tmp_data + 2, size - i * 256);
		} else {
			memcpy(data + i * 256, tmp_data + 2, 256);
		}
	}

#if 0 // for debug
	for (i = 0; i < size; i++) {
		if (i % 16 == 0)
			printk("\n");
		printk("%02X ", data[i]);
	}
	printk("\n");
#endif // for debug

get_oem_data_out:
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);

	NVT_LOG("--\n");

	return ret;
}

static int32_t nvt_get_xiaomi_lockdown_info(void)
{
	uint8_t data_buf[8] = {0};
	int ret = 0;

	ret = nvt_get_oem_data(data_buf, 0x1E000, 8);

	if (ret < 0) {
		NVT_ERR("get oem data failed!\n");
	} else {
		tp_maker_cg_lamination = data_buf[0];
		NVT_LOG("The maker of Touch Panel & CG Lamination: 0x%02X\n", tp_maker_cg_lamination);
		display_maker = data_buf[1];
		NVT_LOG("Display maker: 0x%02X\n", display_maker);
		cg_ink_color = data_buf[2];
		NVT_LOG("CG ink color: 0x%02X\n", cg_ink_color);
		hw_version = data_buf[3];
		NVT_LOG("HW version: 0x%02X\n", hw_version);
		project_id = ((data_buf[4] << 8) | data_buf[5]);
		NVT_LOG("Project ID: 0x%04X\n", project_id);
		cg_maker = data_buf[6];
		NVT_LOG("CG maker: 0x%02X\n", cg_maker);
		reservation_byte = data_buf[7];
		NVT_LOG("Reservation byte: 0x%02X\n", reservation_byte);
	}

	return ret;
}

static int nvt_xiaomi_lockdown_info_show(struct seq_file *m, void *v)
{
	seq_printf(m, "The maker of Touch Panel & CG Lamination: 0x%02X\n", tp_maker_cg_lamination);
	seq_printf(m, "Display maker: 0x%02X\n", display_maker);
	seq_printf(m, "CG ink color: 0x%02X\n", cg_ink_color);
	seq_printf(m, "HW version: 0x%02X\n", hw_version);
	seq_printf(m, "Project ID: 0x%04X\n", project_id);
	seq_printf(m, "CG maker: 0x%02X\n", cg_maker);
	seq_printf(m, "Reservation byte: 0x%02X\n", reservation_byte);

	return 0;
}

static int32_t nvt_xiaomi_lockdown_info_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */
	if (nvt_get_xiaomi_lockdown_info()) {
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return single_open(file, nvt_xiaomi_lockdown_info_show, NULL);
}

static const struct file_operations nvt_xiaomi_lockdown_info_fops = {
	.owner = THIS_MODULE,
	.open = nvt_xiaomi_lockdown_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int32_t nvt_read_flash_cust_prod_info_2(uint8_t *data, int32_t size)
{
	int32_t ret = 0;

	NVT_LOG("++\n");

	if (data == NULL) {
		NVT_ERR("data is null!\n");
		return -EINVAL;
	}

	if ((size <= 0) || (size > 4096)) {
		NVT_ERR("Invalid size = %d!\n", size);
		return -EINVAL;
	}

	ret = nvt_get_oem_data(data, 0x1F000, size);
	if (ret < 0) {
		NVT_ERR("Get oem data failed!, ret = %d\n", ret);
		goto read_flash_cust_prod_info_2_out;
	}

read_flash_cust_prod_info_2_out:

	NVT_LOG("--\n");
	return ret;
}

EXPORT_SYMBOL(nvt_read_flash_cust_prod_info_2);

int32_t nvt_write_flash_cust_prod_info_2(uint8_t *data, int32_t size)
{
	int32_t count_256 = 0;
	uint8_t buf[64] = {0};
	uint32_t cur_sram_addr = ts->mmap->RW_FLASH_DATA_ADDR;
	uint32_t flash_address = 0x1F000;
	uint32_t cur_flash_addr = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t tmpvalue = 0;
	int32_t ret = 0;
	int32_t retry = 0;
	int32_t write_flash_retry = 0;
	uint16_t checksum_get = 0;
	uint16_t checksum_cal = 0;

	NVT_LOG("++\n");

	if (data == NULL) {
		NVT_ERR("data is null!\n");
		return -EINVAL;
	}

	if ((size <= 0) || (size > 4096)) {
		NVT_ERR("Invalid size = %d!\n", size);
		return -EINVAL;
	}

write_flash_cust_prod_info_2_retry:
	nvt_sw_reset_idle();

	// Step 1: Initial BootLoader
	ret = Init_BootLoader();
	if (ret < 0) {
		NVT_ERR("Init BootLoader failed!\n");
		goto write_flash_cust_prod_info_2_out;
	}

	// Step 2: Resume PD
	ret = Resume_PD();
	if (ret < 0) {
		NVT_ERR("Resume PD failed!\n");
		goto write_flash_cust_prod_info_2_out;
	}

	// Step 3: Erase Sector
	ret = Sector_Erase(31);
	if (ret < 0) {
		NVT_ERR("Erase Sector failed!\n");
		goto write_flash_cust_prod_info_2_out;
	}

	// change I2C buffer index
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->RW_FLASH_DATA_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->RW_FLASH_DATA_ADDR >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);
	if (ret < 0) {
		NVT_ERR("change I2C buffer index error!!(%d)\n", ret);
		goto write_flash_cust_prod_info_2_out;
	}

	// maximum 256 bytes each write
	if (size % 256)
		count_256 = size / 256 + 1;
	else
		count_256 = size / 256;

	for (i = 0; i < count_256; i++) {
		cur_flash_addr = flash_address + i * 256;

		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			NVT_ERR("Write Enable error!!(%d)\n", ret);
			goto write_flash_cust_prod_info_2_out;
		}
		// Check 0xAA (Write Enable)
		retry = 0;
		while (1) {
			udelay(100);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				NVT_ERR("Check 0xAA (Write Enable) error!!(%d,%d)\n", ret, i);
				goto write_flash_cust_prod_info_2_out;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 20)) {
				NVT_ERR("Check 0xAA (Write Enable) error!! status=0x%02X\n", buf[1]);
				ret = -1;
				goto write_flash_cust_prod_info_2_out;
			}
		}

		// Write Page : 256 bytes
		for (j = 0; j < min((size_t)size - (i * 256), (size_t)256); j += 32) {
			cur_sram_addr = ts->mmap->RW_FLASH_DATA_ADDR + j;
			buf[0] = cur_sram_addr & 0xFF;
			for (k = 0; k < min((size_t)size - (i * 256) - j, (size_t)32); k++) {
				buf[1 + k] = data[(i * 256) + j + k];
			}
			ret = CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 1 + min((size_t)size - (i * 256) - j, (size_t)32));
			if (ret < 0) {
				NVT_ERR("Write Page error!!(%d), i=%d, j=%d\n", ret, i, j);
				goto write_flash_cust_prod_info_2_out;
			}
		}
		if (size - (i * 256) >= 256)
			tmpvalue = ((cur_flash_addr >> 16) & 0xFF) + ((cur_flash_addr >> 8) & 0xFF) + (cur_flash_addr & 0xFF) + 0x00 + (255);
		else
			tmpvalue = ((cur_flash_addr >> 16) & 0xFF) + ((cur_flash_addr >> 8) & 0xFF) + (cur_flash_addr & 0xFF) + 0x00 + (size - (i * 256) - 1);

		for (k = 0; k < min((size_t)size - (i * 256), (size_t)256); k++)
			tmpvalue += data[(i * 256) + k];

		tmpvalue = 255 - tmpvalue + 1;

		// Page Program
		buf[0] = 0x00;
		buf[1] = 0x02;
		buf[2] = ((cur_flash_addr >> 16) & 0xFF);
		buf[3] = ((cur_flash_addr >> 8) & 0xFF);
		buf[4] = (cur_flash_addr & 0xFF);
		buf[5] = 0x00;
		buf[6] = min((size_t)size - (i * 256), (size_t)256) - 1;
		buf[7] = tmpvalue;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 8);
		if (ret < 0) {
			NVT_ERR("Page Program error!!(%d), i=%d\n", ret, i);
			goto write_flash_cust_prod_info_2_out;
		}
		// Check 0xAA (Page Program)
		retry = 0;
		while (1) {
			mdelay(1);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				NVT_ERR("Check 0xAA (Page Program) error!!(%d)\n", ret);
				goto write_flash_cust_prod_info_2_out;
			}
			if (buf[1] == 0xAA || buf[1] == 0xEA) {
				break;
			}
			retry++;
			if (unlikely(retry > 20)) {
				NVT_ERR("Check 0xAA (Page Program) failed, buf[1]=0x%02X, retry=%d\n", buf[1], retry);
				ret = -1;
				goto write_flash_cust_prod_info_2_out;
			}
		}
		if (buf[1] == 0xEA) {
			NVT_ERR("Page Program error!! i=%d\n", i);
			ret = -3;
			goto write_flash_cust_prod_info_2_out;
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(5);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				NVT_ERR("Read Status error!!(%d)\n", ret);
				goto write_flash_cust_prod_info_2_out;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				NVT_ERR("Check 0xAA (Read Status) error!!(%d)\n", ret);
				goto write_flash_cust_prod_info_2_out;
			}
			if (((buf[1] == 0xAA) && (buf[2] == 0x00)) || (buf[1] == 0xEA)) {
				break;
			}
			retry++;
			if (unlikely(retry > 100)) {
				NVT_ERR("Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", buf[1], buf[2], retry);
				ret = -1;
				goto write_flash_cust_prod_info_2_out;
			}
		}
		if (buf[1] == 0xEA) {
			NVT_ERR("Page Program error!! i=%d\n", i);
			ret = -4;
			goto write_flash_cust_prod_info_2_out;
		}
	}

	ret = Fast_Read_Checksum(flash_address, size, &checksum_get);
	if (ret < 0) {
		NVT_ERR("Fast Read Checksum failed!(%d)\n", ret);
		goto write_flash_cust_prod_info_2_out;
	}
	checksum_cal = (uint16_t)((uint16_t)((flash_address >> 16) & 0xFF) + (uint16_t)((flash_address >> 8) & 0xFF)
			+ (uint16_t)(flash_address & 0xFF) + (uint16_t)(((size - 1) >> 8) & 0xFF) + (uint16_t)((size - 1) & 0xFF));
	for (i = 0; i < size; i++) {
		checksum_cal += data[i];
	}
	checksum_cal = 65535 - checksum_cal + 1;
	//NVT_LOG("checksum_get = 0x%04X, checksum_cal = 0x%04X\n", checksum_get, checksum_cal);
	// compare the checksum got and calculated
	if (checksum_get != checksum_cal) {
		if (write_flash_retry < 3) {
			write_flash_retry++;
			goto write_flash_cust_prod_info_2_retry;
		} else {
			NVT_ERR("Checksum not match error! checksum_get=0x%04X, checksum_cal=0x%04X\n", checksum_get, checksum_cal);
			ret = -5;
			goto write_flash_cust_prod_info_2_out;
		}
	}

write_flash_cust_prod_info_2_out:
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);

	NVT_LOG("--\n");

	return ret;
}

EXPORT_SYMBOL(nvt_write_flash_cust_prod_info_2);



static int nvt_blackshark_color_data_rw_example_show(struct seq_file *m, void *v)
{
	seq_printf(m, "color data write to flash and read back done.");

	return 0;
}

#define BLACKSHARK_COLOR_DATA_SIZE 128
static int32_t nvt_blackshark_color_data_rw_example_open(struct inode *inode, struct file *file)
{
	uint8_t color_data[BLACKSHARK_COLOR_DATA_SIZE] = {0x00};
	uint8_t color_data_check[BLACKSHARK_COLOR_DATA_SIZE] = {0x00};
	int32_t i = 0;

	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	NVT_LOG("++\n");

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	// geterate example color data and write
	printk("color data write to:\n");
	for (i = 0; i < BLACKSHARK_COLOR_DATA_SIZE; i++) {
		color_data[i] = i % 256;
		printk("0x%02X, ", color_data[i]);
		if ((i + 1) % 16 == 0)
			printk("\n");
	}
	if (nvt_write_flash_cust_prod_info_2(color_data, BLACKSHARK_COLOR_DATA_SIZE) < 0) {
		NVT_ERR("Write Blackshark color data failed!\n");
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}

	// read back color data for check
	if (nvt_read_flash_cust_prod_info_2(color_data_check, BLACKSHARK_COLOR_DATA_SIZE) < 0) {
		NVT_ERR("Read Blackshark color data failed!\n");
		mutex_unlock(&ts->lock);
		return -EAGAIN;
	}
	printk("color data read back:\n");
	for (i = 0; i < BLACKSHARK_COLOR_DATA_SIZE; i++) {
		printk("0x%02X, ", color_data_check[i]);
		if ((i + 1) % 16 == 0)
			printk("\n");
	}

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	return single_open(file, nvt_blackshark_color_data_rw_example_show, NULL);
}

static const struct file_operations nvt_blackshark_color_data_rw_example_fops = {
	.owner = THIS_MODULE,
	.open = nvt_blackshark_color_data_rw_example_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#define DOZE_SWITCH_ON 1
#define DOZE_SWITCH_OFF 0
int32_t nvt_set_doze_switch_on_off(uint8_t doze_on_off)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	NVT_LOG("++\n");
	NVT_LOG("set doze switch: %d\n", doze_on_off);

	msleep(35);
	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		NVT_ERR("Set event buffer index fail!\n");
		goto nvt_set_doze_switch_on_off_out;
	}

	if (doze_on_off == DOZE_SWITCH_ON) {
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x77;
		ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		if (ret < 0) {
			NVT_ERR("Write switch doze command fail!\n");
			goto nvt_set_doze_switch_on_off_out;
		}
	} else if (doze_on_off == DOZE_SWITCH_OFF) {
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x78;
		ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		if (ret < 0) {
			NVT_ERR("Write switch doze command fail!\n");
			goto nvt_set_doze_switch_on_off_out;
		}
	} else {
		NVT_ERR("Invalid doze switch parameter!\n");
		ret = -1;
	}

nvt_set_doze_switch_on_off_out:
	NVT_LOG("--\n");
	return ret;
}

int32_t nvt_get_doze_switch_on_off(void)
{
	uint8_t buf[8] = {0};
	int32_t ret = -1;

	NVT_LOG("++\n");
	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		NVT_ERR("Set event buffer index fail!\n");
		goto nvt_get_doze_switch_on_off_out;
	}

	buf[0] = 0x5E;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		NVT_ERR("Get customer command status fail!\n");
		goto nvt_get_doze_switch_on_off_out;
	}

	if (buf[1] & 0x10)
		ret = DOZE_SWITCH_ON;
	else
		ret = DOZE_SWITCH_OFF;

nvt_get_doze_switch_on_off_out:
	NVT_LOG("--\n");
	return ret;
}

#define NVT_MAX_PROC_DOZE_SWITCH_SIZE 64
static char nvt_proc_doze_switch_data[NVT_MAX_PROC_DOZE_SWITCH_SIZE + 1] = {0};

static ssize_t nvt_doze_switch_proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	static int finished = 0;
	int32_t doze_switch_on_off_status = 0;
	int32_t len = 0;

	NVT_LOG("++\n");

	/*
	* We return 0 to indicate end of file, that we have
	* no more information. Otherwise, processes will
	* continue to read from us in an endless loop.
	*/
	if (finished) {
		NVT_LOG("read END\n");
		finished = 0;
		return 0;
	}
	finished = 1;

	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	doze_switch_on_off_status = nvt_get_doze_switch_on_off();
	mutex_unlock(&ts->lock);

	memset(nvt_proc_doze_switch_data, 0, NVT_MAX_PROC_DOZE_SWITCH_SIZE + 1);
	if (doze_switch_on_off_status == DOZE_SWITCH_ON) {
		len = sprintf(nvt_proc_doze_switch_data, "%d\n", 1);
	} else if (doze_switch_on_off_status == DOZE_SWITCH_OFF) {
		len = sprintf(nvt_proc_doze_switch_data, "%d\n", 0);
	} else {
		NVT_ERR("Get doze switch on/off status fail!\n");
		len = sprintf(nvt_proc_doze_switch_data, "Get doze switch on/off status fail!\n");
	}

	nvt_proc_doze_switch_data[NVT_MAX_PROC_DOZE_SWITCH_SIZE] = '\0';
	NVT_LOG("doze mode on/off status: %s\n", nvt_proc_doze_switch_data);
	if (copy_to_user(buf, nvt_proc_doze_switch_data, len)) {
		NVT_ERR("copy_to_user() error!\n");
		return -EFAULT;
	}

	NVT_LOG("--\n");
	return len;
}

static ssize_t nvt_doze_switch_proc_write(struct file *filp,const char __user *buf, size_t count, loff_t *f_pos)
{
	NVT_LOG("++\n");

	if (count == 0 || count > 2) {
		NVT_ERR("Invalid value!\n");
		return -EINVAL;
	}

	memset(nvt_proc_doze_switch_data, 0, NVT_MAX_PROC_DOZE_SWITCH_SIZE + 1);
	if (copy_from_user(nvt_proc_doze_switch_data, buf, count)) {
		NVT_ERR("copy_from_user() error!\n");
		return -EFAULT;
	}
	nvt_proc_doze_switch_data[NVT_MAX_PROC_DOZE_SWITCH_SIZE - 1] = '\0';
	NVT_LOG("write %zu bytes\n", count);
	NVT_LOG("nvt_proc_doze_switch_data: %s\n", nvt_proc_doze_switch_data);

	if (strncmp("1", nvt_proc_doze_switch_data, 1) == 0) {
		NVT_LOG("Switch doze mode on.\n");
		if (mutex_lock_interruptible(&ts->lock)) {
			return -ERESTARTSYS;
		}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

		nvt_set_doze_switch_on_off(DOZE_SWITCH_ON);
		mutex_unlock(&ts->lock);
	} else if (strncmp("0", nvt_proc_doze_switch_data, 1) == 0) {
		NVT_LOG("Switch doze mode off.\n");
		if (mutex_lock_interruptible(&ts->lock)) {
			return -ERESTARTSYS;
		}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

		nvt_set_doze_switch_on_off(DOZE_SWITCH_OFF);
		mutex_unlock(&ts->lock);
	} else {
		NVT_ERR("Invalid value!\n");
		return -EINVAL;
	}

	NVT_LOG("--\n");
	return count;
}

static const struct file_operations nvt_doze_switch_fops = {
	.owner = THIS_MODULE,
	.read = nvt_doze_switch_proc_read,
	.write = nvt_doze_switch_proc_write,
};

int32_t nvt_set_sensitivity(int32_t sensitivity)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	NVT_LOG("++\n");
	NVT_LOG("set sensitivity: %d\n", sensitivity);

	msleep(35);

	if ((sensitivity >= 0) && (sensitivity <= 3)) {
		//---set xdata index to EVENT BUF ADDR---
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
		if (ret < 0) {
			NVT_ERR("Set event buffer index fail!\n");
			goto nvt_set_sensitivity_out;
		}

		buf[0] = EVENT_MAP_HOST_CMD;
		if (sensitivity == 0)
			buf[1] = 0x79;
		else if (sensitivity == 1)
			buf[1] = 0x7A;
		else if (sensitivity == 2)
			buf[1] = 0x7B;
		else if (sensitivity == 3)
			buf[1] = 0x7C;
		ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		if (ret < 0) {
			NVT_ERR("Write sensitivity command fail!\n");
			goto nvt_set_sensitivity_out;
		}
	} else {
		NVT_ERR("Invalid sensitivity parameter %d!\n", sensitivity);
		ret = -1;
	}

nvt_set_sensitivity_out:
	NVT_LOG("--\n");
	return ret;
}

int32_t nvt_get_sensitivity(void)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	NVT_LOG("++\n");
	//---set xdata index to EVENT BUF ADDR---
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		NVT_ERR("Set event buffer index fail!\n");
		goto nvt_get_sensitivity_out;
	}

	buf[0] = 0x5E;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
	if (ret < 0) {
		NVT_ERR("Get customer command status fail!\n");
		goto nvt_get_sensitivity_out;
	}

	ret = ((buf[1] & 0x60) >> 5);

nvt_get_sensitivity_out:
	NVT_LOG("--\n");
	return ret;
}

#define NVT_MAX_PROC_SENSITIVITY_SIZE 64
static char nvt_proc_sensitivity_data[NVT_MAX_PROC_SENSITIVITY_SIZE + 1] = {0};

static ssize_t nvt_sensitivity_proc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	static int finished = 0;
	int32_t sensitivity = 0;
	int32_t len = 0;

	NVT_LOG("++\n");

	/*
	* We return 0 to indicate end of file, that we have
	* no more information. Otherwise, processes will
	* continue to read from us in an endless loop.
	*/
	if (finished) {
		NVT_LOG("read END\n");
		finished = 0;
		return 0;
	}
	finished = 1;

	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	sensitivity = nvt_get_sensitivity();
	mutex_unlock(&ts->lock);

	memset(nvt_proc_sensitivity_data, 0, NVT_MAX_PROC_SENSITIVITY_SIZE + 1);
	if ((sensitivity >= 0) && (sensitivity <= 3)) {
		len = sprintf(nvt_proc_sensitivity_data, "%d\n", sensitivity);
	} else {
		NVT_ERR("Get sensitivity status fail!\n");
		len = sprintf(nvt_proc_sensitivity_data, "Get sensitivity status fail!\n");
	}

	nvt_proc_sensitivity_data[NVT_MAX_PROC_SENSITIVITY_SIZE] = '\0';
	NVT_LOG("sensitivity status: %s\n", nvt_proc_sensitivity_data);
	if (copy_to_user(buf, nvt_proc_sensitivity_data, len)) {
		NVT_ERR("copy_to_user() error!\n");
		return -EFAULT;
	}

	NVT_LOG("--\n");
	return len;
}

static ssize_t nvt_sensitivity_proc_write(struct file *filp,const char __user *buf, size_t count, loff_t *f_pos)
{
	int32_t sensitivity = 0;

	NVT_LOG("++\n");

	if (count == 0 || count > 2) {
		NVT_ERR("Invalid value!\n");
		return -EINVAL;
	}

	memset(nvt_proc_sensitivity_data, 0, NVT_MAX_PROC_SENSITIVITY_SIZE + 1);
	if (copy_from_user(nvt_proc_sensitivity_data, buf, count)) {
		NVT_ERR("copy_from_user() error!\n");
		return -EFAULT;
	}
	nvt_proc_sensitivity_data[NVT_MAX_PROC_SENSITIVITY_SIZE - 1] = '\0';
	NVT_LOG("write %zu bytes\n", count);
	NVT_LOG("nvt_proc_sensitivity_data: %s\n", nvt_proc_sensitivity_data);

	if ((sscanf(nvt_proc_sensitivity_data, "%d\n", &sensitivity) == 1) && (sensitivity <= 3)) {
		NVT_LOG("sensitivity = %d\n", sensitivity);

		if (mutex_lock_interruptible(&ts->lock)) {
			return -ERESTARTSYS;
		}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

		nvt_set_sensitivity(sensitivity);

		mutex_unlock(&ts->lock);
	} else {
		NVT_ERR("Invalid value!\n");
		return -EINVAL;
	}

	NVT_LOG("--\n");
	return count;
}

static const struct file_operations nvt_sensitivity_fops = {
	.owner = THIS_MODULE,
	.read = nvt_sensitivity_proc_read,
	.write = nvt_sensitivity_proc_write,
};

/*******************************************************
Description:
	Novatek touchscreen extra function proc. file node
	initial function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
int32_t nvt_extra_proc_init(void)
{
	NVT_proc_fw_version_entry = proc_create(NVT_FW_VERSION, 0444, NULL,&nvt_fw_version_fops);
	if (NVT_proc_fw_version_entry == NULL) {
		NVT_ERR("create proc/nvt_fw_version Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_fw_version Succeeded!\n");
	}

	NVT_proc_baseline_entry = proc_create(NVT_BASELINE, 0444, NULL,&nvt_baseline_fops);
	if (NVT_proc_baseline_entry == NULL) {
		NVT_ERR("create proc/nvt_baseline Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_baseline Succeeded!\n");
	}

	NVT_proc_raw_entry = proc_create(NVT_RAW, 0444, NULL,&nvt_raw_fops);
	if (NVT_proc_raw_entry == NULL) {
		NVT_ERR("create proc/nvt_raw Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_raw Succeeded!\n");
	}

	NVT_proc_diff_entry = proc_create(NVT_DIFF, 0444, NULL,&nvt_diff_fops);
	if (NVT_proc_diff_entry == NULL) {
		NVT_ERR("create proc/nvt_diff Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_diff Succeeded!\n");
	}

	NVT_proc_xiaomi_config_info_entry = proc_create(NVT_XIAOMI_CONFIG_INFO, 0444, NULL, &nvt_xiaomi_config_info_fops);
	if (NVT_proc_xiaomi_config_info_entry == NULL) {
		NVT_ERR("create proc/%s Failed!\n", NVT_XIAOMI_CONFIG_INFO);
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/%s Succeeded!\n", NVT_XIAOMI_CONFIG_INFO);
	}

	NVT_proc_xiaomi_lockdown_info_entry = proc_create(NVT_XIAOMI_LOCKDOWN_INFO, 0444, NULL, &nvt_xiaomi_lockdown_info_fops);
	if (NVT_proc_xiaomi_lockdown_info_entry == NULL) {
		NVT_ERR("create proc/%s Failed!\n", NVT_XIAOMI_LOCKDOWN_INFO);
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/%s Succeeded!\n", NVT_XIAOMI_LOCKDOWN_INFO);
	}

	NVT_proc_blackshark_color_data_rw_example_entry = proc_create(NVT_BLACKSHARK_COLOR_DATA_RW_EXAMPLE, 0444, NULL, &nvt_blackshark_color_data_rw_example_fops);
	if (NVT_proc_blackshark_color_data_rw_example_entry == NULL) {
		NVT_ERR("create proc/%s Failed!\n", NVT_BLACKSHARK_COLOR_DATA_RW_EXAMPLE);
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/%s Succeeded!\n", NVT_BLACKSHARK_COLOR_DATA_RW_EXAMPLE);
	}

	NVT_proc_doze_switch_entry = proc_create(NVT_DOZE_SWITCH, 0644, NULL,&nvt_doze_switch_fops);
	if (NVT_proc_doze_switch_entry == NULL) {
		NVT_ERR("create proc/nvt_doze_switch Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_doze_switch Succeeded!\n");
	}

	NVT_proc_sensitivity_entry = proc_create(NVT_SENSITIVITY, 0644, NULL,&nvt_sensitivity_fops);
	if (NVT_proc_sensitivity_entry == NULL) {
		NVT_ERR("create proc/nvt_sensitivity Failed!\n");
		return -ENOMEM;
	} else {
		NVT_LOG("create proc/nvt_sensitivity Succeeded!\n");
	}

	return 0;
}
#endif
