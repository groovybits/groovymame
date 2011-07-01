/*
 *
 *   SwitchRes 2010 Chris Kennedy <c@groovy.org>
 * 
 *   SwitchRes is a modeline generator based off of Calamity's 
 *   Methods of generating modelines
 *
 */

#ifndef __SWITCHRES_H__
#define __SWITCHRES_H__

#include <stdio.h>
#include <stdlib.h>

#define SWITCHRES_VERSION "0.013a"

#define MAX_MODES 10
#define MAX_MODELINES 250
#define MONITOR_TYPES "[cga ega vga multi ntsc pal h9110 d9200 d9800 m2929 m3192]"

#define RESULT_RES_INC      0x00000001
#define RESULT_RES_DEC      0x00000002
#define RESULT_INTERLACE    0x00000004
#define RESULT_DOUBLESCAN   0x00000008
#define RESULT_VIRTUALIZE   0x00000010
#define RESULT_HFREQ_CHANGE 0x00000020
#define RESULT_PADDING      0x00000040
#define RESULT_VFREQ_CHANGE 0x00000080
#define RESULT_VFREQ_DOUBLE 0x00000100
#define RESULT_DOUBLERES    0x00000200
#define RESULT_LOWDOTCLOCK  0x00000400

typedef struct ConfigSettings {
	int    verbose;
	char   monitor[256];
	char   connector[256];
	int    interlace;
	int    doublescan;
	char   monitorlimit[MAX_MODES][256];
	int    monitorcount;
	int    dotclockmin;
	int    dotclockmax;
	int    vsync;
	int    ymin;
	char   morientation[256];
	char   aspect[32];
	int    dcalign;
	int    redraw;
	int    cleanstretch;
	int    soundsync;
	int    changeres;
	int    keepaspect;
} ConfigSettings;

typedef struct ModeLine {
	char   name[256];
	int    pclock;
	int    hactive;
	int    hbegin;
	int    hend;
	int    htotal;
	int    vactive;
	int    vbegin;
	int    vend;
	double vtotal;
	int    interlace;
	int    doublescan;
	int    hsync;
	int    vsync;
	//
	int    result;
	int    weight;
	int    vpad;
	char   label[256];
	char   resolution[256];
	int    depth;
	//
	double vfreq;
	int    a_width;
	int    a_height;
	double a_vfreq;
	double hfreq;
	//
	char   game[256];
	//
	int    desktop;
	int    custom;
	//
	char   regdata[256];
	int    regdata_size;
	int    modified;
	double score;
} ModeLine;

typedef struct MonitorMode {
	double HfreqMin;
	double HfreqMax;
	double VfreqMin;
	double VfreqMax;
	double HfrontPorch;
	double HsyncPulse;
	double HbackPorch;
	double VfrontPorch;
	double VsyncPulse;
	double VbackPorch;
	int    HsyncPolarity;
	int    VsyncPolarity;
	//
	double ActiveLinesLimit;
	int    VirtualLinesLimit;
	//
	int    XresMin;
	int    YresUserMin;
	int    YresMin;
	int    YresMax;
	//
	double VerticalBlank;
	//
	struct ModeLine modeLine;
} MonitorMode;

typedef struct GameInfo {
	char   name[256];
	int    o_width;
	int    o_height;
	double o_refresh;
	int    width;
	int    height;
	int    depth;
	double refresh;
	int    orientation;
	int    screens;
	int    vector;
	char   resolution[255];
} GameInfo;

typedef struct Resolution {
	int	width;
	int	height;
	double	refresh;
	int	orientation;
	int	changeres;
	int	count;
} Resolution;

typedef struct SwitchRes {
	struct ConfigSettings cs;
	struct GameInfo gameInfo;
	struct Resolution resolution;
	struct MonitorMode monitorModes[MAX_MODES];
	struct MonitorMode *monitorMode;
	struct ModeLine *modeLine;
	struct ModeLine bestMode;
	struct ModeLine lastMode;
	struct ModeLine videoModes[MAX_MODELINES];
	int modecount;
} SwitchRes;

/* Modeline Creation */
int ModelineCreate(ConfigSettings *cs, GameInfo *game, MonitorMode *monitor, ModeLine *mode);
int ModelineGetLineParams(ModeLine *mode, MonitorMode *monitor);
int ResVirtualize(ModeLine *mode, MonitorMode *monitor);

/* Modeline Utilities */
char * PrintModeline(ModeLine *mode, char *modeline);
int ModelineResult(ModeLine *mode, ConfigSettings *cs);

/* Monitor */
int SetMonitorMode(char *type, MonitorMode *monitor);
int ShowMonitorMode(MonitorMode *monitor);
int FillMonitorMode(const char *line, MonitorMode *monitor);

/* Utilities */
double Normalize(double a, double b);

/* Main */
void calc_modeline(running_machine &machine);
int switchres_calc_modeline(running_machine &machine);
void SetMameOptions(running_machine &machine);
void CalculateMameOptions(running_machine &machine);

#endif
