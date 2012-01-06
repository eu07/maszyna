#ifndef _MENU_H
#define _MENU_H

#include "init.h"				// zakladne nastavenia a zakladne hlavickove subory
#include "bitmap_Font.h"		// trieda pre vypisovanie textu pomocou bitmapoveho fontu
#include <vector>
using namespace std;

#define ITEM_TEXT		1
#define ITEM_KEY		2
#define ITEM_BOOL		3
#define ITEM_INT		4
#define ITEM_FLOAT		5
#define ITEM_KEY_CHANGE	6

struct zMenu_item_int
{
	char name[71];
	int value;			// int, float
};

struct zMenu_item
{
	char name[71];
	int key;		// -1 nepridelena klavesa
	int type;		// bool, int, float, 
	void *p;		// pointer
	vector<zMenu_item_int> IntItems;		// if type == ITEM_INT
	int IntIndex;
	struct zMenu_item_float
	{
		float min,max,aktual;			// value = min+aktual*(max-min); aktual <0,1>
	}float_item;
};

class Menu  
{
public:
	void AddKeyChange( char* name, int key=-1, void* p=NULL);
	void AddBool( char* name, int key=-1, void* p=NULL, int value=0);
	void AddText( char* text);
	void AddKey( char* name, int key=-1, void* p=NULL);
	void AddInt( char* name, int key=-1, void* p=NULL);
	void AddIntItem( char* name, void* p);
	void AddIntSetIndex( int index);
	void AddFloat( char* name, int key, void* p, float min, float max, float aktual);
	void AddFloatComp( char* name, int key, void* p, float min, float max, float value);
	void ChangeSetting( float timeframe=0.02f);
	void Render( float timeframe=0.02);
	Menu();
	Menu( Font *font);
	virtual ~Menu();
	Font	*f;
	vector<zMenu_item> items;
	int		akt_item;			// riadok
	POINT	last_mouse_pos;
};

#endif // _MENU_H
