/*
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on:
 *
 * -------------------------------------------------------------------------
 *
 *  linux/include/asm-arm/arch-davinci/hardware.h
 *
 *  Copyright (C) 2006 Texas Instruments.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */
#ifndef __ASM_DAVINCI_RTC_H
#define __ASM_DAVINCI_RTC_H

struct davinci_rtc {
	unsigned int	second;
	unsigned int	minutes;
	unsigned int	hours;
	unsigned int	day;
	unsigned int	month; /* 0x10 */
	unsigned int	year;
	unsigned int	dotw;
	unsigned int	resv1;
	unsigned int	alarmsecond; /* 0x20 */
	unsigned int	alarmminute;
	unsigned int	alarmhour;
	unsigned int	alarmday;
	unsigned int	alarmmonth; /* 0x30 */
	unsigned int	alarmyear;
	unsigned int	resv2[2];
	unsigned int	ctrl; /* 0x40 */
	#define RTC_STATUS_ALARM2_MASK		0x80
	#define RTC_STATUS_ALARM_MASK		0x40
	unsigned int	status;
	unsigned int	irq;
	unsigned int	complsb;
	unsigned int	compmsb; /* 0x50 */
	unsigned int	osc;
	unsigned int	resv3[2];
	unsigned int	scratch0; /* 0x60 */
	unsigned int	scratch1;
	unsigned int	scratch2;
	unsigned int	kick0r;
	unsigned int	kick1r; /* 0x70 */

	/* am335x specific */
	unsigned int	revision;
	unsigned int	sysconfig;
	unsigned int	irqwakeen;
	unsigned int	alarm2second; /* 0x80 */
	unsigned int	alarm2minute;
	unsigned int	alarm2hour;
	unsigned int	alarm2day;
	unsigned int	alarm2month; /* 0x90 */
	unsigned int	alarm2year;
	#define RTC_PMIC_PWR_ENABLE_EN_MASK	0x10000
	unsigned int	pmic;
	unsigned int	debounce;
};

#define RTC_STATE_BUSY	0x01
#define RTC_STATE_RUN	0x02

#define RTC_KICK0R_WE	0x83e70b13
#define RTC_KICK1R_WE	0x95a4f1e0

int rtcss_pmic_sleep(int off_secs, int on_secs);

#endif
