/*
 * max98506.c -- ALSA SoC MAX98506 driver
 *
 * Copyright 2013-2015 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/max98506.h>
#include "max98506.h"
#include <linux/regulator/consumer.h>

#define DEBUG_MAX98506
#ifdef DEBUG_MAX98506
#define msg_maxim(format, args...) \
	pr_err("[MAX98506_DEBUG] %s: " format "\n", __func__, ## args)
#else
#define msg_maxim(format, args...)
#endif /* DEBUG_MAX98506 */

#ifdef USE_DSM_LOG
struct class *g_class;
#endif /* USE_DSM_LOG */

static struct reg_default max98506_reg[] = {
	{ 0x02, 0x00 }, /* Live Status0 */
	{ 0x03, 0x00 }, /* Live Status1 */
	{ 0x04, 0x00 }, /* Live Status2 */
	{ 0x05, 0x00 }, /* State0 */
	{ 0x06, 0x00 }, /* State1 */
	{ 0x07, 0x00 }, /* State2 */
	{ 0x08, 0x00 }, /* Flag0 */
	{ 0x09, 0x00 }, /* Flag1 */
	{ 0x0A, 0x00 }, /* Flag2 */
	{ 0x0B, 0x00 }, /* IRQ Enable0 */
	{ 0x0C, 0x00 }, /* IRQ Enable1 */
	{ 0x0D, 0x00 }, /* IRQ Enable2 */
	{ 0x0E, 0x00 }, /* IRQ Clear0 */
	{ 0x0F, 0x00 }, /* IRQ Clear1 */
	{ 0x10, 0x00 }, /* IRQ Clear2 */
	{ 0x1A, 0x06 }, /* DAI Clock Mode 1 */
	{ 0x1B, 0xC0 }, /* DAI Clock Mode 2 */
	{ 0x1F, 0x00 }, /* DAI Clock Divider Numerator LSBs */
	{ 0x20, 0x50 }, /* Format */
	{ 0x21, 0x00 }, /* TDM Slot Select */
	{ 0x22, 0x00 }, /* DOUT Configuration VMON */
	{ 0x23, 0x00 }, /* DOUT Configuration IMON */
	{ 0x24, 0x00 }, /* DAI Interleaved Configuration */
	{ 0x26, 0x00 }, /* DOUT Configuration FLAG */
	{ 0x27, 0xFF }, /* DOUT HiZ Configuration 1 */
	{ 0x28, 0xFF }, /* DOUT HiZ Configuration 2 */
	{ 0x29, 0xFF }, /* DOUT HiZ Configuration 3 */
	{ 0x2A, 0xFF }, /* DOUT HiZ Configuration 4 */
	{ 0x2B, 0x02 }, /* DOUT Drive Strength */
	{ 0x2C, 0x90 }, /* Filters */
	{ 0x2D, 0x00 }, /* Gain */
	{ 0x2E, 0x02 }, /* Gain Ramping */
	{ 0x2F, 0x00 }, /* Speaker Amplifier */
	{ 0x30, 0x0A }, /* Threshold */
	{ 0x31, 0x00 }, /* ALC Attack */
	{ 0x32, 0x80 }, /* ALC Atten and Release */
	{ 0x33, 0x00 }, /* ALC Infinite Hold Release */
	{ 0x34, 0x92 }, /* ALC Configuration */
	{ 0x35, 0x01 }, /* Boost Converter */
	{ 0x36, 0x00 }, /* Block Enable */
	{ 0x37, 0x00 }, /* Configuration */
	{ 0x38, 0x00 }, /* Global Enable */
	{ 0x3A, 0x00 }, /* Boost Limiter */
	{ 0xFF, 0x50 }, /* Revision ID */
};

static bool max98506_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98506_R002_LIVE_STATUS0:
	case MAX98506_R003_LIVE_STATUS1:
	case MAX98506_R004_LIVE_STATUS2:
	case MAX98506_R005_STATE0:
	case MAX98506_R006_STATE1:
	case MAX98506_R007_STATE2:
	case MAX98506_R008_FLAG0:
	case MAX98506_R009_FLAG1:
	case MAX98506_R00A_FLAG2:
	case MAX98506_R0FF_VERSION:
		return true;
	default:
		return false;
	}
}

static bool max98506_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0x00:
	case 0x01:
	case MAX98506_R00E_IRQ_CLEAR0:
	case MAX98506_R00F_IRQ_CLEAR1:
	case MAX98506_R010_IRQ_CLEAR2:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x25:
	case MAX98506_R033_ALC_HOLD_RLS:
	case 0x39:
	case 0x3B ... 0xFE:
		return false;
	default:
		return true;
	}
};

#ifdef CONFIG_SND_SOC_MAXIM_DSM
#ifdef USE_DSM_LOG
static int max98506_get_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = maxdsm_get_dump_status();
	return 0;
}

static int max98506_set_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);

	int val;

	regmap_read(max98506->regmap,
		MAX98506_R038_GLOBAL_ENABLE, &val);
	msg_maxim("val: %d", val);

	if (val != 0)
		maxdsm_update_param();

	return 0;
}

static ssize_t max98506_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return maxdsm_log_prepare(buf);
}

static DEVICE_ATTR(dsm_log, S_IRUGO, max98506_log_show, NULL);
#endif /* USE_DSM_LOG */

#ifdef USE_DSM_UPDATE_CAL
static int max98506_get_dsm_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = maxdsm_cal_avail();
	return 0;
}

static int max98506_set_dsm_param(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	maxdsm_update_caldata(ucontrol->value.integer.value[0]);
	return 0;
}

static ssize_t max98506_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return maxdsm_cal_prepare(buf);
}
static DEVICE_ATTR(dsm_cal, S_IRUGO, max98506_cal_show, NULL);
#endif /* USE_DSM_UPDATE_CAL */

#if defined(USE_DSM_LOG) || defined(USE_DSM_UPDATE_CAL)
#define DEFAULT_LOG_CLASS_NAME "dsm"
static const char *class_name_log = DEFAULT_LOG_CLASS_NAME;

static struct attribute *max98506_attributes[] = {
#ifdef USE_DSM_LOG
	&dev_attr_dsm_log.attr,
#endif /* USE_DSM_LOG */
#ifdef USE_DSM_UPDATE_CAL
	&dev_attr_dsm_cal.attr,
#endif /* USE_DSM_UPDATE_CAL */
	NULL
};

static struct attribute_group max98506_attribute_group = {
	.attrs = max98506_attributes
};
#endif /* USE_DSM_LOG || USE_DSM_UPDATE_CAL */
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

static const unsigned int max98506_spk_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	1, 31, TLV_DB_SCALE_ITEM(-600, 100, 0),
};

static int max98506_spk_gain_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_pdata *pdata = max98506->pdata;

	ucontrol->value.integer.value[0] = pdata->spk_gain;

	return 0;
}

static int max98506_spk_gain_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_pdata *pdata = max98506->pdata;
	unsigned int sel = ucontrol->value.integer.value[0];

	regmap_update_bits(max98506->regmap, MAX98506_R02D_GAIN,
			MAX98506_SPK_GAIN_MASK, sel << MAX98506_SPK_GAIN_SHIFT);

	pdata->spk_gain = sel;

	return 0;
}

static int max98506_reg_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	int data;

	regmap_read(max98506->regmap, reg, &data);

	ucontrol->value.integer.value[0] =
		(data & mask) >> shift;

	return 0;
}

static int max98506_reg_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	regmap_update_bits(max98506->regmap, reg, mask, sel << shift);

	return 0;
}

static int max98506_spk_ramp_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_get(kcontrol, ucontrol, MAX98506_R02E_GAIN_RAMPING,
			MAX98506_SPK_RMP_EN_MASK, MAX98506_SPK_RMP_EN_SHIFT);
}

static int max98506_spk_ramp_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_put(kcontrol, ucontrol, MAX98506_R02E_GAIN_RAMPING,
			MAX98506_SPK_RMP_EN_MASK, MAX98506_SPK_RMP_EN_SHIFT);
}

static int max98506_spk_zcd_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_get(kcontrol, ucontrol, MAX98506_R02E_GAIN_RAMPING,
			MAX98506_SPK_ZCD_EN_MASK, MAX98506_SPK_ZCD_EN_SHIFT);
}

static int max98506_spk_zcd_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_put(kcontrol, ucontrol, MAX98506_R02E_GAIN_RAMPING,
			MAX98506_SPK_ZCD_EN_MASK, MAX98506_SPK_ZCD_EN_SHIFT);
}

static int max98506_alc_en_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_get(kcontrol, ucontrol, MAX98506_R030_THRESHOLD,
			MAX98506_ALC_EN_MASK, MAX98506_ALC_EN_SHIFT);
}

static int max98506_alc_en_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_put(kcontrol, ucontrol, MAX98506_R030_THRESHOLD,
			MAX98506_ALC_EN_MASK, MAX98506_ALC_EN_SHIFT);
}

static int max98506_alc_threshold_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_get(kcontrol, ucontrol, MAX98506_R030_THRESHOLD,
			MAX98506_ALC_TH_MASK, MAX98506_ALC_TH_SHIFT);
}

static int max98506_alc_threshold_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98506_reg_put(kcontrol, ucontrol, MAX98506_R030_THRESHOLD,
			MAX98506_ALC_TH_MASK, MAX98506_ALC_TH_SHIFT);
}

static const char * const max98506_boost_voltage_text[] = {"8.5V", "8.25V",
		"8.0V", "7.75V", "7.5V", "7.25V", "7.0V", "6.75V",
		"6.5V", "6.5V", "6.5V", "6.5V", "6.5V", "6.5V", "6.5V", "6.5V"};

static const struct soc_enum max98506_boost_voltage_enum =
	SOC_ENUM_SINGLE(MAX98506_R037_CONFIGURATION,
			MAX98506_BST_VOUT_SHIFT, 16,
			max98506_boost_voltage_text);

static const char * const spk_state_text[] = {"Disable", "Enable"};

static const struct soc_enum spk_state_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_state_text), spk_state_text),
};

static void max98506_spk_enable(struct max98506_priv *max98506,
		int enable)
{
	struct max98506_pdata *pdata = max98506->pdata;

	if (enable) {
		regmap_update_bits(max98506->regmap,
				MAX98506_R02D_GAIN,
				MAX98506_SPK_GAIN_MASK,
				pdata->spk_gain);

		regmap_update_bits(max98506->regmap,
				MAX98506_R036_BLOCK_ENABLE,
				MAX98506_BST_EN_MASK |
				MAX98506_SPK_EN_MASK,
				MAX98506_BST_EN_MASK |
				MAX98506_SPK_EN_MASK);
		regmap_write(max98506->regmap,
				MAX98506_R038_GLOBAL_ENABLE,
				MAX98506_EN_MASK);
	} else {
		regmap_update_bits(max98506->regmap,
				MAX98506_R02D_GAIN,
				MAX98506_SPK_GAIN_MASK,
				0x00);
		usleep_range(4999, 5000);
		regmap_update_bits(max98506->regmap,
				MAX98506_R038_GLOBAL_ENABLE,
				MAX98506_EN_MASK,
				0x00);
	}
}

static int max98506_spk_out_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	unsigned int val;

	regmap_read(max98506->regmap,
			MAX98506_R038_GLOBAL_ENABLE, &val);
	ucontrol->value.integer.value[0] = !!(val & MAX98506_EN_MASK);

	msg_maxim("The status of speaker is '%s'",
			spk_state_text[ucontrol->value.integer.value[0]]);

	return 0;
}

static int max98506_spk_out_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	int enable = !!ucontrol->value.integer.value[0];

	max98506_spk_enable(max98506, enable);

	msg_maxim("Speaker was set by '%s'", spk_state_text[enable]);

	return 0;
}

static int max98506_adc_en_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	int data;

	regmap_read(max98506->regmap, MAX98506_R036_BLOCK_ENABLE, &data);

	if (data & MAX98506_ADC_VIMON_EN_MASK)
		ucontrol->value.integer.value[0] = 1;
	else
		ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int max98506_adc_en_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;
	unsigned int sel = ucontrol->value.integer.value[0];

	if (sel)
		regmap_update_bits(max98506->regmap, MAX98506_R036_BLOCK_ENABLE,
		MAX98506_ADC_VIMON_EN_MASK,
		MAX98506_ADC_VIMON_EN_MASK);
	else
		regmap_update_bits(max98506->regmap, MAX98506_R036_BLOCK_ENABLE,
		MAX98506_ADC_VIMON_EN_MASK, 0);

	vstep->adc_status = !!sel;

	return 0;
}

static int max98506_adc_thres_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;

	ucontrol->value.integer.value[0] = vstep->adc_thres;

	return 0;
}

static int max98506_adc_thres_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;
	int ret = 0;

	if (ucontrol->value.integer.value[0] >= MAX98506_VSTEP_0 &&
			ucontrol->value.integer.value[0] <= MAX98506_VSTEP_15)
		vstep->adc_thres = ucontrol->value.integer.value[0];
	else
		ret = -EINVAL;

	return ret;
}

static int max98506_volume_step_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;

	ucontrol->value.integer.value[0] = vstep->vol_step;

	return 0;
}

static int max98506_volume_step_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;

	unsigned int sel = ucontrol->value.integer.value[0];
	unsigned int mask = 0;

	/*
	 * ADC status will be updated according to the volume.
	 * Under step 7 : Disable
	 * Over step 7  : Enable
	 */
	if (sel <= vstep->adc_thres
			&& vstep->adc_status) {
		regmap_update_bits(max98506->regmap,
				MAX98506_R036_BLOCK_ENABLE,
				MAX98506_ADC_VIMON_EN_MASK,
				0);
		vstep->adc_status = !vstep->adc_status;
	} else if (sel > vstep->adc_thres
			&& !vstep->adc_status) {
		regmap_update_bits(max98506->regmap,
				MAX98506_R036_BLOCK_ENABLE,
				MAX98506_ADC_VIMON_EN_MASK,
				MAX98506_ADC_VIMON_EN_MASK);
		vstep->adc_status = !vstep->adc_status;
	} else if (sel > MAX98506_VSTEP_MAX) {
		msg_maxim("Unknown value %d", sel);
		return -EINVAL;
	}

	/*
	 * Boost voltage will be updated according to the volume.
	 * Step 0 ~ Step 13 : 6.5V
	 * Step 14			: 8.0V
	 * Over step 15		: 8.5V
	 */
	mask |= vstep->boost_step[sel];
	mask <<= MAX98506_BST_VOUT_SHIFT;
	regmap_update_bits(max98506->regmap,
			MAX98506_R037_CONFIGURATION,
			MAX98506_BST_VOUT_MASK,
			mask);

	/* Set volume step to ... */
	vstep->vol_step = sel;

	return 0;
}

static const struct snd_kcontrol_new max98506_snd_controls[] = {
	SOC_SINGLE_EXT_TLV("Speaker Gain", MAX98506_R02D_GAIN,
		MAX98506_SPK_GAIN_SHIFT,
		(1 << MAX98506_SPK_GAIN_WIDTH) - 1,
		0,
		max98506_spk_gain_get,
		max98506_spk_gain_put,
		max98506_spk_tlv),

	SOC_SINGLE_EXT("Speaker Ramp", 0, 0, 1, 0,
		max98506_spk_ramp_get, max98506_spk_ramp_put),

	SOC_SINGLE_EXT("Speaker ZCD", 0, 0, 1, 0,
		max98506_spk_zcd_get, max98506_spk_zcd_put),

	SOC_SINGLE_EXT("ALC Enable", 0, 0, 1, 0,
		max98506_alc_en_get, max98506_alc_en_put),

	SOC_SINGLE_EXT("ALC Threshold", 0, 0, (1<<MAX98506_ALC_TH_WIDTH)-1, 0,
		max98506_alc_threshold_get, max98506_alc_threshold_put),

	SOC_ENUM("Boost Output Voltage", max98506_boost_voltage_enum),

	SOC_ENUM_EXT("SPK out", spk_state_enum[0],
		max98506_spk_out_get, max98506_spk_out_put),

	SOC_SINGLE_EXT("ADC Enable", 0, 0, 1, 0,
			max98506_adc_en_get, max98506_adc_en_put),

	SOC_SINGLE_EXT("ADC Threshold", SND_SOC_NOPM, 0, 15, 0,
			max98506_adc_thres_get, max98506_adc_thres_put),

	SOC_SINGLE_EXT("Volume Step", SND_SOC_NOPM, 0, 15, 0,
			max98506_volume_step_get, max98506_volume_step_put),

#ifdef USE_DSM_LOG
	SOC_SINGLE_EXT("DSM LOG", SND_SOC_NOPM, 0, 3, 0,
		max98506_get_dump_status, max98506_set_dump_status),
#endif /* USE_DSM_LOG */
#ifdef USE_DSM_UPDATE_CAL
	SOC_SINGLE_EXT("DSM SetParam", SND_SOC_NOPM, 0, 1, 0,
		max98506_get_dsm_param, max98506_set_dsm_param),
#endif /* USE_DSM_UPDATE_CAL */
};

/* codec sample rate and n/m dividers parameter table */
static const struct {
	u32 rate;
	u8  sr;
	u32 divisors[3][2];
} rate_table[] = {
	{ 8000, 0, {{  1,   375}, {5, 1764}, {  1,   384} } },
	{11025,	1, {{147, 40000}, {1,  256}, {147, 40960} } },
	{12000, 2, {{  1,   250}, {5, 1176}, {  1,   256} } },
	{16000, 3, {{  2,   375}, {5,  882}, {  1,   192} } },
	{22050, 4, {{147, 20000}, {1,  128}, {147, 20480} } },
	{24000, 5, {{  1,   125}, {5,  588}, {  1,   128} } },
	{32000, 6, {{  4,   375}, {5,  441}, {  1,    96} } },
	{44100, 7, {{147, 10000}, {1,   64}, {147, 10240} } },
	{48000, 8, {{  2,   125}, {5,  294}, {  1,    64} } },
};

static inline int max98506_rate_value(int rate,
	int clock, u8 *value, int *n, int *m)
{
	int ret = -EINVAL;
	int i;

	for (i = 0; i < ARRAY_SIZE(rate_table); i++) {
		if (rate_table[i].rate >= rate) {
			*value = rate_table[i].sr;
			*n = rate_table[i].divisors[clock][0];
			*m = rate_table[i].divisors[clock][1];
			ret = 0;
			break;
		}
	}

	msg_maxim("sample rate is %d, returning %d",
		rate_table[i].rate, *value);

	return ret;
}

static int max98506_set_tdm_slot(struct snd_soc_dai *codec_dai,
		unsigned int tx_mask, unsigned int rx_mask,
		int slots, int slot_width)
{
	msg_maxim("tx_mask 0x%X, rx_mask 0x%X, slots %d, slot width %d",
			tx_mask, rx_mask, slots, slot_width);
	return 0;
}

static void max98506_set_slave(struct max98506_priv *max98506)
{
	struct max98506_pdata *pdata = max98506->pdata;

	msg_maxim("enter");

	/*
	 * 1. use BCLK instead of MCLK
	 */
	regmap_update_bits(max98506->regmap,
			MAX98506_R01A_DAI_CLK_MODE1,
			MAX98506_DAI_CLK_SOURCE_MASK,
			MAX98506_DAI_CLK_SOURCE_MASK);
	/*
	 * 2. set DAI to slave mode
	 */
	regmap_update_bits(max98506->regmap,
			MAX98506_R01B_DAI_CLK_MODE2,
			MAX98506_DAI_MAS_MASK,
			0);
	/*
	 * 3. set BLCKs to LRCLKs to 64
	 */
	regmap_update_bits(max98506->regmap,
			MAX98506_R01B_DAI_CLK_MODE2,
			MAX98506_DAI_BSEL_MASK,
			MAX98506_DAI_BSEL_32);
	/*
	 * 4. set VMON and IMON slots
	 */
	if (!pdata->vmon_slot)	{
		regmap_update_bits(max98506->regmap,
				MAX98506_R022_DOUT_CFG_VMON,
				MAX98506_DAI_VMON_EN_MASK,
				MAX98506_DAI_VMON_EN_MASK);
		regmap_update_bits(max98506->regmap,
				MAX98506_R022_DOUT_CFG_VMON,
				MAX98506_DAI_VMON_SLOT_MASK,
				MAX98506_DAI_VMON_SLOT_02_03);
		regmap_update_bits(max98506->regmap,
				MAX98506_R023_DOUT_CFG_IMON,
				MAX98506_DAI_IMON_EN_MASK,
				MAX98506_DAI_IMON_EN_MASK);
		regmap_update_bits(max98506->regmap,
				MAX98506_R023_DOUT_CFG_IMON,
				MAX98506_DAI_IMON_SLOT_MASK,
				MAX98506_DAI_IMON_SLOT_00_01);
	} else {
		regmap_update_bits(max98506->regmap,
				MAX98506_R022_DOUT_CFG_VMON,
				MAX98506_DAI_VMON_EN_MASK,
				MAX98506_DAI_VMON_EN_MASK);
		regmap_update_bits(max98506->regmap,
				MAX98506_R022_DOUT_CFG_VMON,
				MAX98506_DAI_VMON_SLOT_MASK,
				MAX98506_DAI_VMON_SLOT_00_01);
		regmap_update_bits(max98506->regmap,
				MAX98506_R023_DOUT_CFG_IMON,
				MAX98506_DAI_IMON_EN_MASK,
				MAX98506_DAI_IMON_EN_MASK);
		regmap_update_bits(max98506->regmap,
				MAX98506_R023_DOUT_CFG_IMON,
				MAX98506_DAI_IMON_SLOT_MASK,
				MAX98506_DAI_IMON_SLOT_02_03);
	}
}

static void max98506_set_master(struct max98506_priv *max98506)
{
	msg_maxim("enter");

	/*
	 * 1. use MCLK for Left channel, right channel always BCLK
	 */
	regmap_update_bits(max98506->regmap, MAX98506_R01A_DAI_CLK_MODE1,
			MAX98506_DAI_CLK_SOURCE_MASK, 0);
	/*
	 * 2. set left channel DAI to master mode, right channel always slave
	 */
	regmap_update_bits(max98506->regmap, MAX98506_R01B_DAI_CLK_MODE2,
			MAX98506_DAI_MAS_MASK, MAX98506_DAI_MAS_MASK);
}

static int max98506_dai_set_fmt(struct snd_soc_dai *codec_dai,
				 unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	unsigned int invert = 0;

	msg_maxim("fmt 0x%08X", fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		max98506_set_slave(max98506);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		max98506_set_master(max98506);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
	case SND_SOC_DAIFMT_CBM_CFS:
	default:
		dev_err(codec->dev, "DAI clock mode unsupported");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		msg_maxim("set SND_SOC_DAIFMT_I2S");
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		msg_maxim("set SND_SOC_DAIFMT_LEFT_J");
		break;
	case SND_SOC_DAIFMT_DSP_A:
		msg_maxim("set SND_SOC_DAIFMT_DSP_A");
	default:
		dev_warn(codec->dev, "DAI format unsupported, fmt:0x%x", fmt);
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		invert = MAX98506_DAI_WCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98506_DAI_BCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		invert = MAX98506_DAI_BCI_MASK | MAX98506_DAI_WCI_MASK;
		break;
	default:
		dev_err(codec->dev, "DAI invert mode unsupported");
		return -EINVAL;
	}

	regmap_update_bits(max98506->regmap, MAX98506_R020_FORMAT,
			MAX98506_DAI_BCI_MASK | MAX98506_DAI_WCI_MASK, invert);

	return 0;
}

static int max98506_set_bias_level(struct snd_soc_codec *codec,
				   enum snd_soc_bias_level level)
{
	codec->dapm.bias_level = level;
	return 0;
}

static int max98506_set_clock(struct max98506_priv *max98506, unsigned int rate)
{
	struct snd_soc_codec *codec = max98506->codec;
	struct max98506_pdata *pdata = max98506->pdata;
	unsigned int clock;
	unsigned int mdll;
	unsigned int n;
	unsigned int m;
	u8 dai_sr = 0;

	switch (pdata->sysclk) {
	case 6000000:
		clock = 0;
		mdll  = MAX98506_MDLL_MULT_MCLKx16;
		break;
	case 11289600:
		clock = 1;
		mdll  = MAX98506_MDLL_MULT_MCLKx8;
		break;
	case 12000000:
		clock = 0;
		mdll  = MAX98506_MDLL_MULT_MCLKx8;
		break;
	case 12288000:
		clock = 2;
		mdll  = MAX98506_MDLL_MULT_MCLKx8;
		break;
	default:
		dev_info(codec->dev, "unsupported sysclk %d\n",
					pdata->sysclk);
		return -EINVAL;
	}

	if (max98506_rate_value(rate, clock, &dai_sr, &n, &m))
		return -EINVAL;

	/*
	 * 1. set DAI_SR to correct LRCLK frequency
	 */
	regmap_update_bits(max98506->regmap, MAX98506_R01B_DAI_CLK_MODE2,
			MAX98506_DAI_SR_MASK, dai_sr << MAX98506_DAI_SR_SHIFT);
	/*
	 * 2. set DAI n divider
	 */
	regmap_write(max98506->regmap, MAX98506_R01F_DAI_CLK_DIV_N_LSBS,
			n & 0xFF);
	/*
	 * 4. set MDLL
	 */
	regmap_update_bits(max98506->regmap,
			MAX98506_R01A_DAI_CLK_MODE1,
			MAX98506_MDLL_MULT_MASK,
			mdll << MAX98506_MDLL_MULT_SHIFT);

	return 0;
}

static int max98506_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	unsigned int rate;

	msg_maxim("enter");

	rate = params_rate(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S16_LE");
		regmap_update_bits(max98506->regmap,
				MAX98506_R020_FORMAT,
				MAX98506_DAI_CHANSZ_MASK,
				MAX98506_DAI_CHANSZ_16);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S24_LE");
		regmap_update_bits(max98506->regmap,
				MAX98506_R020_FORMAT,
				MAX98506_DAI_CHANSZ_MASK,
				MAX98506_DAI_CHANSZ_32);
		msg_maxim("(really set to 32 bits)");
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		msg_maxim("set SNDRV_PCM_FORMAT_S32_LE");
		regmap_update_bits(max98506->regmap,
				MAX98506_R020_FORMAT,
				MAX98506_DAI_CHANSZ_MASK,
				MAX98506_DAI_CHANSZ_32);
		break;
	default:
		msg_maxim("format unsupported %d", params_format(params));
		return -EINVAL;
	}

	return max98506_set_clock(max98506, rate);
}

static int max98506_dai_set_sysclk(struct snd_soc_dai *dai,
				   int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_pdata *pdata = max98506->pdata;

	msg_maxim("clk_id %d, freq %d, dir %d", clk_id, freq, dir);

	pdata->sysclk = freq;

	return 0;
}

#define MAX98506_RATES SNDRV_PCM_RATE_8000_48000
#define MAX98506_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct snd_soc_dai_ops max98506_dai_ops = {
	.set_sysclk = max98506_dai_set_sysclk,
	.set_fmt = max98506_dai_set_fmt,
	.set_tdm_slot = max98506_set_tdm_slot,
	.hw_params = max98506_dai_hw_params,
};

static struct snd_soc_dai_driver max98506_dai[] = {
	{
		.name = "max98506-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98506_RATES,
			.formats = MAX98506_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98506_RATES,
			.formats = MAX98506_FORMATS,
		},
		.ops = &max98506_dai_ops,
	}
};

static void max98506_handle_pdata(struct snd_soc_codec *codec)
{
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_pdata *pdata = max98506->pdata;

	if (!pdata) {
		dev_dbg(codec->dev, "No platform data\n");
		return;
	}
}

#ifdef CONFIG_PM
static int max98506_suspend(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}

static int max98506_resume(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}
#else
#define max98506_suspend NULL
#define max98506_resume NULL
#endif /* CONFIG_PM */

#ifdef USE_MAX98506_IRQ
static irqreturn_t max98506_interrupt(int irq, void *data)
{
	struct max98506_priv *max98506 = (struct max98506_priv *) data;

	unsigned int mask0;
	unsigned int mask1;
	unsigned int mask2;
	unsigned int flag0;
	unsigned int flag1;
	unsigned int flag2;

	regmap_read(max98506->regmap, MAX98506_R00B_IRQ_ENABLE0, &mask0);
	regmap_read(max98506->regmap, MAX98506_R008_FLAG0, &flag0);

	regmap_read(max98506->regmap, MAX98506_R00C_IRQ_ENABLE1, &mask1);
	regmap_read(max98506->regmap, MAX98506_R009_FLAG1, &flag1);

	regmap_read(max98506->regmap, MAX98506_R00D_IRQ_ENABLE2, &mask2);
	regmap_read(max98506->regmap, MAX98506_R00A_FLAG2, &flag2);

	flag0 &= mask0;
	flag1 &= mask1;
	flag2 &= mask2;

	if (!flag0 && !flag1 && !flag2)
		return IRQ_NONE;

	/* Send work to be scheduled */
	if (flag0 & MAX98506_THERMWARN_END_STATE_MASK)
		msg_maxim("MAX98506_THERMWARN_STATE_MASK active!");

	if (flag0 & MAX98506_THERMWARN_BGN_STATE_MASK)
		msg_maxim("MAX98506_THERMWARN_BGN_STATE_MASK active!");

	if (flag0 & MAX98506_THERMSHDN_END_STATE_MASK)
		msg_maxim("MAX98506_THERMSHDN_END_STATE_MASK active!");

	if (flag0 & MAX98506_THERMSHDN_BGN_STATE_MASK)
		msg_maxim("MAX98506_THERMSHDN_BGN_STATE_MASK active!");

	if (flag1 & MAX98506_SPRCURNT_STATE_MASK)
		msg_maxim("MAX98506_SPRCURNT_STATE_MASK active!");

	if (flag1 & MAX98506_WATCHFAIL_STATE_MASK)
		msg_maxim("MAX98506_WATCHFAIL_STATE_MASK active!");

	if (flag1 & MAX98506_ALCINFH_STATE_MASK)
		msg_maxim("MAX98506_ALCINFH_STATE_MASK active!");

	if (flag1 & MAX98506_ALCACT_STATE_MASK)
		msg_maxim("MAX98506_ALCACT_STATE_MASK active!");

	if (flag1 & MAX98506_ALCMUT_STATE_MASK)
		msg_maxim("MAX98506_ALCMUT_STATE_MASK active!");

	if (flag1 & MAX98506_ALCP_STATE_MASK)
		msg_maxim("MAX98506_ALCP_STATE_MASK active!");

	if (flag2 & MAX98506_SLOTOVRN_STATE_MASK)
		msg_maxim("MAX98506_SLOTOVRN_STATE_MASK active!");

	if (flag2 & MAX98506_INVALSLOT_STATE_MASK)
		msg_maxim("MAX98506_INVALSLOT_STATE_MASK active!");

	if (flag2 & MAX98506_SLOTCNFLT_STATE_MASK)
		msg_maxim("MAX98506_SLOTCNFLT_STATE_MASK active!");

	if (flag2 & MAX98506_VBSTOVFL_STATE_MASK)
		msg_maxim("MAX98506_VBSTOVFL_STATE_MASK active!");

	if (flag2 & MAX98506_VBATOVFL_STATE_MASK)
		msg_maxim("MAX98506_VBATOVFL_STATE_MASK active!");

	if (flag2 & MAX98506_IMONOVFL_STATE_MASK)
		msg_maxim("MAX98506_IMONOVFL_STATE_MASK active!");

	if (flag2 & MAX98506_VMONOVFL_STATE_MASK)
		msg_maxim("MAX98506_VMONOVFL_STATE_MASK active!");

	regmap_write(max98506->regmap, MAX98506_R00E_IRQ_CLEAR0,
			flag0&0xff);
	regmap_write(max98506->regmap, MAX98506_R00F_IRQ_CLEAR1,
			flag1&0xff);
	regmap_write(max98506->regmap, MAX98506_R010_IRQ_CLEAR2,
			flag2&0xff);

	return IRQ_HANDLED;
}
#endif /* USE_MAX98506_IRQ */

static int reg_set_optimum_mode_check(struct regulator *reg, int load_ua)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_ua) : 0;
}

#define VCC_I2C_MIN_UV	1800000
#define VCC_I2C_MAX_UV	1800000
#define I2C_LOAD_UA	300000
static int max98506_regulator_config(struct device *dev)
{
	struct regulator *max98506_vcc_i2c;
	int ret;

	max98506_vcc_i2c = regulator_get(dev, "vcc_i2c");
	if (IS_ERR(max98506_vcc_i2c)) {
		ret = PTR_ERR(max98506_vcc_i2c);
		dev_err(dev, "%s: regulator get failed ret=%d\n",
				__func__, ret);
		goto err_get_vtg_i2c;
	}
	if (regulator_count_voltages(max98506_vcc_i2c) > 0) {
		ret = regulator_set_voltage(max98506_vcc_i2c,
				VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
		if (ret) {
			dev_err(dev, "%s: regulator set_vtg failed ret=%d\n",
					__func__, ret);
			goto err_set_vtg_i2c;
		}
	}

	ret = reg_set_optimum_mode_check(max98506_vcc_i2c, I2C_LOAD_UA);
	if (ret < 0) {
		dev_err(dev, "%s: regulator vcc_i2c set_opt failed ret=%d\n",
				__func__, ret);
		goto err_reg_opt_i2c;
	}

	ret = regulator_enable(max98506_vcc_i2c);
	if (ret) {
		dev_err(dev, "%s: regulator vcc_i2c enable failed ret=%d\n",
				__func__, ret);
		goto err_reg_en_vcc_i2c;
	}

	msg_maxim("min_uv:%d max_uv:%d load_ua:%d",
		VCC_I2C_MIN_UV, VCC_I2C_MAX_UV, I2C_LOAD_UA);

	return 0;

err_set_vtg_i2c:
	regulator_put(max98506_vcc_i2c);

err_get_vtg_i2c:
	if (regulator_count_voltages(max98506_vcc_i2c) > 0)
		regulator_set_voltage(max98506_vcc_i2c, 0, VCC_I2C_MAX_UV);

err_reg_en_vcc_i2c:
	reg_set_optimum_mode_check(max98506_vcc_i2c, 0);

err_reg_opt_i2c:
	regulator_disable(max98506_vcc_i2c);

	return ret;
}

static int max98506_check_version(struct max98506_priv *max98506)
{
	uint32_t reg = 0x00;
	uint32_t version_table[] = {
		MAX98506_VERSION,
		MAX98506_VERSION1,
		MAX98506_VERSION2,
		MAX98506_VERSION3,
		MAX98506_VERSION4,
		MAX98506_VERSION5,
	};
	int loop;

	regmap_read(max98506->regmap, MAX98506_R0FF_VERSION, &reg);

	for (loop = 0; loop < ARRAY_SIZE(version_table); loop++) {
		if (reg == version_table[loop])
			return reg;
	}

	return 0;
}

static int max98506_probe(struct snd_soc_codec *codec)
{
	struct max98506_priv *max98506 = snd_soc_codec_get_drvdata(codec);
	struct max98506_volume_step_info *vstep = &max98506->vstep;
	int ret = 0;
	int reg = 0;

	dev_info(codec->dev, "build number %s\n", MAX98506_REVISION);

	max98506->codec = codec;
	codec->control_data = max98506->regmap;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	reg = max98506_check_version(max98506);
	if (!reg) {
		dev_err(codec->dev,
			"device initialization error (0x%02X)\n",
			reg);
		goto err_version;
	}
	msg_maxim("device version 0x%02x", reg);

	regmap_write(max98506->regmap, MAX98506_R038_GLOBAL_ENABLE, 0x80);

	/* It's not the default but we need to set DAI_DLY */
	regmap_write(max98506->regmap, MAX98506_R020_FORMAT,
		MAX98506_DAI_DLY_MASK);

	regmap_write(max98506->regmap, MAX98506_R021_TDM_SLOT_SELECT, 0xC8);

	regmap_write(max98506->regmap, MAX98506_R027_DOUT_HIZ_CFG1, 0xFF);
	regmap_write(max98506->regmap, MAX98506_R028_DOUT_HIZ_CFG2, 0xFF);
	regmap_write(max98506->regmap, MAX98506_R029_DOUT_HIZ_CFG3, 0xFF);
	regmap_write(max98506->regmap, MAX98506_R02A_DOUT_HIZ_CFG4, 0xF0);
	regmap_write(max98506->regmap, MAX98506_R02C_FILTERS, 0xD9);

	regmap_update_bits(max98506->regmap,
		MAX98506_R02D_GAIN, MAX98506_DAC_IN_SEL_MASK,
		MAX98506_DAC_IN_SEL_DIV2_SUMMED_DAI);

	regmap_write(max98506->regmap, MAX98506_R02F_SPK_AMP, 0x02);
	regmap_write(max98506->regmap, MAX98506_R034_ALC_CONFIGURATION, 0x12);

	/* Enable ADC and Speaker */
	regmap_update_bits(max98506->regmap,
			MAX98506_R036_BLOCK_ENABLE,
			MAX98506_ADC_VIMON_EN_MASK |
			MAX98506_SPK_EN_MASK,
			MAX98506_ADC_VIMON_EN_MASK |
			MAX98506_SPK_EN_MASK);
	vstep->adc_status = 1;

	/* Set boost output to maximum */
	regmap_write(max98506->regmap, MAX98506_R037_CONFIGURATION, 0x00);

	/* Disable ALC muting */
	regmap_write(max98506->regmap, MAX98506_R03A_BOOST_LIMITER, 0xF8);

	max98506_set_slave(max98506);
	max98506_handle_pdata(codec);

#if defined(USE_DSM_LOG) || defined(USE_DSM_UPDATE_CAL)
	if (!g_class)
		g_class = class_create(THIS_MODULE, class_name_log);
	max98506->dev_log_class = g_class;
	if (max98506->dev_log_class) {
		max98506->dev_log =
			device_create(max98506->dev_log_class,
					NULL, 1, NULL, "max98506");
		if (IS_ERR(max98506->dev_log)) {
			ret = sysfs_create_group(&codec->dev->kobj,
				&max98506_attribute_group);
			if (ret)
				msg_maxim(
				"failed to create sysfs group [%d]", ret);
		} else {
			ret = sysfs_create_group(&max98506->dev_log->kobj,
				&max98506_attribute_group);
			if (ret)
				msg_maxim(
				"failed to create sysfs group [%d]", ret);
		}
	}
	msg_maxim("g_class=%p %p", g_class, max98506->dev_log_class);
#endif /* USE_DSM_LOG */

err_version:
	msg_maxim("exit %d", ret);

	return ret;
}

static int max98506_remove(struct snd_soc_codec *codec)
{
	msg_maxim("enter");

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_max98506 = {
	.probe			= max98506_probe,
	.remove			= max98506_remove,
	.set_bias_level		= max98506_set_bias_level,
	.suspend		= max98506_suspend,
	.resume			= max98506_resume,
	.controls		= max98506_snd_controls,
	.num_controls		= ARRAY_SIZE(max98506_snd_controls),
};

static const struct regmap_config max98506_regmap = {
	.reg_bits         = 8,
	.val_bits         = 8,
	.max_register     = MAX98506_R0FF_VERSION,
	.reg_defaults     = max98506_reg,
	.num_reg_defaults = ARRAY_SIZE(max98506_reg),
	.volatile_reg     = max98506_volatile_register,
	.readable_reg     = max98506_readable_register,
	.cache_type       = REGCACHE_RBTREE,
};

static int max98506_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct max98506_priv *max98506;
	struct max98506_pdata *pdata;
	struct max98506_volume_step_info *vstep;
	int ret;
	int pinfo_status = 0;

	msg_maxim("enter, device '%s'", id->name);

	max98506 = devm_kzalloc(&i2c->dev,
			sizeof(struct max98506_priv), GFP_KERNEL);
	if (!max98506) {
		ret = -ENOMEM;
		goto err_allocate_priv;
	}

	max98506->pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct max98506_pdata), GFP_KERNEL);
	if (!max98506->pdata) {
		ret = -ENOMEM;
		goto err_allocate_pdata;
	}

	i2c_set_clientdata(i2c, max98506);
	pdata = max98506->pdata;
	vstep = &max98506->vstep;

	if (i2c->dev.of_node) {
		/* Read system clock */
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,sysclk", &pdata->sysclk);
		if (ret) {
			dev_err(&i2c->dev, "There is no sysclk property.");
			pdata->sysclk = 12288000;
		}

		/* Read speaker volume */
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,spk-gain", &pdata->spk_gain);
		if (ret) {
			dev_err(&i2c->dev, "There is no spk_gain property.");
			pdata->spk_gain = 0x14;
		}

		/* Read VMON slot info.*/
		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,vmon_slot", &pdata->vmon_slot);
		if (ret) {
			dev_err(&i2c->dev, "There is no vmon_slot property.");
			pdata->vmon_slot = 0;
		}

		/* Regulator status */
		pdata->i2c_pull_up = of_property_read_bool(
				i2c->dev.of_node, "maxim,i2c-pull-up");
		if (pdata->i2c_pull_up)
			max98506_regulator_config(&i2c->dev);

#ifdef USE_MAX98506_IRQ
		pdata->irq = of_get_named_gpio_flags(
				i2c->dev.of_node, "maxim,irq-gpio", 0, NULL);
#endif /* USE_MAX98506_IRQ */

		/* Read information related to DSM */
		ret = of_property_read_u32_array(i2c->dev.of_node,
			"maxim,platform_info", (u32 *) &pdata->pinfo,
			sizeof(pdata->pinfo)/sizeof(uint32_t));
		if (ret)
			dev_err(&i2c->dev, "There is no platform info. property.\n");
		else
			pinfo_status = 1;

		ret = of_property_read_u32_array(i2c->dev.of_node,
			"maxim,boost_step",
			(uint32_t *) &vstep->boost_step,
			sizeof(vstep->boost_step)/sizeof(uint32_t));
		if (ret) {
			dev_err(&i2c->dev, "There is no boost_step property.\n");
			for (ret = 0; ret < MAX98506_VSTEP_14; ret++)
				vstep->boost_step[ret] = 0x0F;
			vstep->boost_step[MAX98506_VSTEP_14] = 0x02;
			vstep->boost_step[MAX98506_VSTEP_15] = 0x00;
		}

		ret = of_property_read_u32(i2c->dev.of_node,
				"maxim,adc_threshold", &vstep->adc_thres);
		if (ret) {
			dev_err(&i2c->dev, "There is no adc_threshold property.");
			vstep->adc_thres = MAX98506_VSTEP_7;
		}

#ifdef USE_DSM_LOG
		ret = of_property_read_string(i2c->dev.of_node,
			"maxim,log_class", &class_name_log);
		if (ret) {
			dev_err(&i2c->dev, "There is no log_class property.\n");
			class_name_log = DEFAULT_LOG_CLASS_NAME;
		}
#endif /* USE_DSM_LOG */
	} else {
		pdata->sysclk = 12288000;
		pdata->spk_gain = 0x14;
		pdata->vmon_slot = 0;
	}

#ifdef USE_MAX98506_IRQ
	if (pdata != NULL && gpio_is_valid(pdata->irq)) {
		ret = gpio_request(pdata->irq, "max98506_irq_gpio");
		if (ret) {
			dev_err(&i2c->dev, "unable to request gpio [%d]",
					pdata->irq);
			goto err_irq_gpio_req;
		}
		ret = gpio_direction_input(pdata->irq);
		if (ret) {
			dev_err(&i2c->dev,
					"unable to set direction for gpio [%d]",
					pdata->irq);
			goto err_irq_gpio_req;
		}
		i2c->irq = gpio_to_irq(pdata->irq);

		ret = request_threaded_irq(i2c->irq, NULL, max98506_interrupt,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"max98506_interrupt", max98506);
		if (ret)
			dev_err(&i2c->dev, "Failed to register interrupt");
	} else {
		dev_err(&i2c->dev, "irq gpio not provided\n");
	}
	dev_dbg(&i2c->dev, "requested irq for max98506");
	goto go_ahead_next_step;

err_irq_gpio_req:
	if (gpio_is_valid(pdata->irq))
		gpio_free(pdata->irq);

go_ahead_next_step:
#endif /* USE_MAX98506_IRQ */

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_init();
	if (pinfo_status)
#if 0
		maxdsm_update_info(pdata->pinfo);
#else
		dev_info(&i2c->dev, "pinfo will be ignored.\n");
#endif
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_max98506,
			max98506_dai, ARRAY_SIZE(max98506_dai));
	if (ret) {
		dev_err(&i2c->dev, "Failed to register codec");
		goto err_register_codec;
	}

	max98506->regmap = regmap_init_i2c(i2c, &max98506_regmap);
	if (IS_ERR(max98506->regmap)) {
		ret = PTR_ERR(max98506->regmap);
		dev_err(&i2c->dev, "Failed to initialize regmap: %d", ret);
		goto err_regmap;
	}

	msg_maxim("exit, device '%s'", id->name);

	return 0;

err_regmap:
	snd_soc_unregister_codec(&i2c->dev);
	if (max98506->regmap)
		regmap_exit(max98506->regmap);

err_register_codec:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_deinit();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	devm_kfree(&i2c->dev, max98506->pdata);

err_allocate_pdata:
	devm_kfree(&i2c->dev, max98506);

err_allocate_priv:
	msg_maxim("exit with errors. ret=%d", ret);

	return ret;
}

static int max98506_i2c_remove(struct i2c_client *client)
{
	struct max98506_priv *max98506 = i2c_get_clientdata(client);

	snd_soc_unregister_codec(&client->dev);
	if (max98506->regmap)
		regmap_exit(max98506->regmap);
	devm_kfree(&client->dev, max98506->pdata);
	devm_kfree(&client->dev, max98506);

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_deinit();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

	msg_maxim("exit");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id max98506_dt_ids[] = {
	{ .compatible = "maxim,max98506" },
	{ }
};
#else
#define max98506_dt_ids NULL
#endif /* CONFIG_OF */

static const struct i2c_device_id max98506_i2c_id[] = {
	{ "max98506", MAX98506 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max98506_i2c_id);

static struct i2c_driver max98506_i2c_driver = {
	.driver = {
		.name = "max98506",
		.owner = THIS_MODULE,
		.of_match_table = max98506_dt_ids,
	},
	.probe  = max98506_i2c_probe,
	.remove = max98506_i2c_remove,
	.id_table = max98506_i2c_id,
};

module_i2c_driver(max98506_i2c_driver);

MODULE_DESCRIPTION("ALSA SoC MAX98506 driver");
MODULE_AUTHOR("Ralph Birt <rdbirt@gmail.com>");
MODULE_AUTHOR("KyoungHun Jeon <hun.jeon@maximintegrated.com>");
MODULE_LICENSE("GPL");
