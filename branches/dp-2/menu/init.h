#ifndef _INIT_H
#define _INIT_H

#pragma once
#include <windows.h>	// hlavièkový súbor Windows knižnice
#include <stdio.h>		// hlavièkový súbor standardneho vstupu a vystupu
#include <math.h>		// hlavièkový súbor matematickej kniznice
#include <GL\gl.h>		// hlavièkový súbor OpenGL32 knižnice
#include <GL\glu.h>		// hlavièkový súbor GLu32 knižnice
//#include <GL\glaux.h>	// hlavièkový súbor GLaux knižnice 
#include "extgl.h"

#define screen_x_font 1280			// horizontalne rozlisenie
#define screen_y_font 1024			// vertikalne rozlisenie

#define uhol_kamery 45.0f		// uhol kamery vo zvyslom smere
#define blizka_orezavacia_rovina 0.1f
#define vzdialena_orezavacia_rovina 100.0f

extern bool	keys[256];				// pole - práca s klavesnicou
extern bool	fullscreen;				// indikátor pre fullscreen
extern		HDC			hDC;		// privátny GDI Device Context
extern		HWND		hWnd;		// Handle na okno
extern		HINSTANCE	hInstance;	// Inštalacia programu
extern int	error;					// ak je 1 nastala chyba a program sa ma ukoncit
extern int	screen_x;				// horizontalne rozlisenie
extern int	screen_y;				// vertikalne rozlisenie
extern int	screen_bit;				// pocet bitov na 1 pixel
extern int	english;

#endif