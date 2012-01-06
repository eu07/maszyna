#ifndef _INIT_H
#define _INIT_H

#pragma once
#include <windows.h>	// hlavi�kov� s�bor Windows kni�nice
#include <stdio.h>		// hlavi�kov� s�bor standardneho vstupu a vystupu
#include <math.h>		// hlavi�kov� s�bor matematickej kniznice
#include <GL\gl.h>		// hlavi�kov� s�bor OpenGL32 kni�nice
#include <GL\glu.h>		// hlavi�kov� s�bor GLu32 kni�nice
//#include <GL\glaux.h>	// hlavi�kov� s�bor GLaux kni�nice 
#include "extgl.h"

#define screen_x_font 1280			// horizontalne rozlisenie
#define screen_y_font 1024			// vertikalne rozlisenie

#define uhol_kamery 45.0f		// uhol kamery vo zvyslom smere
#define blizka_orezavacia_rovina 0.1f
#define vzdialena_orezavacia_rovina 100.0f

extern bool	keys[256];				// pole - pr�ca s klavesnicou
extern bool	fullscreen;				// indik�tor pre fullscreen
extern		HDC			hDC;		// priv�tny GDI Device Context
extern		HWND		hWnd;		// Handle na okno
extern		HINSTANCE	hInstance;	// In�talacia programu
extern int	error;					// ak je 1 nastala chyba a program sa ma ukoncit
extern int	screen_x;				// horizontalne rozlisenie
extern int	screen_y;				// vertikalne rozlisenie
extern int	screen_bit;				// pocet bitov na 1 pixel
extern int	english;

#endif