/*
 *
 *   SwitchRes (C) 2010 Chris Kennedy <c@groovy.org>
 * 
 *   SwitchRes is a modeline generator based off of Calamity's 
 *   Methods of generating modelines
 *
 */

#include "emu.h"
#include "emuopts.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

int ModelineCreate(ConfigSettings *cs, GameInfo *game, MonitorMode *monitor, ModeLine *mode) {
	double margin = 0;
	double interlace = 1;
	double VBlankLines = 0;

	/* Clear modeline */
	memset(mode, 0, sizeof(ModeLine));

	/* Hfreq range */
	sprintf(mode->label, 
		"%.3fKhz -> %.3fKhz", 
		((double)monitor->HfreqMin/1000), ((double)monitor->HfreqMax/1000));

	/* Get games basic resolution */
	mode->hactive = game->width;
	mode->vactive = game->height;
	mode->vfreq   = game->refresh;
	mode->result  = 0;

	/* Vertical blanking */
	monitor->VerticalBlank = round((monitor->VfrontPorch * 1000) +
					(monitor->VsyncPulse * 1000) +
					(monitor->VbackPorch * 1000))/1000000;

	VBlankLines = round(monitor->HfreqMax * monitor->VerticalBlank);

	/* Height limits */
	monitor->YresMin = 
		Normalize((monitor->HfreqMin / monitor->VfreqMax) - VBlankLines, 8);
	monitor->YresMin /= 2;
	monitor->YresMax = 
		Normalize((monitor->HfreqMax / monitor->VfreqMin) - VBlankLines, 8);
	if (cs->interlace)
		monitor->YresMax *= 2;

	monitor->XresMin = 184;

	/* increase to minimum xres */
	if (mode->hactive < monitor->XresMin) {
		mode->hactive = monitor->XresMin;
		mode->result |= RESULT_RES_INC;
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Increased horizontal size to %d\n", mode->hactive);
	}

	/* user minimum height without doublescan */
	if (cs->ymin > 0 && /*monitor->YresMin < cs->ymin && */
	     cs->ymin < monitor->YresMax) 
	{
		monitor->YresMin = cs->ymin;
		if (cs->verbose > 3)
			mame_printf_verbose("SwitchRes: User input for minimum vertical size %d\n", 
				monitor->YresMin);
	}

	if (cs->verbose > 3)
		mame_printf_verbose("SwitchRes: Setup monitor limits min=%dx%d max=%dx%d\n",
			monitor->XresMin, monitor->YresMin, 0, monitor->YresMax);

	/* check vertical resolution */
	if (mode->vactive < monitor->YresMin) {
		if ((mode->vactive > (monitor->ActiveLinesLimit / 2)) && cs->interlace) {
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else if (cs->doublescan) {
			interlace = 0.5;
			mode->doublescan = 1;
			mode->result |= RESULT_DOUBLESCAN;
			if (cs->verbose > 2)
				mame_printf_verbose("SwitchRes: Using doublescan\n");
		} else if (!cs->doublescan && ((mode->vactive * 2) <= monitor->YresMax) &&
			((mode->vactive * 2) >= monitor->YresMin))
		{
			if (cs->verbose > 2)
				mame_printf_verbose("SwitchRes: Double height width instead of doublescan\n");
			mode->vactive *= 2; // Double height
			if (cs->keepaspect)
				mode->hactive *= 2; // Double width
			mode->result |= RESULT_DOUBLERES;
		} else {
                        double num, den;
                        sscanf(cs->aspect, "%lf:%lf", &den, &num);
                        mode->hactive = Normalize((double)monitor->YresMin * (den/num) / (4.0/3.0), 8);

			mode->vactive = monitor->YresMin;
			mode->result |= RESULT_RES_INC;
			if (cs->verbose > 2)
				mame_printf_verbose("SwitchRes: Increased vertical lines to %d\n", 
					mode->vactive);
		}
	}

	/* Vertical Refresh */
	if (mode->vfreq < monitor->VfreqMin) {
		if ((2 * mode->vfreq) <= monitor->VfreqMax)
			mode->vfreq *= 2;
		else
			mode->vfreq = monitor->VfreqMin;
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Increased vertical frequency to %.3f\n", 
				mode->vfreq);
	} else if (mode->vfreq > monitor->VfreqMax) {
		mode->vfreq = monitor->VfreqMax;
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Decreased vertical frequency to %.3f\n", 
				mode->vfreq);
	}

	/* check vertical active lines */
	if (mode->vactive > monitor->ActiveLinesLimit) {
		if (cs->interlace && mode->vactive <= monitor->VirtualLinesLimit) {
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else {
			if (cs->interlace) {
				interlace = 2;
				mode->interlace = 1;
				mode->result |= RESULT_INTERLACE;
				if (cs->verbose > 2)
					mame_printf_verbose("SwitchRes: Using interlace\n");
			} else {
				mode->vactive = monitor->ActiveLinesLimit;
				ResVirtualize(mode, monitor);
				mode->result |= RESULT_VIRTUALIZE;
			}
		}
	}

	/* Horizontal frequency */
	mode->hfreq = mode->vfreq * mode->vactive / 
			(interlace * (1.0 - mode->vfreq * monitor->VerticalBlank));

	if (cs->verbose > 2)
		mame_printf_verbose("SwitchRes: Starting with Horizontal freq of %.3f and Vertical refresh of %.2f\n",
			mode->hfreq/1000, mode->vfreq);

	/* Minimum horizontal frequency */
	if (mode->hfreq < monitor->HfreqMin) {
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Increased horizontal frequency from %.3f to %.3f\n", 
				mode->hfreq/1000, monitor->HfreqMin/1000);
		mode->hfreq = monitor->HfreqMin;
		mode->result |= RESULT_HFREQ_CHANGE;
	} else if (mode->hfreq > monitor->HfreqMax) {
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Horizontal frequency too high %.3f vfreq %.3f\n", 
				mode->hfreq/1000, mode->vfreq);
		if (mode->vactive > monitor->ActiveLinesLimit && cs->interlace)
		{
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else {
			double old_vfreq = mode->vfreq;
			if (cs->verbose > 2)
				mame_printf_verbose("SwitchRes: Lowered horizontal frequency to  %.3f from %.3f\n", 
					monitor->HfreqMax, mode->hfreq/1000);
			mode->hfreq = monitor->HfreqMax;
			VBlankLines = round(mode->hfreq * monitor->VerticalBlank);
			mode->vfreq = mode->hfreq / (mode->vactive / interlace + VBlankLines);
			mode->result |= RESULT_HFREQ_CHANGE;
			if (cs->verbose > 2)
				mame_printf_verbose("SwitchRes: Vertical frequency changed to %.3f from %.3f\n", 
					mode->vfreq, old_vfreq);
		}
	}

	/* Get total vertical lines */
	mode->vtotal = round(mode->hfreq / mode->vfreq);
	if (interlace == 2)
		mode->vtotal += 0.5;
	while ((mode->vfreq * mode->vtotal) < monitor->HfreqMin && (mode->vfreq * (mode->vtotal+1.0)) < monitor->HfreqMax) {
		if (cs->verbose > 2) 
			mame_printf_verbose("SwitchRes: Increasing 1 line from horizontal freq %.3f to %.3f\n",
				(mode->vfreq * mode->vtotal), (mode->vfreq * (mode->vtotal+1)));
		mode->vtotal = mode->vtotal + 1.0;
	}

	while (((mode->vfreq+0.001) * mode->vtotal) < monitor->HfreqMin) 
		mode->vfreq += 0.001;

	if (cs->dcalign) {
		int i;
		int newPclock = 0;
		int vIncr = 0;
		double newDiff = 0, Diff = 0;
		ModeLine newMode;
		memcpy(&newMode, mode, sizeof(struct ModeLine));
		if (cs->verbose > 3)
			mame_printf_verbose("SwitchRes: Vfreq = %f Game Refresh = %f\n", mode->vfreq, game->refresh);
		for (i = 0; i < 5; i++) {
			/* calculate new horizontal frequency */
			mode->hfreq = mode->vfreq * (mode->vtotal + i);
			if (mode->hfreq <= monitor->HfreqMax) {
				/* Fill horizontal part of modeline */
				newMode.hfreq = mode->hfreq;
				newMode.hactive = mode->hactive;
				ModelineGetLineParams(&newMode, monitor);
				newMode.pclock = mode->hfreq * newMode.htotal;
				if (fabs(Normalize(newMode.pclock - cs->dcalign, cs->dcalign)-newMode.pclock) < fabs(Normalize(newMode.pclock, cs->dcalign)-newMode.pclock))
					newMode.pclock = Normalize(newMode.pclock, cs->dcalign) - cs->dcalign;
				else	
					newMode.pclock = Normalize(newMode.pclock, cs->dcalign);
				newMode.vfreq = newMode.pclock / ((mode->vtotal + i) * newMode.htotal);
				newDiff = fabs(newMode.vfreq - mode->vfreq);
				if (newDiff < Diff || Diff == 0) {
					if (cs->verbose > 3)
						mame_printf_verbose("SwitchRes: [%d] Pclock: %d newVfreq: %f - Vfreq: %f newDiff = %f < Diff = %f\n", 
							i, newMode.pclock, newMode.vfreq, mode->vfreq, newDiff, Diff);
					Diff = newDiff;
					vIncr = i;
					newPclock = newMode.pclock;	
					mode->hactive = newMode.hactive;
					mode->hbegin = newMode.hbegin;
					mode->hend = newMode.hend;
					mode->htotal = newMode.htotal;
					if (newDiff < 0.01)
						break;
				}
			}
		}
		if (newPclock) {
			mode->vtotal += vIncr;
			mode->pclock = newPclock;
		} else {
			mode->hfreq = mode->vfreq * mode->vtotal;
			ModelineGetLineParams(mode, monitor);
			mode->pclock = mode->htotal * mode->hfreq;
		}
	} else {
		/* calculate new horizontal frequency */
		mode->hfreq = mode->vfreq * mode->vtotal;

		/* Fill horizontal part of modeline */
		ModelineGetLineParams(mode, monitor);

		/* recalculate dotclock */
		mode->pclock = mode->htotal * mode->hfreq;
	}
	mode->vfreq = mode->pclock / (mode->vtotal * mode->htotal);
	mode->hfreq = mode->vfreq * mode->vtotal;

	/* Check for vertical refresh rate match to original */
	if (mode->vfreq < (game->refresh - 0.10) ||
		mode->vfreq > (game->refresh + 0.10)) 
	{
		if (cs->verbose > 2)
			mame_printf_verbose("SwitchRes: Original Vref %f != %f\n", 
				game->refresh, mode->vfreq);
		if (round(game->refresh*2) != round(mode->vfreq))
			mode->result |= RESULT_VFREQ_CHANGE;
		if (round(game->refresh*2) == round(mode->vfreq))
			mode->result |= RESULT_VFREQ_DOUBLE;
	}

	sprintf(mode->name, "%dx%dx%.2f", mode->hactive, mode->vactive, mode->vfreq);

	/* Vertical blanking */
	VBlankLines = round(mode->hfreq * monitor->VerticalBlank);
	if (interlace == 2)
		VBlankLines += 0.5;
	margin = (round(mode->vtotal * interlace) - 
			mode->vactive - (VBlankLines * interlace)) / 2;
	if (margin) {
		mode->result |= RESULT_PADDING;
		mode->vpad = (margin*2);
		if (cs->verbose > 2) 
			mame_printf_verbose("SwitchRes: Using %d lines padding\n", 
				mode->vpad);
	}

	mode->vbegin = mode->vactive + round(((mode->hfreq/1000)*monitor->VfrontPorch) * 
			interlace + margin);
	mode->vend   = mode->vbegin + round(((mode->hfreq/1000)*monitor->VsyncPulse) *
			interlace);

	mode->vtotal = round(mode->vtotal * interlace);

	mode->hfreq = mode->vfreq * (mode->vtotal/interlace);
	if (!cs->dcalign)
		mode->pclock = mode->htotal * mode->hfreq;

	if (monitor->HsyncPolarity)
		mode->hsync = 1;

	if (monitor->VsyncPolarity)
		mode->vsync = 1;

	return 0;
}

int ModelineGetLineParams(ModeLine *mode, MonitorMode *monitor) {
	int hhh, hhi, hhf, hht;
	int hh, hs, he, ht;
	double LineTime, CharTime, NewCharTime;
	double HfrontPorchMin, HsyncPulseMin, HbackPorchMin;

	HfrontPorchMin = monitor->HfrontPorch - .20;
	HsyncPulseMin  = monitor->HsyncPulse  - .20;
	HbackPorchMin  = monitor->HbackPorch  - .20;

	LineTime = 1 / mode->hfreq * 1000000;
	
	hh = round(mode->hactive / 8);
	hs = 1;
	he = 1;
	ht = 1;

	do {
		CharTime = LineTime / (hh + hs + he + ht);
		if (hs * CharTime < HfrontPorchMin ||
		    abs((hs + 1) * CharTime - monitor->HfrontPorch) < 
		     abs(hs * CharTime - monitor->HfrontPorch)) 
		{
			hs++;
		}
		if (he * CharTime < HsyncPulseMin ||
		    abs((he + 1) * CharTime - monitor->HsyncPulse) < 
		     abs(he * CharTime - monitor->HsyncPulse)) 
		{
			he++;
		}
		if (ht * CharTime < HbackPorchMin ||
		    abs((ht + 1) * CharTime - monitor->HbackPorch) < 
		     abs(ht * CharTime - monitor->HbackPorch)) 
		{
			ht++;
		}
		NewCharTime = LineTime / (hh + hs + he + ht);
	} while (NewCharTime != CharTime);

	hhh = hh * 8;
	hhi = (hh + hs) * 8;
	hhf = (hh + hs + he) * 8;
	hht = (hh + hs + he + ht) * 8;

	mode->hactive = hhh;
	mode->hbegin  = hhi;
	mode->hend    = hhf;
	mode->htotal  = hht;

	return 0;
}

int ResVirtualize(ModeLine *mode, MonitorMode *monitor) {
	double interlace = 1;
	int xresNew, VBlankLines, ActiveLinesMax, ActiveLinesMin;
	double vfreqlimit = 0;
	int i;

	if (mode->interlace)
		interlace = 2;
	else if (mode->doublescan)
		interlace = 0.5;
	
	VBlankLines    = monitor->HfreqMax * monitor->VerticalBlank;
	ActiveLinesMax = monitor->HfreqMax / monitor->VfreqMin - VBlankLines;
	ActiveLinesMin = monitor->HfreqMin / monitor->VfreqMax - VBlankLines;

	if (ActiveLinesMin > monitor->YresMin) 
		ActiveLinesMin = monitor->YresMin;
	if (ActiveLinesMax > monitor->YresMax) 
		ActiveLinesMax = monitor->YresMax;

	if (interlace == 1 && 
	     mode->vactive > (ActiveLinesMin * interlace) && 
	      mode->vactive < ActiveLinesMax)
			ActiveLinesMax = mode->vactive;

	for (i = (Normalize(ActiveLinesMax, 8) * interlace);
		i >= (Normalize(ActiveLinesMin, 16) * interlace); i -= 16) 
	{
		mode->vactive = i;
		VBlankLines = 
			(monitor->HfreqMax - 50) * monitor->VerticalBlank;
		vfreqlimit = 
			(monitor->HfreqMax - 50) / 
				(mode->vactive + VBlankLines * interlace) * 
					interlace;
		if (vfreqlimit >= mode->vfreq) 
			goto ValidYres;
	}

	if (!vfreqlimit)
		return 1;

	mode->vfreq = vfreqlimit;

	ValidYres:
	xresNew = Normalize(((4.0/3.0) * mode->vactive), 8);
	if (mode->hactive < xresNew || interlace < 2)
		mode->hactive = xresNew;
	mode->hfreq = (mode->vactive / interlace + VBlankLines) * mode->vfreq;
 
	mame_printf_verbose("SwitchRes: Virtualized to %dx%d@%.2f %.4fKhz\n",
		mode->hactive, mode->vactive, mode->vfreq,
		mode->hfreq/1000);

	return 0;
}

char * PrintModeline(ModeLine *mode, char *modeline) {
	sprintf(modeline, "     \"%s\" %.6f %d %d %d %d %d %d %d %.0f %s %s%s%s",
		mode->name, ((double)mode->pclock/1000000), 
		mode->hactive, mode->hbegin, mode->hend, mode->htotal,
		mode->vactive, mode->vbegin, mode->vend, mode->vtotal,
		mode->hsync?"+HSync":"-HSync", mode->vsync?"+VSync":"-VSync",
		mode->doublescan?" doublescan":"", mode->interlace?" interlace":"");

	return modeline;
}

int ModelineResult(ModeLine *mode, ConfigSettings *cs) {
	int weight = 0;

	if (cs->verbose > 2) {
		if (mode->result)
			mame_printf_verbose("SwitchRes: # %s: (", mode->label);
		else
			mame_printf_verbose("SwitchRes: # %s: ( Perfect Resolution )\n", mode->label);
	}
	if (mode->result & RESULT_RES_INC) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Increased Size");
		if (cs->vsync)
			weight += 1;
		else
			weight += 5;
	}
	if (mode->result & RESULT_RES_DEC) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Reduced Size");
		if (cs->vsync)
			weight += 1;
		else
			weight += 5;
	}
	if (mode->result & RESULT_HFREQ_CHANGE) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Hfreq Change");
		weight += 1;
	}
	if (mode->result & RESULT_VFREQ_DOUBLE) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Vref Change");
	}
	if (mode->result & RESULT_VFREQ_CHANGE) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Vref Change");
		if (cs->vsync)
			weight += 10;
		else
			weight += 1;
	}
	if (mode->result & RESULT_INTERLACE) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Interlace");
		weight += 3;
	}
	if (mode->result & RESULT_DOUBLESCAN) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Doublescan");
		weight += 3;
	}
	if (mode->result & RESULT_DOUBLERES) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Doubleres");
		weight += 3;
	}
	if (mode->result & RESULT_LOWDOTCLOCK) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Lowdotclock");
		weight += 3;
	}
	if (mode->result & RESULT_VIRTUALIZE) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Virtualize");
		weight += 6;
	}
	if (mode->result & RESULT_PADDING) {
		if (cs->verbose > 2)
			mame_printf_verbose(" | Vpad +%d lines", mode->vpad);
		weight += 1;
		weight += round(mode->vpad/8);
	}
	if (cs->verbose > 2 && weight)
		mame_printf_verbose(" | )\n");

	return weight;
}

