/*
 * (C) Copyright 2011 DENX Software Engineering GmbH
 * Heiko Schocher <hs@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <rtc.h>
#include <asm/io.h>
#include <asm/davinci_rtc.h>
#include <dm.h>
#include <power/tps65217.h>

#if defined(CONFIG_CMD_DATE)
#if defined(CONFIG_DM_RTC)
int davinci_rtc_get(struct udevice *dev, struct rtc_time *tmp)
#else
int rtc_get(struct rtc_time *tmp)
#endif
{
	struct davinci_rtc *rtc = (struct davinci_rtc *)DAVINCI_RTC_BASE;
	unsigned long sec, min, hour, mday, wday, mon_cent, year;
	unsigned long status;

	status = readl(&rtc->status);
	if ((status & RTC_STATE_RUN) != RTC_STATE_RUN) {
		printf("RTC doesn't run\n");
		return -1;
	}
	while (((status = readl(&rtc->status)) & RTC_STATE_BUSY) == RTC_STATE_BUSY) {
		udelay(1);
	}

	sec	= readl(&rtc->second);
	min	= readl(&rtc->minutes);
	hour	= readl(&rtc->hours);
	mday	= readl(&rtc->day);
	wday	= readl(&rtc->dotw);
	mon_cent = readl(&rtc->month);
	year	= readl(&rtc->year);

	debug("Get RTC year: %02lx mon/cent: %02lx mday: %02lx wday: %02lx "
		"hr: %02lx min: %02lx sec: %02lx\n",
		year, mon_cent, mday, wday,
		hour, min, sec);

	tmp->tm_sec  = bcd2bin(sec  & 0x7F);
	tmp->tm_min  = bcd2bin(min  & 0x7F);
	tmp->tm_hour = bcd2bin(hour & 0x3F);
	tmp->tm_mday = bcd2bin(mday & 0x3F);
	tmp->tm_mon  = bcd2bin(mon_cent & 0x1F);
	tmp->tm_year = bcd2bin(year) + 2000;
	tmp->tm_wday = bcd2bin(wday & 0x07);
	tmp->tm_yday = 0;
	tmp->tm_isdst = 0;

	debug("Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
		tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

	return 0;
}

#if defined(CONFIG_DM_RTC)
int davinci_rtc_set(struct udevice *dev, const struct rtc_time *tmp)
#else
int rtc_set(struct rtc_time *tmp)
#endif

{
	struct davinci_rtc *rtc = (struct davinci_rtc *)DAVINCI_RTC_BASE;
	unsigned status;

	debug("Set DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
		tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

	status = readl(&rtc->status);
	if ((status & RTC_STATE_RUN) == RTC_STATE_RUN) {
		while (((status = readl(&rtc->status)) & RTC_STATE_BUSY) == RTC_STATE_BUSY) {
			udelay(1);
		}
	}

	writel(bin2bcd(tmp->tm_year % 100), &rtc->year);
	writel(bin2bcd(tmp->tm_mon), &rtc->month);

	writel(bin2bcd(tmp->tm_wday), &rtc->dotw);
	writel(bin2bcd(tmp->tm_mday), &rtc->day);
	writel(bin2bcd(tmp->tm_hour), &rtc->hours);
	writel(bin2bcd(tmp->tm_min), &rtc->minutes);
	writel(bin2bcd(tmp->tm_sec), &rtc->second);
	return 0;
}

#if defined(CONFIG_DM_RTC)
int davinci_rtc_reset(struct udevice *dev)
#else
void rtc_reset(void)
#endif
{
	struct davinci_rtc *rtc = (struct davinci_rtc *)DAVINCI_RTC_BASE;

	/* run RTC counter */
	writel(0x01, &rtc->ctrl);
	return;
}

/*
 * rtcss_pmic_go()
 * Power off(sleep) PMIC after    arg:off_secs seconds.
 * Then power on(wake) PMIC after arg:on_secs seconds.
 */
static int rtcss_alarm(uint32_t* base, const struct rtc_time *tmp) {
	struct davinci_rtc *rtc = (struct davinci_rtc *)DAVINCI_RTC_BASE;
	unsigned status;

	status = readl(&rtc->status);
	if ((status & RTC_STATE_RUN) == RTC_STATE_RUN) {
		while (((status = readl(&rtc->status)) & RTC_STATE_BUSY) == RTC_STATE_BUSY) {
			udelay(1);
		}
	}

	writel(bin2bcd(tmp->tm_year % 100), &base[5]);
	writel(bin2bcd(tmp->tm_mon),  &base[4]);
	writel(bin2bcd(tmp->tm_mday), &base[3]);
	writel(bin2bcd(tmp->tm_hour), &base[2]);
	writel(bin2bcd(tmp->tm_min),  &base[1]);
	writel(bin2bcd(tmp->tm_sec),  &base[0]);
	return 0;
}
int rtcss_pmic_sleep(int off_secs, int on_secs) {
	struct davinci_rtc *rtc = (struct davinci_rtc *)DAVINCI_RTC_BASE;
	static struct rtc_time default_tm = { 0, 0, 0, 1, 1, 2000, 6, 0, 0 };
	struct rtc_time rtm[1];
	unsigned v, status;
	time_t tm;

	status = readl(&rtc->status);
	if ((status & RTC_STATE_RUN) != RTC_STATE_RUN) {
		/* run RTC counter */
		writel(0x01, &rtc->ctrl);
		/* default time */
#if defined(CONFIG_DM_RTC)
		davinci_rtc_set(NULL, &default_tm);
#else
		rtc_set(&default_tm);
#endif
		mdelay(10);
	}

	/* clear PWR_ENABLE_EN */
	v = readl(&rtc->pmic);
	v &= ~RTC_PMIC_PWR_ENABLE_EN_MASK;
	writel(v, &rtc->pmic);

	/* clear ALARM2 & ALARM status */
	v = readl(&rtc->status);
	v |= RTC_STATUS_ALARM2_MASK | RTC_STATUS_ALARM_MASK;
	writel(v, &rtc->status);

	/* set ALARM & ALARM2 time */
#if defined(CONFIG_DM_RTC)
	davinci_rtc_get(NULL, rtm);
#else
	rtc_get(rtm);
#endif
	tm = rtc_mktime(rtm);

	tm += off_secs;
	rtc_to_tm(tm, rtm);
	rtcss_alarm((uint32_t*)&rtc->alarm2second, rtm);

	tm += on_secs;
	rtc_to_tm(tm, rtm);
	rtcss_alarm((uint32_t*)&rtc->alarmsecond, rtm);

	/* set PWR_ENABLE_EN */
	v = readl(&rtc->pmic);
	debug("RTC: PMIC REG = 0x%08x\n", v);
	v |= RTC_PMIC_PWR_ENABLE_EN_MASK;
	writel(v, &rtc->pmic);

	/* set ALARM2 & ALARM event */
	v = readl(&rtc->irq);
	v |= (0x1 << 4) | (0x1 << 3);
	writel(v, &rtc->irq);

	printf("RTC: PMIC will sleep after %d secs\n"
	       "         Then wakeup after %d secs\n\n", off_secs, on_secs);

	return 0;
}

#if defined(CONFIG_CMD_POWEROFF)
int do_poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	/* Set PWR_EN bit in Status Register */
	tps65217_reg_write(TPS65217_PROT_LEVEL_NONE,
			   TPS65217_STATUS, TPS65217_PWR_OFF, TPS65217_PWR_OFF);
	rtcss_pmic_sleep(2, 1000);
	return 0;
}
#endif

#if defined(CONFIG_DM_RTC)
static const struct rtc_ops davinci_rtc_ops = {
	.get = davinci_rtc_get,
	.set = davinci_rtc_set,
	.reset = davinci_rtc_reset,
};

static const struct udevice_id davinci_rtc_ids[] = {
	{.compatible = "ti,am3352-rtc", .data = 0 },
	{.compatible = "ti,da830-rtc",  .data = 1 },
	{ },
};

U_BOOT_DRIVER(rtc_pcf2127) = {
	.name	= "rtc-davinci",
	.id	= UCLASS_RTC,
	.of_match = davinci_rtc_ids,
	.ops	= &davinci_rtc_ops,
};
#endif

#endif
