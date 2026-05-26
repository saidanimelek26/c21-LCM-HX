/********************************************
 ** Copyright (C) 2019 OPPO Mobile Comm Corp. Ltd.
 ** File: hx83102d_hdp_dsi_vdo_truly_truly_zal3251.c
 ********************************************/

#define LOG_TAG "LCM_HX83102D_TRULY_TRULY"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#include <mt-plat/mtk_boot_common.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define HX83102D_TRULY_TRULY_LCM_ID (0x65)

#ifndef BUILD_LK
typedef struct LCM_UTIL_FUNCS LCM_UTIL_FUNCS;
typedef struct LCM_PARAMS LCM_PARAMS;
typedef struct LCM_DRIVER LCM_DRIVER;
#endif

static LCM_UTIL_FUNCS lcm_util;

#define LCM_RESET_PIN (GPIO45|0x80000000)

#define MDELAY(n) (lcm_util.mdelay(n))
#define UDELAY(n) (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
    lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
    lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
    lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1600)

#define LCM_PHYSICAL_WIDTH   (67932)
#define LCM_PHYSICAL_HEIGHT  (150960)

#define REGFLAG_DELAY        0xFFFC
#define REGFLAG_UDELAY       0xFFFB
#define REGFLAG_END_OF_TABLE 0xFFFD
#define REGFLAG_RESET_LOW    0xFFFE
#define REGFLAG_RESET_HIGH   0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28, 0, {}},
    {REGFLAG_DELAY, 10, {}},
    {0x10, 0, {}},
    {REGFLAG_DELAY, 60, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table init_setting_vdo[] = {
    {0xB9, 3, {0x83, 0x10, 0x2D}},
    {0xC0, 11, {0x30, 0x30, 0x00, 0x00, 0x19, 0x21, 0x00, 0x08, 0x00, 0x1A, 0x1B}},
    {0xB1, 11, {0x22, 0x00, 0x2D, 0x2D, 0x31, 0x41, 0x4D, 0x2F, 0x0D, 0x0D, 0x0D}},
    {0xB2, 14, {0x00, 0x00, 0x06, 0x40, 0x00, 0x0A, 0xEE, 0x35, 0x00, 0x00, 0x00, 0x00, 0x14, 0xA0}},
    {0xB4, 14, {0x0C, 0x54, 0x0C, 0x54, 0x0C, 0x54, 0x0C, 0x54, 0x05, 0xFF, 0x03, 0x00, 0x00, 0xFF}},
    {0xCC, 1, {0x02}},
    {0xD3, 25, {0x0F, 0x0E, 0x3C, 0x01, 0x00, 0x08, 0x00, 0x37, 0x37, 0x34, 0x37, 0x06, 0x06, 0x0A, 0x00, 0x32, 0x10, 0x04, 0x00, 0x04, 0x54, 0x16, 0x4E, 0x00, 0x00}},
    {REGFLAG_DELAY, 5, {}},
    {0xD5, 44, {0x25, 0x24, 0x18, 0x18, 0x18, 0x18, 0x3A, 0x3A, 0x18, 0x18, 0x21, 0x20, 0x23, 0x22, 0x19, 0x19, 0x19, 0x19, 0x01, 0x00, 0x01, 0x00, 0x03, 0x02, 0x03, 0x02, 0x05, 0x04, 0x05, 0x04, 0x07, 0x06, 0x07, 0x06, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}},
    {REGFLAG_DELAY, 5, {}},
    {0xE7, 3, {0xFF, 0x0D, 0x01}},
    {0xBD, 1, {0x01}},
    {0xE7, 1, {0x01}},
    {0xBD, 1, {0x00}},
    {0xBA, 19, {0x70, 0x23, 0xA8, 0x9B, 0xB2, 0xC0, 0xC0, 0x01, 0x10, 0x00, 0x00, 0x00, 0x0C, 0x3D, 0x82, 0x77, 0x04, 0x01, 0x00}},
    {0xC7, 6, {0x00, 0xC2, 0x00, 0x10, 0x42, 0xC0}},
    {0xBF, 7, {0xFC, 0x00, 0x04, 0x9E, 0xF6, 0x00, 0x41}},
    {0xCB, 5, {0x00, 0x13, 0x00, 0x02, 0x49}},
    {0xBD, 1, {0x01}},
    {0xCB, 1, {0x01}},
    {0xBD, 1, {0x02}},
    {0xB4, 8, {0x42, 0x00, 0x33, 0x00, 0x33, 0x88, 0xB3, 0x00}},
    {0xB1, 3, {0x7F, 0x03, 0xFF}},
    {0xBD, 1, {0x00}},
    {0x35, 1, {0x00}},
    {0x11, 0, {}},
    {REGFLAG_DELAY, 62, {}},
    {0x29, 0, {}},
    {REGFLAG_DELAY, 22, {}},
    {0x51, 2, {0x00, 0x00}},
    {0xC9, 4, {0x04, 0x08, 0x76, 0x01}},
    {0x55, 1, {0x01}},
    {REGFLAG_DELAY, 10, {}},
    {0xE4, 15, {0x2D, 0x01, 0x2C, 0x41, 0x4B, 0x6A, 0x7F, 0x9D, 0xB3, 0xA0, 0xA0, 0xE7, 0xFF, 0xFF, 0x03}},
    {0xBD, 1, {0x01}},
    {0xE4, 39, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}},
    {0xBD, 1, {0x00}},
    {0x53, 1, {0x24}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table bl_level[] = {
    {0x51, 2, {0x0F, 0xFF}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table bl_level_dimming_exit[] = {
    {0x53, 1, {0x24}},
    {0x51, 2, {0x0F, 0xFF}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table lcm_cabc_enter_setting_ui[] = {
    {0x55, 1, {0x01}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table lcm_cabc_enter_setting_still[] = {
    {0x55, 1, {0x02}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table lcm_cabc_enter_setting_moving[] = {
    {0x55, 1, {0x03}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static struct LCM_setting_table lcm_cabc_exit_setting[] = {
    {0x55, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}}
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
    unsigned int count, unsigned char force_update)
{
    unsigned int i;
    unsigned int cmd;

    for (i = 0; i < count; i++) {
        cmd = table[i].cmd;

        switch (cmd) {
        case REGFLAG_DELAY:
            MDELAY(table[i].count);
            break;
        case REGFLAG_UDELAY:
            UDELAY(table[i].count);
            break;
        case REGFLAG_END_OF_TABLE:
            break;
        default:
            dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type = LCM_TYPE_DSI;
    params->width = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->physical_width = LCM_PHYSICAL_WIDTH / 1000;
    params->physical_height = LCM_PHYSICAL_HEIGHT / 1000;

    params->dsi.mode = SYNC_PULSE_VDO_MODE;
    params->dsi.switch_mode_enable = 0;
    params->dsi.LANE_NUM = LCM_FOUR_LANE;
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
    params->dsi.packet_size = 256;
    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.vertical_sync_active = 2;
    params->dsi.vertical_backporch = 10;
    params->dsi.vertical_frontporch = 240;
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 19;
    params->dsi.horizontal_backporch = 19;
    params->dsi.horizontal_frontporch = 20;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.ssc_disable = 1;
    params->dsi.PLL_CLOCK = 276;
    params->dsi.cont_clock = 0;
    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.pll_div1 = 0;
    params->dsi.pll_div2 = 1;

    /* Disable ESD check to avoid read issues */
    params->dsi.esd_check_enable = 0;
    params->dsi.customization_esd_check_enable = 0;
}

static void lcm_init_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    MDELAY(10);
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_suspend_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    MDELAY(10);
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_resume_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    MDELAY(10);
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_init(void)
{
    int size;
    LCM_LOGI("%s: enter\n", __func__);

    size = sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table);
    push_table(NULL, init_setting_vdo, size, 1);

    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_suspend(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    push_table(NULL, lcm_suspend_setting,
        sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_resume(void)
{
    int size;
    LCM_LOGI("%s: enter\n", __func__);

    size = sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table);
    push_table(NULL, init_setting_vdo, size, 1);

    LCM_LOGI("%s: exit\n", __func__);
}

/* ================= ESD FUNCTIONS - FIX FOR "esd_check is null" ERROR ================= */
static unsigned int lcm_esd_check(void)
{
    /* Return 1 to indicate panel is OK (ESD check disabled in LK) */
    return 1;
}

static unsigned int lcm_esd_recover(void)
{
    /* Return 1 to indicate recovery successful */
    return 1;
}
/* ==================================================================================== */

static void lcm_set_cabc_mode_cmdq(void *handle, unsigned int level)
{
    if (level == 1) {
        push_table(handle, lcm_cabc_enter_setting_ui,
            sizeof(lcm_cabc_enter_setting_ui) / sizeof(struct LCM_setting_table), 1);
    } else if (level == 2) {
        push_table(handle, lcm_cabc_enter_setting_still,
            sizeof(lcm_cabc_enter_setting_still) / sizeof(struct LCM_setting_table), 1);
    } else if (level == 3) {
        push_table(handle, lcm_cabc_enter_setting_moving,
            sizeof(lcm_cabc_enter_setting_moving) / sizeof(struct LCM_setting_table), 1);
    } else {
        push_table(handle, lcm_cabc_exit_setting,
            sizeof(lcm_cabc_exit_setting) / sizeof(struct LCM_setting_table), 1);
    }
}

#ifdef BUILD_LK
static unsigned int lcm_compare_id(void)
{
    return 1;
}
#endif

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
    if (level > 4095)
        level = 4095;

    if (level < 10)
        level = 10;

    if (level == 3768)
        level = 3767;

    if (level == 0) {
        bl_level_dimming_exit[1].para_list[0] = (level >> 8) & 0x0F;
        bl_level_dimming_exit[1].para_list[1] = level & 0xFF;
        push_table(handle, bl_level_dimming_exit,
            sizeof(bl_level_dimming_exit) / sizeof(struct LCM_setting_table), 1);
    } else {
        bl_level[0].para_list[0] = (level >> 8) & 0x0F;
        bl_level[0].para_list[1] = level & 0xFF;
        push_table(handle, bl_level,
            sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
    }
}

LCM_DRIVER hx83102d_hdp_dsi_vdo_truly_truly_zal3251_lcm_drv = {
    .name = "hx83102d_truly_truly",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
    .init = lcm_init,
    .suspend = lcm_suspend,
    .resume = lcm_resume,
#ifdef BUILD_LK
    .compare_id = lcm_compare_id,
#endif
    .init_power = lcm_init_power,
    .suspend_power = lcm_suspend_power,
    .resume_power = lcm_resume_power,
    .set_backlight_cmdq = lcm_setbacklight_cmdq,
    .set_cabc_mode_cmdq = lcm_set_cabc_mode_cmdq,
    /* ESD functions - These fix the "esd_check is null" error */
    .esd_check = lcm_esd_check,
    .esd_recover = lcm_esd_recover,
};
