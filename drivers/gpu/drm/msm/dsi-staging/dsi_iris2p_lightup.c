



#include "dsi_iris2p_def.h"

struct dsi_iris_info iris_info;
char grcp_header[GRCP_HEADER] = {
	PWIL_TAG('P', 'W', 'I', 'L'),
	PWIL_TAG('G', 'R', 'C', 'P'),
	PWIL_U32(0x3),
	0x00,
	0x00,
	PWIL_U16(0x2),
};
struct iris_grcp_cmd init_cmd[INIT_CMD_NUM];
struct iris_grcp_cmd grcp_cmd;
struct iris_grcp_cmd meta_cmd;
struct iris_grcp_cmd gamma_cmd;

char iris_read_cmd_buf[16];
int iris_debug_dtg_v12 = 1;
u8 *app_fw_buf = NULL;
u8 *lut_fw_buf = NULL;
u8 *gamma_fw_buf = NULL;
u8 *iris_cmlut_buf = NULL;
struct drm_device *iris_drm;
struct dsi_panel *iris_panel;
bool iris_lightup_flag = false;

//#define DUMP_DATA_FOR_BOOTLOADER
#ifdef DUMP_DATA_FOR_BOOTLOADER
void iris_dump_packet(u8 *data, int size)
{
	int i = 0;
	pr_err("size = %d\n", size);
	for (i = 0; i < size; i += 4)
		pr_err("%02x %02x %02x %02x\n",
			*(data+i), *(data+i+1), *(data+i+2), *(data+i+3));
}
#define DUMP_PACKET   iris_dump_packet
#else
#define DUMP_PACKET(...)
#endif

void iris_drm_device_handle_get(struct drm_device *drm_dev)
{
	iris_drm = drm_dev;
}

void iris_workmode_init(struct dsi_iris_info *iris_info, struct dsi_panel *panel)
{
	struct iris_work_mode *pwork_mode = &(iris_info->work_mode);

	/*iris mipirx mode*/
	if (DSI_OP_CMD_MODE == panel->panel_mode) {
		pwork_mode->rx_mode = DSI_OP_CMD_MODE;
	} else {
		pr_info("iris abypass mode\n");
		pwork_mode->rx_mode = DSI_OP_VIDEO_MODE;
	}

	pwork_mode->rx_ch = 0;
	pwork_mode->rx_dsc = 0;
	pwork_mode->rx_pxl_mode = 0;
	/*iris mipitx mode*/
	pwork_mode->tx_mode = DSI_OP_VIDEO_MODE;
	pwork_mode->tx_ch = 0;
	pwork_mode->tx_dsc = 0;
	pwork_mode->tx_pxl_mode = 1;
}

void iris_timing_init(struct dsi_iris_info *iris_info)
{
	struct iris_timing_info *pinput_timing = &(iris_info->input_timing);
	struct iris_timing_info *poutput_timing = &(iris_info->output_timing);

	pinput_timing->hres = 1080;
	pinput_timing->hfp = 100;
	pinput_timing->hbp = 80;
	pinput_timing->hsw = 20;
	pinput_timing->vres = 2160;
	pinput_timing->vbp = 24;
	pinput_timing->vfp = 4;
	pinput_timing->vsw = 2;
	pinput_timing->fps = 60;

	poutput_timing->hres = 1080;
	poutput_timing->hfp = 100;
	poutput_timing->hbp = 80;
	poutput_timing->hsw = 20;
	poutput_timing->vres = 2160;
	poutput_timing->vbp = 24;
	poutput_timing->vfp = 4;
	poutput_timing->vsw = 2;
	poutput_timing->fps = 60;
}

void iris_clock_tree_init(struct dsi_iris_info *iris_info)
{
	struct iris_pll_setting *pll = &(iris_info->hw_setting.pll_setting);
	struct iris_clock_source *clock = &(iris_info->hw_setting.clock_setting.dclk);
	u8 clk_default[6] = {0x01, 0x01, 0x03, 0x01, 0x03, 0x00};
	u8 indx;

	pll->ppll_ctrl0 = 0x2;
	pll->ppll_ctrl1 = 0x281101;
	pll->ppll_ctrl2 = 0x58258C;

	pll->dpll_ctrl0 = 0x2002;
	pll->dpll_ctrl1 = 0x231101;
	pll->dpll_ctrl2 = 0xA3D71;

	pll->mpll_ctrl0 = 0x2;
	pll->mpll_ctrl1 = 0x3e0901;
	pll->mpll_ctrl2 = 0x800000;

	pll->txpll_div = 0x0;
	pll->txpll_sel = 0x2;
	pll->reserved = 0x0;

	for (indx = 0; indx < 6; indx++) {
		clock->sel = clk_default[indx] & 0xf;
		clock->div = (clk_default[indx] >> 4) & 0xf;
		clock->div_en = !!clock->div;
		clock++;
	}

}

void iris_mipi_info_init(struct dsi_iris_info *iris_info)
{
	struct iris_mipirx_setting *rx_setting = &(iris_info->hw_setting.rx_setting);
	struct iris_mipitx_setting *tx_setting = &(iris_info->hw_setting.tx_setting);

	rx_setting->mipirx_dsi_functional_program = 0x64;
	rx_setting->mipirx_eot_ecc_crc_disable = 7;
	rx_setting->mipirx_data_lane_timing_param = 0xff09;

	tx_setting->mipitx_dsi_tx_ctrl = 0x0A00C039;
	tx_setting->mipitx_hs_tx_timer = 0x0083D600;
	tx_setting->mipitx_bta_lp_timer = 0x00ffff17;
	tx_setting->mipitx_initialization_reset_timer = 0x0a8c0780;
	tx_setting->mipitx_dphy_timing_margin = 0x00040401;
	tx_setting->mipitx_lp_timing_para = 0x0f010007;
	tx_setting->mipitx_data_lane_timing_param = 0x140C1106;
	tx_setting->mipitx_clk_lane_timing_param = 0x140B2C08;
	tx_setting->mipitx_dphy_pll_para = 0x00000780;
	tx_setting->mipitx_dphy_trim_1 = 0xedb5384d;
}

void iris_feature_setting_init(struct feature_setting *setting)
{
	setting->pq_setting.peaking = 0;
	setting->pq_setting.sharpness = 0;
	setting->pq_setting.memcdemo = 0;
	setting->pq_setting.gamma = 0;
	setting->pq_setting.memclevel = 3;
	setting->pq_setting.contrast = 0x32;
	setting->pq_setting.cinema_en = 0;
	setting->pq_setting.peakingdemo = 0;
	setting->pq_setting.update = 1;

	setting->dbc_setting.brightness = 0;
	setting->dbc_setting.ext_pwm = 0;
	setting->dbc_setting.cabcmode = 0;
	setting->dbc_setting.dlv_sensitivity = 0;
	setting->dbc_setting.update = 1;

	setting->lp_memc_setting.level = 0;
	setting->lp_memc_setting.value = iris_lp_memc_calc(0);
	setting->lp_memc_setting.update = 1;

	setting->lce_setting.mode = 0;
	setting->lce_setting.mode1level = 1;
	setting->lce_setting.mode2level = 1;
	setting->lce_setting.demomode = 0;
	setting->lce_setting.graphics_detection = 0;
	setting->lce_setting.update = 1;

	setting->cm_setting.cm6axes = 0;
	setting->cm_setting.cm3d = 0;
	setting->cm_setting.demomode = 0;
	setting->cm_setting.ftc_en = 0;
	setting->cm_setting.color_temp_en = 0;
	setting->cm_setting.color_temp = 65;
	setting->cm_setting.color_temp_adjust = 32;
	setting->cm_setting.sensor_auto_en = 0;
	setting->cm_setting.update = 1;

	setting->hdr_setting.hdrlevel = 0;
	setting->hdr_setting.hdren = 0;
	setting->hdr_setting.update = 1;

	setting->lux_value.luxvalue = 0;
	setting->lux_value.update = 1;

	setting->cct_value.cctvalue = 0;
	setting->cct_value.update = 1;

	setting->reading_mode.readingmode = 0;
	setting->reading_mode.update = 1;

	setting->gamma_enable = 1;
}

void iris_feature_setting_update_check(void)
{
	struct feature_setting *chip_setting = &iris_info.chip_setting;
	struct feature_setting *user_setting = &iris_info.user_setting;
	struct iris_setting_update *settint_update = &iris_info.settint_update;

	iris_feature_setting_init(chip_setting);

	if ((user_setting->pq_setting.peaking != chip_setting->pq_setting.peaking)
		|| (user_setting->pq_setting.sharpness != chip_setting->pq_setting.sharpness)
		|| (user_setting->pq_setting.memcdemo != chip_setting->pq_setting.memcdemo)
		|| (user_setting->pq_setting.gamma != chip_setting->pq_setting.gamma)
		|| (user_setting->pq_setting.memclevel != chip_setting->pq_setting.memclevel)
		|| (user_setting->pq_setting.contrast != chip_setting->pq_setting.contrast)
		|| (user_setting->pq_setting.cinema_en != chip_setting->pq_setting.cinema_en)
		|| (user_setting->pq_setting.peakingdemo != chip_setting->pq_setting.peakingdemo))
		settint_update->pq_setting = true;

	if ((user_setting->dbc_setting.brightness != chip_setting->dbc_setting.brightness)
		|| (user_setting->dbc_setting.ext_pwm != chip_setting->dbc_setting.ext_pwm)
		|| (user_setting->dbc_setting.cabcmode != chip_setting->dbc_setting.cabcmode)
		|| (user_setting->dbc_setting.dlv_sensitivity != chip_setting->dbc_setting.dlv_sensitivity))
		settint_update->dbc_setting = true;

	if ((user_setting->lp_memc_setting.level != chip_setting->lp_memc_setting.level)
		|| (user_setting->lp_memc_setting.value != chip_setting->lp_memc_setting.value))
		settint_update->lp_memc_setting = true;

	if ((user_setting->lce_setting.mode != chip_setting->lce_setting.mode)
		|| (user_setting->lce_setting.mode1level != chip_setting->lce_setting.mode1level)
		|| (user_setting->lce_setting.mode2level != chip_setting->lce_setting.mode2level)
		|| (user_setting->lce_setting.demomode != chip_setting->lce_setting.demomode)
		|| (user_setting->lce_setting.graphics_detection != chip_setting->lce_setting.graphics_detection))
		settint_update->lce_setting = true;

	if ((user_setting->cm_setting.cm6axes != chip_setting->cm_setting.cm6axes)
		|| (user_setting->cm_setting.cm3d != chip_setting->cm_setting.cm3d)
		|| (user_setting->cm_setting.demomode != chip_setting->cm_setting.demomode)
		|| (user_setting->cm_setting.ftc_en != chip_setting->cm_setting.ftc_en)
		|| (user_setting->cm_setting.color_temp_en != chip_setting->cm_setting.color_temp_en)
		|| (user_setting->cm_setting.color_temp != chip_setting->cm_setting.color_temp)
		|| (user_setting->cm_setting.color_temp_adjust != chip_setting->cm_setting.color_temp_adjust)
		|| (user_setting->cm_setting.sensor_auto_en != chip_setting->cm_setting.sensor_auto_en)) {
		if (iris_info.lut_info.lut_fw_state && iris_info.gamma_info.gamma_fw_state)
			settint_update->cm_setting = true;
		else
			settint_update->cm_setting = false;
	}

	if ((user_setting->hdr_setting.hdrlevel != chip_setting->hdr_setting.hdrlevel)
		|| (user_setting->hdr_setting.hdren != chip_setting->hdr_setting.hdren))
		settint_update->hdr_setting = true;

	if (user_setting->lux_value.luxvalue != chip_setting->lux_value.luxvalue)
		settint_update->lux_value = true;

	if (user_setting->cct_value.cctvalue != chip_setting->cct_value.cctvalue)
		settint_update->cct_value = true;

	if (user_setting->reading_mode.readingmode != chip_setting->reading_mode.readingmode)
		settint_update->reading_mode = true;

	if (user_setting->gamma_enable != chip_setting->gamma_enable)
		settint_update->gamma_table = true;

	/*todo: update FRC setting*/
	settint_update->pq_setting = true;
	settint_update->lp_memc_setting = true;
}

void iris_cmd_reg_add(struct iris_grcp_cmd *pcmd, u32 addr, u32 val)
{
	*(u32 *)(pcmd->cmd + pcmd->cmd_len) = cpu_to_le32(addr);
	*(u32 *)(pcmd->cmd + pcmd->cmd_len + 4) = cpu_to_le32(val);
	pcmd->cmd_len += 8;
}

void iris_mipitx_reg_config(struct dsi_iris_info *iris_info, struct iris_grcp_cmd *pcmd)
{
	struct iris_work_mode *pwork_mode = &(iris_info->work_mode);
	struct iris_mipitx_setting *tx_setting = &(iris_info->hw_setting.tx_setting);
	struct iris_timing_info *poutput_timing = &(iris_info->output_timing);
	u32 tx_ch, mipitx_addr = IRIS_MIPI_TX_ADDR, dual_ch_ctrl, dsi_tx_ctrl = 0;
	u32 ddrclk_div, ddrclk_div_en;

	ddrclk_div = iris_info->hw_setting.pll_setting.txpll_div;
	ddrclk_div_en = !!ddrclk_div;

	for (tx_ch = 0; tx_ch < (pwork_mode->tx_ch + 1); tx_ch++) {
#ifdef MIPI_SWAP
		mipitx_addr -= tx_ch * IRIS_MIPI_ADDR_OFFSET;
#else
		mipitx_addr += tx_ch * IRIS_MIPI_ADDR_OFFSET;
#endif

		if (pwork_mode->tx_mode)
			dsi_tx_ctrl = tx_setting->mipitx_dsi_tx_ctrl | (0x1 << 8);
		else
			dsi_tx_ctrl = tx_setting->mipitx_dsi_tx_ctrl & (~(0x1 << 8));
		iris_cmd_reg_add(pcmd, mipitx_addr + DSI_TX_CTRL, dsi_tx_ctrl & 0xfffffffe);

		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_TIMING_MARGIN, tx_setting->mipitx_dphy_timing_margin);
		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_LP_TIMING_PARA, tx_setting->mipitx_lp_timing_para);
		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_DATA_LANE_TIMING_PARA, tx_setting->mipitx_data_lane_timing_param);
		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_CLOCK_LANE_TIMING_PARA, tx_setting->mipitx_clk_lane_timing_param);
		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_TRIM_1, tx_setting->mipitx_dphy_trim_1);
		iris_cmd_reg_add(pcmd, mipitx_addr + DPHY_CTRL, 1 | (ddrclk_div << 26) | (ddrclk_div_en << 28));

		if (pwork_mode->tx_ch) {
			dual_ch_ctrl = pwork_mode->tx_ch + ((poutput_timing->hres * 2) << 16);
			if (0 == tx_ch)
				dual_ch_ctrl += 1 << 1;
			else
				dual_ch_ctrl += 2 << 1;
			iris_cmd_reg_add(pcmd, mipitx_addr + DUAL_CH_CTRL, dual_ch_ctrl);
		}
		iris_cmd_reg_add(pcmd, mipitx_addr + HS_TX_TIMER, tx_setting->mipitx_hs_tx_timer);
		iris_cmd_reg_add(pcmd, mipitx_addr + BTA_LP_TIMER, tx_setting->mipitx_bta_lp_timer);
		iris_cmd_reg_add(pcmd, mipitx_addr + INITIALIZATION_RESET_TIMER, tx_setting->mipitx_initialization_reset_timer);

		iris_cmd_reg_add(pcmd, mipitx_addr + TX_RESERVED_0, 4);

		iris_cmd_reg_add(pcmd, mipitx_addr + DSI_TX_CTRL, dsi_tx_ctrl);
	}

}

void iris_mipirx_reg_config(struct dsi_iris_info *iris_info, struct iris_grcp_cmd *pcmd)
{
	struct iris_work_mode *pwork_mode = &(iris_info->work_mode);
	struct iris_timing_info *pinput_timing = &(iris_info->input_timing);
	struct iris_mipirx_setting *rx_setting = &(iris_info->hw_setting.rx_setting);
	u32 dbi_handler_ctrl = 0, frame_col_addr = 0;
	u32 rx_ch, mipirx_addr = IRIS_MIPI_RX_ADDR;

	for (rx_ch = 0; rx_ch < (pwork_mode->rx_ch + 1); rx_ch++) {
#ifdef MIPI_SWAP
		mipirx_addr -= rx_ch * IRIS_MIPI_ADDR_OFFSET;
#else
		mipirx_addr += rx_ch * IRIS_MIPI_ADDR_OFFSET;
#endif
		iris_cmd_reg_add(pcmd, mipirx_addr + DEVICE_READY, 0x00000000);
		/*reset for DFE*/
		iris_cmd_reg_add(pcmd, mipirx_addr + RESET_ENABLE_DFE, 0x00000000);
		iris_cmd_reg_add(pcmd, mipirx_addr + RESET_ENABLE_DFE, 0x00000001);

		dbi_handler_ctrl = 0xf0000 + (pwork_mode->rx_ch << 23);
		/* left side enable */
		if (pwork_mode->rx_ch && (0 == rx_ch))
		dbi_handler_ctrl += (1 << 24);
		/*ext_mipi_rx_ctrl*/
		if (1 == rx_ch)
			dbi_handler_ctrl += (1 << 22);
		iris_cmd_reg_add(pcmd, mipirx_addr + DBI_HANDLER_CTRL, dbi_handler_ctrl);

		if (pwork_mode->rx_mode) {
			frame_col_addr = (pwork_mode->rx_ch) ? (pinput_timing->hres * 2 - 1) : (pinput_timing->hres - 1);
			iris_cmd_reg_add(pcmd, mipirx_addr + FRAME_COLUMN_ADDR, frame_col_addr << 16);
			iris_cmd_reg_add(pcmd, mipirx_addr + ABNORMAL_COUNT_THRES, 0xffffffff);
		}
		iris_cmd_reg_add(pcmd, mipirx_addr + INTEN, 0x3);
		iris_cmd_reg_add(pcmd, mipirx_addr + INTERRUPT_ENABLE, 0x0);
		iris_cmd_reg_add(pcmd, mipirx_addr + DSI_FUNCTIONAL_PROGRAMMING, rx_setting->mipirx_dsi_functional_program);
		iris_cmd_reg_add(pcmd, mipirx_addr + EOT_ECC_CRC_DISABLE, rx_setting->mipirx_eot_ecc_crc_disable);
		iris_cmd_reg_add(pcmd, mipirx_addr + DATA_LANE_TIMING_PARAMETER, rx_setting->mipirx_data_lane_timing_param);
		iris_cmd_reg_add(pcmd, mipirx_addr + DPI_SYNC_COUNT, pinput_timing->hsw + (pinput_timing->vsw << 16));
		iris_cmd_reg_add(pcmd, mipirx_addr + DEVICE_READY, 0x00000001);
	}
}

void iris_sys_reg_config(struct dsi_iris_info *iris_info, struct iris_grcp_cmd *pcmd)
{
	u32 clkmux_ctrl = 0x42180100, clkdiv_ctrl = 0x08;
	struct iris_pll_setting *pll = &iris_info->hw_setting.pll_setting;
	struct iris_clock_setting *clock = &iris_info->hw_setting.clock_setting;

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + CLKMUX_CTRL, clkmux_ctrl | (clock->escclk.sel << 11) | pll->txpll_sel);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + CLKDIV_CTRL, clkdiv_ctrl | (clock->escclk.div << 3) | (clock->escclk.div_en << 7));

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + DCLK_SRC_SEL, clock->dclk.sel | (clock->dclk.div << 8) | (clock->dclk.div_en << 10));
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + INCLK_SRC_SEL, clock->inclk.sel | (clock->inclk.div << 8) | (clock->inclk.div_en << 10));
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + MCUCLK_SRC_SEL, clock->mcuclk.sel | (clock->mcuclk.div << 8) | (clock->mcuclk.div_en << 12));
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PCLK_SRC_SEL, clock->pclk.sel | (clock->pclk.div << 8) | (clock->pclk.div_en << 10));
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + MCLK_SRC_SEL, clock->mclk.sel | (clock->mclk.div << 8) | (clock->mclk.div_en << 10));

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PPLL_B_CTRL0, pll->ppll_ctrl0);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PPLL_B_CTRL1, pll->ppll_ctrl1);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PPLL_B_CTRL2, pll->ppll_ctrl2);

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + DPLL_B_CTRL0, pll->dpll_ctrl0);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + DPLL_B_CTRL1, pll->dpll_ctrl1);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + DPLL_B_CTRL2, pll->dpll_ctrl2);

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + MPLL_B_CTRL0, pll->mpll_ctrl0);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + MPLL_B_CTRL1, pll->mpll_ctrl1);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + MPLL_B_CTRL2, pll->mpll_ctrl2);

	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PLL_CTRL, 0x1800e);
	iris_cmd_reg_add(pcmd, IRIS_SYS_ADDR + PLL_CTRL, 0x18000);
	if (DSI_OP_VIDEO_MODE == iris_info->work_mode.rx_mode) {
		iris_cmd_reg_add(pcmd, 0xf000002c, 2);
		iris_cmd_reg_add(pcmd, 0xf000002c, 0);
		iris_cmd_reg_add(pcmd, 0xf0010008, 0x80f5);
	}
}


void iris_init_cmd_build(struct dsi_iris_info *iris_info)
{
	u32 indx = 0, grcp_len = 0;

	memset(init_cmd, 0, sizeof(init_cmd));
	for (indx = 0; indx < INIT_CMD_NUM; indx++) {
		memcpy(init_cmd[indx].cmd, grcp_header, GRCP_HEADER);
		init_cmd[indx].cmd_len = GRCP_HEADER;
	}

	iris_mipitx_reg_config(iris_info, &init_cmd[0]);
	iris_mipirx_reg_config(iris_info, &init_cmd[1]);
	iris_sys_reg_config(iris_info, &init_cmd[2]);

	for (indx = 0; indx < INIT_CMD_NUM; indx++) {
		grcp_len = (init_cmd[indx].cmd_len - GRCP_HEADER) / 4;
		*(u32 *)(init_cmd[indx].cmd + 8) = cpu_to_le32(grcp_len + 1);
		*(u16 *)(init_cmd[indx].cmd + 14) = cpu_to_le16(grcp_len);
	}
}


u32 iris_get_vtotal(struct iris_timing_info *info)
{
	u32 vtotal;

	vtotal = info->vfp + info->vsw + info->vbp + info->vres;
	return vtotal;
}

u32 iris_get_htotal(struct iris_timing_info *info)
{
	u32 htotal;

	htotal = info->hfp + info->hsw + info->hbp + info->hres;
	return htotal;
}

void iris_dtg_setting_calc(struct dsi_iris_info *iris_info)
{
#define DELTA_PERIOD_MAX 0
	u32 vtotal = iris_get_vtotal(&iris_info->output_timing);
	u32 htotal = iris_get_htotal(&iris_info->output_timing);
	u32 vfp = iris_info->output_timing.vfp;
	u32 ovs_lock_te_en =0, psr_mask = 0, evs_sel = 0;
	u32 te_en = 0, te_interval = 0, te_sel = 0, sw_te_en = 0, sw_fix_te_en = 0, te_auto_adj_en = 0, te_ext_en = 0, te_ext_filter_en = 0, te_ext_filter_thr = 0;
	u32 cmd_mode_en = 0, cm_hw_rfb_mode = 0, cm_hw_frc_mode = 0;
	u32 dtg_en = 1, ivsa_sel = 1, vfp_adj_en = 1, dframe_ratio = 1, vframe_ratio = 1, lock_sel = 1;
	u32 sw_dvs_period = (vtotal << 8);
	u32 sw_te_scanline = 0, sw_te_scanline_frc = 0, te_ext_dly = 0, te_out_sel = 0, te_out_filter_thr = 0, te_out_filter_en = 0, te_ext_dly_frc = 0;
	u32 te2ovs_dly = 0, te2ovs_dly_frc = 0;
	u32 evs_dly = 6, evs_new_dly = 1;
	u32 vfp_max = 0;
	u32 vfp_extra = DELTA_PERIOD_MAX;
	u32 lock_mode = 0;
	u32 vres_mem = 0, psr_rd = 2, frc_rd = 4, scale_down = 2, dsc = 2, margin = 2, scale_up = 2;
	u32 peaking = 2;
	u32 i2o_dly = 0;
	struct iris_dtg_setting *dtg_setting = &iris_info->dtg_setting;
	struct iris_work_mode *work_mode = &iris_info->work_mode;

	//dtg 1.1 mode, video in
	if (DSI_OP_VIDEO_MODE == work_mode->rx_mode)
	{
		evs_sel = 1;
		psr_mask = 1;
		dtg_setting->dtg_delay = 3;
		vfp_max = vfp + vfp_extra;
	}
	else if (DSI_OP_VIDEO_MODE == work_mode->tx_mode)
	{
		if (!iris_debug_dtg_v12) {
			//dtg 1.3 mode, command in and video out
			ovs_lock_te_en = 1;
			cmd_mode_en = 1;
			te_en = 1;
			sw_fix_te_en = 1;
			te_auto_adj_en = 1;
			te2ovs_dly = vfp - 1;
			te2ovs_dly_frc = (vtotal*3)/4;
			dtg_setting->dtg_delay = 2;
			vfp_max = (te2ovs_dly_frc > vfp) ? te2ovs_dly_frc : vfp;
		} else {
			//dtg 1.2 mode, command in and video out
			evs_sel = 1;
			evs_dly = 2;
			evs_new_dly = 1;
			cmd_mode_en = 1;
			cm_hw_frc_mode = 3;
			cm_hw_rfb_mode = 3;
			te_en = 1;
			te_sel = 1;
			sw_te_en = 1;
			te_auto_adj_en = 1;
			vres_mem = iris_lp_memc_calc(LEVEL_MAX - 1) >> 16;
			i2o_dly = ((psr_rd > frc_rd? psr_rd: frc_rd) + scale_down + dsc + margin) * iris_info->input_timing.vres +
					scale_up * iris_info->output_timing.vres +
					peaking * vres_mem;
			sw_te_scanline = vtotal * vres_mem - (i2o_dly - iris_info->output_timing.vbp * vres_mem - iris_info->output_timing.vsw * vres_mem);
			sw_te_scanline /= vres_mem;
			sw_te_scanline_frc = (vtotal)/2;
			te_out_filter_thr = (vtotal)/2;
			te_out_filter_en = 1;
			dtg_setting->dtg_delay = 2;
			vfp_max = vfp + vfp_extra;
		}
	}
	//dtg 1.4 mode, command in and command out
	else if (DSI_OP_CMD_MODE == work_mode->tx_mode)
	{
		vfp_max = vfp + vfp_extra;
		evs_dly = 2;
		evs_sel = 1;
		ovs_lock_te_en = 1;
		cmd_mode_en = 1;
		te_en = 1;
		te_auto_adj_en = 1;
		te_ext_en = 1;

		te_ext_filter_thr = (((u32)iris_info->output_timing.hres * (u32)iris_info->output_timing.vres * 100)/vtotal/htotal)*vtotal/100;
		te2ovs_dly = 2;
		te2ovs_dly_frc = 2;
		te_ext_dly = 1;
		te_out_sel = 1;
		te_out_filter_thr = (vtotal)/2;
		te_out_filter_en = 1;
		te_ext_dly_frc = (vtotal)/4;
		dtg_setting->dtg_delay = 1;
		te_ext_filter_en = 1;
		lock_mode = 2;

		te_sel = 1;
		sw_te_en = 1;
		vres_mem = iris_lp_memc_calc(LEVEL_MAX - 1) >> 16;
		i2o_dly = ((psr_rd > frc_rd? psr_rd: frc_rd) + scale_down + dsc + margin) * iris_info->input_timing.vres +
				scale_up * iris_info->output_timing.vres +
				peaking * vres_mem;
		sw_te_scanline = vtotal * vres_mem - (i2o_dly - iris_info->output_timing.vbp * vres_mem - iris_info->output_timing.vsw * vres_mem);
		sw_te_scanline /= vres_mem;
		sw_te_scanline_frc = (vtotal)/4;
		evs_new_dly = (scale_down + dsc + margin) * iris_info->input_timing.vres / vres_mem - te2ovs_dly;
	}

	dtg_setting->dtg_ctrl = dtg_en + (ivsa_sel << 3) + (dframe_ratio << 4) + (vframe_ratio << 9) + (vfp_adj_en << 17) +
								(ovs_lock_te_en << 18) + (lock_sel << 26) + (evs_sel << 28) + (psr_mask << 30);
	dtg_setting->dtg_ctrl_1 = (cmd_mode_en) + (lock_mode << 5) + (cm_hw_rfb_mode << 10) + (cm_hw_frc_mode << 12);
	dtg_setting->evs_dly = evs_dly;
	dtg_setting->evs_new_dly = evs_new_dly;
	dtg_setting->te_ctrl = (te_en) + (te_interval << 1) + (te_sel << 2) + (sw_te_en << 3) + (sw_fix_te_en << 5) +
						(te_auto_adj_en << 6) + (te_ext_en << 7) + (te_ext_filter_en << 8) + (te_ext_filter_thr << 9);
	dtg_setting->dvs_ctrl = sw_dvs_period + (2 << 30);
	dtg_setting->te_ctrl_1 = sw_te_scanline;
	dtg_setting->te_ctrl_2 = sw_te_scanline_frc;
	dtg_setting->te_ctrl_3 = te_ext_dly + (te_out_sel << 24);
	dtg_setting->te_ctrl_4 = te_out_filter_thr + (te_out_filter_en << 24);
	dtg_setting->te_ctrl_5 = te_ext_dly_frc;
	dtg_setting->te_dly = te2ovs_dly;
	dtg_setting->te_dly_1 = te2ovs_dly_frc;
	dtg_setting->vfp_ctrl_0 = vfp + (1<<24);
	dtg_setting->vfp_ctrl_1 = vfp_max;

}

void iris_buffer_alloc(void)
{
	if(!app_fw_buf) {
		app_fw_buf = kzalloc(DSI_DMA_TX_BUF_SIZE, GFP_KERNEL);
		if (!app_fw_buf) {
			pr_err("%s: failed to alloc app fw mem, size = %d\n", __func__, DSI_DMA_TX_BUF_SIZE);
		}
	}

	if (!lut_fw_buf) {
		lut_fw_buf = kzalloc(IRIS_CM_LUT_LENGTH * 4 * CMI_TOTAL, GFP_KERNEL);
		if (!lut_fw_buf) {
			pr_err("%s: failed to alloc lut fw mem, size = %d\n", __func__, IRIS_CM_LUT_LENGTH * 4 * CMI_TOTAL);
		}
	}

	if (!gamma_fw_buf) {
		gamma_fw_buf = kzalloc(8 * 1024, GFP_KERNEL);
		if (!gamma_fw_buf) {
			pr_err("%s: failed to alloc gamma fw mem, size = %d\n", __func__, 8 * 1024);
		}
	}

	if (!iris_cmlut_buf) {
		iris_cmlut_buf = kzalloc(32 * 1024, GFP_KERNEL);
		if (!iris_cmlut_buf) {
			pr_err("%s: failed to alloc cm lut mem, size = %d\n", __func__, 32 * 1024);
		}
	}
}

void iris_info_init(struct dsi_panel *panel)
{
	iris_panel = panel;
	memset(&iris_info, 0, sizeof(iris_info));

	iris_workmode_init(&iris_info, panel);
	iris_timing_init(&iris_info);
	iris_clock_tree_init(&iris_info);
	iris_mipi_info_init(&iris_info);
	iris_feature_setting_init(&iris_info.user_setting);
	iris_feature_setting_init(&iris_info.chip_setting);

	iris_init_cmd_build(&iris_info);
	iris_dtg_setting_calc(&iris_info);
	iris_buffer_alloc();
	mutex_init(&iris_info.config_mutex);
}

int iris_dsi_cmds_send(struct dsi_panel *panel,
				struct dsi_cmd_desc *cmds,
				u32 count,
				enum dsi_cmd_set_state state)
{
	int rc = 0, i = 0;
	ssize_t len;
	const struct mipi_dsi_host_ops *ops = panel->host->ops;

	if (!panel || !panel->cur_mode)
		return -EINVAL;

	if (count == 0) {
		pr_debug("[%s] No commands to be sent for state\n",
			 panel->name);
		goto error;
	}

	for (i = 0; i < count; i++) {
		/* TODO:  handle last command */
		if (state == DSI_CMD_SET_STATE_LP)
			cmds->msg.flags |= MIPI_DSI_MSG_USE_LPM;
		if (cmds->last_command)
			cmds->msg.flags |= MIPI_DSI_MSG_LASTCOMMAND;

		len = ops->transfer(panel->host, &cmds->msg);
		if (len < 0) {
			rc = len;
			pr_err("failed to set cmds(%d), rc=%d\n", cmds->msg.type, rc);
			goto error;
		}
		if (cmds->post_wait_ms)
			usleep_range(cmds->post_wait_ms*1000,
					((cmds->post_wait_ms*1000)+10));

		cmds++;
	}
error:
	return rc;
}

u8 iris_mipi_power_mode_read(struct dsi_panel *panel, enum dsi_cmd_set_state state)
{
	char get_power_mode[1] = {0x0a};
	const struct mipi_dsi_host_ops *ops = panel->host->ops;
	struct dsi_cmd_desc cmds = {
		{0, MIPI_DSI_DCS_READ, 0, 0, sizeof(get_power_mode), get_power_mode, 1, iris_read_cmd_buf}, 1, 0};
	int rc = 0;
	u8 data = 0xff;

	memset(iris_read_cmd_buf, 0, sizeof(iris_read_cmd_buf));
	if (state == DSI_CMD_SET_STATE_LP)
		cmds.msg.flags |= MIPI_DSI_MSG_USE_LPM;
	if (cmds.last_command)
		cmds.msg.flags |= MIPI_DSI_MSG_LASTCOMMAND;
	cmds.msg.flags |= BIT(6);

	rc = ops->transfer(panel->host, &cmds.msg);
	if (rc < 0) {
		pr_err("failed to set cmds(%d), rc=%d, read=%d\n", cmds.msg.type, rc, iris_read_cmd_buf[0]);
		return data;
	}
	data = iris_read_cmd_buf[0];
	pr_err("power_mode: %d\n", data);

	return data;
}

u8 iris_i2c_power_mode_read(void)
{
	u32 reg_value = 0;

	iris_i2c_reg_read(IRIS_MIPI_RX_ADDR + DCS_CMD_PARA_1, &reg_value);

	return (u8)(reg_value & 0xff);
}

int iris_Romcode_state_check(struct dsi_panel *panel, u8 data, u8 retry_cnt)
{
#ifdef READ_CMD_ENABLE
	u8 cnt = 0, powermode = 0;

	do {
		#ifdef I2C_ENABLE
			powermode = iris_i2c_power_mode_read();
		#else
			powermode = iris_mipi_power_mode_read(panel, DSI_CMD_SET_STATE_LP);
		#endif
		pr_debug("read back powermode: %d, cnt: %d\n", powermode, cnt);

		if (powermode == data)
			break;
		usleep_range(500, 510);
		cnt++;
	} while ((powermode != data) && cnt < retry_cnt);

	/* read failed */
	if (cnt == retry_cnt) {
		pr_err("iris power mode check %x failed\n", data);
		return -1;
	}
#endif
	return 0;
}

void iris_mipirx_mode_set(struct dsi_panel *panel, int mode, enum dsi_cmd_set_state state)
{
	char mipirx_mode[1] = {0x3f};
	struct dsi_cmd_desc iris_mipirx_mode_cmds = {
		{0, MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM, 0, 0, sizeof(mipirx_mode), mipirx_mode, 1, iris_read_cmd_buf}, 1, CMD_PROC};
	switch (mode) {
	case MCU_VIDEO:
		mipirx_mode[0] = 0x3f;
		break;
	case MCU_CMD:
		mipirx_mode[0] = 0x1f;
		break;
	case PWIL_VIDEO:
		mipirx_mode[0] = 0xbf;
		break;
	case PWIL_CMD:
		mipirx_mode[0] = 0x7f;
		break;
	case BYPASS_VIDEO:
		mipirx_mode[0] = 0xff;
		break;
	case BYPASS_CMD:
		mipirx_mode[0] = 0xdf;
		break;
	default:
		break;
	}

	iris_dsi_cmds_send(panel, &iris_mipirx_mode_cmds, 1, state);
}

void iris_init_cmd_send(struct dsi_panel *panel, enum dsi_cmd_set_state state)
{
	struct dsi_cmd_desc iris_init_info_cmds[] = {
		{{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, CMD_PKT_SIZE, init_cmd[0].cmd, 1, iris_read_cmd_buf}, 1, CMD_PROC},
		{{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, CMD_PKT_SIZE, init_cmd[1].cmd, 1, iris_read_cmd_buf}, 1, MCU_PROC * 2},
		{{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, CMD_PKT_SIZE, init_cmd[2].cmd, 1, iris_read_cmd_buf}, 1, MCU_PROC * 2},
	};

	iris_init_info_cmds[0].msg.tx_len = init_cmd[0].cmd_len;
	iris_init_info_cmds[1].msg.tx_len = init_cmd[1].cmd_len;
	iris_init_info_cmds[2].msg.tx_len = init_cmd[2].cmd_len;

	pr_debug("iris: send init cmd\n");
	iris_dsi_cmds_send(panel, iris_init_info_cmds, ARRAY_SIZE(iris_init_info_cmds), state);

	DUMP_PACKET(init_cmd[0].cmd, init_cmd[0].cmd_len);
	DUMP_PACKET(init_cmd[1].cmd, init_cmd[1].cmd_len);
}

void iris_timing_info_send(struct dsi_panel *panel, enum dsi_cmd_set_state state)
{
	char iris_workmode[] = {
		0x80, 0x87, 0x0, 0x3,
		PWIL_U32(0x0),
	};
	char iris_timing[] = {
		0x80, 0x87, 0x0, 0x0,
		PWIL_U32(0x01e00010),
		PWIL_U32(0x00160010),
		PWIL_U32(0x0320000a),
		PWIL_U32(0x00080008),
		PWIL_U32(0x3c1f),
		PWIL_U32(0x01e00014),
		PWIL_U32(0x00160010),
		PWIL_U32(0x0320000a),
		PWIL_U32(0x00080008),
		PWIL_U32(0x3c1f),
		PWIL_U32(0x00100008),
		PWIL_U32(0x80),
		PWIL_U32(0x00100008),
		PWIL_U32(0x80)
	};
	struct dsi_cmd_desc iris_timing_info_cmd[] = {
		{{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(iris_workmode), iris_workmode, 1, iris_read_cmd_buf}, 1, MCU_PROC},
		{{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(iris_timing), iris_timing, 1, iris_read_cmd_buf}, 1, MCU_PROC},
	};
	struct iris_timing_info *pinput_timing = &(iris_info.input_timing);
	struct iris_timing_info *poutput_timing = &(iris_info.output_timing);

	memcpy(iris_workmode + 4, &(iris_info.work_mode), 4);

	*(u32 *)(iris_timing + 4) = cpu_to_le32((pinput_timing->hres << 16) + pinput_timing->hfp);
	*(u32 *)(iris_timing + 8) = cpu_to_le32((pinput_timing->hsw << 16) + pinput_timing->hbp);
	*(u32 *)(iris_timing + 12) = cpu_to_le32((pinput_timing->vres << 16) + pinput_timing->vfp);
	*(u32 *)(iris_timing + 16) = cpu_to_le32((pinput_timing->vsw << 16) + pinput_timing->vbp);
	*(u32 *)(iris_timing + 20) = cpu_to_le32((pinput_timing->fps << 8) + 0x1f);

	*(u32 *)(iris_timing + 24) = cpu_to_le32((poutput_timing->hres << 16) + poutput_timing->hfp);
	*(u32 *)(iris_timing + 28) = cpu_to_le32((poutput_timing->hsw << 16) + poutput_timing->hbp);
	*(u32 *)(iris_timing + 32) = cpu_to_le32((poutput_timing->vres << 16) + poutput_timing->vfp);
	*(u32 *)(iris_timing + 36) = cpu_to_le32((poutput_timing->vsw << 16) + poutput_timing->vbp);
	*(u32 *)(iris_timing + 40) = cpu_to_le32((poutput_timing->fps << 8) + 0x1f);

	pr_debug("iris: send timing info\n");
	iris_dsi_cmds_send(panel, iris_timing_info_cmd, ARRAY_SIZE(iris_timing_info_cmd), state);

	DUMP_PACKET(iris_workmode, sizeof(iris_workmode));
	DUMP_PACKET(iris_timing, sizeof(iris_timing));
}

void iris_ctrl_cmd_send(struct dsi_panel *panel, u8 cmd, enum dsi_cmd_set_state state)
{
	char romcode_ctrl[] = {
		PWIL_TAG('P', 'W', 'I', 'L'),
		PWIL_TAG('G', 'R', 'C', 'P'),
		PWIL_U32(0x00000005),
		0x00,
		0x00,
		PWIL_U16(0x04),
		PWIL_U32(IRIS_MODE_ADDR),  /*proxy_MB1*/
		PWIL_U32(0x00000000),
		PWIL_U32(IRIS_PROXY_MB7_ADDR),  /*proxy_MB7*/
		PWIL_U32(0x00040000),
	};
	struct dsi_cmd_desc iris_romcode_ctrl_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(romcode_ctrl), romcode_ctrl, 1, iris_read_cmd_buf}, 1, CMD_PROC};

	if ((cmd | CONFIG_DATAPATH) || (cmd | ENABLE_DPORT) || (cmd | REMAP))
		iris_romcode_ctrl_cmd.post_wait_ms = INIT_WAIT;

	romcode_ctrl[20] = cmd;
	*(u32 *)(romcode_ctrl + 28) = cpu_to_le32(iris_info.firmware_info.app_fw_size);

	iris_dsi_cmds_send(panel, &iris_romcode_ctrl_cmd, 1, state);
}

void iris_feature_init_cmd_send(struct dsi_panel *panel, enum dsi_cmd_set_state state)
{
	struct feature_setting *chip_setting = &iris_info.chip_setting;
	struct feature_setting *user_setting = &iris_info.user_setting;
	struct iris_setting_update *settint_update = &iris_info.settint_update;
	u32 grcp_len = 0;
	struct dsi_cmd_desc iris_pq_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, CMD_PKT_SIZE, grcp_cmd.cmd, 1, iris_read_cmd_buf}, 1, CMD_PROC};

	if (iris_debug_fw_download_disable)
		return;

	memset(&grcp_cmd, 0, sizeof(grcp_cmd));
	memcpy(grcp_cmd.cmd, grcp_header, GRCP_HEADER);
	grcp_cmd.cmd_len = GRCP_HEADER;

	iris_feature_setting_update_check();

	if (settint_update->pq_setting) {
		chip_setting->pq_setting = user_setting->pq_setting;
		iris_cmd_reg_add(&grcp_cmd, IRIS_PQ_SETTING_ADDR, *((u32 *)&chip_setting->pq_setting));
		settint_update->pq_setting = false;
	}

	if (settint_update->dbc_setting) {
		chip_setting->dbc_setting = user_setting->dbc_setting;
		iris_cmd_reg_add(&grcp_cmd, IRIS_DBC_SETTING_ADDR, *((u32 *)&chip_setting->dbc_setting));
		settint_update->dbc_setting = false;
	}

	if (settint_update->lp_memc_setting) {
		chip_setting->lp_memc_setting = user_setting->lp_memc_setting;
		iris_cmd_reg_add(&grcp_cmd, IRIS_LPMEMC_SETTING_ADDR, chip_setting->lp_memc_setting.value | 0x80000000);
		settint_update->lp_memc_setting = false;
	}

	if (settint_update->lce_setting) {
		chip_setting->lce_setting = user_setting->lce_setting;
		iris_cmd_reg_add(&grcp_cmd, IRIS_LCE_SETTING_ADDR, *((u32 *)&chip_setting->lce_setting));
		settint_update->lce_setting = false;
	} else {
		iris_cmd_reg_add(&grcp_cmd, IRIS_LCE_SETTING_ADDR, 0x0);
	}

	if (settint_update->cm_setting) {
		chip_setting->cm_setting = user_setting->cm_setting;
		iris_cmd_reg_add(&grcp_cmd, IRIS_CM_SETTING_ADDR, *((u32 *)&chip_setting->cm_setting));
		settint_update->cm_setting = false;
	} else {
		iris_cmd_reg_add(&grcp_cmd, IRIS_CM_SETTING_ADDR, 0x0);
	}

	if (settint_update->reading_mode) {
		chip_setting->reading_mode = user_setting->reading_mode;
		iris_cmd_reg_add(&grcp_cmd, IRIS_READING_MODE_ADDR, *((u32 *)&chip_setting->reading_mode));
		settint_update->reading_mode= false;
	} else {
		iris_cmd_reg_add(&grcp_cmd, IRIS_READING_MODE_ADDR, 0x0);
	}
	iris_cmd_reg_add(&grcp_cmd, IRIS_COLOR_ADJUST_ADDR, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_HDR_SETTING_ADDR, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_TRUECUT_INFO_ADDR, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_LUX_VALUE_ADDR, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_CCT_VALUE_ADDR, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DRC_INFO_ADDR, 0x0);

	grcp_len = (grcp_cmd.cmd_len - GRCP_HEADER) / 4;
	*(u32 *)(grcp_cmd.cmd + 8) = cpu_to_le32(grcp_len + 1);
	*(u16 *)(grcp_cmd.cmd + 14) = cpu_to_le16(grcp_len);

	iris_pq_cmd.msg.tx_len = grcp_cmd.cmd_len;
	iris_dsi_cmds_send(panel, &iris_pq_cmd, 1, state);

	iris_cmlut_table_load(panel);
}


void iris_dtg_setting_cmd_send(struct dsi_panel *panel, enum dsi_cmd_set_state state)
{
	u32 grcp_len = 0;
	struct iris_dtg_setting *dtg_setting = &iris_info.dtg_setting;
	struct dsi_cmd_desc iris_dtg_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, CMD_PKT_SIZE, grcp_cmd.cmd, 1, iris_read_cmd_buf}, 1, CMD_PROC};

	memset(&grcp_cmd, 0, sizeof(grcp_cmd));
	memcpy(grcp_cmd.cmd, grcp_header, GRCP_HEADER);
	grcp_cmd.cmd_len = GRCP_HEADER;

	/*0x29 cmd*/
	iris_cmd_reg_add(&grcp_cmd, IRIS_MIPI_TX_ADDR + WR_PACKET_HEADER_OFFS, 0x03002905);

	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DTG_DELAY, dtg_setting->dtg_delay);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL, dtg_setting->te_ctrl);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL_1, dtg_setting->te_ctrl_1);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL_2, dtg_setting->te_ctrl_2);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL_3, dtg_setting->te_ctrl_3);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL_4, dtg_setting->te_ctrl_4);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_CTRL_5, dtg_setting->te_ctrl_5);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DTG_CTRL_1,dtg_setting->dtg_ctrl_1);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DTG_CTRL, dtg_setting->dtg_ctrl);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + EVS_DLY, dtg_setting->evs_dly);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + EVS_NEW_DLY, dtg_setting->evs_new_dly);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DVS_CTRL, dtg_setting->dvs_ctrl);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_DLY, dtg_setting->te_dly);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + TE_DLY_1, dtg_setting->te_dly_1);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + VFP_CTRL_0, dtg_setting->vfp_ctrl_0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + VFP_CTRL_1, dtg_setting->vfp_ctrl_1);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DTG_RESERVE, 0x1000000b);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DTG_ADDR + DTG_REGSEL, 1);
	/* set PWM output to 25 Khz */
	iris_cmd_reg_add(&grcp_cmd, 0xf10c0000, 0x000004a4);
	iris_cmd_reg_add(&grcp_cmd, 0xf10c0004, 0x0000ffff);
	iris_cmd_reg_add(&grcp_cmd, 0xf10c0010, 0x00000003);
	iris_cmd_reg_add(&grcp_cmd, 0xf10c0014, 0x00011008);

	iris_cmd_reg_add(&grcp_cmd, IRIS_DPORT_ADDR + DPORT_DBIB, 0x0);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DPORT_ADDR + DPORT_CTRL1, 0x21000803);
	iris_cmd_reg_add(&grcp_cmd, IRIS_DPORT_ADDR + DPORT_REGSEL, 1);

	grcp_len = (grcp_cmd.cmd_len - GRCP_HEADER) / 4;
	*(u32 *)(grcp_cmd.cmd + 8) = cpu_to_le32(grcp_len + 1);
	*(u16 *)(grcp_cmd.cmd + 14) = cpu_to_le16(grcp_len);

	iris_dtg_cmd.msg.tx_len = grcp_cmd.cmd_len;
	iris_dsi_cmds_send(panel, &iris_dtg_cmd, 1, state);

	DUMP_PACKET(grcp_cmd.cmd, grcp_cmd.cmd_len);
}

void iris_pwil_mode_set(struct dsi_panel *panel, u8 mode, enum dsi_cmd_set_state state)
{
	char pwil_mode[2] = {0x00, 0x00};
	struct dsi_cmd_desc iris_pwil_mode_cmd = {
		{0, MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM, 0, 0, sizeof(pwil_mode), pwil_mode, 1, iris_read_cmd_buf}, 1, CMD_PROC};

	if (PT_MODE == mode) {
		pwil_mode[0] = 0x0;
		pwil_mode[1] = 0x1;
	} else if (RFB_MODE == mode) {
		pwil_mode[0] = 0xc;
		pwil_mode[1] = 0x1;
	} else if (BIN_MODE == mode) {
		pwil_mode[0] = 0xc;
		pwil_mode[1] = 0x20;
	} else if (FRC_MODE == mode) {
		pwil_mode[0] = 0x4;
		pwil_mode[1] = 0x2;
	}

	iris_dsi_cmds_send(panel, &iris_pwil_mode_cmd, 1, state);
}

void iris_mipi_mem_addr_cmd_send(struct dsi_panel *panel, u16 column, u16 page, enum dsi_cmd_set_state state)
{
	char mem_addr[2] = {0x36, 0x0};
	char pixel_format[2] = {0x3a, 0x77};
	char col_addr[5] = {0x2a, 0x00, 0x00, 0x03, 0xff};
	char page_addr[5] = {0x2b, 0x00, 0x00, 0x03, 0xff};
	struct dsi_cmd_desc iris_mem_addr_cmd[] = {
		{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, sizeof(mem_addr), mem_addr, 1, iris_read_cmd_buf}, 0, 0},
		{{0, MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0, 0, sizeof(pixel_format), pixel_format, 1, iris_read_cmd_buf}, 0, 0},
		{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, sizeof(col_addr), col_addr, 1, iris_read_cmd_buf}, 0, 0},
		{{0, MIPI_DSI_DCS_LONG_WRITE, 0, 0, sizeof(page_addr), page_addr, 1, iris_read_cmd_buf}, 1, 0},
	};

	col_addr[3] = (column >> 8) & 0xff;
	col_addr[4] = column & 0xff;
	page_addr[3] = (page >> 8) & 0xff;
	page_addr[4] = page & 0xff;

	pr_debug("iris: set mipi mem addr: %x, %x\n", column, page);
	iris_dsi_cmds_send(panel, iris_mem_addr_cmd, ARRAY_SIZE(iris_mem_addr_cmd), state);

}

int iris_firmware_data_read(const u8 *fw_data, size_t fw_size)
{
	u32 pkt_size = FW_COL_CNT * 3, cmd_len = 0, cmd_cnt = 0, buf_indx = 0;


	if(!app_fw_buf) {
		app_fw_buf = kzalloc(DSI_DMA_TX_BUF_SIZE, GFP_KERNEL);
		if (!app_fw_buf) {
			pr_err("%s: failed to alloc app fw mem, size = %d\n", __func__, DSI_DMA_TX_BUF_SIZE);
			return false;
		}
	}
	memset(app_fw_buf, 0, DSI_DMA_TX_BUF_SIZE);

	while (fw_size) {
		if (fw_size >= pkt_size)
			cmd_len = pkt_size;
		else
			cmd_len = fw_size;

		if (0 == cmd_cnt)
			app_fw_buf[0] = DCS_WRITE_MEM_START;
		else
			app_fw_buf[buf_indx] = DCS_WRITE_MEM_CONTINUE;

		memcpy(app_fw_buf + buf_indx + 1, fw_data, cmd_len);

		fw_size -= cmd_len;
		fw_data += cmd_len;
		cmd_cnt++;
		buf_indx += cmd_len + 1;
	}
	return true;
}

int iris_firmware_download_init(struct dsi_panel *panel, const char *name)
{
	const struct firmware *fw = NULL;
	int ret = 0, result = true;

	//iris_info.firmware_info.app_fw_size = 0;
	if (name) {
		/* Firmware file must be in /system/etc/firmware/ */
		ret = request_firmware(&fw, name, iris_drm->dev);
		if (ret) {
			pr_err("%s: failed to request firmware: %s, ret = %d\n",
					__func__, name, ret);
			result = false;
		} else {
			pr_err("%s: request firmware: name = %s, size = %zu bytes\n",
						__func__, name, fw->size);
			if (iris_firmware_data_read(fw->data, fw->size))
				iris_info.firmware_info.app_fw_size = fw->size;
			release_firmware(fw);
		}
	} else {
		pr_err("%s: firmware is null\n", __func__);
		result = false;
	}

	return result;
}

void iris_firmware_download_prepare(struct dsi_panel *panel, size_t size)
{
#define TIME_INTERVAL 20  /*ms*/

	char fw_download_config[] = {
		PWIL_TAG('P', 'W', 'I', 'L'),
		PWIL_TAG('G', 'R', 'C', 'P'),
		PWIL_U32(0x000000013),
		0x00,
		0x00,
		PWIL_U16(0x0012),
		PWIL_U32(IRIS_PWIL_ADDR + 0x0004),  /*PWIL ctrl1 confirm transfer mode and cmd mode, single channel.*/
		PWIL_U32(0x00004144),
		PWIL_U32(IRIS_PWIL_ADDR + 0x0218),  /*CAPEN*/
		PWIL_U32(0xc0000003),
		PWIL_U32(IRIS_PWIL_ADDR + 0x1140),  /*channel order*/
		PWIL_U32(0xc6120010),
		PWIL_U32(IRIS_PWIL_ADDR + 0x1144),  /*pixelformat*/
		PWIL_U32(0x888),
		PWIL_U32(IRIS_PWIL_ADDR + 0x114c),  /*capt size*/
		PWIL_U32(0x08700438),
		PWIL_U32(IRIS_PWIL_ADDR + 0x1158),	/*mem addr*/
		PWIL_U32(0x00000000),
		PWIL_U32(IRIS_PWIL_ADDR + 0x10000), /*update setting. using SW update mode*/
		PWIL_U32(0x00000100),
		PWIL_U32(IRIS_PWIL_ADDR + 0x1fff0), /*clear down load int*/
		PWIL_U32(0x00008000),
		PWIL_U32(IRIS_MIPI_RX_ADDR + 0xc),	/*mipi_rx setting DBI_bus*/
		PWIL_U32(0x000f0000),
		PWIL_U32(IRIS_MIPI_RX_ADDR + 0x001c), /*mipi_rx time out threshold*/
		PWIL_U32(0xffffffff)
	};
	u32 threshold = 0, fw_hres, fw_vres;
	struct dsi_cmd_desc fw_download_config_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(fw_download_config), fw_download_config, 1, iris_read_cmd_buf}, 1, 0};


	threshold = 200000; //ctrl->pclk_rate / 1000;
	threshold *= TIME_INTERVAL;
	*(u32 *)(fw_download_config + 92) = cpu_to_le32(threshold);

	/*firmware download need mipirx work on single cmd mode, pwil work on binarary mode.*/
	iris_dsi_cmds_send(panel, &fw_download_config_cmd, 1, DSI_CMD_SET_STATE_HS);


	iris_pwil_mode_set(panel, BIN_MODE, DSI_CMD_SET_STATE_HS);

	fw_hres = FW_COL_CNT - 1;
	fw_vres = (size + FW_COL_CNT * 3 - 1) / (FW_COL_CNT * 3) - 1;
	iris_mipi_mem_addr_cmd_send(panel, fw_hres, fw_vres, DSI_CMD_SET_STATE_HS);

}

int iris_firmware_data_send(struct dsi_panel *panel, u8 *buf, size_t fw_size)
{
	u32 pkt_size = FW_COL_CNT * 3, cmd_len = 0, cmd_cnt = 0, buf_indx = 0, cmd_indx = 0;
	static struct dsi_cmd_desc fw_send_cmd[FW_DW_CMD_CNT];

	memset(fw_send_cmd, 0, sizeof(fw_send_cmd));

	while (fw_size) {
		if (fw_size >= pkt_size)
			cmd_len = pkt_size;
		else
			cmd_len = fw_size;

		cmd_indx = cmd_cnt % FW_DW_CMD_CNT;
		fw_send_cmd[cmd_indx].last_command = 0;
		fw_send_cmd[cmd_indx].msg.type = 0x39;
		fw_send_cmd[cmd_indx].msg.tx_len = pkt_size + 1;
		fw_send_cmd[cmd_indx].msg.tx_buf = buf + buf_indx;
		fw_send_cmd[cmd_indx].msg.rx_len = 1;
		fw_send_cmd[cmd_indx].msg.rx_buf = iris_read_cmd_buf;

		fw_size -= cmd_len;
		cmd_cnt++;
		buf_indx += cmd_len + 1;

		if (((FW_DW_CMD_CNT - 1) == cmd_indx) || (0 == fw_size)) {
			fw_send_cmd[cmd_indx].last_command = 1;
			fw_send_cmd[cmd_indx].post_wait_ms = 0;
			iris_dsi_cmds_send(panel, fw_send_cmd, cmd_indx + 1, DSI_CMD_SET_STATE_HS);
		}
	}
	return true;
}

u16 iris_mipi_fw_download_result_read(struct dsi_panel *panel)
{
	char mipirx_status[1] = {0xaf};
	const struct mipi_dsi_host_ops *ops = panel->host->ops;
	struct dsi_cmd_desc cmds = {
		{0, MIPI_DSI_DCS_READ, 0, 0, sizeof(mipirx_status), mipirx_status, 2, iris_read_cmd_buf}, 1, 0};
	int rc = 0;
	u16 data = 0xffff;

	memset(iris_read_cmd_buf, 0, sizeof(iris_read_cmd_buf));
	cmds.msg.flags |= MIPI_DSI_MSG_USE_LPM | MIPI_DSI_MSG_LASTCOMMAND;
	cmds.msg.flags |= BIT(6);

	rc = ops->transfer(panel->host, &cmds.msg);
	if (rc < 0) {
		pr_err("failed to set cmds(%d), rc=%d\n", cmds.msg.type, rc);
		return 0xff;
	}
	data = iris_read_cmd_buf[0] | ((iris_read_cmd_buf[1] & 0x0f) << 8);
	pr_err("download_reslut: %x\n", data);

	return (data & 0x0f00);
}

u16 iris_i2c_fw_download_result_read(void)
{
	u32 reg_value = 0;

	iris_i2c_reg_read(IRIS_MIPI_RX_ADDR + INTSTAT_RAW, &reg_value);
	reg_value &= (1 << 10);

	return (u16)(reg_value >> 2);
}

int iris_fw_download_result_check(struct dsi_panel *panel)
{
#define FW_CHECK_COUNT 20
	u16 cnt = 0, result = 0;
	u32 reg_value;

	do {
		#ifdef I2C_ENABLE
			result = iris_i2c_fw_download_result_read();
		#else
			result = iris_mipi_fw_download_result_read(panel);
		#endif
		if (0x0100 == result)
			break;

		usleep_range(1000, 1010);
		cnt++;
	} while ((result != 0x0100) && cnt < FW_CHECK_COUNT);

	iris_i2c_reg_read(IRIS_MIPI_RX_ADDR + INTSTAT_RAW, &reg_value);
	if (reg_value & (0x3 << 7)) {
		pr_err("abnormal error: %x\n", reg_value);
		iris_i2c_reg_write(IRIS_MIPI_RX_ADDR + INTCLR, 0x780);
		return false;
	}

	/*read failed*/
	if (FW_CHECK_COUNT == cnt) {
		pr_err("firmware download failed\n");
		return false;
	} else
		pr_debug("firmware download success\n");

	return true;
}

void iris_firmware_download_restore(struct dsi_panel *panel, bool cont_splash)
{
	char fw_download_restore[] = {
		PWIL_TAG('P', 'W', 'I', 'L'),
		PWIL_TAG('G', 'R', 'C', 'P'),
		PWIL_U32(0x00000007),
		0x00,
		0x00,
		PWIL_U16(0x06),
		PWIL_U32(IRIS_PWIL_ADDR + 0x0004),
		PWIL_U32(0x00004140),
		PWIL_U32(IRIS_MIPI_RX_ADDR + 0xc),
		PWIL_U32(0x000f0000),
		PWIL_U32(IRIS_MIPI_RX_ADDR + 0x001c),
		PWIL_U32(0xffffffff)
	};
	u32 col_addr = 0, page_addr = 0;
	struct dsi_cmd_desc fw_download_restore_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(fw_download_restore), fw_download_restore, 1, iris_read_cmd_buf}, 1, 0};

	if (DSI_OP_CMD_MODE == iris_info.work_mode.rx_mode)
		fw_download_restore[20] += (2 << 1);

	if (1 == iris_info.work_mode.rx_ch) {
		fw_download_restore[20] += 1;
		fw_download_restore[30] = 0x8f;
	}

	if (1 == iris_info.work_mode.rx_ch)
		col_addr = iris_info.input_timing.hres * 2 - 1;
	else
		col_addr = iris_info.input_timing.hres - 1;

	page_addr = iris_info.input_timing.vres - 1;

	iris_dsi_cmds_send(panel, &fw_download_restore_cmd, 1, DSI_CMD_SET_STATE_HS);

	if (cont_splash) {
		iris_pwil_mode_set(panel, RFB_MODE, DSI_CMD_SET_STATE_HS);

		if (DSI_OP_CMD_MODE == iris_info.work_mode.rx_mode)
			iris_mipi_mem_addr_cmd_send(panel, col_addr, page_addr, DSI_CMD_SET_STATE_HS);
	}
}

int iris_firmware_download(struct dsi_panel *panel,
							const char *name,
							bool cont_splash)
{
#define FW_DOWNLOAD_RETRYCNT_MAX 3
	int ret = true, result = false, cnt = 0;
	int fw_size = 0, cm3d = iris_info.user_setting.cm_setting.cm3d;

	if (!iris_info.firmware_info.app_fw_size)
		ret = iris_firmware_download_init(panel, name);

	if (true == ret) {
		fw_size = iris_info.firmware_info.app_fw_size;

		//if (!cm3d)
		iris_firmware_cmlut_update(app_fw_buf, cm3d);
		iris_firmware_gamma_table_update(app_fw_buf, cm3d);

		iris_firmware_download_prepare(panel, fw_size);
		for (cnt = 0; cnt < FW_DOWNLOAD_RETRYCNT_MAX; cnt++) {
			iris_firmware_data_send(panel, app_fw_buf, fw_size);
			#ifdef READ_CMD_ENABLE
				result = iris_fw_download_result_check(panel);
			#else
				result = true;
				msleep(10);
			#endif
			if (result == true)
				break;
			pr_info("iris firmware download again\n");
		}
		iris_firmware_download_restore(panel, cont_splash);
	}
	return result;
}

int iris_lut_firmware_init(const char *name)
{
	const struct firmware *fw = NULL;
	struct iris_lut_info *lut_info = &iris_info.lut_info;
	int indx, ret = 0;

	if (lut_info->lut_fw_state)
		return true;

	if (!lut_fw_buf) {
		lut_fw_buf = kzalloc(IRIS_CM_LUT_LENGTH * 4 * CMI_TOTAL, GFP_KERNEL);
		if (!lut_fw_buf) {
			pr_err("%s: failed to alloc  lut fw mem, size = %d\n", __func__, IRIS_CM_LUT_LENGTH * 4 * CMI_TOTAL);
			return false;
		}
	}

	for (indx = 0; indx < CMI_TOTAL; indx++)
		lut_info->cmlut[indx] = (u32 *)(lut_fw_buf + indx * IRIS_CM_LUT_LENGTH * 4);

	if (name) {
		/* Firmware file must be in /system/etc/firmware/ */
		ret = request_firmware(&fw, name, iris_drm->dev);
		if (ret) {
			pr_err("%s: failed to request firmware: %s, ret = %d\n",
					__func__, name, ret);
			return false;
		} else {
			pr_info("%s: request firmware: name = %s, size = %zu bytes\n",
						__func__, name, fw->size);
			memcpy(lut_fw_buf, fw->data, fw->size);
			lut_info->lut_fw_state = true;
			release_firmware(fw);
		}
	} else {
		pr_err("%s: firmware is null\n", __func__);
		return false;
	}

	return true;
}

int iris_gamma_firmware_init(const char *name)
{
	const struct firmware *fw = NULL;
	struct iris_gamma_info *gamma_info = &iris_info.gamma_info;
	int ret = 0;

	if (gamma_info->gamma_fw_state)
		return true;

	if (!gamma_fw_buf) {
		gamma_fw_buf = kzalloc(8 * 1024, GFP_KERNEL);
		if (!gamma_fw_buf) {
			pr_err("%s: failed to alloc gamma fw mem, size = %d\n", __func__, 8 * 1024);
			return false;
		}
	}

	if (name) {
		/* Firmware file must be in /system/etc/firmware/ */
		ret = request_firmware(&fw, name, iris_drm->dev);
		if (ret) {
			pr_err("%s: failed to request firmware: %s, ret = %d\n",
					__func__, name, ret);
			return false;
		} else {
			pr_info("%s: request firmware: name = %s, size = %zu bytes\n",
						__func__, name, fw->size);
			memcpy(gamma_fw_buf, fw->data, fw->size);
			gamma_info->gamma_fw_size = fw->size;
			gamma_info->gamma_fw_state = true;
			release_firmware(fw);
		}
	} else {
		pr_err("%s: firmware is null\n", __func__);
		return false;
	}

	return true;
}

void iris_firmware_cmlut_table_copy(u8 *fw_buf, u32 lut_offset, u8 *lut_buf, u32 lut_size)
{
	u32 pkt_size = FW_COL_CNT * 3, buf_offset, pkt_offset;
	u32 len;

	buf_offset = (lut_offset / pkt_size) *(pkt_size + 1);
	pkt_offset = lut_offset % pkt_size;
	fw_buf += buf_offset + pkt_offset + 1;

	while (lut_size) {
		if (pkt_offset) {
			len = pkt_size - pkt_offset;
			pkt_offset = 0;
		} else if (lut_size > pkt_size)
			len = pkt_size;
		else
			len = lut_size;

		memcpy(fw_buf, lut_buf, len);
		fw_buf += len + 1;
		lut_buf += len;
		lut_size -= len;
	}
}

void iris_firmware_cmlut_update(u8 *buf, u32 cm3d)
{
	struct iris_lut_info *lut_info = &iris_info.lut_info;

	if (!lut_info->lut_fw_state) {
		pr_debug("cm lut read failed\n");
		return;
	}
	switch (cm3d) {
		case CM_NTSC:		//table for NTSC
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_2800K_ADDR, (u8 *)(lut_info->cmlut[CMI_NTSC_2800]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_6500K_ADDR, (u8 *)(lut_info->cmlut[CMI_NTSC_6500_C51_ENDIAN]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_8000K_ADDR, (u8 *)(lut_info->cmlut[CMI_NTSC_8000]), IRIS_CM_LUT_LENGTH * 4);
		break;
		case CM_sRGB:		//table for SRGB
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_2800K_ADDR, (u8 *)(lut_info->cmlut[CMI_SRGB_2800]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_6500K_ADDR, (u8 *)(lut_info->cmlut[CMI_SRGB_6500_C51_ENDIAN]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_8000K_ADDR, (u8 *)(lut_info->cmlut[CMI_SRGB_8000]), IRIS_CM_LUT_LENGTH * 4);
		break;
		case CM_DCI_P3:		//table for DCI-P3
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_2800K_ADDR, (u8 *)(lut_info->cmlut[CMI_P3_2800]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_6500K_ADDR, (u8 *)(lut_info->cmlut[CMI_P3_6500_C51_ENDIAN]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_8000K_ADDR, (u8 *)(lut_info->cmlut[CMI_P3_8000]), IRIS_CM_LUT_LENGTH * 4);
		break;
		case CM_HDR:		//table for hdr
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_2800K_ADDR, (u8 *)(lut_info->cmlut[CMI_HDR_2800]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_6500K_ADDR, (u8 *)(lut_info->cmlut[CMI_HDR_6500_C51_ENDIAN]), IRIS_CM_LUT_LENGTH * 4);
		iris_firmware_cmlut_table_copy(buf, IRIS_CMLUT_8000K_ADDR, (u8 *)(lut_info->cmlut[CMI_HDR_8000]), IRIS_CM_LUT_LENGTH * 4);
		break;
	}
}

void trigger_download_ccf_fw(void)
{
	iris_lut_firmware_init(LUT_FIRMWARE_NAME);
	iris_gamma_firmware_init(GAMMA_FIRMWARE_NAME);
}

void iris_gamma_table_endian_convert(u8 *dst_buf, u8 *src_buf)
{
	u32 cnt = IRIS_GAMMA_TABLE_LENGTH * 3;

	for (cnt = 0; cnt < IRIS_GAMMA_TABLE_LENGTH * 3; cnt++) {
		*dst_buf = *(src_buf + 3);
		*(dst_buf + 1) = *(src_buf + 2);
		*(dst_buf + 2) = *(src_buf + 1);
		*(dst_buf + 3) = *src_buf;

		dst_buf += 4;
		src_buf += 4;
	}
}

void iris_firmware_gamma_table_update(u8 *buf, u32 cm3d)
{
	u8 *table_buf = gamma_fw_buf;
	u8 *tmp_buf = gamma_fw_buf + 6 * 1024;

	if (!iris_info.gamma_info.gamma_fw_state || !gamma_fw_buf)
		return;

	table_buf += 16;
	switch (cm3d) {
		case CM_NTSC:
		iris_gamma_table_endian_convert(tmp_buf, table_buf);
		iris_firmware_cmlut_table_copy(buf, IRIS_GAMMA_TABLE_ADDR, tmp_buf, IRIS_GAMMA_TABLE_LENGTH * 3 * 4);
		break;
		case CM_sRGB:
		table_buf += IRIS_GAMMA_TABLE_LENGTH * 3 * 4;
		iris_gamma_table_endian_convert(tmp_buf, table_buf);
		iris_firmware_cmlut_table_copy(buf, IRIS_GAMMA_TABLE_ADDR, tmp_buf, IRIS_GAMMA_TABLE_LENGTH * 3 * 4);
		break;
		case CM_DCI_P3:
		table_buf += 2 * IRIS_GAMMA_TABLE_LENGTH * 3 * 4;
		iris_gamma_table_endian_convert(tmp_buf, table_buf);
		iris_firmware_cmlut_table_copy(buf, IRIS_GAMMA_TABLE_ADDR, tmp_buf, IRIS_GAMMA_TABLE_LENGTH * 3 * 4);
		break;
		default:
		break;
	}

}

void iris_reset_failure_recover(struct dsi_panel *panel)
{
	u8 cnt = 0;
	int ret = 0;

	do {
		/* reset iris */
		if (gpio_is_valid(panel->dsp_cfg.dsp_reset_gpio)) {
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 0);
			usleep_range(5000, 5010);
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 1);
			usleep_range(5000, 5010);
		}
		ret = iris_Romcode_state_check(panel, 0x01, 50);
		cnt++;
		pr_err("iris_reset_failure_recover: %d\n", cnt);
	} while((cnt < 3) && (ret < 0));

}

void iris_init_failure_recover(struct dsi_panel *panel)
{
	u8 cnt = 0;
	int ret = 0;

	do {
		/* reset iris */
		if (gpio_is_valid(panel->dsp_cfg.dsp_reset_gpio)) {
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 0);
			usleep_range(5000, 5010);
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 1);
			usleep_range(5000, 5010);
		}
		iris_Romcode_state_check(panel, 0x01, 50);

		iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);
		/* init sys/mipirx/mipitx */
		iris_init_cmd_send(panel, DSI_CMD_SET_STATE_LP);

		/* send work mode and timing info */
		iris_mipirx_mode_set(panel, MCU_CMD, DSI_CMD_SET_STATE_LP);
		iris_timing_info_send(panel, DSI_CMD_SET_STATE_LP);
		iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);

		iris_ctrl_cmd_send(panel, CONFIG_DATAPATH, DSI_CMD_SET_STATE_LP);
		usleep_range(20000, 20010);
		/*init feature*/
		iris_feature_init_cmd_send(panel, DSI_CMD_SET_STATE_HS);
		/* wait Romcode ready */
		ret = iris_Romcode_state_check(panel, 0x03, 50);
		cnt++;
		pr_err("iris_init_failure_recover: %d\n", cnt);
	} while((cnt < 3) && (ret < 0));

}

void iris_enable_failure_recover(struct dsi_panel *panel)
{
	u8 cnt = 0;
	int ret = 0;
	u32 column, page;

	do {
		/* reset iris */
		if (gpio_is_valid(panel->dsp_cfg.dsp_reset_gpio)) {
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 0);
			usleep_range(5000, 5010);
			gpio_direction_output(panel->dsp_cfg.dsp_reset_gpio, 1);
			usleep_range(5000, 5010);
		}
		iris_Romcode_state_check(panel, 0x01, 50);

		iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);
		/* init sys/mipirx/mipitx */
		iris_init_cmd_send(panel, DSI_CMD_SET_STATE_LP);

		/* send work mode and timing info */
		iris_mipirx_mode_set(panel, MCU_CMD, DSI_CMD_SET_STATE_LP);
		iris_timing_info_send(panel, DSI_CMD_SET_STATE_LP);
		iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);

		iris_ctrl_cmd_send(panel, CONFIG_DATAPATH, DSI_CMD_SET_STATE_LP);
		usleep_range(20000, 20010);
		/*init feature*/
		iris_feature_init_cmd_send(panel, DSI_CMD_SET_STATE_HS);
		/* wait Romcode ready */
		iris_Romcode_state_check(panel, 0x03, 50);

		iris_dtg_setting_cmd_send(panel, DSI_CMD_SET_STATE_HS);
		iris_ctrl_cmd_send(panel, ENABLE_DPORT, DSI_CMD_SET_STATE_HS);
		iris_pwil_mode_set(panel, RFB_MODE, DSI_CMD_SET_STATE_HS);
		/* update rx column/page addr */
		column = iris_info.work_mode.rx_ch ? (iris_info.input_timing.hres * 2 - 1) : (iris_info.input_timing.hres - 1);
		page = iris_info.input_timing.vres - 1;
		iris_mipi_mem_addr_cmd_send(panel, column, page, DSI_CMD_SET_STATE_HS);
		/* wait Romcode ready */
		ret = iris_Romcode_state_check(panel, 0x07, 50);
		cnt++;
		pr_err("iris_enable_failure_recover: %d\n", cnt);
	} while((cnt < 3) && (ret < 0));

}

void iris_firmware_download_cont_splash(struct dsi_panel *panel)
{
	u32 cmd = 0;
	static bool first_boot = true;
	int ret = 0;

	if (!first_boot)
		return;

	pr_info("iris_firmware_download_cont_splash\n");
	first_boot = false;
	ret = iris_Romcode_state_check(panel, 0x07, 50);
	if (ret < 0)
		iris_enable_failure_recover(panel);

	if (iris_debug_fw_download_disable) {
		iris_info.firmware_info.app_fw_state = false;
		iris_info.firmware_info.app_cnt = 0;
	} else {
		/* firmware download */
		iris_info.firmware_info.app_fw_state =
				iris_firmware_download(panel, APP_FIRMWARE_NAME, true);
		if (true == iris_info.firmware_info.app_fw_state) {
			cmd = REMAP + ITCM_COPY;
			iris_ctrl_cmd_send(panel, cmd, DSI_CMD_SET_STATE_HS);
			iris_info.firmware_info.app_cnt = 0;
		}
	}
	iris_feature_setting_update_check();
	iris_mode_switch_state_reset();
	iris_info.power_status.low_power_state = LP_STATE_POWER_UP;
}

void iris_appcode_init_done_wait(struct dsi_panel *panel)
{
	if (iris_lightup_flag) {
		iris_lightup_flag = false;
		pr_debug("iris_appcode_init_done_wait\n");
		/*wait appcode init done*/
		if (!iris_debug_fw_download_disable)
			iris_Romcode_state_check(panel, 0x9b, 200);
	}
}

void iris_dport_enable_cmd_send(struct dsi_panel *panel, bool enable)
{
	char dport_enable[] = {
		PWIL_TAG('P', 'W', 'I', 'L'),
		PWIL_TAG('G', 'R', 'C', 'P'),
		PWIL_U32(0x00000005),
		0x00,
		0x00,
		PWIL_U16(0x04),
		PWIL_U32(IRIS_DPORT_ADDR + DPORT_CTRL1),
		PWIL_U32(0x21000803),
		PWIL_U32(IRIS_DPORT_ADDR + DPORT_REGSEL),
		PWIL_U32(0x000f0001)
	};
	struct dsi_cmd_desc dport_enable_cmd = {
		{0, MIPI_DSI_GENERIC_LONG_WRITE, 0, 0, sizeof(dport_enable), dport_enable, 1, iris_read_cmd_buf}, 1, 16};

	if (enable)
		dport_enable[20] = 0x03;
	else
		dport_enable[20] = 0x00;

	iris_dsi_cmds_send(panel, &dport_enable_cmd, 1, DSI_CMD_SET_STATE_HS);
}


void iris_power_down(void)
{
	iris_i2c_reg_write(0xf104000c, 0xffffffff);

	iris_i2c_reg_write(0xf0000210, 0x0000070b);
	iris_i2c_reg_write(0xf0000214, 0x0000070b);
	iris_i2c_reg_write(0xf0000218, 0x0003ff03);
	iris_i2c_reg_write(0xf000021c, 0x0000070b);
	iris_i2c_reg_write(0xf0000228, 0x0000070b);
	iris_i2c_reg_write(0xf0000014, 0x00000001);
	iris_i2c_reg_write(0xf0000014, 0x00000000);

	iris_i2c_reg_write(0xf0000140, 0x00000003);
	iris_i2c_reg_write(0xf0000150, 0x00002003);
	iris_i2c_reg_write(0xf0000160, 0x00000002);

	iris_i2c_reg_write(0xf0180000, 0x0a00c138);
	iris_i2c_reg_write(0xf0180004, 0x00000000);

	iris_i2c_reg_write(0xf0000000, 0x3fc87fff);
	iris_i2c_reg_write(0xf0000004, 0x00007f8e);
}

u32 timeus0, timeus1, timeus2;
void iris_pre_lightup(struct dsi_panel *panel)
{
	ktime_t ktime0, ktime1;
	int ret = 0;

	ktime0 = ktime_get();
	ret = iris_Romcode_state_check(panel, 0x01, 50);
	if (ret < 0)
		iris_reset_failure_recover(panel);

	iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);
	/* init sys/mipirx/mipitx */
	iris_init_cmd_send(panel, DSI_CMD_SET_STATE_LP);

	if (DSI_OP_VIDEO_MODE == panel->panel_mode) {
		msleep(100);
		iris_power_down();
		return;
	}
	/* send work mode and timing info */
	iris_mipirx_mode_set(panel, MCU_CMD, DSI_CMD_SET_STATE_LP);
	iris_timing_info_send(panel, DSI_CMD_SET_STATE_LP);
	iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_LP);

	iris_ctrl_cmd_send(panel, CONFIG_DATAPATH, DSI_CMD_SET_STATE_LP);
	/*init feature*/
	iris_feature_init_cmd_send(panel, DSI_CMD_SET_STATE_HS);

	/* bypass panel on command */
	iris_mipirx_mode_set(panel, BYPASS_CMD, DSI_CMD_SET_STATE_LP);

	/* wait panel reset ready */
	ktime1 = ktime_get();
	timeus0 = (u32) ktime_to_us(ktime1) - (u32)ktime_to_us(ktime0);
	if (timeus0 < 10000)
		usleep_range((10000 - timeus0), (10010 - timeus0));
}

void iris_lightup(struct dsi_panel *panel)
{
	u32 column, page;
	ktime_t ktime0, ktime1, ktime2, ktime3;
	int ret = 0;

	if (DSI_OP_VIDEO_MODE == panel->panel_mode) {
		return;
	}
	ktime0 = ktime_get();
	iris_mipirx_mode_set(panel, PWIL_CMD, DSI_CMD_SET_STATE_HS);

	if (iris_debug_fw_download_disable) {
		iris_info.firmware_info.app_fw_state = false;
		iris_info.firmware_info.app_cnt = 0;
	} else {
		iris_lut_firmware_init(LUT_FIRMWARE_NAME);
		iris_gamma_firmware_init(GAMMA_FIRMWARE_NAME);
		/* wait Romcode ready */
		ret = iris_Romcode_state_check(panel, 0x03, 50);
		if (ret < 0)
			iris_init_failure_recover(panel);
		/* firmware download */
		iris_info.firmware_info.app_fw_state =
				iris_firmware_download(panel, APP_FIRMWARE_NAME, false);
		if (true == iris_info.firmware_info.app_fw_state) {
			iris_ctrl_cmd_send(panel, (REMAP + ITCM_COPY), DSI_CMD_SET_STATE_HS);
			iris_info.firmware_info.app_cnt = 0;
		}
	}

	/* wait panel 0x11 cmd ready */
	ktime1 = ktime_get();
	timeus1 = (u32) ktime_to_us(ktime1) - (u32)ktime_to_us(ktime0);
	if (timeus1 < 120000)
		usleep_range((120000 - timeus1), (120010 - timeus1));


	ktime2 = ktime_get();
	if (DSI_OP_VIDEO_MODE == iris_info.work_mode.rx_mode) {
		iris_mipirx_mode_set(panel, PWIL_VIDEO, DSI_CMD_SET_STATE_HS);
		iris_dtg_setting_cmd_send(panel, DSI_CMD_SET_STATE_HS);
		iris_pwil_mode_set(panel, RFB_MODE, DSI_CMD_SET_STATE_HS);
	} else {
		iris_dtg_setting_cmd_send(panel, DSI_CMD_SET_STATE_HS);
		iris_pwil_mode_set(panel, RFB_MODE, DSI_CMD_SET_STATE_HS);
		/* update rx column/page addr */
		column = iris_info.work_mode.rx_ch ? (iris_info.input_timing.hres * 2 - 1) : (iris_info.input_timing.hres - 1);
		page = iris_info.input_timing.vres - 1;
		iris_mipi_mem_addr_cmd_send(panel, column, page, DSI_CMD_SET_STATE_HS);
	}
	iris_lightup_flag = true;

	iris_mode_switch_state_reset();
	iris_info.power_status.low_power_state = LP_STATE_POWER_UP;

	/* wait panel 0x29 cmd ready */
	ktime3 = ktime_get();
	timeus2 = (u32) ktime_to_us(ktime3) - (u32)ktime_to_us(ktime2);

	pr_err("iris light up: %d %d %d\n", timeus0, timeus1, timeus2);
}

void iris_lightdown(struct dsi_panel *panel)
{
	iris_dport_enable_cmd_send(panel, false);
	iris_mipirx_mode_set(panel, BYPASS_CMD, DSI_CMD_SET_STATE_HS);
}







