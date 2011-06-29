// MAME headers
#include "osdepend.h"
#include "emu.h"
#include "clifront.h"
#include "emuopts.h"

// OSD headers
#include "video.h"
#include "input.h"
#include "output.h"
#include "osdsdl.h"
#include "sdlos.h"

extern void pick_best_mode(sdl_window_info *window, int *fswidth, int *fsheight);

#define XRANDR_ARGS ""

static int SetXrandrDisplay(const char *connector, ModeLine *mode) {
        char cmd[1024]={'\x00'};
        char modeline[255]={'\x00'};

        /* Add new modeline */
        sprintf(cmd, "xrandr %s --newmode %s", XRANDR_ARGS, PrintModeline(mode, modeline)); 
	mame_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

        /* Add modeline to interface */
        sprintf(cmd, "xrandr %s --addmode %s %s", XRANDR_ARGS, connector, mode->name);
	mame_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

        return 0;
}

static int DelXrandrDisplay(const char *connector, const char *name) {
        char cmd[1024]={'\x00'};

        /* Delete  modeline from interface */
        sprintf(cmd, "xrandr %s --delmode %s \"%s\"", XRANDR_ARGS, connector, name);
	mame_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

        /* remove modeline */
        sprintf(cmd, "xrandr %s --rmmode \"%s\"", XRANDR_ARGS, name);
	mame_printf_verbose("SwitchRes: Running '%s'\n", cmd);
	system(cmd);

        return 0;
}

//============================================================
// switchres_resolution_reset
//
//============================================================

bool switchres_resolution_reset(running_machine &machine, sdl_window_info *temp) {
	// Kick back to original screen resolution then to game resolution
	// This gets SDL to remember what the original resolution really was
	if (!temp) {
		mame_printf_error("SwitchRes: Error with switchres_resolution_reset(), window is NULL!!!\n");
		return false;
	}
	if (temp->fullscreen && video_config.switchres && machine.options().changeres()) {
		if (machine.switchRes.resolution.count > 1) {
			mame_printf_verbose("[%d] Switching from %dx%d to %dx%d to help SDL recover\n",
				machine.switchRes.resolution.count,
				temp->monitor->center_width, temp->monitor->center_height,
				temp->minwidth, temp->minheight);

			sdlwindow_resize(temp, temp->monitor->center_width, temp->monitor->center_height);
			sdlwindow_resize(temp, temp->minwidth, temp->minheight);
		}
	}
	return true;
}

//============================================================
// switchres_modeline_remove
//
//============================================================

bool switchres_modeline_remove(running_machine &machine) {
	char modeline[1024]={'\x00'};

	PrintModeline(machine.switchRes.modeLine, modeline);
		mame_printf_verbose("SwitchRes: Xrandr REMOVE %s: \tModeLine     %s\n",
		machine.switchRes.cs.connector, machine.switchRes.modeLine->name);

	// Remove modeline
	DelXrandrDisplay(machine.switchRes.cs.connector, machine.switchRes.modeLine->name);

	return true;
}

//============================================================
// switchres_modeline_setup
//
//============================================================

bool switchres_modeline_setup(running_machine &machine)
{
        bool success = true;
        bool virtualize = false;
        char modeline[1024]={'\x00'};

	sdl_options &options = downcast<sdl_options &>(machine.options());
	astring error_string;

	// Get .ini resolution if any
	if (!machine.switchRes.resolution.count) {
		strcpy(machine.switchRes.gameInfo.resolution, options.resolution());
		machine.switchRes.cs.keepaspect = 1;
	}

	// Get connector name
        if (!machine.switchRes.resolution.count && !strcmp(machine.switchRes.cs.connector, "auto")) {
                FILE *pi;
                char temp[255] = "xrandr -q | grep ' connected ' | awk '{print $1}' | head -1";
                pi=popen(temp, "r");
                if(pi != NULL) {
                        char c;
                        int i = 0;
                        c=fgetc(pi);
                        while(c != '\n' && i < 255) {
                                machine.switchRes.cs.connector[i++] = c;
                                c=fgetc(pi);
                        }
                        machine.switchRes.cs.connector[i] = '\0';
                        pclose(pi);
                        mame_printf_verbose("SwitchRes: Found output connector '%s'\n",
                                machine.switchRes.cs.connector);
                } else
                        mame_printf_error("SwitchRes: Error getting connector with xrandr");
        }

        if (machine.switchRes.modeLine) {
                ModeLine *lastMode;
                lastMode = machine.switchRes.modeLine;

                memcpy(&machine.switchRes.lastMode, lastMode, sizeof(ModeLine));
                mame_printf_verbose("SwitchRes: Copy lastMode name %s\n",
                        lastMode->name);
        }

        // Generate modeline
        switchres_calc_modeline(machine);

        // Calculate flags to set options
        CalculateMameOptions(machine);

        // If not virtualizing, do 1:1 scaling from original for height/width
        if (!(machine.switchRes.modeLine->result & RESULT_VIRTUALIZE)) {
                if ((machine.switchRes.modeLine->result & (RESULT_RES_INC|RESULT_RES_DEC)))
                        virtualize = true;
        } else
                virtualize = true;

        if (machine.switchRes.modeLine->result & RESULT_VFREQ_CHANGE) {
                mame_printf_verbose("SwitchRes: Setting Option -throttle\n");
                machine.options().set_value(OPTION_THROTTLE, true, OPTION_PRIORITY_MAXIMUM, error_string);
                mame_printf_verbose("SwitchRes: Setting Option -norefreshspeed\n");
                machine.options().set_value(OPTION_REFRESHSPEED, false, OPTION_PRIORITY_MAXIMUM, error_string);
		mame_printf_verbose("SwitchRes: Setting Option -nowaitvsync\n");
		options.set_value(SDLOPTION_WAITVSYNC, false, OPTION_PRIORITY_MAXIMUM, error_string);
        } else {
                mame_printf_verbose("SwitchRes: Setting Option -nothrottle\n");
                machine.options().set_value(OPTION_THROTTLE, false, OPTION_PRIORITY_MAXIMUM, error_string);
                mame_printf_verbose("SwitchRes: Setting Option -refreshspeed\n");
                machine.options().set_value(OPTION_REFRESHSPEED, true, OPTION_PRIORITY_MAXIMUM, error_string);
		mame_printf_verbose("SwitchRes: Setting Option -waitvsync\n");
		options.set_value(SDLOPTION_WAITVSYNC, true, OPTION_PRIORITY_MAXIMUM, error_string);
        }

        if (virtualize) {
                // Adjust settings if stretched resolution
                mame_printf_verbose("SwitchRes: Setting Option -unevenstretch\n");
		options.set_value(SDLOPTION_UNEVENSTRETCH, true, OPTION_PRIORITY_MAXIMUM, error_string);
                mame_printf_verbose("SwitchRes: Setting Option -filter\n");
		options.set_value(SDLOPTION_FILTER, true, OPTION_PRIORITY_MAXIMUM, error_string);
                mame_printf_verbose("SwitchRes: Setting Option -keepaspect\n");
		options.set_value(SDLOPTION_KEEPASPECT, true, OPTION_PRIORITY_MAXIMUM, error_string);

                // Aspect ratio specified
                if ((machine.switchRes.gameInfo.orientation && !strcmp(machine.switchRes.cs.morientation, "horizontal")) ||
                    (!machine.switchRes.gameInfo.orientation && !strcmp(machine.switchRes.cs.morientation, "vertical")))
                {
                        mame_printf_verbose("SwitchRes: Setting Option -screen_aspect %s\n", machine.switchRes.cs.aspect);
			options.set_value(SDLOPTION_ASPECT, machine.switchRes.cs.aspect, OPTION_PRIORITY_MAXIMUM, error_string);
                }
        }

        // Add resolution to display for SDL to pick
        PrintModeline(machine.switchRes.modeLine, modeline);
        mame_printf_verbose("SwitchRes: Xrandr ADD %s: \tModeLine     %s\n", machine.switchRes.cs.connector, modeline);
        SetXrandrDisplay(machine.switchRes.cs.connector, machine.switchRes.modeLine);

	if (!strcmp(machine.switchRes.gameInfo.resolution, "auto")) {
                // Force resolution
                sprintf(machine.switchRes.modeLine->resolution, "%dx%dx32@%f",
                        machine.switchRes.modeLine->hactive, machine.switchRes.modeLine->vactive, machine.switchRes.modeLine->vfreq);
                mame_printf_verbose("SwitchRes: Setting Option -resolution %s\n", machine.switchRes.modeLine->resolution);
		options.set_value(SDLOPTION_RESOLUTION, machine.switchRes.modeLine->resolution, OPTION_PRIORITY_MAXIMUM, error_string);
        } else
                mame_printf_verbose("SwitchRes: INI File resolution: %s\n",
			machine.switchRes.gameInfo.resolution);

        machine.switchRes.cs.cleanstretch = machine.options().cleanstretch();

        machine.switchRes.resolution.count++;

        return success;
}

//============================================================
// switchres_resolution_change
//
//============================================================

bool switchres_resolution_change(running_machine &machine, sdl_window_info *window, int width, int height) {
        SDL_Rect **modes;
        int i;
        bool found = false;
        int oldwidth, oldheight;

        // Set change resolution back to 0
        machine.switchRes.resolution.changeres = 0;

        oldwidth = machine.switchRes.gameInfo.width;
        oldheight = machine.switchRes.gameInfo.height;

        // Create new resolution
        if (machine.options().modeline())
                switchres_modeline_setup(machine);

        // Get resolution height and width
        if (machine.options().modeline()) {
                width = machine.switchRes.gameInfo.width;
                height = machine.switchRes.gameInfo.height;
        } else {
                width = machine.switchRes.resolution.width;
                height = machine.switchRes.resolution.height;
        }
        window->minwidth = width;
        window->minheight = height;

        // Reread resolutions, only works with hacked SDL
        modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_DOUBLEBUF);
        for (i = 0; modes[i]; ++i)
        {
                float weight = 0;

                // Make sure we setup a resolution for SDL to use
                if ((int)modes[i]->w == width)
                        weight += .5;
                if ((int)modes[i]->h == height)
                        weight += .5;
                if (weight == 1.0)
                        found = true;

                mame_printf_verbose("SwitchRes: %4dx%4d -> %f\n", (int)modes[i]->w, (int)modes[i]->h, weight);
        }

        // See if we got it setup right
        if (!found) {
                mame_printf_error("SwitchRes: Couldn't get %dx%d resolution, using best mode instead\n",
                        width, height);
                pick_best_mode(window, &width, &height);
        }
        sdlwindow_resize(window, width, height);

        mame_printf_verbose("SwitchRes: Resolution switch from %dx%d to %dx%d on %dx%d default\n",
                oldwidth, oldwidth, width, height,
                window->monitor->center_width, window->monitor->center_height);

        // Delete old modeline
        if (machine.options().modeline()) {
                if (found && strcmp(machine.switchRes.lastMode.name, "")) {
                        mame_printf_verbose("SwitchRes: REMOVE %s: \tMode      %s\n",
                                machine.switchRes.cs.connector, machine.switchRes.lastMode.name);
                        DelXrandrDisplay(machine.switchRes.cs.connector, machine.switchRes.lastMode.name);
                } else
                        mame_printf_verbose("SwitchRes: No mode named %s\n",
                                machine.switchRes.lastMode.name);
        }
	return true;
}

