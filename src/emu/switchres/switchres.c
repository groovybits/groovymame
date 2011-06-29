#include "emu.h"
#include "emuopts.h"

/*-------------------------------------------------
    Calculate Modeline
--------------------------------------------------*/
void calc_modeline(running_machine &machine)
{
	SwitchRes *switchRes = &machine.switchRes;
	const game_driver *game = &machine.system();
        const screen_device *devconfig;
        machine_config config(*game, machine.options());
        int w = 640, h = 480;
        double r = 60.00;
        int orientation = 0;
	ConfigSettings *cs = &switchRes->cs;
        GameInfo *gameInfo = &switchRes->gameInfo;
	Resolution *resolution = &switchRes->resolution;
        char modeline[1024]={'\x00'};
        int monitorModeCnt = 0;
        int lowest_weight = 99999;
        int best_weight = 0;
        int i;

        mame_printf_verbose("SwitchRes: Monitor: %s Orientation: %s Aspect %s\n",
                cs->monitor, cs->morientation, cs->aspect);

        switchRes->monitorMode = &switchRes->monitorModes[0];
        memset(&switchRes->monitorModes[0], 0, sizeof(struct MonitorMode));

        devconfig = config.first_screen();

        switch (devconfig->screen_type())
        {
                case SCREEN_TYPE_RASTER:        gameInfo->vector = 0;       break;
                case SCREEN_TYPE_VECTOR:        gameInfo->vector = 1;       break;
                case SCREEN_TYPE_LCD:           gameInfo->vector = 0;       break;
                default:                        gameInfo->vector = 0;       break;
        }

        /* output the orientation as a string */
        switch (game->flags & ORIENTATION_MASK)
        {
                case ORIENTATION_FLIP_X:
                        //fprintf(out, " rotate=\"0\" flipx=\"yes\"");
                        orientation = 0;
                        break;
                case ORIENTATION_FLIP_Y:
                        //fprintf(out, " rotate=\"180\" flipx=\"yes\"");
                        orientation = 0;
                        break;
                case ORIENTATION_FLIP_X|ORIENTATION_FLIP_Y:
                        //fprintf(out, " rotate=\"180\"");
                        orientation = 0;
                        break;
                case ORIENTATION_SWAP_XY:
                        //fprintf(out, " rotate=\"90\" flipx=\"yes\"");
                        orientation = 1;
                        break;
                case ORIENTATION_SWAP_XY|ORIENTATION_FLIP_X:
                        //fprintf(out, " rotate=\"90\"");
                        orientation = 1;
                        break;
                case ORIENTATION_SWAP_XY|ORIENTATION_FLIP_Y:
                        //fprintf(out, " rotate=\"270\"");
                        orientation = 1;
                        break;
                case ORIENTATION_SWAP_XY|ORIENTATION_FLIP_X|ORIENTATION_FLIP_Y:
                        //fprintf(out, " rotate=\"270\" flipx=\"yes\"");
                        orientation = 1;
                        break;
                default:
                        //fprintf(out, " rotate=\"0\"");
                        orientation = 0;
                        break;
        }

        /* output width and height only for games that are not vector */
        if (devconfig->screen_type() != SCREEN_TYPE_VECTOR)
        {
                const rectangle &visarea = devconfig->visible_area();
                int dx = visarea.max_x - visarea.min_x + 1;
                int dy = visarea.max_y - visarea.min_y + 1;

                w = dx;
                h = dy;
        }

        /* refresh rate */
        r = ATTOSECONDS_TO_HZ(devconfig->refresh_attoseconds());

	gameInfo->screens = 0;
        for (devconfig = config.first_screen(); devconfig != NULL; devconfig = devconfig->next_screen())
                gameInfo->screens++;

	if (resolution->width != 0 && resolution->height != 0) {
		mame_printf_verbose("SwitchRes: Using changed resolution of %dx%d instead of original\n",
			resolution->width, resolution->height);
		gameInfo->width = gameInfo->o_width = resolution->width;
		gameInfo->height = gameInfo->o_height = resolution->height;
	} else {
		gameInfo->width = gameInfo->o_width = w;
		gameInfo->height = gameInfo->o_height = h;
	}
        gameInfo->refresh = gameInfo->o_refresh = r;
        gameInfo->orientation = orientation;

        // Frogger fix
        if (gameInfo->width == 224 && gameInfo->height == 768)
                gameInfo->height = 256;
        else if (gameInfo->height == 224 && gameInfo->width == 768)
                gameInfo->width = 256;

	// Fill in current resolution settings
	resolution->refresh = gameInfo->refresh;
	resolution->orientation = gameInfo->orientation;

	/* Orientation */
	if (!gameInfo->vector) {
		if (gameInfo->orientation)
		{       // orientation = vertical
			int wx = gameInfo->width;
			int hy = gameInfo->height;
			if (!strcmp(cs->morientation, "horizontal")) {
				gameInfo->width = hy;
				gameInfo->height = wx;
			}
		} else { // horizontal
			if (!strcmp(cs->morientation, "vertical")) {
				int wx = gameInfo->width;
				int hy = gameInfo->height;
				gameInfo->width = hy;
				gameInfo->height = wx;
			}
		}
		if ((gameInfo->orientation && !strcmp(cs->morientation, "horizontal")) || (!gameInfo->orientation &&  (!strcmp(cs->morientation, "vertical")))) {
			double num, den;
			sscanf(cs->aspect, "%lf:%lf", &den, &num);

			gameInfo->width = Normalize((double)gameInfo->width * (4.0/3.0) / (num/den), 8);
		}
	} else {
		gameInfo->orientation = 0;
	}

	/* Dual screen games */
	if (gameInfo->screens > 1 && cs->monitorcount <= 1) {
		double num, den;
		sscanf(cs->aspect, "%lf:%lf", &den, &num);

		gameInfo->orientation = 1;
		gameInfo->height *= 2;
		gameInfo->width = Normalize((double)gameInfo->width * (4.0/3.0) / (num/den), 8);
	}

	// Need to run this only on first run if resolution isn't set to auto, and other runs if it wasn't
	if (strcmp(gameInfo->resolution, "auto")) 
	{
		astring error_string;
		mame_printf_verbose("SwitchRes: Resolution was set at command line or in INI file as %s\n", 
			gameInfo->resolution);

		machine.options().set_value(OPTION_CHANGERES, false, OPTION_PRIORITY_MAXIMUM, error_string);
		if (sscanf(gameInfo->resolution, "%dx%dx%d@%lf", &gameInfo->width, &gameInfo->height, &gameInfo->depth, &gameInfo->refresh) < 4)
			if (sscanf(gameInfo->resolution, "%dx%dx%d@%lf", &gameInfo->width, &gameInfo->height, &gameInfo->depth, &gameInfo->refresh) < 3)
				if (sscanf(gameInfo->resolution, "%dx%d@%lf", &gameInfo->width, &gameInfo->height, &gameInfo->refresh) < 3)
					if (sscanf(gameInfo->resolution, "%dx%d@%lf", &gameInfo->width, &gameInfo->height, &gameInfo->refresh) < 2)
							mame_printf_error("SwitchRes: Illegal resolution value = %s\n", gameInfo->resolution);
	}

	gameInfo->width = Normalize(gameInfo->width, 8);
	gameInfo->height = Normalize(gameInfo->height, 8);

	if (!strcmp(machine.options().monitor_specs0(), "auto"))
        	monitorModeCnt = SetMonitorMode(cs->monitor, switchRes->monitorModes);
	else {
        	monitorModeCnt = 0;
		if (!FillMonitorMode(machine.options().monitor_specs0(), switchRes->monitorModes))
        		monitorModeCnt++;
		if (strcmp(machine.options().monitor_specs1(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs1(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs2(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs2(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs3(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs3(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs4(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs4(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs5(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs5(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs6(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs6(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (strcmp(machine.options().monitor_specs7(), "auto")) {
			if (!FillMonitorMode(machine.options().monitor_specs7(), switchRes->monitorModes))
        			monitorModeCnt++;
		} 
		if (!monitorModeCnt) {
			mame_printf_error("SwitchResh: ERROR, no custom modes were the correct syntax!!!\n");
        		monitorModeCnt = SetMonitorMode(cs->monitor, switchRes->monitorModes);
		}

	}

        /* Get modeline for each monitor mode */
        for (i = 0 ; i < monitorModeCnt ; i++) {
                /* Modeline */
                struct ModeLine *modeLine = NULL;

                switchRes->monitorMode = &switchRes->monitorModes[i];				
				memset(&switchRes->monitorMode->modeLine, 0, sizeof(struct ModeLine));

                if (switchRes->monitorMode->HfreqMin == switchRes->monitorMode->HfreqMax)
                        switchRes->monitorMode->HfreqMin -= .01;

                if (cs->verbose > 2)
                        ShowMonitorMode(switchRes->monitorMode);

                /* create modeline */
                ModelineCreate(cs, gameInfo, switchRes->monitorMode, &switchRes->monitorMode->modeLine);
		modeLine = &switchRes->monitorMode->modeLine;
                modeLine->weight = ModelineResult(modeLine, cs);

		/* check dotclock */
		if (modeLine->pclock < cs->dotclockmin) {
			mame_printf_verbose("SwitchRes: Dotclock too low, doubling horizontal size\n");
			gameInfo->width *= 2;
			if (cs->keepaspect)
				gameInfo->height *= 2;

                	ModelineCreate(cs, gameInfo, switchRes->monitorMode, &switchRes->monitorMode->modeLine);
			switchRes->monitorMode->modeLine.result |= RESULT_LOWDOTCLOCK;
			modeLine = &switchRes->monitorMode->modeLine;
                	modeLine->weight = ModelineResult(modeLine, cs);
		}

                if (cs->verbose > 2) {
                	mame_printf_verbose("SwitchRes: # %s [%d] %dx%d@%.2f %.4fKhz\n", gameInfo->name,
                        	modeLine->weight,
                        	modeLine->hactive, modeLine->vactive,
                        	modeLine->vfreq, modeLine->hfreq/1000);
                	PrintModeline(modeLine, modeline);
                	mame_printf_verbose("SwitchRes: ModeLine     %s\n\n", modeline);
		}
        }

        /* Find best weight */
        for (i = 0 ; i < monitorModeCnt ; i++) {
                switchRes->monitorMode = &switchRes->monitorModes[i];
                if (switchRes->monitorMode->modeLine.weight < lowest_weight) {
                        lowest_weight = switchRes->monitorMode->modeLine.weight;
                        best_weight = i;
                }
        }

        /* Save best weight */
        switchRes->monitorMode = &switchRes->monitorModes[best_weight];
        switchRes->modeLine = &switchRes->monitorMode->modeLine;

        mame_printf_info("SwitchRes v%s: [%s] (%d) %s (%dx%d@%.2f)->(%dx%d@%.2f)->(%dx%d@%.2f)\n",
		SWITCHRES_VERSION,
                gameInfo->name,
                gameInfo->screens,
                orientation?"vertical":"horizontal",
                gameInfo->o_width, gameInfo->o_height, gameInfo->o_refresh,
		gameInfo->width, gameInfo->height, gameInfo->refresh,
		switchRes->modeLine->hactive, switchRes->modeLine->vactive, switchRes->modeLine->vfreq);
}

int switchres_calc_modeline(running_machine &machine)
{
	SwitchRes *switchRes = &machine.switchRes;
	
        /* Modeline Generation */
        GameInfo *gameInfo = &switchRes->gameInfo;
	ModeLine *modeLine = switchRes->modeLine;
        char modeline[1024]={'\x00'};

        /* Calculate modeline */
        calc_modeline(machine);

        /* Extract modeline as string */
	modeLine = switchRes->modeLine;

        /* print out modeline */
       	PrintModeline(modeLine, modeline);
        mame_printf_verbose("SwitchRes: # %s %dx%d@%.2f %.4fKhz\n",
                gameInfo->name,
                modeLine->hactive, modeLine->vactive,
                modeLine->vfreq, modeLine->hfreq/1000);
        mame_printf_verbose("SwitchRes: \tModeLine     %s\n", modeline);

        return 0;
}

void SetMameOptions(running_machine &machine) 
{
	SwitchRes *switchRes = &machine.switchRes;
	ConfigSettings *cs = &switchRes->cs;
	GameInfo *gameInfo = &switchRes->gameInfo;

	/* Monitor information */
	memset(cs, 0, sizeof(struct ConfigSettings));
	memset(gameInfo, 0, sizeof(struct GameInfo));
	sprintf(gameInfo->name, "%s", machine.options().system_name());
	sprintf(cs->monitor, "%s", machine.options().monitor());
	sprintf(cs->connector, "%s", machine.options().monitor_connector());
	sprintf(cs->morientation, "%s", machine.options().monitor_orientation());
	sprintf(cs->aspect, "%s", machine.options().monitor_aspect());
	sscanf(machine.options().monitor_debug(), "%d", &cs->verbose);
	sscanf(machine.options().monitor_dotclock(), "%d", &cs->dotclockmin);
	sscanf(machine.options().monitor_ymin(), "%d", &cs->ymin);

	cs->vsync      = 1;
	cs->interlace  = 1;

	if (machine.options().monitor_doublescan())
		cs->doublescan = 1;
	else
		cs->doublescan = 0;

        if (machine.options().cleanstretch())
		switchRes->cs.cleanstretch = 1;
	else
		switchRes->cs.cleanstretch = 0;
        if (machine.options().changeres())
		switchRes->cs.changeres = 1;
	else
		switchRes->cs.changeres = 0;
        if (machine.options().soundsync())
		switchRes->cs.soundsync = 1;
	else
		switchRes->cs.soundsync = 0;

	sscanf(machine.options().redraw(), "%d", &switchRes->cs.redraw);
}

void CalculateMameOptions(running_machine &machine) 
{
	SwitchRes *switchRes = &machine.switchRes;
	astring error_string;

	/* Change Mame Options */
        if (switchRes->modeLine->result & RESULT_VFREQ_DOUBLE) {
		mame_printf_verbose("SwitchRes: Setting Option -redraw 2\n");
		switchRes->cs.redraw = 2;
	} else {
		mame_printf_verbose("SwitchRes: Setting Option -redraw 0\n");
		switchRes->cs.redraw = 0;
	}
			
        if (switchRes->modeLine->result & RESULT_VIRTUALIZE || switchRes->modeLine->result & (RESULT_RES_INC|RESULT_RES_DEC)) {
		mame_printf_verbose("SwitchRes: Setting Option -nocleanstretch\n");
		switchRes->cs.cleanstretch = 0;
		machine.options().set_value(OPTION_CLEANSTRETCH, false, OPTION_PRIORITY_MAXIMUM, error_string);
	} else if (machine.options().cleanstretch()) {
		mame_printf_verbose("SwitchRes: Setting Option -cleanstretch\n");
		switchRes->cs.cleanstretch = 1;
	}

	if (!strcmp(switchRes->cs.morientation, "horizontal")) {
		mame_printf_verbose("SwitchRes: Setting Option -rotate\n");
		machine.options().set_value(OPTION_ROTATE, true, OPTION_PRIORITY_MAXIMUM, error_string);
	} else if (!strcmp(switchRes->cs.morientation, "vertical")) {
		mame_printf_verbose("SwitchRes: Setting Option -rotate\n");
		machine.options().set_value(OPTION_ROTATE, true, OPTION_PRIORITY_MAXIMUM, error_string);
		if (switchRes->gameInfo.orientation == 1) {
			mame_printf_verbose("SwitchRes: Setting Option -autorol\n");
			machine.options().set_value(OPTION_AUTOROL, true, OPTION_PRIORITY_MAXIMUM, error_string);
		} else {
			mame_printf_verbose("SwitchRes: Setting Option -rol\n");
			machine.options().set_value(OPTION_ROL, true, OPTION_PRIORITY_MAXIMUM, error_string);
		}
	} else if (!strcmp(switchRes->cs.morientation, "rotate")) {
		mame_printf_verbose("SwitchRes: Setting Option -rotate\n");
		machine.options().set_value(OPTION_ROTATE, true, OPTION_PRIORITY_MAXIMUM, error_string);
		if (switchRes->gameInfo.orientation == 1) {
			mame_printf_verbose("SwitchRes: Setting Option -autorol\n");
			machine.options().set_value(OPTION_AUTOROL, true, OPTION_PRIORITY_MAXIMUM, error_string);
		}
	}
}
