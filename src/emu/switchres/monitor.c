/*
 *
 *   SwitchRes 2010 Chris Kennedy <c@groovy.org>
 * 
 *   SwitchRes is a modeline generator based off of Calamity's 
 *   Methods of generating modelines
 *
 */

#include "emu.h"
#include "emuopts.h"
#include <stdio.h>
#include <string.h>

int FillMonitorMode(const char *line, MonitorMode *monitor) {
	int e = sscanf(line, "%lf-%lf,%lf-%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%lf,%d",
		&monitor->HfreqMin, &monitor->HfreqMax, 
		&monitor->VfreqMin, &monitor->VfreqMax,
		&monitor->HfrontPorch, &monitor->HsyncPulse, &monitor->HbackPorch,
		&monitor->VfrontPorch, &monitor->VsyncPulse, &monitor->VbackPorch,
		&monitor->HsyncPolarity, &monitor->VsyncPolarity,
		&monitor->ActiveLinesLimit, &monitor->VirtualLinesLimit);

	if (e != 14) {
		mame_printf_error("SwitchRes: Error trying to fill monitor mode with\n  %s\n",
			line);
		return -1;
	}

	return 0;
}

int ShowMonitorMode(MonitorMode *monitor) {
	mame_printf_info("SwitchRes: MonitorLimits %.2f-%.2f,%.2f-%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%.1f,%d\n",
		monitor->HfreqMin, monitor->HfreqMax, 
		monitor->VfreqMin, monitor->VfreqMax,
		monitor->HfrontPorch, monitor->HsyncPulse, monitor->HbackPorch,
		monitor->VfrontPorch, monitor->VsyncPulse, monitor->VbackPorch,
		monitor->HsyncPolarity, monitor->VsyncPolarity,
		monitor->ActiveLinesLimit, monitor->VirtualLinesLimit);

	return 0;
}

int SetMonitorMode(char *type, MonitorMode monitor[MAX_MODES]) {
	if (!strcmp(type, "m2929")) {
		monitor[0].HfreqMin = 30000;
		monitor[0].HfreqMax = 40000;
		monitor[0].VfreqMin = 47;
		monitor[0].VfreqMax = 90;
		monitor[4].HfrontPorch = 0.636;
		monitor[4].HsyncPulse  = 3.813;
		monitor[4].HbackPorch  = 1.906;
		monitor[4].VfrontPorch = 0.020;
		monitor[4].VsyncPulse  = 0.106;
		monitor[4].VbackPorch  = 0.607;
		monitor[4].HsyncPolarity = 1;
		monitor[4].VsyncPolarity = 1;
		monitor[4].ActiveLinesLimit = 576;
		monitor[4].VirtualLinesLimit = 768;
		return 1;
	} else if (!strcmp(type, "d9800") || !strcmp(type, "d9400")) {
		/* CGA */
		monitor[0].HfreqMin = 15250;
		monitor[0].HfreqMax = 18000;
		monitor[0].VfreqMin = 40;
		monitor[0].VfreqMax = 80;
		monitor[0].HfrontPorch = 2.187;
		monitor[0].HsyncPulse  = 4.688;
		monitor[0].HbackPorch  = 6.719;
		monitor[0].VfrontPorch = 0.190;
		monitor[0].VsyncPulse  = 0.191;
		monitor[0].VbackPorch  = 1.018;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 288;
		monitor[0].VirtualLinesLimit = 448;
		/* CGA-> EGA */
		monitor[1].HfreqMin = 18001;
		monitor[1].HfreqMax = 19000;
		monitor[1].VfreqMin = 40;
		monitor[1].VfreqMax = 80;
		monitor[1].HfrontPorch = 2.187;
		monitor[1].HsyncPulse  = 4.688;
		monitor[1].HbackPorch  = 6.719;
		monitor[1].VfrontPorch = 0.140;
		monitor[1].VsyncPulse  = 0.191;
		monitor[1].VbackPorch  = 0.950;
		monitor[1].HsyncPolarity = 0;
		monitor[1].VsyncPolarity = 0;
		monitor[1].ActiveLinesLimit = 288;
		monitor[1].VirtualLinesLimit = 448;
		/* EGA */
		monitor[2].HfreqMin = 20501;
		monitor[2].HfreqMax = 29000;
		monitor[2].VfreqMin = 40;
		monitor[2].VfreqMax = 80;
		monitor[2].HfrontPorch = 2.910;
		monitor[2].HsyncPulse  = 3.000;
		monitor[2].HbackPorch  = 4.440;
		monitor[2].VfrontPorch = 0.451;
		monitor[2].VsyncPulse  = 0.164;
		monitor[2].VbackPorch  = 1.048;
		monitor[2].HsyncPolarity = 0;
		monitor[2].VsyncPolarity = 0;
		monitor[2].ActiveLinesLimit = 480;
		monitor[2].VirtualLinesLimit = 768;
		/* VGA */
		monitor[3].HfreqMin = 29001;
		monitor[3].HfreqMax = 32000;
		monitor[3].VfreqMin = 40;
		monitor[3].VfreqMax = 80;
		monitor[3].HfrontPorch = 0.636;
		monitor[3].HsyncPulse  = 3.813;
		monitor[3].HbackPorch  = 1.906;
		monitor[3].VfrontPorch = 0.318;
		monitor[3].VsyncPulse  = 0.064;
		monitor[3].VbackPorch  = 1.048;
		monitor[3].HsyncPolarity = 0;
		monitor[3].VsyncPolarity = 0;
		monitor[3].ActiveLinesLimit = 576;
		monitor[3].VirtualLinesLimit = 768;
		/* VGA->SVGA */
		monitor[4].HfreqMin = 32001;
		monitor[4].HfreqMax = 34000;
		monitor[4].VfreqMin = 40;
		monitor[4].VfreqMax = 80;
		monitor[4].HfrontPorch = 0.636;
		monitor[4].HsyncPulse  = 3.813;
		monitor[4].HbackPorch  = 1.906;
		monitor[4].VfrontPorch = 0.020;
		monitor[4].VsyncPulse  = 0.106;
		monitor[4].VbackPorch  = 0.607;
		monitor[4].HsyncPolarity = 0;
		monitor[4].VsyncPolarity = 0;
		monitor[4].ActiveLinesLimit = 576;
		monitor[4].VirtualLinesLimit = 768;
		/* SVGA */
		monitor[5].HfreqMin = 34001;
		monitor[5].HfreqMax = 38000;
		monitor[5].VfreqMin = 40;
		monitor[5].VfreqMax = 80;
		monitor[5].HfrontPorch = 1.000;
		monitor[5].HsyncPulse  = 3.200;
		monitor[5].HbackPorch  = 2.200;
		monitor[5].VfrontPorch = 0.020;
		monitor[5].VsyncPulse  = 0.106;
		monitor[5].VbackPorch  = 0.607;
		monitor[5].HsyncPolarity = 0;
		monitor[5].VsyncPolarity = 0;
		monitor[5].ActiveLinesLimit = 600;
		monitor[5].VirtualLinesLimit = 768;
		return 6;
	} else if (!strcmp(type, "d9200") || !strcmp(type, "m3192")) {
		/* CGA */
		monitor[0].HfreqMin = 15250;
		monitor[0].HfreqMax = 16500;
		monitor[0].VfreqMin = 40;
		monitor[0].VfreqMax = 80;
		monitor[0].HfrontPorch = 2.187;
		monitor[0].HsyncPulse  = 4.688;
		monitor[0].HbackPorch  = 6.719;
		monitor[0].VfrontPorch = 0.190;
		monitor[0].VsyncPulse  = 0.191;
		monitor[0].VbackPorch  = 1.018;
		if (!strcmp(type, "d9200")) {
			monitor[0].HsyncPolarity = 0;
			monitor[0].VsyncPolarity = 0;
		} else {
			monitor[0].HsyncPolarity = 1;
			monitor[0].VsyncPolarity = 1;
		}
		monitor[0].ActiveLinesLimit = 288;
		monitor[0].VirtualLinesLimit = 448;
		/* EGA */
		monitor[1].HfreqMin = 23900;
		monitor[1].HfreqMax = 24420;
		monitor[1].VfreqMin = 40;
		monitor[1].VfreqMax = 80;
		monitor[1].HfrontPorch = 2.910;
		monitor[1].HsyncPulse  = 3.000;
		monitor[1].HbackPorch  = 4.440;
		monitor[1].VfrontPorch = 0.451;
		monitor[1].VsyncPulse  = 0.164;
		monitor[1].VbackPorch  = 1.048;
		if (!strcmp(type, "d9200")) {
			monitor[1].HsyncPolarity = 0;
			monitor[1].VsyncPolarity = 0;
		} else {
			monitor[1].HsyncPolarity = 1;
			monitor[1].VsyncPolarity = 1;
		}
		monitor[1].ActiveLinesLimit = 384;
		monitor[1].VirtualLinesLimit = 768;
		/* VGA */
		monitor[2].HfreqMin = 31000;
		monitor[2].HfreqMax = 32000;
		monitor[2].VfreqMin = 40;
		monitor[2].VfreqMax = 80;
		monitor[2].HfrontPorch = 0.636;
		monitor[2].HsyncPulse  = 3.813;
		monitor[2].HbackPorch  = 1.906;
		monitor[2].VfrontPorch = 0.318;
		monitor[2].VsyncPulse  = 0.064;
		monitor[2].VbackPorch  = 1.048;
		if (!strcmp(type, "d9200")) {
			monitor[2].HsyncPolarity = 0;
			monitor[2].VsyncPolarity = 0;
		} else {
			monitor[2].HsyncPolarity = 1;
			monitor[2].VsyncPolarity = 1;
		}
		monitor[2].ActiveLinesLimit = 576;
		monitor[2].VirtualLinesLimit = 768;
		if (!strcmp(type, "d9200")) {
			/* SVGA */
			monitor[3].HfreqMin = 37000;
			monitor[3].HfreqMax = 38000;
			monitor[3].VfreqMin = 40;
			monitor[3].VfreqMax = 80;
			monitor[3].HfrontPorch = 1.000;
			monitor[3].HsyncPulse  = 3.200;
			monitor[3].HbackPorch  = 2.200;
			monitor[3].VfrontPorch = 0.020;
			monitor[3].VsyncPulse  = 0.106;
			monitor[3].VbackPorch  = 0.607;
			monitor[3].HsyncPolarity = 0;
			monitor[3].VsyncPolarity = 0;
			monitor[3].ActiveLinesLimit = 600;
			monitor[3].VirtualLinesLimit = 768;
			return 4;
		} else
			return 3;
	} else if (!strcmp(type, "ega")) {
		monitor[0].HfreqMin = 24960;
		monitor[0].HfreqMax = 24960;
		monitor[0].VfreqMin = 50;
		monitor[0].VfreqMax = 60;
		monitor[0].HfrontPorch = 2.910;
		monitor[0].HsyncPulse  = 3.000;
		monitor[0].HbackPorch  = 4.440;
		monitor[0].VfrontPorch = 0.451;
		monitor[0].VsyncPulse  = 0.164;
		monitor[0].VbackPorch  = 1.048;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 384;
		monitor[0].VirtualLinesLimit = 576;
		return 1;
	} else if (!strcmp(type, "vga")) {
		monitor[0].HfreqMin = 31500;
		monitor[0].HfreqMax = 31500;
		monitor[0].VfreqMin = 50;
		monitor[0].VfreqMax = 70;
		monitor[0].HfrontPorch = 0.636;
		monitor[0].HsyncPulse  = 3.813;
		monitor[0].HbackPorch  = 1.906;
		monitor[0].VfrontPorch = 0.318;
		monitor[0].VsyncPulse  = 0.064;
		monitor[0].VbackPorch  = 1.048;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 1;
		monitor[0].ActiveLinesLimit = 576;
		monitor[0].VirtualLinesLimit = 768;
		return 1;
	} else if (!strcmp(type, "multi")) {
		monitor[0].HfreqMin = 54200;
		monitor[0].HfreqMax = 83800;
		monitor[0].VfreqMin = 49;
		monitor[0].VfreqMax = 75;
		monitor[0].HfrontPorch = 1.000;
		monitor[0].HsyncPulse  = 3.200;
		monitor[0].HbackPorch  = 2.200;
		monitor[0].VfrontPorch = 0.020;
		monitor[0].VsyncPulse  = 0.106;
		monitor[0].VbackPorch  = 0.607;
		monitor[0].HsyncPolarity = 1;
		monitor[0].VsyncPolarity = 1;
		monitor[0].ActiveLinesLimit = 1080;
		monitor[0].VirtualLinesLimit = 1080;
		return 1;
	} else if (!strcmp(type, "h9110")) {
		monitor[0].HfreqMin = 15625;
		monitor[0].HfreqMax = 16670;
		monitor[0].VfreqMin = 49.5;
		monitor[0].VfreqMax = 65;
		monitor[0].HfrontPorch = 2.000;
		monitor[0].HsyncPulse  = 4.700;
		monitor[0].HbackPorch  = 8.000;
		monitor[0].VfrontPorch = 0.064;
		monitor[0].VsyncPulse  = 0.160;
		monitor[0].VbackPorch  = 1.056;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 288;
		monitor[0].VirtualLinesLimit = 448;
		return 1;
	} else if (!strcmp(type, "pal")) {
		monitor[0].HfreqMin = 15625;
		monitor[0].HfreqMax = 15625;
		monitor[0].VfreqMin = 50;
		monitor[0].VfreqMax = 50;
		monitor[0].HfrontPorch = 1.650;
		monitor[0].HsyncPulse  = 4.700;
		monitor[0].HbackPorch  = 5.000;
		monitor[0].VfrontPorch = 0.064;
		monitor[0].VsyncPulse  = 0.192;
		monitor[0].VbackPorch  = 1.024;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 312.5;
		monitor[0].VirtualLinesLimit = 576;
		return 1;
	} else if (!strcmp(type, "ntsc")) {
		monitor[0].HfreqMin = 15734.26;
		monitor[0].HfreqMax = 15734.26;
		monitor[0].VfreqMin = 58.95;
		monitor[0].VfreqMax = 59.95;
		monitor[0].HfrontPorch = 2.000;
		monitor[0].HsyncPulse  = 4.700;
		monitor[0].HbackPorch  = 8.000;
		monitor[0].VfrontPorch = 0.064;
		monitor[0].VsyncPulse  = 0.192;
		monitor[0].VbackPorch  = 1.024;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 262.5;
		monitor[0].VirtualLinesLimit = 480;
		return 1;
	} else if (!strcmp(type, "generic")) {
		monitor[0].HfreqMin = 15725;
		monitor[0].HfreqMax = 15750;
		monitor[0].VfreqMin = 50;
		monitor[0].VfreqMax = 60;
		monitor[0].HfrontPorch = 2.000;
		monitor[0].HsyncPulse  = 4.700;
		monitor[0].HbackPorch  = 8.000;
		monitor[0].VfrontPorch = 0.064;
		monitor[0].VsyncPulse  = 0.192;
		monitor[0].VbackPorch  = 1.024;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 288;
		monitor[0].VirtualLinesLimit = 448;
		return 1;
	} else {
		monitor[0].HfreqMin = 15250;
		monitor[0].HfreqMax = 15700;
		monitor[0].VfreqMin = 49.5;
		monitor[0].VfreqMax = 65;
		monitor[0].HfrontPorch = 2.000;
		monitor[0].HsyncPulse  = 4.700;
		monitor[0].HbackPorch  = 8.000;
		monitor[0].VfrontPorch = 0.064;
		monitor[0].VsyncPulse  = 0.192;
		monitor[0].VbackPorch  = 1.024;
		monitor[0].HsyncPolarity = 0;
		monitor[0].VsyncPolarity = 0;
		monitor[0].ActiveLinesLimit = 288;
		monitor[0].VirtualLinesLimit = 448;
		return 1;
	}
	return 0;
}
