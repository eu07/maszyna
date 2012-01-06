// Menu.cpp: implementation of the Menu class.
//
//////////////////////////////////////////////////////////////////////

#include "Menu.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Menu::Menu()
{
	error = 1;
}

Menu::~Menu()
{
}

Menu::Menu( Font *font)
{
	if(font==NULL)error=1;
	f = font;
	akt_item=0;
}

void Menu::Render( float timeframe)
{
	POINT MousePoint;
	RECT WindowRect,ClientRect;
	GetCursorPos(&MousePoint);
	GetWindowRect( hWnd, &WindowRect);
	GetClientRect( hWnd, &ClientRect);
	int vyska = ClientRect.bottom;
	int sirka = ClientRect.right;
	int mouse_move=0;
	if( last_mouse_pos.x != MousePoint.x || last_mouse_pos.y != MousePoint.y) mouse_move=1;

	if (fullscreen)
		AdjustWindowRectEx(&ClientRect, WS_POPUP, FALSE, WS_EX_APPWINDOW);
	else
		AdjustWindowRectEx(&ClientRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);	
	
	float xf = (screen_x_font/(float)sirka)*(MousePoint.x-WindowRect.left+ClientRect.left)/(float)11;
	float yf = (screen_y_font/(float)vyska)*(MousePoint.y-WindowRect.top+ClientRect.top)/16;

	int ydelta=1;
	int xdelta=1;
	int znakov=0;		// max 70;
	int riadkov=0;		// max 35
	
//	y = screen_y_font - (y+1)*16;

	int maxNameLen=0;
	int maxExtLen=0;
	for( int i=0; i<items.size(); i++)			// vypocet dlzky
	{
		int len=0;
		if( items[i].type!=ITEM_INT)
		{
			len += strlen(items[i].name);
			if(len>maxNameLen)maxNameLen=len;
		}
		else
		{
			for(int j=0; j<items[i].IntItems.size(); j++)
			{
				if( strlen(items[i].IntItems[j].name)>len) len = strlen(items[i].IntItems[j].name);
			}
			if(len>maxNameLen)maxNameLen=len;
		}
		len=0;		
		if( items[i].type==ITEM_BOOL )len+=7;		// :_OFF_-	= 7
		else if( items[i].type==ITEM_KEY )len+=3;	// :_-		= 3
		else if( items[i].type==ITEM_KEY_CHANGE )len+=3;	// :_-		= 3
		else if( items[i].type==ITEM_INT )
		{
			len+=3;									// :_-		= 3
			if( items[i].p == NULL) len+=4;			// _!!!		= 4
		}
		else if( items[i].type==ITEM_FLOAT )len+=11;// :_1.34567_-
		else len+=7;								// :_!!!_!	= 7
		if(len>maxExtLen)maxExtLen = len;
		riadkov++;
	}
	znakov = maxNameLen+maxExtLen;
	if(znakov>70)znakov=70;
	xdelta = (70-znakov)/2+1;
	ydelta = (35-riadkov)/2+1;

	int x = (int)xf-xdelta;
	int y = (int)yf-ydelta;

	int akt_item_old = akt_item;
	if(mouse_move)
	{
		if(y>=0 && y<items.size() && x>=0 && x<znakov) akt_item=y;
	}
	else
	{
		if(keys[VK_DOWN])
		{
			akt_item = (akt_item+1)%items.size();
			keys[VK_DOWN]=0;
		}
		if(keys[VK_UP])
		{
			akt_item--;
			if(akt_item<0)akt_item=items.size()-1;
			keys[VK_UP]=0;
		}
	}

	if( akt_item != akt_item_old )
	{
		keys[VK_LBUTTON]=0;
		keys[VK_RIGHT]=0;
		keys[VK_LEFT]=0;
	}

	if(items[akt_item].type==ITEM_BOOL)
	{
		if(keys[VK_LBUTTON])
		{
			if(items[akt_item].p!=NULL)*(int*)items[akt_item].p=!*(int*)items[akt_item].p;
			keys[VK_LBUTTON]=0;
		}
		if(keys[VK_RIGHT])
		{
			if(items[akt_item].p!=NULL)*(int*)items[akt_item].p=1;
			keys[VK_RIGHT]=0;
		}
		if(keys[VK_LEFT])
		{
			if(items[akt_item].p!=NULL)*(int*)items[akt_item].p=0;
			keys[VK_LEFT]=0;
		}
	}
	else if( items[akt_item].type==ITEM_INT)
	{
		if( items[akt_item].IntItems.size()>0)
		{
			if( keys[VK_LBUTTON] || keys[VK_RIGHT] )
			{
				items[akt_item].IntIndex = (items[akt_item].IntIndex+1)%items[akt_item].IntItems.size();
			}
			if(keys[VK_LEFT])
			{
				items[akt_item].IntIndex--;
				if(items[akt_item].IntIndex<0) items[akt_item].IntIndex = items[akt_item].IntItems.size()-1;
			}
			if(items[akt_item].p!=NULL)
			{
				*(int*)items[akt_item].p = items[akt_item].IntItems[items[akt_item].IntIndex].value;
			}
		}
		keys[VK_LBUTTON]=0;
		keys[VK_RIGHT]=0;
		keys[VK_LEFT]=0;
	}
	else if( items[akt_item].type==ITEM_FLOAT)
	{
		float step=0.1f;
		if(keys[VK_SHIFT])step=0.01f;
		if(keys[VK_LBUTTON] && mouse_move)
		{
			items[akt_item].float_item.aktual+= (float)(MousePoint.x - last_mouse_pos.x)*step*0.03f;
			if(items[akt_item].float_item.aktual>1.0f)items[akt_item].float_item.aktual=1.f;
			if(items[akt_item].float_item.aktual<0.0f)items[akt_item].float_item.aktual=0.f;
			if(items[akt_item].p!=NULL)
			{
				*(float*)items[akt_item].p = items[akt_item].float_item.min + \
					items[akt_item].float_item.aktual*(items[akt_item].float_item.max-items[akt_item].float_item.min);
			}
		}
		if(keys[VK_RIGHT])
		{
			items[akt_item].float_item.aktual+=step*timeframe;
			if(items[akt_item].float_item.aktual>1.0f)items[akt_item].float_item.aktual=1.f;
			if(items[akt_item].p!=NULL)
			{
				*(float*)items[akt_item].p = items[akt_item].float_item.min + \
					items[akt_item].float_item.aktual*(items[akt_item].float_item.max-items[akt_item].float_item.min);
			}
		}
		if(keys[VK_LEFT])
		{
			items[akt_item].float_item.aktual-=step*timeframe;
			if(items[akt_item].float_item.aktual<0.0f)items[akt_item].float_item.aktual=0.f;
			if(items[akt_item].p!=NULL)
			{
				*(float*)items[akt_item].p = items[akt_item].float_item.min + \
					items[akt_item].float_item.aktual*(items[akt_item].float_item.max-items[akt_item].float_item.min);
			}
		}
	}
	else if( items[akt_item].type==ITEM_KEY_CHANGE)
	{
		if(keys[VK_LBUTTON])
		{
			if(items[akt_item].p!=NULL) *(int*)items[akt_item].p=1;
			keys[VK_LBUTTON]=0;
		}
	}

	for(int j=0x30; j<=0x39; j++)
	{
		if(keys[j])
		{
			items[akt_item].key = j;
			keys[j]=0;
		}
	}
	for( int j=0x41; j<=0x5A; j++)
	{
		if(j=='A' || j=='S' || j=='D' || j=='W' || j=='R')continue;
		if(keys[j])
		{
			items[akt_item].key = j;
			keys[j]=0;
		}
	}
	if(keys[VK_BACK])
	{
		items[akt_item].key = -1;
		keys[VK_BACK]=0;
	}
	f->Begin();
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0,0,0,0.3f);
	glLoadIdentity();
	glBegin(GL_QUADS);
		glVertex2i(-5+xdelta*11,5+screen_y_font-(ydelta+riadkov+1)*16);
		glVertex2i((xdelta+znakov+1)*11,5+screen_y_font-(ydelta+riadkov+1)*16);
		glVertex2i((xdelta+znakov+1)*11,-5+screen_y_font-(ydelta-1)*16);
		glVertex2i(-5+xdelta*11,-5+screen_y_font-(ydelta-1)*16);
	glEnd();
	glColor4f(1,1,1,1);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_TEXTURE_2D);

	for(int i=0; i<items.size(); i++)
	{
		if(i==akt_item)glColor3f(1,0,0);
		if(items[i].type==ITEM_TEXT)
		{
			f->Print( xdelta, i+ydelta, items[i].name, 0);
		}
		else if(items[i].type==ITEM_KEY || items[i].type==ITEM_KEY_CHANGE)
		{
			if(items[i].p!=NULL)
			{
				char text_keys[2]={0};
				f->Print( xdelta, i+ydelta, items[i].name, 0);
				if(items[i].key==-1)text_keys[0]='-';
				else text_keys[0]=items[i].key;
				f->Print( xdelta+maxNameLen, i+ydelta, ":", 0);
				f->Print( xdelta+maxNameLen+2, i+ydelta, text_keys, 0);
			}
			else
			{
				f->Print( xdelta+maxNameLen, i+ydelta, ": !!! !", 0);
			}
		}
		else if(items[i].type==ITEM_BOOL)
		{
			if(items[i].p!=NULL)
			{
				char text_keys[2]={0};
				f->Print( xdelta, i+ydelta, items[i].name, 0);
				if( *(int*)items[i].p )f->Print( xdelta+maxNameLen, i+ydelta, ": ON", 0);
				else f->Print( xdelta+maxNameLen, i+ydelta, ": OFF", 0);
				if(items[i].key==-1)text_keys[0]='-';
				else text_keys[0]=items[i].key;
				f->Print( xdelta+maxNameLen+6, i+ydelta, text_keys, 0);
			}
			else
			{
				f->Print( xdelta+maxNameLen, i+ydelta, ": !!! !", 0);
			}
		}
		else if(items[i].type==ITEM_INT)
		{
			char text_keys[2]={0};
			if( items[i].IntItems.size()<=0)f->Print( xdelta, i+ydelta, items[i].name, 0);
			else f->Print( xdelta, i+ydelta, items[i].IntItems[items[i].IntIndex].name, 0);
			if(items[i].key==-1)text_keys[0]='-';
			else text_keys[0]=items[i].key;
			f->Print( xdelta+maxNameLen, i+ydelta, ":", 0);
			f->Print( xdelta+maxNameLen+2, i+ydelta, text_keys, 0);
			if(items[i].p==NULL)f->Print( xdelta+maxNameLen+4, i+ydelta, "!!!", 0);
		}
		else if(items[i].type==ITEM_FLOAT)
		{
			char text_keys[2]={0};
			char text_float[50];
			f->Print( xdelta, i+ydelta, items[i].name, 0);
			if(items[i].key==-1)text_keys[0]='-';
			else text_keys[0]=items[i].key;
			f->Print( xdelta+maxNameLen+10, i+ydelta, text_keys, 0);
			float a = items[i].float_item.min+items[i].float_item.aktual*(items[i].float_item.max-items[i].float_item.min);
			gcvt( (double)a, 4, text_float);
			f->Print( xdelta+maxNameLen, i+ydelta, ":", 0);
			f->Print( xdelta+maxNameLen+2, i+ydelta, text_float, 0);
		}
		glColor3f(1,1,1);
	}

	for(int i=0;i<znakov;i++){f->Print(xdelta+i,ydelta-1,"\x13",1);}		//top
	for(int i=0;i<znakov;i++){f->Print(xdelta+i,ydelta+riadkov,"\x19",1);} 	//botton
	for(int i=0;i<riadkov;i++){f->Print(xdelta-1,ydelta+i,"\x15",1);}		//left
	for(int i=0;i<riadkov;i++){f->Print(xdelta+znakov,ydelta+i,"\x17",1);}	//right
	f->Print(xdelta-1,ydelta-1,"\x12",1);								//top and left
	f->Print(xdelta+znakov,ydelta-1,"\x14",1);							//top and right
	f->Print(xdelta-1,ydelta+riadkov,"\x18",1);							//botton and left
	f->Print(xdelta+znakov,ydelta+riadkov,"\x1A",1);					//botton and right
	f->End();
	last_mouse_pos = MousePoint;
}

void Menu::ChangeSetting( float timeframe)
{
	for(int i=0; i<items.size(); i++)
	{
		if(items[i].key!=-1 )
		{
			if(keys[items[i].key])
			{
				if(items[i].type==ITEM_BOOL)
				{
					if(items[i].p!=NULL)
					{
						*(int*)items[i].p=!*(int*)items[i].p;
					}
				}
				else if(items[i].type==ITEM_INT)
				{
					items[i].IntIndex = (items[i].IntIndex+1)%items[i].IntItems.size();
					if(items[i].p!=NULL)
					{
						*(int*)items[i].p = items[i].IntItems[items[i].IntIndex].value;
					}
				}
				else if(items[i].type==ITEM_FLOAT)
				{
					if(keys[VK_SHIFT])items[i].float_item.aktual-=0.1f*timeframe;
					else items[i].float_item.aktual+=0.1f*timeframe;
					if(items[i].float_item.aktual>1.0f)items[i].float_item.aktual=1.f;
					if(items[i].float_item.aktual<0.0f)items[i].float_item.aktual=0.f;
					if(items[i].p!=NULL)
						*(float*)items[i].p = items[i].float_item.min + items[i].float_item.aktual*(items[i].float_item.max-items[i].float_item.min);
				}
				else if(items[i].type==ITEM_KEY_CHANGE)
				{
					if(items[i].p!=NULL) *(int*)items[i].p=1;
				}

			}
			if(items[i].type==ITEM_KEY)
			{
				if(items[i].p!=NULL)
				{
					*(int*)items[i].p=keys[items[i].key];
				}
			}
		}
	}
	
	for( int i=0; i<items.size(); i++)
	{
		if(items[i].key!=-1 && keys[items[i].key])
		{
			if(items[i].type==ITEM_BOOL || items[i].type==ITEM_INT || items[i].type==ITEM_KEY_CHANGE)
			{
				keys[items[i].key]=0;
			}
		}
	}
}

void Menu::AddBool(char *name, int key, void *p, int value)
{
	zMenu_item i;

	strncpy( i.name, name, 50);
	i.name[51]=NULL;
	i.key = key;
	i.p = p;
	if(i.p!=NULL)*(int*)i.p = value;
	i.type = ITEM_BOOL;
	items.push_back(i);
}

void Menu::AddText(char *text)
{
	zMenu_item i;

	strncpy( i.name, text, 70);
	i.name[70]=NULL;
	i.key = -1;
	i.p = NULL;
	i.type = ITEM_TEXT;
	items.push_back(i);
}

void Menu::AddKey(char *name, int key, void* p)
{
	zMenu_item i;

	strncpy( i.name, name, 50);
	i.name[50]=NULL;
	i.key = key;
	if(i.key!=-1)keys[i.key]=0;
	i.p = p;
	if(i.p!=NULL)*(int*)i.p = 0;
	i.type = ITEM_KEY;
	items.push_back(i);
}

void Menu::AddInt(char *name, int key, void *p)
{
	zMenu_item i;

	strncpy( i.name, name, 70);
	i.name[70]=NULL;
	i.key = key;
	i.p = p;
	i.type = ITEM_INT;
	i.IntIndex = 0;
	items.push_back(i);
}

void Menu::AddIntItem(char *name, void *p)
{
	if(items.size()<=0)return;
	int index = items.size()-1;
	if(items[index].type!=ITEM_INT)return;
	zMenu_item_int i;

	strncpy( i.name, name, 70);
	i.name[70]=NULL;
	if(p!=NULL)i.value = *(int*)p;
	else i.value=0;
	items[index].IntItems.push_back( i);
}

void Menu::AddFloat(char *name, int key, void *p, float min, float max, float aktual)
{
	zMenu_item i;

	strncpy( i.name, name, 50);
	i.name[50]=NULL;
	i.key = key;
	i.p = p;
	i.type = ITEM_FLOAT;
	i.float_item.min = min;
	i.float_item.max = max;
	if( aktual<0) aktual=0;
	else if(aktual>1)aktual=1;
	i.float_item.aktual = aktual;
	items.push_back(i);
}

void Menu::AddFloatComp(char *name, int key, void *p, float min, float max, float value)
{
	float aktual = (value-min)/(max-min);
	if(p!=NULL)*(float*)p=value;
	AddFloat( name, key, p, min, max, aktual);
}

void Menu::AddIntSetIndex(int index)
{
	if(items.size()<=0)return;
	int i = items.size()-1;
	if(items[i].type!=ITEM_INT)return;
	if(index<0)index=0;
	if(index>=items[i].IntItems.size()) index = items[i].IntItems.size()-1;
	items[i].IntIndex = index;
	*(int*)items[i].p = items[i].IntItems[index].value;
}

void Menu::AddKeyChange(char *name, int key, void *p)
{
	zMenu_item i;

	strncpy( i.name, name, 50);
	i.name[50]=NULL;
	i.key = key;
	if(i.key!=-1)keys[i.key]=0;
	i.p = p;
	if(i.p!=NULL)*(int*)i.p = 0;
	i.type = ITEM_KEY_CHANGE;
	items.push_back(i);
}
