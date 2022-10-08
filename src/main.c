/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "main.h"

#include "timer.h"
#include "io.h"
#include "gfx.h"
#include "audio.h"
#include "pad.h"
#include "network.h"

#include "pause.h"
#include "menu.h"
#include "stage.h"
#include "save.h"

//Game loop
GameLoop gameloop;
SCREEN screen;

//Error handler
char error_msg[0x200];

void ErrorLock(void)
{
	while (1)
	{
		FntPrint(-1, "A fatal error has occured\n~c700%s\n", error_msg);
		Gfx_Flip();
	}
}

//Entry point                                                                             
int main(int argc, char **argv)                                                                                                                                                        
{
	//Remember arguments
	my_argc = argc;
	my_argv = argv;

	//Initialize system
	PSX_Init();

	stage.pal_i = 1;
	stage.wide_i = 1;	
	InitGeom();
	Gfx_Init();
	Pad_Init();
	InitCARD(1);
	StartPAD();
	StartCARD();
	_bu_init();	
	ChangeClearPAD(0);

	IO_Init();
	Audio_Init();
	Network_Init();
	Timer_Init(false, false);

	if (readSaveFile() == false)
		defaultSettings();

	//Start game
	gameloop = GameLoop_Menu;
	Menu_Load(MenuPage_Opening);

	//Game loop
	while (PSX_Running())
	{
		if (stage.prefs.widescreen) {
			if (stage.wide_i == 1)
			{		
				screen.SCREEN_WIDTH   = 512;
				screen.SCREEN_HEIGHT  = 240;
				screen.SCREEN_WIDTH2  = (screen.SCREEN_WIDTH >> 1);
				screen.SCREEN_HEIGHT2 = (screen.SCREEN_HEIGHT >> 1);

				screen.SCREEN_WIDEADD = (screen.SCREEN_WIDTH - 512);
				screen.SCREEN_TALLADD = (screen.SCREEN_HEIGHT - 240);
				screen.SCREEN_WIDEADD2 = (screen.SCREEN_WIDEADD >> 1);
				screen.SCREEN_TALLADD2 = (screen.SCREEN_TALLADD >> 1);

				screen.SCREEN_WIDEOADD = (screen.SCREEN_WIDEADD > 0 ? screen.SCREEN_WIDEADD : 0);
				screen.SCREEN_TALLOADD = (screen.SCREEN_TALLADD > 0 ? screen.SCREEN_TALLADD : 0);
				screen.SCREEN_WIDEOADD2 = (screen.SCREEN_WIDEOADD >> 1);
				screen.SCREEN_TALLOADD2 = (screen.SCREEN_TALLOADD >> 1);	

				Gfx_Init();
				stage.wide_i = 2;
				stage.pal_i = 1; //check for pal mode again
			}
		}
		else {
			if (stage.wide_i == 1)
			{
				screen.SCREEN_WIDTH   = 320;
				screen.SCREEN_HEIGHT  = 240;
				screen.SCREEN_WIDTH2  = (screen.SCREEN_WIDTH >> 1);
				screen.SCREEN_HEIGHT2 = (screen.SCREEN_HEIGHT >> 1);
				screen.SCREEN_WIDEADD = (screen.SCREEN_WIDTH - 320);

				screen.SCREEN_TALLADD = (screen.SCREEN_HEIGHT - 240);
				screen.SCREEN_WIDEADD2 = (screen.SCREEN_WIDEADD >> 1);
				screen.SCREEN_TALLADD2 = (screen.SCREEN_TALLADD >> 1);
				screen.SCREEN_WIDEOADD = (screen.SCREEN_WIDEADD > 0 ? screen.SCREEN_WIDEADD : 0);

				screen.SCREEN_TALLOADD = (screen.SCREEN_TALLADD > 0 ? screen.SCREEN_TALLADD : 0);
				screen.SCREEN_WIDEOADD2 = (screen.SCREEN_WIDEOADD >> 1);
				screen.SCREEN_TALLOADD2 = (screen.SCREEN_TALLOADD >> 1);	

				Gfx_Init();
				stage.wide_i = 2;
				stage.pal_i = 1; //check for pal mode again
			}
		}

		//Prepare frame
		Timer_Tick();
		Audio_ProcessXA();
		Pad_Update();

		//Set video mode
		if (stage.prefs.palmode)
		{
			if (stage.pal_i == 1)
			{
				SetVideoMode(MODE_PAL);
				stage.disp[0].screen.y = stage.disp[1].screen.y = 24;
				Timer_Init(true, true);
				stage.pal_i = 2;
			}
		}
		else
		{
			if (stage.pal_i == 1)
			{
				SetVideoMode(MODE_NTSC);
				stage.disp[0].screen.y = stage.disp[1].screen.y = 0;
				Timer_Init(false, false);
				stage.pal_i = 2;
			}
		}

		//Tick and draw game
		Network_Process();
		switch (gameloop)
		{
			case GameLoop_Menu:
				Menu_Tick();
				break;
			case GameLoop_Stage:
				Stage_Tick();
				break;
		}
		
		//Flip gfx buffers
		Gfx_Flip();
	}
	
	//Deinitialize system
	Network_Quit();
	Pad_Quit();
	Gfx_Quit();
	Audio_Quit();
	IO_Quit();
	
	PSX_Quit();
	return 0;
}
