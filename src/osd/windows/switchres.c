// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <tchar.h>
#include <wingdi.h>

// standard C headers
#include <ctype.h>
#include <stdarg.h>
#include <psapi.h>
#include <dbghelp.h>

// MAME headers
#include "emu.h"
#include "clifront.h"
#include "emuopts.h"

// MAMEOS headers
#include "winmain.h"
#include "window.h"
#include "video.h"
#include "sound.h"
#include "input.h"
#include "output.h"
#include "config.h"
#include "osdepend.h"
#include "strconv.h"
#include "winutf8.h"
#include "winutil.h"
#include "debugger.h"

#define DM_INTERLACED 0x00000002
#define WM_USER_CHANGERES                      (WM_USER + 8)

static void swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

static DWORD RealRes( DWORD x) {
        return (int) (x / 8) * 8;
}

//============================================================
//  reg_query_string
//============================================================

static TCHAR *reg_query_string(HKEY key, const TCHAR *path)
{
	TCHAR *buffer;
	DWORD datalen;
	LONG result;

	// first query to get the length
	result = RegQueryValueEx(key, path, NULL, NULL, NULL, &datalen);
	if (result != ERROR_SUCCESS)
		return NULL;

	// allocate a buffer
	buffer = global_alloc_array(TCHAR, datalen + sizeof(*buffer));
	buffer[datalen / sizeof(*buffer)] = 0;

	// now get the actual data
	result = RegQueryValueEx(key, path, NULL, NULL, (LPBYTE)buffer, &datalen);
	if (result == ERROR_SUCCESS)
		return buffer;

	// otherwise return a NULL buffer
	global_free(buffer);
	return NULL;
}

static void ResetVideoModes(void) {
	int iModeNum = 0;
	HDC hDC;
	DEVMODE lpDevMode;
	LPCTSTR lpDriverName, lpDeviceName;
	
	lpDriverName = TEXT("DISPLAY");

	hDC = CreateIC ( lpDriverName, NULL, NULL, NULL );

	lpDevMode.dmSize = sizeof(DEVMODE);
	lpDeviceName = TEXT("\\\\.\\Display1");

	while (EnumDisplaySettings( NULL, iModeNum, &lpDevMode ) != 0)
		iModeNum++;
		
	DeleteDC(hDC);
	
	return;
}

static int GetAvailableVideoModes(ModeLine VideoMode[MAX_MODELINES]) {
	int iModeNum = 0, k = 0;
	int i, j;
	HDC hDC;
	DEVMODE lpDevMode;
	LPCTSTR lpDriverName, lpDeviceName;
	int DesktopBPP;
	ModeLine DesktopMode;

	lpDriverName = TEXT("DISPLAY");

	hDC = CreateIC ( lpDriverName, NULL, NULL, NULL );
	DesktopMode.hactive = GetDeviceCaps ( hDC, HORZRES );
	DesktopMode.vactive = GetDeviceCaps ( hDC, VERTRES );
	DesktopBPP = GetDeviceCaps ( hDC, BITSPIXEL );
	DesktopMode.vfreq = GetDeviceCaps ( hDC, VREFRESH );

	lpDevMode.dmSize = sizeof(DEVMODE);
	lpDeviceName = TEXT("\\\\.\\Display1");

	while (EnumDisplaySettings( NULL, iModeNum, &lpDevMode ) != 0) {
		if (k > MAX_MODELINES) {
			mame_printf_error("SwitchRes: Warning, too many active modelines for storage %d\n",
				k);
		} else if (lpDevMode.dmBitsPerPel == 32) {
			memset(&VideoMode[k], 0, sizeof(struct ModeLine));
			VideoMode[k].hactive = lpDevMode.dmPelsWidth;
			VideoMode[k].vactive = lpDevMode.dmPelsHeight;
			VideoMode[k].vfreq = lpDevMode.dmDisplayFrequency;
			VideoMode[k].interlace =
					(lpDevMode.dmDisplayFlags & DM_INTERLACED)?1:0;
			VideoMode[k].desktop = 0;
			
			sprintf(VideoMode[k].label, "SYSTEMMODE%dx%dx0x%d", 
				VideoMode[k].hactive, VideoMode[k].vactive, (int)VideoMode[k].vfreq);
			VideoMode[k].a_width = VideoMode[k].hactive;
			VideoMode[k].a_height = VideoMode[k].vactive;
			VideoMode[k].a_vfreq = VideoMode[k].vfreq;

			if (VideoMode[k].hactive == DesktopMode.hactive &&
					VideoMode[k].vactive == DesktopMode.vactive &&
							VideoMode[k].vfreq == DesktopMode.vfreq)
			{
					VideoMode[k].desktop = 1;
			}

			k++;
		}
		iModeNum++;
	}
	
	DeleteDC(hDC);
	
	for (i = 0; i < k; i++) {
		for (j = i; j < k; j++) {
			if (RealRes ( VideoMode[j].hactive ) <
					RealRes ( VideoMode[i].hactive ) ||
			( RealRes ( VideoMode[j].hactive ) == RealRes (
					VideoMode[i].hactive ) &&
							VideoMode[j].vactive <
									VideoMode[i].vactive ) ||
			( RealRes ( VideoMode[j].hactive ) == RealRes (
					VideoMode[i].hactive ) &&
							VideoMode[j].vactive ==
									VideoMode[i].vactive &&
					 VideoMode[j].vfreq < VideoMode[i].vfreq ) ||
			( RealRes ( VideoMode[j].hactive ) == RealRes (
							VideoMode[i].hactive ) &&
					VideoMode[j].vactive == VideoMode[i].vactive &&
					VideoMode[j].vfreq == VideoMode[i].vfreq &&
							VideoMode[j].hactive < VideoMode[i].hactive ))
			{
					swap(&VideoMode[j].hactive, &VideoMode[i].hactive);
					swap(&VideoMode[j].vactive, &VideoMode[i].vactive);
					swap((int*)&VideoMode[j].vfreq, (int*)&VideoMode[i].vfreq);
			}
		}
	}
	
	return k;
}

static int CustomModeDataWord(int i, char *lpData) {
	char out[32] = "";
	int x;

	sprintf(out, "%02X%02X", lpData[i]&0xFF, lpData[i+1]&0xFF);
	sscanf(out, "%d", &x);
	return x;
}

static int CustomModeDataWordBCD(int i, char *lpData) {
	char out[32] = "";
	int x;

	sprintf(out, "%02X%02X", lpData[i]&0xFF, lpData[i+1]&0xFF);
	sscanf(out, "%04X", &x);
	return x;
}

static void SetCustomModeDataWord (char *DataString, long DataWord, int offset) {
	int DataLow, DataHigh;

	if (DataWord < 65536) {
		DataLow = DataWord % 256;
		DataHigh = DataWord / 256;
		DataString[offset] = DataHigh;
		DataString[offset+1] = DataLow;
	}
}

static void SetCustomModeDataWordBCD (char *DataString, long DataWord, int offset) {
	if (DataWord < 10000) {
		int DataLow, DataHigh;
		int a, b;
		char out[32] = "";

		DataLow = DataWord % 100;
		DataHigh = DataWord / 100;

		sprintf(out, "%d %d", DataHigh, DataLow);
		sscanf(out, "%02X %02X", &a, &b);

		DataString[offset] = a;
		DataString[offset+1] = b;
	}
}

static int SetCustomVideoModes(ConfigSettings *cs, ModeLine *VideoMode, ModeLine *mode, int reset) {
	HKEY hKey;
	LONG lRes;
	TCHAR	dv[1024];
	TCHAR *DefaultVideo = NULL;
	DWORD type;
	char lpValueName[1024];
	char lpData[1024];
	int hhh = 0, hhi = 0, hhf = 0, hht = 0, vvv = 0, vvi = 0, vvf = 0, vvt = 0, interlace = 0;
	double dotclock = 0;
	long checksum;
	int old_checksum;
	int i;

	if (!strcmp(VideoMode->label, "") || !VideoMode->custom)
		return 0;
	if (reset && VideoMode->modified != 1) {
		mame_printf_error("SwitchRes: Error, the %s registry modeline was never modified!!!\n",
			VideoMode->label);
		return -1;
	} else if (!reset && VideoMode->modified == 1) {
		mame_printf_error("SwitchRes: Error, the %s registry modeline has already been modified!!!\n",
			VideoMode->label);
		return -1;
	}
	
	mame_printf_verbose("SwitchRes: %s modeline registry entry for %s\n",
		reset?"Resetting":"Setting", VideoMode->label);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\VIDEO"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
		TCHAR *chsrc, *chdst;	
		
		DefaultVideo = reg_query_string(hKey, TEXT("\\Device\\Video0"));
		RegCloseKey(hKey);

		if (DefaultVideo == NULL)
			return -1;
		
		chdst = dv;
		for (chsrc = DefaultVideo + 18; *chsrc != 0; chsrc++)
			*chdst++ = *chsrc;
		*chdst = 0;
	} else {
		mame_printf_info("SwitchRes: Failed opening DefaultVideo registry\n");
		return -1;
	}

	sprintf(lpValueName, "%s", VideoMode->label);

	for(i=0; i < VideoMode->regdata_size; i++) {
		lpData[i] = VideoMode->regdata[i];
		if (cs->verbose > 4)
			mame_printf_verbose("[%02X]", lpData[i]);
	}
	if (cs->verbose > 4)
		mame_printf_verbose("\n");

	SetCustomModeDataWordBCD (lpData, (int)mode->pclock/10000, 38);
	SetCustomModeDataWordBCD (lpData, mode->hactive, 10);
	SetCustomModeDataWordBCD (lpData, mode->hbegin, 14);
	SetCustomModeDataWordBCD (lpData, mode->hend - mode->hbegin, 18);
	SetCustomModeDataWordBCD (lpData, mode->htotal, 6);
	SetCustomModeDataWordBCD (lpData, mode->vactive, 26);
	SetCustomModeDataWordBCD (lpData, mode->vbegin, 30);
	SetCustomModeDataWordBCD (lpData, mode->vend - mode->vbegin, 34);
	SetCustomModeDataWordBCD (lpData, mode->vtotal, 22);

	if (mode->interlace)
		lpData[3] = 0x0e;
	else
		lpData[3] = 0x0c;

	dotclock = (double)CustomModeDataWord(38, lpData);
	hhh = CustomModeDataWord(10, lpData);
	hhi = CustomModeDataWord(14, lpData);
	hhf = CustomModeDataWord(18, lpData) + hhi;
	hht = CustomModeDataWord(6, lpData);
	vvv = CustomModeDataWord(26, lpData);
	vvi = CustomModeDataWord(30, lpData);
	vvf = CustomModeDataWord(34, lpData) + vvi;
	vvt = CustomModeDataWord(22, lpData);
	interlace = (lpData[3] == 0x0e)?1:0;

	old_checksum = CustomModeDataWordBCD(66, lpData);

	checksum = 65535 - ((lpData[3] == 0x0e)?0x0e:0x0c) - hht - hhh - hhf - vvt - vvv - vvf - dotclock;

	SetCustomModeDataWord (lpData, checksum, 66);

	for(i=0; i < VideoMode->regdata_size; i++) {
		if (cs->verbose > 4)
			mame_printf_verbose("[%02X]", lpData[i]);
	}
	if (cs->verbose > 4)
		mame_printf_verbose("\n");

	if (cs->verbose) { 
		mame_printf_verbose("SwitchRes: Set Registry mode %s with:\n", VideoMode->label);
		mame_printf_verbose("SwitchRes: (%d/%d/%ld) Modeline %.6f %d %d %d %d %d %d %d %d%s\n",
			CustomModeDataWordBCD(66, lpData), 
			old_checksum, checksum,
			(double)((double)dotclock * 10000.0)/1000000.0, 
			(int)RealRes (hhh), (int)RealRes (hhi), 
			(int)RealRes (hhf), (int)RealRes (hht), 
			vvv, vvi, vvf, vvt, (interlace)?" interlace":"");
	}

	if ((lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, dv, 0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS) {
		type = REG_BINARY;

		// Write registry entry here	
		if (RegSetValueExA(hKey, lpValueName, 0, type, (LPBYTE)lpData, VideoMode->regdata_size) != ERROR_SUCCESS)
			mame_printf_info("SwitchRes: Failed saving registry entry for %s modeline\n", VideoMode->label);

		RegCloseKey(hKey);
	} else {
		mame_printf_info("SwitchRes: Failed opening %s registry entry with error %d\n", (char*)dv, (int)lRes);
	}
	
	if (DefaultVideo != NULL)
		global_free(DefaultVideo);
	if (reset)
		VideoMode->modified = 0;
	else
		VideoMode->modified = 1;
	return 0;
}

static int GetCustomVideoModes(ConfigSettings *cs, ModeLine VideoMode[MAX_MODELINES]) {
	HKEY hKey;
	int dwIndex = 0, j = -1;
	int  hactive, vactive, vfreq;
	LONG lRes;
	TCHAR dv[1024];
	TCHAR *DefaultVideo = NULL;
	DWORD type;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\VIDEO"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
		TCHAR *chsrc, *chdst;
		
		DefaultVideo = reg_query_string(hKey, TEXT("\\Device\\Video0"));
		RegCloseKey(hKey);
		
		if (DefaultVideo == NULL) {
			mame_printf_error("SwitchRes: Failed opening \\Device\\Video0 registry\n");
			return -1;
		}

		chdst = dv;

		for (chsrc = DefaultVideo + 18; *chsrc != 0; chsrc++)
				*chdst++ = *chsrc;
		*chdst = 0;
	} else {
		mame_printf_error("SwitchRes: Failed opening DefaultVideo registry\n");
		return -1;
	}

	if (cs->verbose)
		mame_printf_verbose("SwitchRes: DefaultVideo '%s'\n", utf8_from_tstring(dv));

	if ((lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, dv, 0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS) {
		type = 0;
		TCHAR lpValueName[1024];
		char lpData[1024];
		DWORD lpcValueName = 1024;
		DWORD lpcData = 1024;

		while (RegEnumValue (hKey, dwIndex, lpValueName, &lpcValueName, NULL, &type, (LPBYTE)lpData, &lpcData) != ERROR_NO_MORE_ITEMS) {
			dwIndex++;

			if (_tcsstr(lpValueName, TEXT("DALDTMCRTBCD"))) {
				int hhh = 0, hhi = 0, hhf = 0, hht = 0, vvv = 0, vvi = 0, vvf = 0, vvt = 0, interlace = 0;
				double dotclock = 0;
				int checksum;
				int i = 0, k = 0;
				int active = 0;
				
				if (cs->verbose)
					mame_printf_verbose("SwitchRes: %s:\n ", utf8_from_tstring(lpValueName));				

				dotclock = (double)CustomModeDataWord(38, lpData);
				hhh = (int)RealRes(CustomModeDataWord(10, lpData));
				hhi = (int)RealRes(CustomModeDataWord(14, lpData));
				hhf = (int)RealRes(CustomModeDataWord(18, lpData)) + hhi;
				hht = (int)RealRes(CustomModeDataWord(6, lpData));
				vvv = CustomModeDataWord(26, lpData);
				vvi = CustomModeDataWord(30, lpData);
				vvf = CustomModeDataWord(34, lpData) + vvi;
				vvt = CustomModeDataWord(22, lpData);
				interlace = (lpData[3] == 0x0e)?1:0;

				checksum = CustomModeDataWordBCD(66, lpData);

				if (cs->verbose)
					mame_printf_verbose("SwitchRes: (%d/%d) Modeline %.6f %d %d %d %d %d %d %d %d%s\n",
						checksum, (int)lpcData, (double)((double)dotclock * 10000.0)/1000000.0, 
						(int)RealRes (hhh), (int)RealRes (hhi), (int)RealRes (hhf), (int)RealRes (hht), 
						vvv, vvi, vvf, vvt, (interlace)?" interlace":"");

				if (sscanf(utf8_from_tstring(lpValueName), "DALDTMCRTBCD%dx%dx0x%d", &hactive, &vactive, &vfreq) != 3) {
					if (sscanf(utf8_from_tstring(lpValueName), "DALDTMCRTBCD%dX%dX0X%d", &hactive, &vactive, &vfreq) != 3) {
							mame_printf_info("SwitchRes: Failed getting resolution values from %s\n", utf8_from_tstring(lpValueName));
							continue;
					}
				}
				
				for (k = 0; k < MAX_MODELINES; k++) {
					if (VideoMode[k].hactive == hactive &&
						VideoMode[k].vactive == vactive &&
						VideoMode[k].vfreq == vfreq)
					{
						active = 1;
						break;
					}
				}

				if (active) {
					sprintf(VideoMode[k].name, "%dx%d@%d", hactive, vactive, vfreq);

					VideoMode[k].a_width = hactive;
					VideoMode[k].a_height = vactive;

					sprintf(VideoMode[k].resolution, "%dx%d@%d", hactive, vactive, vfreq);
					VideoMode[k].vfreq = vfreq;
					VideoMode[k].a_vfreq = (double)(dotclock * 10000.0) / (vvt * hht);
					VideoMode[k].pclock  = dotclock * 10000;
					VideoMode[k].hactive = hhh;
					VideoMode[k].hbegin  = hhi;
					VideoMode[k].hend    = hhf;
					VideoMode[k].htotal  = hht;
					VideoMode[k].vactive = vvv;
					VideoMode[k].vbegin  = vvi;
					VideoMode[k].vend    = vvf;
					VideoMode[k].vtotal  = vvt;
					VideoMode[k].interlace  = interlace;
					VideoMode[k].doublescan  = 0;
					VideoMode[k].custom  = 1;

					for(i=0; i < lpcValueName; i++) {
						VideoMode[k].label[i] = lpValueName[i];
					}
					VideoMode[k].regdata_size = lpcData;
					
					for(i=0; i < VideoMode[k].regdata_size; i++) {
						VideoMode[k].regdata[i] = lpData[i];
						if (cs->verbose > 4)
							mame_printf_verbose("[%02X]", lpData[i]);
					}
					if (cs->verbose > 4)
						mame_printf_verbose("\n");
					
					j++;
				}
			}

			lpcValueName = 1024;
			lpcData = 1024;
		} 

		RegCloseKey(hKey);
	} else {
		mame_printf_error("SwitchRes: Failed opening %s registry entry with error %d\n", utf8_from_tstring(dv), (int)lRes);
		j = -1;
	}
		
	if (DefaultVideo != NULL)
		global_free(DefaultVideo);

	return j;
}

static int findBestMode(int orientation, ModeLine *mode, ModeLine modes[MAX_MODELINES], ModeLine *bestmode) {
	int i;
	int index = -1;
	int hactive, vactive, vfreq;
	int best_index = -1;
	int best_interlace_index = -1;
	int best_custom_index = -1;
	int best_system_index = -1;

	// First try an exact match
	for (i = 0; i < MAX_MODELINES; i++) {
		if (!modes[i].hactive)
			break;
		if (modes[i].custom && sscanf(modes[i].label, "DALDTMCRTBCD%dx%dx0x%d", &hactive, &vactive, &vfreq) != 3) {
			if (sscanf(modes[i].label, "DALDTMCRTBCD%dX%dX0X%d", &hactive, &vactive, &vfreq) != 3) {
				hactive = modes[i].hactive;
				vactive = modes[i].vactive;
				vfreq = modes[i].a_vfreq;
				mame_printf_verbose("SwitchRes: Failed scanning registry modeline label %s, using values %dx%dinstead\n",
					modes[i].label, hactive, vactive);
			}
		} else {
			hactive = modes[i].hactive;
			vactive = modes[i].vactive;
			vfreq = modes[i].vfreq;
		}

		// Set score to 0 first
		modes[i].score = 0;

		// Check for exact match first, then double width, then double height
		if (hactive == mode->hactive && vactive == mode->vactive) {
			// width && height match
			modes[i].score += 100;
		} else if (hactive == (2*mode->hactive) && vactive == mode->vactive) {
			// width x 2 && height match
			modes[i].score += 90;
		} else if (hactive == mode->hactive && vactive == (2*mode->vactive)) {
			// width && height x 2 match
			modes[i].score += 80;
		} else if (hactive == (2*mode->hactive) && vactive == (2*mode->vactive)) {
			// width x 2 && height x 2 match
			modes[i].score += 70;
		} else if (vactive >= mode->vactive) {
			// Fuzzy match
			double hdiff = abs(mode->hactive - hactive) * .01;
			double vdiff = abs(mode->vactive - vactive) * .01;

			modes[i].score += 50;

			if (hactive >= mode->hactive && vactive >= mode->vactive)
				modes[i].score += 5; // width and height larger
			else if (orientation && vactive >= mode->vactive && hactive < mode->hactive)
				modes[i].score += 5; // Vertical game, height larger, width smaller

			// height is exact match
			if (vactive == mode->vactive)
				modes[i].score += 5; 

			// Reduce score depending on extra width and height
			modes[i].score = modes[i].score - hdiff - vdiff;
		} 
	}

	// find best score
	for (i = 0; i < MAX_MODELINES; i++) {
		if (!modes[i].hactive)
			break;
		if (modes[i].score > 0) {
			if (best_interlace_index == -1 && modes[i].interlace)
				best_interlace_index = i;
			else if (best_custom_index == -1 && modes[i].custom)
				best_custom_index = i;
			else if (best_system_index == -1 && !modes[i].custom)
				best_system_index = i;
			else if (modes[i].interlace) {
				// Interlaced
				if (modes[i].custom) {
					// Custom
					if (modes[i].score > modes[best_custom_index].score)
						best_custom_index = i;
				} else {
					// System
					if (modes[i].score > modes[best_system_index].score)
						best_system_index = i;
				}
				if (modes[i].score > modes[best_interlace_index].score)
					best_interlace_index = i;
			} else {
				// Progressive
				if (modes[i].custom) {
					// Custom
					if (modes[i].score > modes[best_custom_index].score)
						best_custom_index = i;
				} else {
					// System
					if (modes[i].score > modes[best_system_index].score)
						best_system_index = i;
				}
				
			}
		}
		mame_printf_verbose("SwitchRes: %d x %d -> %.02f %s\n",
			modes[i].hactive, modes[i].vactive, modes[i].score,
			modes[i].custom?"Custom Modeline":"System Modeline");
	}
	if (best_custom_index != -1)
		best_index = best_custom_index;
	else if (best_interlace_index != -1 && modes[best_interlace_index].custom)
		best_index = best_interlace_index;
	else if (best_system_index != -1)
		best_index = best_system_index;
	else if (best_interlace_index != -1)
		best_index = best_interlace_index;

	if (best_index == -1)
		return -1;

	index = best_index;
	if (modes[index].modified != 1)
		memcpy((ModeLine*)bestmode, (ModeLine*)&modes[index], sizeof(ModeLine));

	return index;
}


//============================================================
// switchres_modeline_remove
//
//============================================================

bool switchres_modeline_remove(running_machine &machine)
{
	SetCustomVideoModes(&machine.switchRes.cs, &machine.switchRes.bestMode, &machine.switchRes.bestMode, 1);
	ResetVideoModes();

	return true;
}


//============================================================
// switchres_modeline_setup
//
//============================================================
bool switchres_modeline_setup(running_machine &machine)
{
        ModeLine *bestMode; 
        char modeline[1024]={'\x00'};
        bool success = false;
        int got_res = 0;

	mame_printf_verbose("SwitchRes: Entering switchres_modeline_setup (%d)\n",
		machine.switchRes.resolution.count);

	bestMode = &machine.switchRes.bestMode;

	windows_options &options = downcast<windows_options &>(machine.options());
	astring error_string;

	if (!machine.switchRes.resolution.count) {
		strcpy(machine.switchRes.gameInfo.resolution, options.resolution());
        	machine.switchRes.cs.monitorcount = options.numscreens();
		machine.switchRes.cs.doublescan = 0;
		if (!strcmp(options.video(), "d3d")) {
			machine.switchRes.cs.cleanstretch = 1;
			options.set_value(OPTION_CLEANSTRETCH, true, OPTION_PRIORITY_MAXIMUM, error_string);
                        mame_printf_verbose("SwitchRes: Setting Option -cleanstretch\n");
		}
	}

        // Save old modeline first if it exists
        if (machine.switchRes.resolution.count > 0) {
                memcpy(&machine.switchRes.lastMode, bestMode, sizeof(ModeLine));
                mame_printf_verbose("SwitchRes: Copy lastMode name %s\n",
                        machine.switchRes.lastMode.name);
        }

        // Generate modeline
        switchres_calc_modeline(machine);

        if (!machine.switchRes.modeLine) {
                mame_printf_error("SwitchRes: Modeline was NULL!!!\n");
                return false;
        }

        // Initially get the registry modelines
        if (machine.switchRes.modecount == 0) {
		int custom_count = 0;
				
		machine.switchRes.modecount = GetAvailableVideoModes(machine.switchRes.videoModes);			                
                custom_count = GetCustomVideoModes(&machine.switchRes.cs,
			machine.switchRes.videoModes);
                mame_printf_verbose("SwitchRes: Found %d custom of %d active modelines\n",
                        custom_count, machine.switchRes.modecount);
        }

        // If we got any, check for the best modeline
        if (machine.switchRes.modecount > 0) {
		int orientation = 0;
		if (machine.switchRes.gameInfo.orientation && !strcmp(machine.switchRes.cs.morientation, "horizontal"))
			orientation = 1;
                got_res = findBestMode(orientation, machine.switchRes.modeLine,
                        machine.switchRes.videoModes,
                        bestMode);
                mame_printf_verbose("SwitchRes: Index %d/%d modeline %s score %.02f %s\n",
                        got_res, machine.switchRes.modecount, bestMode->label, bestMode->score,
			(got_res >= 0)?"matches":"has no match");
        } else {
                got_res = -1;
                machine.switchRes.modecount = -1;
                mame_printf_error("SwitchRes: Didn't get any system or custom modelines %d\n",
                        got_res);
        }

        // If we got a best mode, setup for use
        if (got_res != -1) {
		int use_ini = strcmp(machine.switchRes.gameInfo.resolution, "auto")?1:0;
		bool virtualize = false;

                // Got a registry modeline to change refresh rate of
                mame_printf_verbose("SwitchRes: Got %s modeline %s - %s:\n\t%s\n",
                        bestMode->custom?"Custom":"System",
			bestMode->resolution, bestMode->label,
                        PrintModeline(bestMode, modeline));

                // Tweak modeline if height/width changed
                if (machine.switchRes.modeLine->hactive != bestMode->a_width
                    || machine.switchRes.modeLine->vactive != bestMode->a_height)
                {
			int minwidth = machine.switchRes.modeLine->hactive;

			// Turn off .ini file support, since we can't do that 
			use_ini = 0;

                        // Use values from registry
                        machine.switchRes.gameInfo.width = bestMode->a_width;
                        machine.switchRes.gameInfo.height = bestMode->a_height;

			mame_printf_verbose("SwitchRes: Trying to recalculate modeline %d x %d != %d x %d\n",
				machine.switchRes.modeLine->hactive,
				machine.switchRes.modeLine->vactive,
				bestMode->a_width, bestMode->a_height);

                        // Generate new modeline
                        ModelineCreate(&machine.switchRes.cs,
                                &machine.switchRes.gameInfo,
                                machine.switchRes.monitorMode,
                                machine.switchRes.modeLine);
                        machine.switchRes.modeLine->weight =
                                ModelineResult(machine.switchRes.modeLine,
                                &machine.switchRes.cs);

			// Double check the height/width, recalculate if necessary
                	if (machine.switchRes.modeLine->hactive != bestMode->a_width
                    		|| machine.switchRes.modeLine->vactive != bestMode->a_height) 
			{
				mame_printf_verbose("SwitchRes: Trying again to recalculate modeline %d x %d != %d x %d\n",
					machine.switchRes.modeLine->hactive,
					machine.switchRes.modeLine->vactive,
					bestMode->a_width, bestMode->a_height);

        			if (machine.switchRes.modecount > 0) {
					int orientation = 0;
					int idx = 0;
					if (machine.switchRes.gameInfo.orientation 
						&& !strcmp(machine.switchRes.cs.morientation, "horizontal"))
							orientation = 1;

                			idx = findBestMode(orientation, machine.switchRes.modeLine,
                        				machine.switchRes.videoModes, bestMode);

                			mame_printf_verbose("SwitchRes: Recalculated index %d/%d modeline %s score %.02f %s\n",
                        			idx, machine.switchRes.modecount, bestMode->label, bestMode->score,
						(got_res >= 0)?"matches":"has no match");

					if (idx) {
						if (bestMode->custom) {
					        	mame_printf_verbose("SwitchRes: Got %s modeline %s - %s:\n\t%s\n",
								bestMode->custom?"Custom":"System",
								bestMode->resolution, bestMode->label,
								PrintModeline(bestMode, modeline));
						} else {
					        	mame_printf_verbose("SwitchRes: Got %s modeline %s\n",
								bestMode->custom?"Custom":"System",
								bestMode->label);
						}
                        			machine.switchRes.gameInfo.width = bestMode->a_width;
                        			machine.switchRes.gameInfo.height = bestMode->a_height;
					} else
						virtualize = true;
        			} else
					virtualize = true;
				if (virtualize)
					mame_printf_error("SwitchRes: Failed at finding a good modeline!!!\n");
			}

			// Check if aspect ratio needs to be fixed
			if (bestMode->a_width > minwidth) {
                        	mame_printf_verbose("SwitchRes: Setting Option -keepaspect\n");
				options.set_value(WINOPTION_KEEPASPECT, true, OPTION_PRIORITY_MAXIMUM, error_string);
			} 

                        PrintModeline(machine.switchRes.modeLine, modeline);
                        mame_printf_verbose("SwitchRes: New Modeline: \tModeLine     %s\n",
                                modeline);
                }
		
                // Set new resolution settings and reload it into the system
		if (!bestMode->custom || virtualize) {
			machine.switchRes.modeLine->result |= RESULT_VFREQ_CHANGE;
			success = true;
                } else if (!SetCustomVideoModes(&machine.switchRes.cs,
                        		bestMode, machine.switchRes.modeLine, 0))
                {
			ResetVideoModes();
                        success = true;
                } else {
                        machine.switchRes.modeLine->result |= RESULT_VIRTUALIZE;
                        mame_printf_error("SwitchRes: Failed setting modeline!!!\n");
                        success = false;
                }

                // Calculate flags to set options
                CalculateMameOptions(machine);

                // If not virtualizing, do 1:1 scaling from original for height/width
                if (!(machine.switchRes.modeLine->result & RESULT_VIRTUALIZE)) {
                        if ((machine.switchRes.modeLine->result & (RESULT_RES_INC|RESULT_RES_DEC)))
                                virtualize = true;
                } else
                        virtualize = true;

                if (virtualize) {
                        // Adjust settings if stretched resolution
                        mame_printf_verbose("SwitchRes: Setting Option -hwstretch\n");
			options.set_value(WINOPTION_HWSTRETCH, true, OPTION_PRIORITY_MAXIMUM, error_string);
                        mame_printf_verbose("SwitchRes: Setting Option -keepaspect\n");
			options.set_value(WINOPTION_KEEPASPECT, true, OPTION_PRIORITY_MAXIMUM, error_string);

                        // Aspect ratio specified
                        if ((machine.switchRes.gameInfo.orientation && !strcmp(machine.switchRes.cs.morientation, "horizontal")) ||
                            (!machine.switchRes.gameInfo.orientation && !strcmp(machine.switchRes.cs.morientation, "vertical")))
                        {
                                mame_printf_verbose("SwitchRes: Setting Option -screen_aspect %s\n", machine.switchRes.cs.aspect);
				options.set_value(WINOPTION_ASPECT, machine.switchRes.cs.aspect, OPTION_PRIORITY_MAXIMUM, error_string);
                        }
                }

		// If interlacing, we need the filter on
		if (bestMode->interlace) {
                        mame_printf_verbose("SwitchRes: Setting Option -filter\n");
			options.set_value(WINOPTION_FILTER, true, OPTION_PRIORITY_MAXIMUM, error_string);
		}

		// Only use soundsync if set by user and also vsync doesn't match, else turn it off
               	if (machine.switchRes.modeLine->result & RESULT_VFREQ_CHANGE) {
               		if (machine.options().soundsync()) {
                       		mame_printf_verbose("SwitchRes: Setting Option -soundsync\n");
                       		machine.switchRes.cs.soundsync = 1;
			}
		} else {
                	mame_printf_verbose("SwitchRes: Setting Option -nosoundsync\n");
                	machine.switchRes.cs.soundsync = 0;
		}

		if ((machine.switchRes.modeLine->result & RESULT_VFREQ_CHANGE && !options.sync_refresh()) || options.triple_buffer()) {
			// Resolution doesn't match refresh rate
			mame_printf_verbose("SwitchRes: Disabling VSYNC\n");
			mame_printf_verbose("SwitchRes: Setting Option -throttle\n");
			options.set_value(OPTION_THROTTLE, true, OPTION_PRIORITY_MAXIMUM, error_string);
			mame_printf_verbose("SwitchRes: Setting Option -nowaitvsync\n");
			options.set_value(WINOPTION_WAITVSYNC, false, OPTION_PRIORITY_MAXIMUM, error_string);
		} else {
			// Resolution matches refresh rate
			mame_printf_verbose("SwitchRes: Enabling VSYNC\n");
			mame_printf_verbose("SwitchRes: Setting Option -nothrottle\n");
			options.set_value(OPTION_THROTTLE, false, OPTION_PRIORITY_MAXIMUM, error_string);
			mame_printf_verbose("SwitchRes: Setting Option -waitvsync\n");
			options.set_value(WINOPTION_WAITVSYNC, true, OPTION_PRIORITY_MAXIMUM, error_string);
		}

                // Force resolution
                if (!use_ini) {
                        sprintf(machine.switchRes.bestMode.resolution, "%dx%d@%d",
                                machine.switchRes.bestMode.hactive, machine.switchRes.bestMode.vactive,
                                (int)machine.switchRes.bestMode.vfreq);

                        mame_printf_verbose("SwitchRes: Setting Option -resolution %s\n", machine.switchRes.bestMode.resolution);
			options.set_value(WINOPTION_RESOLUTION, machine.switchRes.bestMode.resolution, 
				OPTION_PRIORITY_MAXIMUM, error_string);

                        // Setup video config resolution parameters screen 1
                        video_config.window[0].refresh = (int)machine.switchRes.bestMode.vfreq;
                        video_config.window[0].width = machine.switchRes.bestMode.a_height;
                        video_config.window[0].height = machine.switchRes.bestMode.a_width;
                } else
                        mame_printf_verbose("SwitchRes: INI File resolution: %dx%d@%d\n",
                                video_config.window[0].width, video_config.window[0].height,
                                video_config.window[0].refresh);

                machine.switchRes.cs.cleanstretch = machine.options().cleanstretch();

                machine.switchRes.resolution.count++;
        } else {
                mame_printf_error("SwitchRes: Couldn't find a working resolution for %dx%d@%d\n",
                        machine.switchRes.modeLine->hactive,
                        machine.switchRes.modeLine->vactive,
                        (int)machine.switchRes.gameInfo.refresh);
        }
        return success;
}

//============================================================
// switchres_resolution_change
//
//============================================================

bool switchres_resolution_change(win_window_info *window) 
{
        window->machine().switchRes.resolution.changeres = 0;
        if (window->machine().options().modeline()) {
                mame_printf_verbose("SwitchRes: Resolution change to %dx%d@%d\n",
                        window->machine().switchRes.resolution.width,
                        window->machine().switchRes.resolution.height,
                        (int)window->machine().switchRes.resolution.refresh);

                switchres_modeline_setup(window->machine());

                window->maxwidth =
                        window->machine().switchRes.bestMode.a_width;
                window->maxheight =
                        window->machine().switchRes.bestMode.a_height;

                // Reset old video modeline
                if (window->machine().switchRes.resolution.count > 1 
			&& (window->machine().switchRes.lastMode.a_height != 
				window->machine().switchRes.bestMode.a_height 
			|| window->machine().switchRes.lastMode.a_width !=
				window->machine().switchRes.bestMode.a_width)) 
		{
                        SetCustomVideoModes(&window->machine().switchRes.cs,
                                &window->machine().switchRes.lastMode,
                                &window->machine().switchRes.lastMode, 1);
			ResetVideoModes();
                }
        } else {
                window->maxwidth =
                        window->machine().switchRes.resolution.width;
                window->maxheight =
                        window->machine().switchRes.resolution.height;
        }

        // Change resolution
        SendMessage(window->hwnd, WM_USER_CHANGERES, 0, 0);

	return true;
}

