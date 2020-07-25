/*
	Copyright (C) 2007 Pascal Giard
	Copyright (C) 2007-2011 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ctrl.h"
#include "saves.h"
#include "SPU.h"
#include "commandline.h"
#include "NDSSystem.h"
#include "GPU_osd.h"

#ifdef FAKE_MIC
#include "mic.h"
#endif

// EMULATOR SETTINGS MENU START
#define MOV_ARRIBA 0
#define MOV_ABAJO 1
#define MOV_AA 2
#define MOV_BB 3
//#define MENU_FIRST_OPTION_Y  150
//////////////#define MENU_FIRST_OPTION_Y   40
//HCF Added 3d and dynarec options
#define MENU_FIRST_OPTION_Y   35
//#define MENU_OPTION_HEIGHT    20
#define MENU_OPTION_HEIGHT    15
#define MENU_RECTANGLE_OFFSET_Y 0
#define MENU_RECTANGLE_OFFSET_X 16
// EMULATOR SETTINGS MENU END

// ====== Input ====== //
int 		joyx, joyy;
short 		Buttons[NUM_BUTTONS];

// ===== Video Modes ===== //
int 		iStretchMode 			= STRETCH_MODE_NONE;
int 		iStretchModeNew 		= STRETCH_MODE_NONE;

// ===== Settings ===== //
int 		iAutoFrameskip;
int 		iFrameskipCount;
int 		iEnableSound;
int 		iEmulate3D 				= 1;
int 		iUseDynaRec 			= 1;
int 		iLimitFramerate 		= 0;
int 		iGraphicsMode 			= 0;
int 		iGraphicsModeNew 		= 0;
int 		iMouseSpeed 			= MOUSE_DEFAULT_SPEED;
bool 		bBlitAll 				= true;
bool 		iShowFramesAndMemory 	= false;
bool 		ChangingDisplayInfo 	= false;
bool 		ChangingVideoMode  		= false;
bool 		ChangingCursorColor 	= false;
bool 		ChangingGraphicsMode  	= false;
//
extern int 	iSoundQuality;
extern int 	iSoundMode;
extern int 	frameskip;
extern int 	enable_sound;
extern CACHE_ALIGN u8 GPU_screen[4*256*192];
//
void 		 Gu_draw();
mouse_status mouse;

// This is the default Input Configuration.
// The order of Buttons are as Follows:
// 1. A
// 2. B
// 3. Select
// 4. Start
// 5. Right
// 6. Left
// 7. Up
// 8. Down
// 9. Right Trigger
// 10. Left Trigger
// 11. X
// 12. Y
const u16 	CTRL_Default_NDS_CFG[NB_KEYS] =
{
	BUTTON_CIRCLE,
	BUTTON_CROSS,
	BUTTON_SELECT,
	BUTTON_START,
	BUTTON_RIGHT,
	BUTTON_LEFT,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_RTRIGGER,
	BUTTON_LTRIGGER,
	BUTTON_TRIANGLE,
	BUTTON_SQUARE
};


// MotoLegacy (7/25/2020) - rewrote vdRellenaBotones (vdGetButtons) using
// native calls instead of relying on SDL.
void vdGetButtons(void)
{
	// Init the Pad
	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlReadBufferPositive(&pad, 1);

	// Update Joystick X/Y Axis
	joyx = pad.Lx - 127;
	joyy = pad.Ly - 127;

	// Face Buttons
	// TODO: Rename variables to match Face Buttons and be in English.
	if (pad.Buttons & PSP_CTRL_SQUARE) {
		Buttons[BUTTON_SQUARE] = 1;
	} else {
		Buttons[BUTTON_SQUARE] = 0;
	}
	if (pad.Buttons & PSP_CTRL_CROSS) {
		Buttons[BUTTON_CROSS] = 1;
	} else {
		Buttons[BUTTON_CROSS] = 0;
	}
	if (pad.Buttons & PSP_CTRL_TRIANGLE) {
		Buttons[BUTTON_TRIANGLE] = 1;
	} else {
		Buttons[BUTTON_TRIANGLE] = 0;
	}
	if (pad.Buttons & PSP_CTRL_CIRCLE) {
		Buttons[BUTTON_CIRCLE] = 1;
	} else {
		Buttons[BUTTON_CIRCLE] = 0;
	}

	// Directional Pad
	if (pad.Buttons & PSP_CTRL_UP) {
		Buttons[BUTTON_UP] = 1;
	} else {
		Buttons[BUTTON_UP] = 0;
	}
	if (pad.Buttons & PSP_CTRL_DOWN) {
		Buttons[BUTTON_DOWN] = 1;
	} else {
		Buttons[BUTTON_DOWN] = 0;
	}
	if (pad.Buttons & PSP_CTRL_LEFT) {
		Buttons[BUTTON_LEFT] = 1;
	} else {
		Buttons[BUTTON_LEFT] = 0;
	}
	if (pad.Buttons & PSP_CTRL_RIGHT) {
		Buttons[BUTTON_RIGHT] = 1;
	} else {
		Buttons[BUTTON_RIGHT] = 0;
	}

	// Left and Right Triggers
	if (pad.Buttons & PSP_CTRL_LTRIGGER) {
		Buttons[BUTTON_LTRIGGER] = 1;
	} else {
		Buttons[BUTTON_LTRIGGER] = 0;
	}
	if (pad.Buttons & PSP_CTRL_RTRIGGER) {
		Buttons[BUTTON_RTRIGGER] = 1;
	} else {
		Buttons[BUTTON_RTRIGGER] = 0;
	}

	// Start, Select
	if (pad.Buttons & PSP_CTRL_START) {
		Buttons[BUTTON_START] = 1;
	} else {
		Buttons[BUTTON_START] = 0;
	}
	if (pad.Buttons & PSP_CTRL_SELECT) {
		Buttons[BUTTON_SELECT] = 1;
	} else {
		Buttons[BUTTON_SELECT] = 0;
	}

	// Music Note (WHITE)
	if (pad.Buttons & PSP_CTRL_NOTE) {
		Buttons[BUTTON_NOTE] = 1;
	} else {
		Buttons[BUTTON_NOTE] = 0;
	}
}

/*void DrawText(short * screen, unsigned int x, unsigned int y, bool invert, const char * text)
{
	char string[4096];

	strcpy(string, text);
	unsigned int pitch = 256;
	unsigned int length = strlen(string), address = x + (y * pitch);

	for(unsigned int i=0; i<length; i++)
	{
		unsigned int fontAddr = (unsigned int)string[i] * 64;

		for(unsigned int yy=0; yy<8; yy++)
		{
			for(unsigned int xx=0; xx<8; xx++)
			{
				if ((font1[fontAddr] && !invert) || (!font1[fontAddr] && invert))
					*(screen + address + xx + (yy * pitch)) = 0xFE00;
				fontAddr++;
			}
		}

		address += 8;
	}
}
void DrawColorText(short * screen, unsigned int x, unsigned int y, bool invert, const char * text)
{
	char string[4096];

	strcpy(string, text);
	unsigned int pitch = 256;
	unsigned int length = strlen(string), address = x + (y * pitch);

	for(unsigned int i=0; i<length; i++)
	{
		unsigned int fontAddr = (unsigned int)string[i] * 64;

		for(unsigned int yy=0; yy<8; yy++)
		{
			for(unsigned int xx=0; xx<8; xx++)
			{
				if ((font1[fontAddr] && !invert) || (!font1[fontAddr] && invert))
					*(screen + address + xx + (yy * pitch)) = ashCursorColors[shCurrentCursorColor];
				fontAddr++;
			}
		}

		address += 8;
	}
}*/

/*void vdXBOptionsMenu(int iMenuInicial)
{
	char cAux = '0';
	int iRedibujar = 1;
	int iLongitud;
	int salir = 0;
	int movanterior = MOV_AA;
	int iNumOpciones;
	int iOpcionSeleccionada = 0;

	int iVideoModeChanged = 0;
	int rectVentanax, rectVentanay;
	int rectVentanaw, rectVentanah;
	int rectTextox, rectTextoy;

	char achTexto[100];

	rectVentanax = 3;
	rectVentanay = 0;
	rectVentanaw = 250;
	rectVentanah = 200;

	int iDesplModoGrafico = 0;
	int iResetSound = 0;

	//HCF Temporary select video mode 1 for menu
	int iModoGraficoBackup = iModoGrafico;
	iModoGraficoNuevo = 1;

	//Mute audio during menu
	SDL_PauseAudio(1);

	rectTextox = rectVentanax + 20;
	rectTextoy = 30;

	if( iMenuInicial == 1 )
	{
		//iNumOpciones = 4;
		//HCF desmumeX!!

		//iNumOpciones = 6;
		//Se anyade bAutoFrameskip

		//iNumOpciones = 8;
		//Se anyaden emula3D y bUsaDynarec

		//iNumOpciones = 9;
		//Se anyade Limit to 60 FPS

		iNumOpciones = 10;
		//Se anyade Sound Mode
	}
	else
	{
		//iNumOpciones = 1;

		//En desmumeX mostramos todas las opciones
		//iNumOpciones = 8;

		//iNumOpciones = 9;
		//Se anyade Limit to 60 FPS

		iNumOpciones = 10;
		//Se anyade Sound Mode
	}

	while(!salir)
	{

		//g_input.Update();
		vdGetButtons();
		//if(g_input.IsButtonPressed(Generic_DPadUp))
		if( ashBotones[BOTON_ARRIBA] )
		{
			if( movanterior != MOV_ARRIBA )
			{
				if( iOpcionSeleccionada > 0 )
					iOpcionSeleccionada--;
				else
					iOpcionSeleccionada = iNumOpciones - 1;

				iRedibujar = 1;

			}
			movanterior = MOV_ARRIBA;
		}
		else if(movanterior == MOV_ARRIBA)
		{
			movanterior = -1;
		}

		//if(g_input.IsButtonPressed(Generic_DPadDown))
		if( ashBotones[BOTON_ABAJO] )
		{
			if( movanterior != MOV_ABAJO )
			{
				if( iOpcionSeleccionada < iNumOpciones - 1 )
					iOpcionSeleccionada++;
				else
					iOpcionSeleccionada = 0;

				iRedibujar = 1;
				if( iOpcionSeleccionada < iNumOpciones - 1 )
				{
					iOpcionSeleccionada++;
					iRedibujar = 1;
				}
			}
			movanterior = MOV_ABAJO;
		}
		else if(movanterior == MOV_ABAJO)
		{
			movanterior = -1;
		}

		//if(g_input.IsButtonPressed(Generic_A))
		if( ashBotones[BOTON_AA] )
		{
			if( movanterior != MOV_AA )
			{
				if( iOpcionSeleccionada == 5 )
				{
					//SOUND
					//Asi era para solo ON y OFF
					/////iEnableSound = ( ( iEnableSound + 1 ) % 2 );
					//HCF Add audio overclocking
					iEnableSound = ( ( iEnableSound + 1 ) % 5 );
					enable_sound = iEnableSound;
					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 3 )
				{
					//MOUSE Speed
					if(iMouseSpeed < 9)
						iMouseSpeed++;

					//else
					//	iSpeedMouse = 1;


					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 4 )
				{
				    //Se sustituye global speed por blit all
					//BLIT ALL
					if(bBlitAll == true)
					{
						bBlitAll = false;
					}
					else
					{
						bBlitAll = true;
					}
					//Global Speed
					//else
					//	iGlobalSpeed = 1;


					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 6 )
				{
					switch(iSoundQuality)
					{
						case 8:
							iSoundQuality = 4;
							break;
						case 4:
							iSoundQuality = 2;
							break;
						case 2:
							iSoundQuality = 1;
							break;
						default:
							break;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 7 )
				{
					//Sound Mode
					if(iSoundMode < SOUND_MODE_SYNC_INTERPOLATED)
						iSoundMode++;
					else
						iSoundMode = SOUND_MODE_ASYNC;

					iResetSound = 1;
					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 2 )
				{
					//FS
					if( nFrameskip < 9 )
					{
						nFrameskip++;
						iRedibujar = 1;

						if(iAutoFrameskip == FRAMESKIP_AUTO_BOTH || iAutoFrameskip == FRAMESKIP_FIXED)
							frameskip = nFrameskip;
					}
				}
				else if( iOpcionSeleccionada == 1 )
				{
					//Auto Frameskip
					iAutoFrameskip = (iAutoFrameskip + 1) % (FRAMESKIP_AUTO_BOTH + 1);

					if(iAutoFrameskip == FRAMESKIP_AUTO_EVEN)
						frameskip = 0;
					else if(iAutoFrameskip == FRAMESKIP_AUTO_ODD)
						frameskip = 1;

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 0 )
				{
					//3D
					if(emula3D == true)
					{
						emula3D = false;
					}
					else
					{
						emula3D = true;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 8 )
				{
					//Use Dynarec
					if(iUsarDynarec == true)
					{
						iUsarDynarec = false;
					}
					else
					{
						iUsarDynarec = true;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 9 )
				{
					//Limit to 60 FPS
					if(iLimitFramerate < 2)
						iLimitFramerate++;
					else
						iLimitFramerate = 0;

					iRedibujar = 1;
				}

			}
			movanterior = MOV_AA;
		}
		else if(movanterior == MOV_AA)
		{
			movanterior = -1;
		}

		//if(g_input.IsButtonPressed(Generic_B))
		if( ashBotones[BOTON_BB] )
		{
			if( movanterior != MOV_BB )
			{
				if( iOpcionSeleccionada == 5 )
				{
					//SOUND
					//iEnableSound = ( ( iEnableSound + 1 ) % 2 );
					if(iEnableSound > 0)
						iEnableSound--;
					else
						iEnableSound = 4;
						//HCF Asi era para solo ON/OFF
						//iEnableSound = 1;

					enable_sound = iEnableSound;
					iRedibujar = 1;

				}
				else if( iOpcionSeleccionada == 3 )
				{
					//MOUSE SPEED
					if(iMouseSpeed > 1)
						iMouseSpeed--;

					//else
					//	iSpeedMouse = 9;


					iRedibujar = 1;

				}
				else if( iOpcionSeleccionada == 4 )
				{
				    //Se sustituye global speed por blit all
					//BLIT ALL
					if(bBlitAll == true)
					{
						bBlitAll = false;
					}
					else
					{
						bBlitAll = true;
					}
					//GLOBAL SPEED
					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 6 )
				{
					switch(iSoundQuality)
					{
						case 1:
							iSoundQuality = 2;
							break;
						case 2:
							iSoundQuality = 4;
							break;
						case 4:
							iSoundQuality = 8;
							break;
						default:
							break;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 7 )
				{
					//Sound Mode
					if(iSoundMode > 0)
						iSoundMode--;
					else
						iSoundMode = SOUND_MODE_SYNC_INTERPOLATED;

					iResetSound = 1;
					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 2 )
				{
					//FS
					if( nFrameskip > 0 )
					{
						nFrameskip--;
						iRedibujar = 1;

						if(iAutoFrameskip == FRAMESKIP_FIXED || iAutoFrameskip == FRAMESKIP_AUTO_BOTH)
							frameskip = nFrameskip;
					}
				}
				else if( iOpcionSeleccionada == 1 )
				{
					//Auto Framekip
					if(iAutoFrameskip > 0)
						iAutoFrameskip--;
					else
						iAutoFrameskip = FRAMESKIP_AUTO_BOTH;

					if(iAutoFrameskip == FRAMESKIP_AUTO_EVEN)
						frameskip = 0;
					else if(iAutoFrameskip == FRAMESKIP_AUTO_ODD)
						frameskip = 1;


					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 0 )
				{
					//3D
					if(emula3D == true)
					{
						emula3D = false;
					}
					else
					{
						emula3D = true;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 8 )
				{
					//Use Dynarec
					if(iUsarDynarec == true)
					{
						iUsarDynarec = false;
					}
					else
					{
						iUsarDynarec = true;
					}

					iRedibujar = 1;
				}
				else if( iOpcionSeleccionada == 9 )
				{
					//Limit to 60 FPS
					//Limit to 60 FPS
					if(iLimitFramerate > 0)
						iLimitFramerate--;
					else
						iLimitFramerate = 2;


					iRedibujar = 1;
				}

			}
			movanterior = MOV_BB;
		}
		else if(movanterior == MOV_BB)
		{
			movanterior = -1;
		}

		//if(g_input.IsButtonPressed(Generic_Start))
		if( ashBotones[BOTON_START] )
		{
			salir = 1;
		}

		if( iRedibujar)
		{
			rectVentanax = 3;
			rectVentanay = 0;
			rectVentanah = 200;

			if(iModoGraficoNuevo != 2)
			{
				//HCF Menu is displayed in first screen
				//for(int ko = 0; ko < 4*256*192; ko++)
				for(int ko = 0; ko < 2*256*192; ko++)
					GPU_screen[ko] = 0;
			}
			else
			{
				//HCF Menu is displayed in second screen
				for(int ko = 2*256*192; ko < 4*256*192; ko++)
					GPU_screen[ko] = 0;
			}

			//TEXTO OPTIONS
			//strcpy(achTexto, "OPTIONS");

			//TEXTO 3D Emulation (sustituye al texto de OPTIONS)
			if(emula3D)
			{
				strcpy(achTexto, "3D emulation: ON");
			}
			else
			{
				strcpy(achTexto, "3D emulation: OFF");
			}

			/////rectTexto.y = 17;
			rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y - MENU_OPTION_HEIGHT;
			DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

			//TEXTO AUTO FRAMESKIP
			switch(iAutoFrameskip)
			{
				case FRAMESKIP_FIXED:
					strcpy(achTexto, "Frameskip: Fixed");
				break;
				case FRAMESKIP_AUTO_ODD:
					strcpy(achTexto, "Frameskip: Auto (odd)");
				break;
				case FRAMESKIP_AUTO_EVEN:
					strcpy(achTexto, "Frameskip: Auto (even)");
				break;
				case FRAMESKIP_AUTO_BOTH:
					strcpy(achTexto, "Frameskip: Auto (both)");
				break;
				default:
					strcpy(achTexto, "");
				break;
			}

			rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y;
			DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

			//TEXTO FRAMESKIP
			//if(bAutoFrameskip)
			if(iAutoFrameskip > FRAMESKIP_FIXED)
			{
				strcpy(achTexto, "Frameskip: <=");
			}
			else
			{
				strcpy(achTexto, "Frameskip: ");
			}
			cAux = '0' + nFrameskip;
			iLongitud = strlen(achTexto);
			achTexto[iLongitud] = cAux;
			achTexto[iLongitud + 1] = '\0';

			rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y + MENU_OPTION_HEIGHT;
			DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

			//TEXTO SOUND, solo en menu inicial
			//if( iMenuInicial == 1 )
			//{
				//TEXTO MOUSE
				sprintf(achTexto, "Pointer Speed: %d", iMouseSpeed);
				rectTextoy = iDesplModoGrafico + ( (2 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

				//TEXTO BUS SPEED
				//SE CAMBIA POR BLIT ALL
				if(bBlitAll)
				{
					strcpy(achTexto, "Graphics: everything");
				}
				else
				{
					strcpy(achTexto, "Graphics: optimized");
				}
				//sprintf(achTexto, "Bus Speed: x%d", iGlobalSpeed);
				rectTextoy = iDesplModoGrafico + ( (3 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

				//SOUND
				if(iEnableSound == 0)
				{
					strcpy(achTexto, "Sound OFF");
				}
				else if(iEnableSound == 1)
				{
					strcpy(achTexto, "Sound On");
				}
				////
				else if(iEnableSound == 2)
				{
					strcpy(achTexto, "Sound On, Overclocked x2");
				}
				else if(iEnableSound == 3)
				{
					strcpy(achTexto, "Sound On, Overclocked x3");
				}
				else if(iEnableSound == 4)
				{
					strcpy(achTexto, "Sound On, Overclocked x4");
				}
				//////

				rectTextoy = iDesplModoGrafico + ( (4 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);


				//TEXTO SOUND QUALITY
				if(iSoundQuality == 1)
				{
					strcpy(achTexto, "Audio Channels: 16");
				}
				else if(iSoundQuality == 2)
				{
					strcpy(achTexto, "Audio Channels: 8");
				}
				/////
				else if(iSoundQuality == 4)
				{
					strcpy(achTexto, "Audio Channels: 4");
				}
				else if(iSoundQuality == 8)
				{
					strcpy(achTexto, "Audio Channels: 2");
				}

				rectTextoy = iDesplModoGrafico + ( (5 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

				//SOUND MODE
				if(iSoundMode == SOUND_MODE_ASYNC)
				{
					strcpy(achTexto, "Sound Mode: Async");
				}
				else if(iSoundMode == SOUND_MODE_SYNC)
				{
					strcpy(achTexto, "Sound Mode: Sync");
				}
				else if(iSoundMode == SOUND_MODE_SYNC_INTERPOLATED)
				{
					strcpy(achTexto, "Sound Mode: Sync Interpolated");
				}
				rectTextoy = iDesplModoGrafico + ( (6 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

				//TEXTO USE DYNAREC
				if(iUsarDynarec)
				{
					strcpy(achTexto, "Use Dynarec: Yes (faster)");
				}
				else
				{
					strcpy(achTexto, "Use Dynarec: No (slower)");
				}

				rectTextoy = iDesplModoGrafico + ( (7 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

				//TEXTO LIMIT FRAMERATE
				if(iLimitFramerate == LIMIT_FRAMERATE_HCF)
				{
					strcpy(achTexto, "Limit to 60 FPS: Yes - HCF");
				}
				else if(iLimitFramerate == LIMIT_FRAMERATE_DESMUME)
				{
					strcpy(achTexto, "Limit to 60 FPS: Yes - Desmume");
				}
				else
				{
					strcpy(achTexto, "Limit to 60 FPS: No");
				}

				rectTextoy = iDesplModoGrafico + ( (8 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
				DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

			//} //Menu inicial



			//TEXTO INSTRUCTIONS
			strcpy(achTexto, "A/B: Toggle   START: Confirm ");
			//DesmumeX
			//rectTexto.y = 330;
			//Anyado Sound Quality

			//Sound Mode
			rectTextoy = iDesplModoGrafico + 175;

			//rectTexto.y = 360;
			//Anyado Auto Frameskip

			DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, achTexto);

			//RECTANGLE
			switch(iOpcionSeleccionada)
			{
				case 0:
					rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y - MENU_OPTION_HEIGHT;
					break;
				case 1:
					rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y;
					break;
				case 2:
					rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y + MENU_OPTION_HEIGHT;
					break;
				case 3:
					rectTextoy = iDesplModoGrafico + ( (2 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 4:
					rectTextoy = iDesplModoGrafico + ( (3 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 5:
					rectTextoy = iDesplModoGrafico + ( (4 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 6:
					rectTextoy = iDesplModoGrafico + ( (5 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 7:
					rectTextoy = iDesplModoGrafico + ( (6 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 8:
					rectTextoy = iDesplModoGrafico + ( (7 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				case 9:
					rectTextoy = iDesplModoGrafico + ( (8 * MENU_OPTION_HEIGHT) + MENU_FIRST_OPTION_Y );
					break;
				default:
					rectTextoy = iDesplModoGrafico + MENU_FIRST_OPTION_Y - MENU_OPTION_HEIGHT;
					break;
			}

			rectTextoy -= MENU_RECTANGLE_OFFSET_Y;
			rectTextox -= MENU_RECTANGLE_OFFSET_X;

			///DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, "->");
			DrawText((short*)GPU_screen, rectTextox, rectTextoy - 1, false, "-");
			DrawText((short*)GPU_screen, rectTextox, rectTextoy, false, "-");
			DrawText((short*)GPU_screen, rectTextox, rectTextoy + 1, false, "-");

			DrawText((short*)GPU_screen, rectTextox + 4, rectTextoy, false, ">");
			DrawText((short*)GPU_screen, rectTextox + 5, rectTextoy, false, ">");
			DrawText((short*)GPU_screen, rectTextox + 6, rectTextoy, false, ">");

			rectTextox += MENU_RECTANGLE_OFFSET_X;

			//Refresh screen
			Gu_draw();
		}

		iRedibujar = 0;

		SDL_Delay(20);

	}

	iModoGraficoNuevo = iModoGraficoBackup;

	//HCF If sound settings were changed, a reset is needed
	if(iResetSound)
	{
		//HCF Adjusting sound mode (SYNC)
		if(iSoundMode != SOUND_MODE_ASYNC)
			SPU_SetSynchMode(1, CommonSettings.SPU_sync_method);
		else
			SPU_SetSynchMode(0, CommonSettings.SPU_sync_method);

		//HCF Adjusting sound mode (INTERPOLATION)
		if(iSoundMode == SOUND_MODE_SYNC_INTERPOLATED)
			CommonSettings.spuInterpolationMode = 1;
		else
			CommonSettings.spuInterpolationMode = 0;
	}

	//HCF Sound is only unpaused if we selected AUDIO ON (this avoids noise)
	if(iEnableSound)
		SDL_PauseAudio(0);
}*/

/* Load default joystick and keyboard configurations */
void load_default_config(const u16 kbCfg[])
{
  	memcpy(keyboard_cfg, kbCfg, sizeof(keyboard_cfg));
  	memcpy(joypad_cfg, default_joypad_cfg, sizeof(joypad_cfg));
}

/* Set all buttons at once */
static void set_joy_keys(const u16 joyCfg[])
{
  	memcpy(joypad_cfg, joyCfg, sizeof(joypad_cfg));
}

/* Initialize joysticks */
BOOL init_joy( void) {
  	int i;
  	BOOL joy_init_good = TRUE;


  	set_joy_keys(default_joypad_cfg);

  	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
    {
      	fprintf(stderr, "Error trying to initialize joystick support: %s\n",
              	SDL_GetError());
      	return FALSE;
    }

	//GAMEPAD = SDL_JoystickOpen(0); //Open Pad 1
	SDL_JoystickEventState(SDL_ENABLE); //Enable event stuff for our pad
    for(i = 0; i < NUM_BUTTONS; i++)
	{
	   Buttons[i] = 0;
	}

  	return joy_init_good;
}

/* Unload joysticks */
void uninit_joy( void)
{
	int i;

	if (open_joysticks != NULL) {
		for (i = 0; i < SDL_NumJoysticks(); i++) {
			SDL_JoystickClose( open_joysticks[i]);
		}

		free(open_joysticks);
	}

	open_joysticks = NULL;
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/* Return keypad vector with given key set to 1 */
u16 lookup_joy_key (u16 keyval) {
  int i;
  u16 Key = 0;

  for(i = 0; i < NB_KEYS; i++)
    if(keyval == joypad_cfg[i]) {
      Key = KEYMASK_(i);
      break;
    }

  return Key;
}

/* Return keypad vector with given key set to 1 */
u16 lookup_key (u16 keyval) {
	int i;
	u16 Key = 0;

	for (i = 0; i < NB_KEYS; i++) {
		if(keyval == keyboard_cfg[i]) {
			Key = KEYMASK_(i);
			break;
		}
	}

	return Key;
}

/* Get pressed joystick key */
u16 get_joy_key(int index) {
	BOOL done = FALSE;
	SDL_Event event;
	u16 key = joypad_cfg[index];

	/* Enable joystick events if needed */
	if( SDL_JoystickEventState(SDL_QUERY) == SDL_IGNORE )
		SDL_JoystickEventState(SDL_ENABLE);

	while(SDL_WaitEvent(&event) && !done)
		{
		switch(event.type)
			{
			case SDL_JOYBUTTONDOWN:
			printf( "Device: %d; Button: %d\n", event.jbutton.which, event.jbutton.button );
			key = ((event.jbutton.which & 15) << 12) | JOY_BUTTON << 8 | (event.jbutton.button & 255);
			done = TRUE;
			break;
			case SDL_JOYAXISMOTION:
			/* Dead zone of 50% */
			if( (abs(event.jaxis.value) >> 14) != 0 )
				{
				key = ((event.jaxis.which & 15) << 12) | JOY_AXIS << 8 | ((event.jaxis.axis & 127) << 1);
				if (event.jaxis.value > 0) {
					printf( "Device: %d; Axis: %d (+)\n", event.jaxis.which, event.jaxis.axis );
					key |= 1;
				}
				else
					printf( "Device: %d; Axis: %d (-)\n", event.jaxis.which, event.jaxis.axis );
				done = TRUE;
				}
			break;
			case SDL_JOYHATMOTION:
			/* Diagonal positions will be treated as two separate keys being activated, rather than a single diagonal key. */
			/* JOY_HAT_* are sequential integers, rather than a bitmask */
			if (event.jhat.value != SDL_HAT_CENTERED) {
				key = ((event.jhat.which & 15) << 12) | JOY_HAT << 8 | ((event.jhat.hat & 63) << 2);
				/* Can't just use a switch here because SDL_HAT_* make up a bitmask. We only want one of these when assigning keys. */
				if ((event.jhat.value & SDL_HAT_UP) != 0) {
				key |= JOY_HAT_UP;
				printf( "Device: %d; Hat: %d (Up)\n", event.jhat.which, event.jhat.hat );
				}
				else if ((event.jhat.value & SDL_HAT_RIGHT) != 0) {
				key |= JOY_HAT_RIGHT;
				printf( "Device: %d; Hat: %d (Right)\n", event.jhat.which, event.jhat.hat );
				}
				else if ((event.jhat.value & SDL_HAT_DOWN) != 0) {
				key |= JOY_HAT_DOWN;
				printf( "Device: %d; Hat: %d (Down)\n", event.jhat.which, event.jhat.hat );
				}
				else if ((event.jhat.value & SDL_HAT_LEFT) != 0) {
				key |= JOY_HAT_LEFT;
				printf( "Device: %d; Hat: %d (Left)\n", event.jhat.which, event.jhat.hat );
				}
				done = TRUE;
			}
			break;
			}
		}

	if( SDL_JoystickEventState(SDL_QUERY) == SDL_ENABLE )
		SDL_JoystickEventState(SDL_IGNORE);

	return key;
}

/* Get and set a new joystick key */
u16 get_set_joy_key(int index) {
  joypad_cfg[index] = get_joy_key(index);

  return joypad_cfg[index];
}

static signed long
screen_to_touch_range( signed long scr, float size_ratio) {
  return (signed long)((float)scr * size_ratio);
}

/* Set mouse coordinates */
static void set_mouse_coord(signed long x,signed long y)
{
  if(x<0) x = 0; else if(x>255) x = 255;
  if(y<0) y = 0; else if(y>192) y = 192;
  mouse.x = x;
  mouse.y = y;
}

// Adapted from Windows port
bool allowUpAndDown = false;
static buttonstruct<int> cardinalHeldTime = {0};

static void RunAntipodalRestriction(const buttonstruct<bool>& pad)
{
	if(allowUpAndDown)
		return;

	pad.U ? (cardinalHeldTime.U++) : (cardinalHeldTime.U=0);
	pad.D ? (cardinalHeldTime.D++) : (cardinalHeldTime.D=0);
	pad.L ? (cardinalHeldTime.L++) : (cardinalHeldTime.L=0);
	pad.R ? (cardinalHeldTime.R++) : (cardinalHeldTime.R=0);
}
static void ApplyAntipodalRestriction(buttonstruct<bool>& pad)
{
	if(allowUpAndDown)
		return;

	// give preference to whichever direction was most recently pressed
	if(pad.U && pad.D)
		if(cardinalHeldTime.U < cardinalHeldTime.D)
			pad.D = false;
		else
			pad.U = false;
	if(pad.L && pad.R)
		if(cardinalHeldTime.L < cardinalHeldTime.R)
			pad.R = false;
		else
			pad.L = false;
}

/* Update NDS keypad */
void update_keypad(u16 keys)
{
	// Set raw inputs
	{
		buttonstruct<bool> input = {};
		input.G = (keys>>12)&1;
		input.E = (keys>>8)&1;
		input.W = (keys>>9)&1;
		input.X = (keys>>10)&1;
		input.Y = (keys>>11)&1;
		input.A = (keys>>0)&1;
		input.B = (keys>>1)&1;
		input.S = (keys>>3)&1;
		input.T = (keys>>2)&1;
		input.U = (keys>>6)&1;
		input.D = (keys>>7)&1;
		input.L = (keys>>5)&1;
		input.R = (keys>>4)&1;
		input.F = (keys>>14)&1;
		RunAntipodalRestriction(input);
		NDS_setPad(
			input.R, input.L, input.D, input.U,
			input.T, input.S, input.B, input.A,
			input.Y, input.X, input.W, input.E,
			input.G, input.F);
	}

	// Set real input
	NDS_beginProcessingInput();
	{
		UserButtons& input = NDS_getProcessingUserInput().buttons;
		ApplyAntipodalRestriction(input);
	}
	NDS_endProcessingInput();
}

/* Retrieve current NDS keypad */
u16 get_keypad( void)
{
  u16 keypad;
  keypad = ~MMU.ARM7_REG[0x136];
  keypad = (keypad & 0x3) << 10;
#ifdef WORDS_BIGENDIAN
  keypad |= ~(MMU.ARM9_REG[0x130] | (MMU.ARM9_REG[0x131] << 8)) & 0x3FF;
#else
  keypad |= ~((u16 *)MMU.ARM9_REG)[0x130>>1] & 0x3FF;
#endif
  return keypad;
}

/*
 * The internal joystick events processing function
 */
static int
do_process_joystick_events( u16 *keypad, SDL_Event *event) {
  int processed = 1;
  u16 key_code;
  u16 key;
  u16 key_o;
  u16 key_u;
  u16 key_r;
  u16 key_d;
  u16 key_l;

  switch ( event->type)
    {
      /* Joystick axis motion
         Note: button constants have a 1bit offset. */
    case SDL_JOYAXISMOTION:
      key_code = ((event->jaxis.which & 15) << 12) | JOY_AXIS << 8 | ((event->jaxis.axis & 127) << 1);
      if( (abs(event->jaxis.value) >> 14) != 0 )
        {
          if (event->jaxis.value > 0)
            key_code |= 1;
          key = lookup_joy_key( key_code );
          key_o = lookup_joy_key( key_code ^ 1 );
          if (key != 0)
            ADD_KEY( *keypad, key );
          if (key_o != 0)
            RM_KEY( *keypad, key_o );
        }
      else
        {
          // Axis is zeroed
          key = lookup_joy_key( key_code );
          key_o = lookup_joy_key( key_code ^ 1 );
          if (key != 0)
            RM_KEY( *keypad, key );
          if (key_o != 0)
            RM_KEY( *keypad, key_o );
        }
      break;

    case SDL_JOYHATMOTION:
      /* Diagonal positions will be treated as two separate keys being activated, rather than a single diagonal key. */
      /* JOY_HAT_* are sequential integers, rather than a bitmask */
      key_code = ((event->jhat.which & 15) << 12) | JOY_HAT << 8 | ((event->jhat.hat & 63) << 2);
      key_u = lookup_joy_key( key_code | JOY_HAT_UP );
      key_r = lookup_joy_key( key_code | JOY_HAT_RIGHT );
      key_d = lookup_joy_key( key_code | JOY_HAT_DOWN );
      key_l = lookup_joy_key( key_code | JOY_HAT_LEFT );
      if ((key_u != 0) && ((event->jhat.value & SDL_HAT_UP) != 0))
        ADD_KEY( *keypad, key_u );
      else if (key_u != 0)
        RM_KEY( *keypad, key_u );
      if ((key_r != 0) && ((event->jhat.value & SDL_HAT_RIGHT) != 0))
        ADD_KEY( *keypad, key_r );
      else if (key_r != 0)
        RM_KEY( *keypad, key_r );
      if ((key_d != 0) && ((event->jhat.value & SDL_HAT_DOWN) != 0))
        ADD_KEY( *keypad, key_d );
      else if (key_d != 0)
        RM_KEY( *keypad, key_d );
      if ((key_l != 0) && ((event->jhat.value & SDL_HAT_LEFT) != 0))
        ADD_KEY( *keypad, key_l );
      else if (key_l != 0)
        RM_KEY( *keypad, key_l );
      break;

      /* Joystick button pressed */
      /* FIXME: Add support for BOOST */
    case SDL_JOYBUTTONDOWN:
      key_code = ((event->jbutton.which & 15) << 12) | JOY_BUTTON << 8 | (event->jbutton.button & 255);
      key = lookup_joy_key( key_code );
      if (key != 0)
        ADD_KEY( *keypad, key );
      break;

      /* Joystick button released */
    case SDL_JOYBUTTONUP:
      key_code = ((event->jbutton.which & 15) << 12) | JOY_BUTTON << 8 | (event->jbutton.button & 255);
      key = lookup_joy_key( key_code );
      if (key != 0)
        RM_KEY( *keypad, key );
      break;

    default:
      processed = 0;
      break;
    }

  return processed;
}

/*
 * Process only the joystick events
 */
void
process_joystick_events( u16 *keypad) {
  SDL_Event event;

  /* IMPORTANT: Reenable joystick events if needed. */
  if(SDL_JoystickEventState(SDL_QUERY) == SDL_IGNORE)
    SDL_JoystickEventState(SDL_ENABLE);

  /* There's an event waiting to be processed? */
  while (SDL_PollEvent(&event))
    {
      do_process_joystick_events( keypad, &event);
    }
}

// MotoLegacy (7/25/2020) - cleaned this function up a bit, rewrote some
// variables in english. original creds to HCF
void process_ctrls_event(struct ctrls_event_config *cfg)
{
	int i;
	bool MouseUpdate; // MotoLegacy - originally an int.. lol
	int x, y;
	unsigned short key;

	int cause_quit = 0;

	vdGetButtons();

	for (i = 0; i < 12; ++i) {
		if (Buttons[CTRL_Default_NDS_CFG[i]])
			ADD_KEY(cfg->keypad, KEYMASK_(i));
		else
			RM_KEY(cfg->keypad, KEYMASK_(i));
	}

	// Touchscreen Pointer Movement
	x = joyx;
	y = joyy;

	MouseUpdate = false;

	// Update Mouse Position
	// FIXME: Cull SDL
	SDL_WarpMouse(mouse.x, mouse.y);

	// X Axis
	if (x > 15 || x < -15) {
		mouse.x += (x/5);
	}

	// Y Axis
	if (y > 15 || y < -15) {
		mouse.y += (y/5);
	}

	// MotoLegacy - This is just one big waste of instructions at the moment.
	// TODO - Do any MouseWarping for Graphics Mode, if needbe.
	/*
	// Warp Mouse depending on Graphics Mode
	if (MouseUpdate == true) {
		// iGraphicsMode
		switch (iModoGrafico) {
			case 1:
			case 2:
				break;
			default:
				//HCF Una pantalla encima de otra
				switch (iModoStretchNuevo) {
					case STRETCH_MODE_FULL:
					case STRETCH_MODE_HALF:
					case STRETCH_MODE_NONE:
						break;
				}
		}
	}*/

	// Drag and Drop Mode(??)
	if (Buttons[BUTTON_NOTE]) {
		mouse.down = TRUE;
	} else {
		if (mouse.down == TRUE) {
			mouse.click = TRUE;
			mouse.down = FALSE;
		} else {
			mouse.click = FALSE;
		}
	}

	// Graphics Mode Toggle
	// MotoLegacy - Just another waste of precious cycles..
	// TODO - Graphics Modes
	/*if (ashBotones[BOTON_NEGRO]) {
		// iChangingGraphicsMode
		if (!iCambiandoModoGrafico) {
			iModoGraficoNuevo = ((iModoGrafico+1)%3);
			iCambiandoModoGrafico = 1;

			switch (iModoGraficoNuevo) {
				case 1:
				case 2:
					break;
				default:
					switch (iModoStretchNuevo) {
						case STRETCH_MODE_FULL:
						case STRETCH_MODE_HALF:
						case STRETCH_MODE_NONE:
							break;
					}
					break;
			}
		}
	} else if (iChangingGraphicsMode) {
		iChangingGraphicsMode = 0;
	}*/

	// In-Game Menu
	if ((!Buttons[BUTTON_START]) && (Buttons[BUTTON_SELECT]))
      	vdXBOptionsMenu(0);

	// Quit Application
	if (Buttons[BUTTON_START] && Buttons[BUTTON_SELECT])
		exit(0);
}
