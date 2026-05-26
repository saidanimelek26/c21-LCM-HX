/********************************************
 ** Copyright (C) 2019 OPPO Mobile Comm Corp. Ltd.
 ** ODM_HQ_EDIT
 ** File: hx83102d_hdp_dsi_vdo_truly_truly_zal3251.c
 ** Description: Source file for LCD driver
 **          To Control LCD driver
 ** Version :1.1 (LK port)
 ** Date : 2020/08/31
 ** Author: wangcheng@ODM_HQ.Multimedia.LCD
 ** ---------------- Revision History: --------------------------
 ** <version>    <date>          < author >              <desc>
 **  1.0           2020/08/31   wangcheng@ODM_HQ   Source file for LCD driver
 **  1.1           ported to LK: merged NT5038 I2C, GPIO reset, CABC state restore
 ********************************************/

#define LOG_TAG "LCM_HX83102D_TRULY_TRULY"

/* ── build-environment guards ─────────────────────────────────────────────── */
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
#include <platform/boot_mode.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif

/* ── logging macros ───────────────────────────────────────────────────────── */
#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

/* ── panel ID ─────────────────────────────────────────────────────────────── */
#define HX83102D_TRULY_TRULY_LCM_ID (0x65)

/* ── typedef aliases (kernel only) ───────────────────────────────────────── */
#ifndef BUILD_LK
typedef struct LCM_UTIL_FUNCS LCM_UTIL_FUNCS;
typedef struct LCM_PARAMS     LCM_PARAMS;
typedef struct LCM_DRIVER     LCM_DRIVER;
#endif

/* ── NT5038 bias IC – I²C settings ───────────────────────────────────────── */
struct NT5038_SETTING_TABLE {
    unsigned char cmd;
    unsigned char data;
};

/*
 * reg 0x00 = VPOS = 0x12 → +5.8 V
 * reg 0x01 = VNEG = 0x12 → -5.8 V
 * reg 0x03 = control = 0x73
 */
static struct NT5038_SETTING_TABLE nt5038_cmd_data[3] = {
    { 0x00, 0x12 },
    { 0x01, 0x12 },
    { 0x03, 0x73 },
};

/* ── NT5038 I²C implementation ────────────────────────────────────────────── */
#ifdef BUILD_LK
/*
 * In LK we talk to the NT5038 directly via mt_i2c.
 * Channel 3, 7-bit address = 0x3E  (write byte = 0x7C >> 1).
 */
#define NT5038_SLAVE_ADDR_WRITE  0x7C
#define I2C_I2C_LCD_BIAS_CHANNEL 3

static struct mt_i2c_t NT5038_i2c;

static int nt5038_i2c_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code;
    kal_uint8  write_data[2];

    write_data[0] = addr;
    write_data[1] = value;

    NT5038_i2c.id    = I2C_I2C_LCD_BIAS_CHANNEL;
    NT5038_i2c.addr  = (NT5038_SLAVE_ADDR_WRITE >> 1);
    NT5038_i2c.mode  = ST_MODE;
    NT5038_i2c.speed = 100;

    ret_code = i2c_write(&NT5038_i2c, write_data, 2);
    LCM_LOGI("%s: addr=0x%02x val=0x%02x ret=%d\n",
             __func__, addr, value, ret_code);
    return ret_code;
}

#else   /* kernel */

#define DCDC_I2C_BUSNUM  3
#define DCDC_I2C_ID_NAME "nt5038"
#define DCDC_I2C_ADDR    0x3E

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <soc/oppo/device_info.h>

extern struct i2c_client *nt5038_i2c_client;
extern void lcd_queue_load_tp_fw(void);
extern int  tp_gesture_enable_flag(void);

static int nt5038_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    nt5038_i2c_client = client;
    return 0;
}

static int nt5038_remove(struct i2c_client *client)
{
    nt5038_i2c_client = NULL;
    i2c_unregister_device(client);
    return 0;
}

static int nt5038_i2c_write_byte(unsigned char addr, unsigned char value)
{
    int ret = 0;
    struct i2c_client *client = nt5038_i2c_client;
    char write_data[2] = {0};

    if (client == NULL) {
        LCM_LOGI("ERROR!! nt5038_i2c_client is null\n");
        return 0;
    }
    write_data[0] = addr;
    write_data[1] = value;
    ret = i2c_master_send(client, write_data, 2);
    if (ret < 0)
        LCM_LOGI("nt5038 write data fail !!\n");
    return ret;
}

static const struct of_device_id lcm_of_match[] = {
    { .compatible = "mediatek,I2C_LCD_BIAS" },
    { },
};

static const struct i2c_device_id nt5038_id[] = {
    { DCDC_I2C_ID_NAME, 0 },
    { },
};

static struct i2c_driver nt5038_iic_driver = {
    .id_table = nt5038_id,
    .probe    = nt5038_probe,
    .remove   = nt5038_remove,
    .driver   = {
        .owner          = THIS_MODULE,
        .name           = "nt5038",
        .of_match_table = lcm_of_match,
    },
};

static int __init nt5038_iic_init(void)
{
    i2c_add_driver(&nt5038_iic_driver);
    return 0;
}
static void __exit nt5038_iic_exit(void)
{
    i2c_del_driver(&nt5038_iic_driver);
}
module_init(nt5038_iic_init);
module_exit(nt5038_iic_exit);
MODULE_AUTHOR("cheng.wang <wangcheng7@huaqin.com>");
MODULE_DESCRIPTION("MTK LCD BIAS I2C Driver");
MODULE_LICENSE("GPL");

#endif  /* BUILD_LK */

/* ── GPIO pin definitions ─────────────────────────────────────────────────── */
#define LCM_RESET_PIN           (GPIO45  | 0x80000000)
#ifndef GPIO_LCD_BIAS_ENP_PIN
#define GPIO_LCD_BIAS_ENP_PIN   (GPIO166 | 0x80000000)
#endif
#ifndef GPIO_LCD_BIAS_ENN_PIN
#define GPIO_LCD_BIAS_ENN_PIN   (GPIO165 | 0x80000000)
#endif

/* ── utility wrappers ─────────────────────────────────────────────────────── */
static LCM_UTIL_FUNCS lcm_util;

#define MDELAY(n)  (lcm_util.mdelay(n))
#define UDELAY(n)  (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
    lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                   lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, n)       lcm_util.dsi_write_regs(addr, pdata, n)
#define read_reg(cmd)                    lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buf, bufsz)     lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buf, bufsz)

/* ── frame geometry ───────────────────────────────────────────────────────── */
#define FRAME_WIDTH          (720)
#define FRAME_HEIGHT         (1600)
#define LCM_PHYSICAL_WIDTH   (67932)
#define LCM_PHYSICAL_HEIGHT  (150960)

/* ── register-table sentinel codes ───────────────────────────────────────── */
#define REGFLAG_DELAY        0xFFFC
#define REGFLAG_UDELAY       0xFFFB
#define REGFLAG_END_OF_TABLE 0xFFFD
#define REGFLAG_RESET_LOW    0xFFFE
#define REGFLAG_RESET_HIGH   0xFFFF

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ── CABC state (persisted across resume) ─────────────────────────────────── */
static int cabc_lastlevel = 1;

/* ── register table type ──────────────────────────────────────────────────── */
struct LCM_setting_table {
    unsigned int  cmd;
    unsigned char count;
    unsigned char para_list[64];
};

/* ── brightness remap table ───────────────────────────────────────────────── */
static int blmap_table[] = {
    48, 17,   29, 23,   25, 26,   30, 23,   35, 24,
    34, 28,   39, 26,   40, 25,   42, 21,   41, 23,
    44, 19,   44, 19,   50,  3,   51,  7,   52,  9,
    58, 34,   58, 33,   64, 65,   61, 48,   71,106,
    68, 87,   70,100,   77,146,   78,146,   87,211,
    88,219,   87,210,   88,213,  125,542,   96,272,
    91,221,  145,757,  122,522,  183,1166, 122,502,
   155,872,  161,943,  151,823,  190,1300, 164,975,
   264,2255, 152,785,  277,2467, 187,1228, 239,1961,
   203,1440, 242,2015, 271,2446, 284,2645, 310,3045,
   303,2936, 338,3510, 329,3357, 374,4123, 371,4074,
   387,4357, 352,3720, 493,6294, 445,5407, 477,6015,
   490,6255, 516,6760, 584,8110, 326,2906,
};

/* ─────────────────────────────────────────────────────────────────────────── */
/*  COMMAND TABLES                                                             */
/* ─────────────────────────────────────────────────────────────────────────── */

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28, 0, {}},
    {REGFLAG_DELAY, 10, {}},
    {0x10, 0, {}},
    {REGFLAG_DELAY, 60, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table init_setting_vdo[] = {
    {0xB9, 3,  {0x83, 0x10, 0x2D}},
    {0xC0, 11, {0x30, 0x30, 0x00, 0x00, 0x19, 0x21, 0x00, 0x08, 0x00, 0x1A, 0x1B}},
    {0xB1, 11, {0x22, 0x00, 0x2D, 0x2D, 0x31, 0x41, 0x4D, 0x2F, 0x0D, 0x0D, 0x0D}},
    {0xB2, 14, {0x00, 0x00, 0x06, 0x40, 0x00, 0x0A, 0xEE, 0x35,
                0x00, 0x00, 0x00, 0x00, 0x14, 0xA0}},
    {0xB4, 14, {0x0C, 0x54, 0x0C, 0x54, 0x0C, 0x54, 0x0C, 0x54,
                0x05, 0xFF, 0x03, 0x00, 0x00, 0xFF}},
    {0xCC,  1, {0x02}},
    {0xD3, 25, {0x0F, 0x0E, 0x3C, 0x01, 0x00, 0x08, 0x00, 0x37,
                0x37, 0x34, 0x37, 0x06, 0x06, 0x0A, 0x00, 0x32,
                0x10, 0x04, 0x00, 0x04, 0x54, 0x16, 0x4E, 0x00, 0x00}},
    {REGFLAG_DELAY, 5, {}},
    {0xD5, 44, {0x25, 0x24, 0x18, 0x18, 0x18, 0x18, 0x3A, 0x3A,
                0x18, 0x18, 0x21, 0x20, 0x23, 0x22, 0x19, 0x19,
                0x19, 0x19, 0x01, 0x00, 0x01, 0x00, 0x03, 0x02,
                0x03, 0x02, 0x05, 0x04, 0x05, 0x04, 0x07, 0x06,
                0x07, 0x06, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
                0x18, 0x18, 0x18, 0x18}},
    {REGFLAG_DELAY, 5, {}},
    {0xE7,  3, {0xFF, 0x0D, 0x01}},
    {0xBD,  1, {0x01}},
    {0xE7,  1, {0x01}},
    {0xBD,  1, {0x00}},
    {0xBA, 19, {0x70, 0x23, 0xA8, 0x9B, 0xB2, 0xC0, 0xC0, 0x01,
                0x10, 0x00, 0x00, 0x00, 0x0C, 0x3D, 0x82, 0x77,
                0x04, 0x01, 0x00}},
    {0xC7,  6, {0x00, 0xC2, 0x00, 0x10, 0x42, 0xC0}},
    {0xBF,  7, {0xFC, 0x00, 0x04, 0x9E, 0xF6, 0x00, 0x41}},
    {0xCB,  5, {0x00, 0x13, 0x00, 0x02, 0x49}},
    {0xBD,  1, {0x01}},
    {0xCB,  1, {0x01}},
    {0xBD,  1, {0x02}},
    {0xB4,  8, {0x42, 0x00, 0x33, 0x00, 0x33, 0x88, 0xB3, 0x00}},
    {0xB1,  3, {0x7F, 0x03, 0xFF}},
    {0xBD,  1, {0x00}},
    {0x35,  1, {0x00}},           /* TE on */
    {0x11,  0, {}},
    {REGFLAG_DELAY, 62, {}},
    {0x29,  0, {}},
    {REGFLAG_DELAY, 22, {}},
    {0x51,  2, {0x00, 0x00}},
    {0xC9,  4, {0x04, 0x08, 0x76, 0x01}}, /* PWM freq 30 kHz */
    {0x55,  1, {0x01}},
    {REGFLAG_DELAY, 10, {}},
    {0xE4, 15, {0x2D, 0x01, 0x2C, 0x41, 0x4B, 0x6A, 0x7F, 0x9D,
                0xB3, 0xA0, 0xA0, 0xE7, 0xFF, 0xFF, 0x03}},
    {0xBD,  1, {0x01}},
    {0xE4, 39, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55,
                0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}},
    {0xBD,  1, {0x00}},
    {0x53,  1, {0x24}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table bl_level[] = {
    {0x51, 2, {0x0F, 0xFF}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table bl_level_dimming_exit[] = {
    {0x53, 1, {0x24}},
    {0x51, 2, {0x0F, 0xFF}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table lcm_cabc_enter_setting_ui[] = {
    {0x55, 1, {0x01}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table lcm_cabc_enter_setting_still[] = {
    {0x55, 1, {0x02}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table lcm_cabc_enter_setting_moving[] = {
    {0x55, 1, {0x03}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table lcm_cabc_exit_setting[] = {
    {0x55, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    {REGFLAG_END_OF_TABLE, 0, {}},
};

/* ─────────────────────────────────────────────────────────────────────────── */
/*  HELPERS                                                                    */
/* ─────────────────────────────────────────────────────────────────────────── */

static void push_table(void *cmdq, struct LCM_setting_table *table,
                       unsigned int count, unsigned char force_update)
{
    unsigned int i, cmd;

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
            return;
        default:
            dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
                             table[i].para_list, force_update);
            break;
        }
    }
}

/*
 * nt5038_power_on – shared helper used by init_power and resume_power.
 * In LK we use mt_i2c; in kernel we use the registered i2c_client.
 */
static void nt5038_power_on(void)
{
#ifdef BUILD_LK
    /* Enable VPOS/VNEG rails via GPIO before I²C */
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN,  GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN,  GPIO_OUT_ONE);
    MDELAY(5);
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN,  GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN,  GPIO_OUT_ONE);
    MDELAY(5);
#else
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP1);
    MDELAY(5);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN1);
    MDELAY(5);
#endif

    /* NT5038 voltage configuration */
    nt5038_i2c_write_byte(nt5038_cmd_data[0].cmd, nt5038_cmd_data[0].data);
    MDELAY(1);
    nt5038_i2c_write_byte(nt5038_cmd_data[1].cmd, nt5038_cmd_data[1].data);
    MDELAY(1);
    nt5038_i2c_write_byte(nt5038_cmd_data[2].cmd, nt5038_cmd_data[2].data);
    MDELAY(1);
}

/*
 * lcm_reset_pulse – drive the RESET line: HIGH → LOW → HIGH.
 * Used identically in lcm_init() and lcm_resume().
 */
static void lcm_reset_pulse(void)
{
#ifdef BUILD_LK
    mt_set_gpio_mode(LCM_RESET_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(LCM_RESET_PIN,  GPIO_DIR_OUT);

    mt_set_gpio_out(LCM_RESET_PIN, GPIO_OUT_ONE);
    MDELAY(5);
    mt_set_gpio_out(LCM_RESET_PIN, GPIO_OUT_ZERO);
    MDELAY(2);
    mt_set_gpio_out(LCM_RESET_PIN, GPIO_OUT_ONE);
    MDELAY(50);
#else
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
    MDELAY(5);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
    MDELAY(2);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
    MDELAY(50);
#endif
}

/* ─────────────────────────────────────────────────────────────────────────── */
/*  LCM DRIVER CALLBACKS                                                       */
/* ─────────────────────────────────────────────────────────────────────────── */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type            = LCM_TYPE_DSI;
    params->width           = FRAME_WIDTH;
    params->height          = FRAME_HEIGHT;
    params->physical_width  = LCM_PHYSICAL_WIDTH  / 1000;
    params->physical_height = LCM_PHYSICAL_HEIGHT / 1000;
#ifndef BUILD_LK
    params->physical_width_um  = LCM_PHYSICAL_WIDTH;
    params->physical_height_um = LCM_PHYSICAL_HEIGHT;
#endif

    params->dsi.mode               = SYNC_PULSE_VDO_MODE;
    params->dsi.switch_mode_enable = 0;
    params->dsi.LANE_NUM           = LCM_FOUR_LANE;

    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.packet_size = 256;
    params->dsi.PS          = LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active  = 2;
    params->dsi.vertical_backporch    = 10;
    params->dsi.vertical_frontporch   = 240;
    params->dsi.vertical_active_line  = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active  = 19;
    params->dsi.horizontal_backporch    = 19;
    params->dsi.horizontal_frontporch   = 20;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.ssc_disable   = 1;
    params->dsi.PLL_CLOCK     = 276;
    params->dsi.cont_clock    = 0;
    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.pll_div1 = 0;
    params->dsi.pll_div2 = 1;

#ifndef BUILD_LK
    /* Dynamic clock parameters (kernel only) */
    params->dsi.horizontal_backporch_dyn = 0x86;
    params->dsi.data_rate_dyn            = 564;

    /* ESD check: disabled in factory/meta modes */
    {
        int boot_mode = 0;
        if (get_boot_mode() == META_BOOT)        boot_mode++;
        if (get_boot_mode() == ADVMETA_BOOT)     boot_mode++;
        if (get_boot_mode() == ATE_FACTORY_BOOT) boot_mode++;
        if (get_boot_mode() == FACTORY_BOOT)     boot_mode++;

        if (boot_mode == 0) {
            params->dsi.esd_check_enable              = 1;
            params->dsi.customization_esd_check_enable = 0;
            params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
            params->dsi.lcm_esd_check_table[0].count        = 1;
            params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9D;
        }
    }

    /* Brightness mapping */
    params->blmap        = blmap_table;
    params->blmap_size   = sizeof(blmap_table) / sizeof(blmap_table[0]);
    params->brightness_max = 4095;
    params->brightness_min = 10;

    register_device_proc("lcd", "hx83102d", "TRULY_TRULY_SEVEN");
#else
    /* In LK, disable ESD to avoid any null-pointer crash */
    params->dsi.esd_check_enable              = 0;
    params->dsi.customization_esd_check_enable = 0;
#endif
}

/* ── power callbacks ──────────────────────────────────────────────────────── */

static void lcm_init_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    nt5038_power_on();
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_suspend_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);

#ifdef BUILD_LK
    /* In LK there is no gesture mode; always power down */
    mt_set_gpio_out(LCM_RESET_PIN,        GPIO_OUT_ZERO);
    MDELAY(2);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
    MDELAY(5);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
    MDELAY(5);
#else
    if (tp_gesture_enable_flag() == 0) {
        disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
        MDELAY(2);
        disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN0);
        MDELAY(5);
        disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP0);
        MDELAY(5);
    }
#endif

    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_resume_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    nt5038_power_on();
    LCM_LOGI("%s: exit\n", __func__);
}

#ifndef BUILD_LK
/* Kernel-only: clean shutdown (e.g. power-off) */
static void lcm_shutdown_power(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
    MDELAY(2);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENN0);
    MDELAY(2);
    disp_dts_gpio_select_state(DTS_GPIO_STATE_LCD_BIAS_ENP0);
    LCM_LOGI("%s: exit\n", __func__);
}
#endif

/* ── init / suspend / resume ──────────────────────────────────────────────── */

static void lcm_init(void)
{
    int size;
    LCM_LOGI("%s: enter\n", __func__);

    lcm_reset_pulse();

#ifndef BUILD_LK
    lcd_queue_load_tp_fw();
#endif

    size = sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table);
    push_table(NULL, init_setting_vdo, size, 1);

    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_suspend(void)
{
    LCM_LOGI("%s: enter\n", __func__);
    push_table(NULL, lcm_suspend_setting,
               sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
    LCM_LOGI("%s: exit\n", __func__);
}

static void lcm_resume(void)
{
    int size;
    LCM_LOGI("%s: enter\n", __func__);

    lcm_reset_pulse();

#ifndef BUILD_LK
    lcd_queue_load_tp_fw();
#endif

    size = sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table);
    push_table(NULL, init_setting_vdo, size, 1);

    /* Restore CABC mode that was active before suspend */
    switch (cabc_lastlevel) {
    case 1:
        push_table(NULL, lcm_cabc_enter_setting_ui,
                   sizeof(lcm_cabc_enter_setting_ui) /
                   sizeof(struct LCM_setting_table), 1);
        break;
    case 2:
        push_table(NULL, lcm_cabc_enter_setting_still,
                   sizeof(lcm_cabc_enter_setting_still) /
                   sizeof(struct LCM_setting_table), 1);
        break;
    case 3:
        push_table(NULL, lcm_cabc_enter_setting_moving,
                   sizeof(lcm_cabc_enter_setting_moving) /
                   sizeof(struct LCM_setting_table), 1);
        break;
    default:
        break;
    }

    LCM_LOGI("%s: exit\n", __func__);
}

/* ── compare_id (LK only) ─────────────────────────────────────────────────── */
#ifdef BUILD_LK
static unsigned int lcm_compare_id(void)
{
    /*
     * A real implementation would read register 0xDA/0xDB/0xDC and
     * compare against HX83102D_TRULY_TRULY_LCM_ID (0x65).
     * For now return 1 so the panel is always accepted in LK.
     */
    LCM_LOGI("%s: always match in LK\n", __func__);
    return 1;
}
#endif

/* ── backlight ────────────────────────────────────────────────────────────── */

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
    LCM_LOGI("%s: level=%d\n", __func__, level);

    /* Hardware quirk: skip level 3768 */
    if (level == 3768)
        level = 3767;

    if (level == 0) {
        bl_level_dimming_exit[1].para_list[0] = (level >> 8) & 0x0F;
        bl_level_dimming_exit[1].para_list[1] =  level       & 0xFF;
        push_table(handle, bl_level_dimming_exit,
                   sizeof(bl_level_dimming_exit) /
                   sizeof(struct LCM_setting_table), 1);
    } else {
        if (level > 4095)
            level = 4095;
        else if (level < 10)
            level = 10;

        bl_level[0].para_list[0] = (level >> 8) & 0x0F;
        bl_level[0].para_list[1] =  level       & 0xFF;
        push_table(handle, bl_level,
                   sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
    }
}

/* ── CABC ─────────────────────────────────────────────────────────────────── */

static void lcm_set_cabc_mode_cmdq(void *handle, unsigned int level)
{
    LCM_LOGI("%s: cabc_mode=%d\n", __func__, level);

    if (level == 1) {
        push_table(handle, lcm_cabc_enter_setting_ui,
                   sizeof(lcm_cabc_enter_setting_ui) /
                   sizeof(struct LCM_setting_table), 1);
    } else if (level == 2) {
        push_table(handle, lcm_cabc_enter_setting_still,
                   sizeof(lcm_cabc_enter_setting_still) /
                   sizeof(struct LCM_setting_table), 1);
    } else if (level == 3) {
        push_table(handle, lcm_cabc_enter_setting_moving,
                   sizeof(lcm_cabc_enter_setting_moving) /
                   sizeof(struct LCM_setting_table), 1);
    } else {
        push_table(handle, lcm_cabc_exit_setting,
                   sizeof(lcm_cabc_exit_setting) /
                   sizeof(struct LCM_setting_table), 1);
    }

    /* Remember last non-zero level so resume can restore it */
    if (level > 0)
        cabc_lastlevel = level;
}

/* ─────────────────────────────────────────────────────────────────────────── */
/*  DRIVER REGISTRATION                                                        */
/* ─────────────────────────────────────────────────────────────────────────── */

LCM_DRIVER hx83102d_hdp_dsi_vdo_truly_truly_zal3251_lcm_drv = {
    .name             = "hx83102d_truly_truly",
    .set_util_funcs   = lcm_set_util_funcs,
    .get_params       = lcm_get_params,
    .init             = lcm_init,
    .suspend          = lcm_suspend,
    .resume           = lcm_resume,
#ifdef BUILD_LK
    .compare_id       = lcm_compare_id,
#endif
    .init_power       = lcm_init_power,
    .suspend_power    = lcm_suspend_power,
    .resume_power     = lcm_resume_power,
#ifndef BUILD_LK
    .shutdown_power   = lcm_shutdown_power,
#endif
    .set_backlight_cmdq = lcm_setbacklight_cmdq,
    .set_cabc_mode_cmdq = lcm_set_cabc_mode_cmdq,
};
