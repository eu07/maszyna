//////////////////////////////////////////////////////////////////////
// scene.cpp: implementation of the scene class.
//
//////////////////////////////////////////////////////////////////////

#include "scene.h"

int scene::sirka,scene::vyska;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

scene::scene()
{
	//////////////////////////////////////
	// smerniky na instancie na triedy nastavim na NULL, aby som ich potom nemazal
	c = NULL;
	font0 = NULL;
//	base = NULL;
	menu[0] = NULL;
	menu[1] = NULL;
	if(!extInit()){ error=1; return;}
	smenu = 1;
	
	ReadInit("data/init.txt");

	if(!ext.ARB_texture_cube_map)
	{	MessageBox(hWnd,english ? "ARB_texture_cube_map unsupported !\nI'll use sphere mapping." : "ARB_texture_cube_map nepodporovany !\nPouzije sa sphere mapping.", "ERROR", MB_OK);}
	if(!ext.ARB_multitexture)
	{	MessageBox(hWnd,english ? "GL_ARB_multitexture unsupported !":"GL_ARB_multitexture nepodporovany !", "ERROR", MB_OK);}
//	if(!(ext.max.max_texture_units>1))
//	{	MessageBox(hWnd,english ? "Only one texturing unit, requires minimal 2 units !" : "Iba jedna texturovacia jednotka, pozadovane su minimalne 2!", "ERROR", MB_OK);	error=1;return;}
	if(!ext.ARB_texture_env_combine)
	{	MessageBox(hWnd,english ? "GL_ARB_texture_env_combine unsupported !":"GL_ARB_texture_env_combine nepodporovany !", "ERROR", MB_OK);	}
	
	font0 = new Font("data/font.bmp",&error);		// vytvorenie fontu
	if(error)return;

	//////////////////////////////////////
	// prepinace

	fps = 200;							// v pripade ze nieco zavisy od fps
	timeframe = 1.f/fps;
	clip_distance = vzdialena_orezavacia_rovina;
	kurzor=1;
			
/*	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//cisti buffer v aktualnom viewporte a to iba color a hlbkovy bit
	glEnable(GL_TEXTURE_2D);
	font0->Begin();
	font0->Print_xy_scale(270,400,"Water",0,2.f,2);
	font0->Print_xy_scale(300,320,"Marek Mizanin",0,1.5f,1.5f);
	font0->Print_xy_scale(260,280,"www.mizanin.szm.sk",0,1.5f,1.5f);
	font0->End();
	SwapBuffers(hDC);*/

	//////////////////////////////////////
	// nacitanie textur

	char *dir = "data/bazen/";  char *file_ext = "jpg";
//	char *dir = "data/bazen_bmp/";  char *file_ext = "bmp";
//	char *dir = "data/mars/";	char *file_ext = "jpg";
//	char *dir = "data/nvidia/";	char *file_ext = "bmp";
//	char *dir = "data/dx/";	char *file_ext = "bmp";

	if(ext.ARB_texture_cube_map)
	{
		refra.loadCube(dir, file_ext, filterForTexture);
		refle.loadCube(dir, file_ext, filterForTexture);
	}
	refra_sp.MakeSphereMap(dir, file_ext, filterForTexture);
	refle_sp.MakeSphereMap(dir, file_ext, filterForTexture);

	this->LoadTextureSkyBox(dir, file_ext, skybox, filterForTexture);

	//////////////////////////////////////
	// vytvarame instancie pouzivanych tried
	c = new camera;
//	base = new Case("data/027model/room.ase", filterForTexture, 1, 0.1f);
//	model = new C3ds("data/model/model.3ds", filterForTexture);
	
	menu[0] = new Menu( font0);
	menu[0]->AddText( "Show/hide menu - key F1");
	menu[0]->AddKey ( "Help", 'H', &set.help);
	menu[0]->AddText("");
	int val;
	menu[0]->AddInt( "Drawing", 'K', &set.r.type);
	val = 0; menu[0]->AddIntItem( "Points", &val);
	val = 1; menu[0]->AddIntItem( "Lines", &val);
	val = 2; menu[0]->AddIntItem( "Triangles", &val);
	val = 3; menu[0]->AddIntItem( "Fresnel, light - reflection, dark - refraction", &val);
	val = 4; menu[0]->AddIntItem( "Reflection", &val);
	val = 5; menu[0]->AddIntItem( "Refraction", &val);
	val = 6; menu[0]->AddIntItem( "reflection + refraction", &val);
	menu[0]->AddIntSetIndex( 6);

	menu[0]->AddText("Setting equation for reflection / refraction");
	menu[0]->AddInt( "Fresnel equation", -1, &set.fresnel_vzorec);
	val = 0; menu[0]->AddIntItem( "Fresnel ^5", &val);
	val = 1; menu[0]->AddIntItem( "Fresnel ^3", &val);
	val = 2; menu[0]->AddIntItem( "Fresnel ^1", &val);
	val = 3; menu[0]->AddIntItem( "Set ratio", &val);		w.p_fresnel = &set.w.fresnel;
//	val = 4; menu[0]->AddIntItem( "Fresnel2", &val);
		
	menu[0]->AddIntSetIndex( 0);
	menu[0]->AddFloatComp("Ratio reflection to refraction", -1, &set.w.fresnel, 0.f, 1.f, 0.5f);

	if(ext.ARB_texture_cube_map)menu[0]->AddBool( "Use cubemapping", -1, &set.cubemapping, 1);
	else set.cubemapping=0;
	menu[0]->AddInt( "Quality refraction ray", -1, &set.quality_of_refraction);
	val = 0; menu[0]->AddIntItem( "Low quality to computation refraction ray", &val);
	val = 1; menu[0]->AddIntItem( "Medium quality to computation refraction ray", &val);
	val = 2; menu[0]->AddIntItem( "High quality to computation refraction ray", &val);
	w.p_quality_of_refraction = &set.quality_of_refraction;
	menu[0]->AddIntSetIndex( 1);

	menu[0]->AddBool( "Draw normal vectors", -1, &set.r.normals, 0);
	menu[0]->AddBool( "Draw sky box", -1, &set.draw_sky_box, 1);
	menu[0]->AddText("");
	menu[0]->AddText( "    Rain setting");
	menu[0]->AddFloatComp("Wavelength", -1, &set.w.length, 0.01f, 10, 1);
	menu[0]->AddFloatComp("Amplitude", -1, &set.w.amplitude, 0, 5, 0.85f);
	menu[0]->AddFloatComp("Raindrops per second", -1, &set.w.drops_per_sec, 0, 10, 1.0f);
	menu[0]->AddKeyChange( "One drop", 'B', &set.w.one_drop);
	menu[0]->AddText("");
	menu[0]->AddText( "    Water setting");
	menu[0]->AddFloatComp("Change of velocity", -1, &set.w.change_velocity, 0, 1, 0.3f);
	menu[0]->AddFloatComp("Falling off in percent", -1, &set.w.utlm, 0.f, 10.f, 1.f);
	menu[0]->AddFloatComp("Relative refraction index", -1, &set.w.nr, 0.f, 3.f, 1.33f);
	menu[0]->AddKey("Reset height surface", 'N', &set.w.reset_water);
	menu[0]->AddText("Divide sets to use left and right arrow");
	menu[0]->AddInt( "Divide", -1, &set.w.division);
	val = 10;  menu[0]->AddIntItem( "Divide grid fow water:   10", &val);
	val = 20;  menu[0]->AddIntItem( "Divide grid fow water:   20", &val);
	val = 30;  menu[0]->AddIntItem( "Divide grid fow water:   30", &val);
	val = 40;  menu[0]->AddIntItem( "Divide grid fow water:   40", &val);
	val = 50;  menu[0]->AddIntItem( "Divide grid fow water:   50", &val);
	val = 60;  menu[0]->AddIntItem( "Divide grid fow water:   60", &val);
	val = 70;  menu[0]->AddIntItem( "Divide grid fow water:   70", &val);
	val = 80;  menu[0]->AddIntItem( "Divide grid fow water:   80", &val);
	val = 90;  menu[0]->AddIntItem( "Divide grid fow water:   90", &val);
	val = 100; menu[0]->AddIntItem( "Divide grid fow water:  100", &val);
	val = 120; menu[0]->AddIntItem( "Divide grid fow water:  120", &val);
	val = 140; menu[0]->AddIntItem( "Divide grid fow water:  140", &val);
	val = 160; menu[0]->AddIntItem( "Divide grid fow water:  160", &val);
	val = 180; menu[0]->AddIntItem( "Divide grid fow water:  180", &val);
	val = 200; menu[0]->AddIntItem( "Divide grid fow water:  200", &val);
	val = 250; menu[0]->AddIntItem( "Divide grid fow water:  250", &val);
	val = 300; menu[0]->AddIntItem( "Divide grid fow water:  300", &val);
	val = 400; menu[0]->AddIntItem( "Divide grid fow water:  400", &val);
	val = 500; menu[0]->AddIntItem( "Divide grid fow water:  500", &val);
	menu[0]->AddIntSetIndex( 9);
	set.w.division=100;
	set.language_switch=0;
	menu[0]->AddKeyChange( "Slovensky jazyk", -1, &set.language_switch);
//#ifdef _DEBUG
//	menu[0]->AddFloatComp( "Delay pre znizenie FPS", -1, &set.time_delay, 0.f, 0.3f, 0.f);
//#endif


	menu[1] = new Menu( font0);
	menu[1]->AddText( "Zobrazenie/skrytie menu klavesa F1");
	menu[1]->AddKey ( "Help", 'H', &set.help);
	menu[1]->AddText("");
	menu[1]->AddInt( "Kreslenie", 'K', &set.r.type);
	val = 0; menu[1]->AddIntItem( "Body", &val);
	val = 1; menu[1]->AddIntItem( "Ciary", &val);
	val = 2; menu[1]->AddIntItem( "Trojuholniky", &val);
	val = 3; menu[1]->AddIntItem( "Fresnel, svetle je odraz, tmave je lom", &val);
	val = 4; menu[1]->AddIntItem( "Reflection - odraz", &val);
	val = 5; menu[1]->AddIntItem( "Refraction - lom", &val);
	val = 6; menu[1]->AddIntItem( "reflection + refraction", &val);
	menu[1]->AddIntSetIndex( 6);

	menu[1]->AddText("Nastavenie vzorca pre pomer odrazu k lomu");
	menu[1]->AddInt( "Fresnel vzorec", -1, &set.fresnel_vzorec);
	val = 0; menu[1]->AddIntItem( "Fresnel ^5", &val);
	val = 1; menu[1]->AddIntItem( "Fresnel ^3", &val);
	val = 2; menu[1]->AddIntItem( "Fresnel ^1", &val);
	val = 3; menu[1]->AddIntItem( "Nastavitelny pomer", &val);		w.p_fresnel = &set.w.fresnel;
//	val = 4; menu[1]->AddIntItem( "Fresnel2", &val);
		
	menu[1]->AddIntSetIndex( 0);
	menu[1]->AddFloatComp("Pomer medzi lomom a odrazom", -1, &set.w.fresnel, 0.f, 1.f, 0.5f);

	if(ext.ARB_texture_cube_map)menu[1]->AddBool( "Pouzivaj cubemapping", -1, &set.cubemapping, 1);
	else set.cubemapping=0;
	menu[1]->AddInt( "Kvalita lomeneho luca", -1, &set.quality_of_refraction);
	val = 0; menu[1]->AddIntItem( "Nizka kvalita vypoctu lomeneho luca", &val);
	val = 1; menu[1]->AddIntItem( "Stredna kvalita vypoctu lomeneho luca", &val);
	val = 2; menu[1]->AddIntItem( "Vysoka kvalita vypoctu lomeneho luca", &val);
	w.p_quality_of_refraction = &set.quality_of_refraction;
	menu[1]->AddIntSetIndex( 1);

	menu[1]->AddBool( "Kresli normaly", -1, &set.r.normals, 0);
	menu[1]->AddBool( "Kresli sky box", -1, &set.draw_sky_box, 1);
	menu[1]->AddText("");
	menu[1]->AddText( "    Nastavenie dazda");
	menu[1]->AddFloatComp("Vlnova dlzka", -1, &set.w.length, 0.01f, 10, 1);
	menu[1]->AddFloatComp("Amplituda", -1, &set.w.amplitude, 0, 5, 0.85f);
	menu[1]->AddFloatComp("Kvapiek za sekundu", -1, &set.w.drops_per_sec, 0, 10, 1.0f);
	menu[1]->AddKeyChange( "Pad jednej kvapky", 'B', &set.w.one_drop);
	menu[1]->AddText("");
	menu[1]->AddText( "    Nastavenie vody");
	menu[1]->AddFloatComp("Zmena rychlosti", -1, &set.w.change_velocity, 0, 1, 0.3f);
	menu[1]->AddFloatComp("Utlm v percentach", -1, &set.w.utlm, 0.f, 10.f, 1.f);
	menu[1]->AddFloatComp("Relativny index lomu", -1, &set.w.nr, 0.f, 3.f, 1.33f);
	menu[1]->AddKey("Reset vysky hladiny", 'N', &set.w.reset_water);
	menu[1]->AddText("Delenie nastavuj sipkami vlavo a vpravo");
	menu[1]->AddInt( "Delenie", -1, &set.w.division);
	val = 10;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   10", &val);
	val = 20;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   20", &val);
	val = 30;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   30", &val);
	val = 40;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   40", &val);
	val = 50;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   50", &val);
	val = 60;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   60", &val);
	val = 70;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   70", &val);
	val = 80;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   80", &val);
	val = 90;  menu[1]->AddIntItem( "Delenie mriezky pre vodu:   90", &val);
	val = 100; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  100", &val);
	val = 120; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  120", &val);
	val = 140; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  140", &val);
	val = 160; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  160", &val);
	val = 180; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  180", &val);
	val = 200; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  200", &val);
	val = 250; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  250", &val);
	val = 300; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  300", &val);
	val = 400; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  400", &val);
	val = 500; menu[1]->AddIntItem( "Delenie mriezky pre vodu:  500", &val);
	menu[1]->AddIntSetIndex( 9);
	set.w.division=100;
	menu[1]->AddKeyChange( "English language", -1, &set.language_switch);
//#ifdef _DEBUG
//	menu[1]->AddFloatComp( "Delay pre znizenie FPS", -1, &set.time_delay, 0.f, 0.3f, 0.f);
//#endif

	if(error)return;
	InitGL();		// nastavenie z-buffra,svetla,...
	c->vpd = 11.6f;
	c->vp.set(0, 7.3f, 9.1f);
	c->vu.set(0, 0.78f, -0.63f);
	c->vd.set(0, -0.63f,-0.78f);
	w.Wave_time0(vec(0,1,0), 1.f);
	w.drop_time=-4.f;
	if(smenu)c->set_3ds_mouse_style();
}

scene::~scene()
{
	//////////////////////////////////////
	// mazeme instancie tried
	if(font0!=NULL)delete font0;
	if(c!=NULL)delete c;
//	if(base!=NULL)delete base;
	if(menu[0]!=NULL)delete menu[0];
	if(menu[1]!=NULL)delete menu[1];
//	if(model!=NULL)delete model;
	//////////////////////////////////////
	KillGL();
}

void scene::InitGL(void)
{
//	if(smenu)ShowCursor(1);
	if(!kurzor)ShowCursor(0);
	if(!fullscreen)
	{
		RECT		WindowRect;					// miesto pre velkost okna
		WindowRect.left=(long)0;
		WindowRect.top=(long)0;
		GetClientRect(hWnd,&WindowRect);				//zistenie rozmerov uzivatelskej casti okna
		ReSizeGLScene(WindowRect.right,WindowRect.bottom);	//nastavenie perspektivy
	}
	else ReSizeGLScene(screen_x, screen_y);	//definovana v tomto subore

	glEnable(GL_TEXTURE_2D);		// Povolenie zobrazovanie textúr
	glShadeModel(GL_SMOOTH);		/*nastavuje tienovanie
	GL_SMOOTH - objekty su tienovane t.j. prechody medzi farbami bodov su plinule (gouradovo (s)tienovanie)
	GL_FLAT   - objekty nie su tienovane ziadne plinule prechody */

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); //specifikuje hodnoty na cistenie pomocou glClear par.: red, green, blue, alpha (cierny podklad)
//	glClearColor(0.6f, 0.6f, 0.6f, 0.0f); //specifikuje hodnoty na cistenie pomocou glClear par.: red, green, blue, alpha (cierny podklad)
	glClearDepth(1.0f);  //specifikuje hodnoty na cistenie pomocou glClear par.: depth (hlbka)
	
	/* ZAPNUTIE HLBKOVEHO TESTU */
	glEnable(GL_DEPTH_TEST);		// zapne håbkový test
	glDepthFunc(GL_LESS);			// typ håbkového testu
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);	// “reálnejšie” perspektívne výpoèty

	GLfloat LightAmbient[]	=	{ 0.3f, 0.3f, 0.3f, 1.0f };		//svetlo ktore osvetluje zo vsetkych stran rovnako (okolite svetlo)
	GLfloat LightDiffuse[]	=	{ 0.7f, 0.7f, 0.7f, 1.0f };		//smerove, alebo bodove svetlo - svetlo svieti z pozicie zadanej pomocou premennej LightPosition
	GLfloat LightPosition[4]=	{ 0.0f, 0.0f, 1.0f, 0.0f };		//smer difuzneho svetla - ak je 4 zlozka 0 - smerove svetlo, ak nie je 0 potom s jedna o bodove svetlo so suradnicami x,y,z
	
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	glLightfv ( GL_LIGHT0, GL_POSITION, LightPosition  );		// pozicia difuzneho svetla
	glEnable  ( GL_LIGHT0 );									// aktivovanie svetla 0
	
	GLfloat mat_diffuse[]	= { 1.0f, 1.0f, 1.0f, 1.0f };
//	GLfloat mat_specular[]	= { 0.9f, 0.9f, 0.9f, 1.0f };
//	GLfloat mat_shininess[] = { 128.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE , mat_diffuse);
//	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
//	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	if(ext.EXT_separate_specular_color)
		glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);

//	GLfloat	fogColor[4] = {0.6f,0.6f,0.6f,1.0f};		// Fog Color
//	glFogfv(GL_FOG_COLOR, fogColor);					// farba hmly
//	glFogi(GL_FOG_MODE, GL_EXP);						// mod hmly
//	glFogf(GL_FOG_DENSITY, 0.1f);						// hustota hmly
//	glFogi(GL_FOG_MODE,GL_LINEAR);
//	glHint(GL_FOG_HINT, GL_DONT_CARE);					// dalsie nastavenie
//	glFogf(GL_FOG_START, 0.5*clip_distance);			// startovacia vzdialenost
//	glFogf(GL_FOG_END, clip_distance);					// ukoncovacia vzdialenost
//	glEnable(GL_FOG);

	glCullFace( GL_BACK );
//	glEnable(GL_CULL_FACE );
	glColor3f(1.0f,1.0f,1.0f);				// biela farba	
}

void scene::KillGL()
{
//	if(smenu)ShowCursor(0);
	if(!kurzor)ShowCursor(1);
}


void scene::Kurzor(void)			// zisti poziciu mysi a zobrazy kurzor
{
/*	GetCursorPos(&MouseBod);		//nacitanie pozicie mysi
	glBegin(GL_LINES);
		glVertex2i(MouseBod.x+velkost_kurzoru+1,screen_y-1-MouseBod.y);
		glVertex2i(MouseBod.x-velkost_kurzoru+1,screen_y-1-MouseBod.y);
		glVertex2i(MouseBod.x+1,screen_y-1-MouseBod.y+velkost_kurzoru);
		glVertex2i(MouseBod.x+1,screen_y-1-MouseBod.y-velkost_kurzoru);
	glEnd();
*/
	glBegin(GL_LINES);
		glVertex2i(mouse_x+velkost_kurzoru+1,mouse_y);
		glVertex2i(mouse_x-velkost_kurzoru+1,mouse_y);
		glVertex2i(mouse_x+1,mouse_y+velkost_kurzoru);
		glVertex2i(mouse_x+1,mouse_y-velkost_kurzoru);
	glEnd();
}

void scene::ReSizeGLScene(int width, int height)
{
	sirka = width;
	vyska = height;
	ReSizeGLScene_3D(width, height);
}

void scene::ReSizeGLScene_3D(int width, int height)		// funkcia pre zmenu velkosti okna
{										// vola sa v pripade zmeny velkosti okna, zmenu z(na) fullscreen
	if (height==0) height=1;
	glViewport(0,0,width,height);		// Viewport (pohlad)
	glMatrixMode(GL_PROJECTION);		// projekcia
	glLoadIdentity();					// Reset pohladu - nacita jednotkovu maticu
//	gluPerspective(uhol_kamery,(GLfloat)width/(GLfloat)height,blizka_orezavacia_rovina,vzdialena_orezavacia_rovina);
	gluPerspective(uhol_kamery,(GLfloat)width/(GLfloat)height,blizka_orezavacia_rovina,clip_distance);
	/* gluPerspective - nastavuje perspektivu kamery, ako bude vypadat priestor z pohladu kamery.
	1. parameter urcuje zorný uhol kamery. 2.param. - gldAspect je pomer výšky a šírky zobrazovanej plochy.
	3. par. udava vzdialenost od ktorej je vidiet vykreslovane objekty
	4. par. udava vzdialenost do ktorej je vidiet vykreslovane objekty */
	glMatrixMode(GL_MODELVIEW);			// Modelview
	glLoadIdentity();					// Reset The Modelview Matrix
}

void scene::ReSizeGLScene_2D(int width, int height)		// funkcia pre zmenu velkosti okna
{
	if (height==0) height=1;
	glViewport(0,0,width,height);		// Viewport (pohlad)
	glMatrixMode(GL_PROJECTION);		// projekcia
	glLoadIdentity();					// Reset pohladu - nacita jednotkovu maticu
	glOrtho(0,screen_x,0,screen_y,-1,1);	// nastavuje standardne pravouhle premietanie
	glMatrixMode(GL_MODELVIEW);			// Modelview
	glLoadIdentity();					// Reset The Modelview Matrix
}

void scene::Prepni_do_3D(void)			// rychle prepnutie do 3D
{
	glEnable(GL_DEPTH_TEST );

	glMatrixMode(GL_PROJECTION);		// projekcia
	glLoadIdentity();					// Reset pohladu - nacita jednotkovu maticu
	//gluPerspective(uhol_kamery,(float)sirka/(float)vyska,blizka_orezavacia_rovina,vzdialena_orezavacia_rovina);
	gluPerspective(uhol_kamery,(GLfloat)sirka/(GLfloat)vyska,blizka_orezavacia_rovina,clip_distance);
	/* gluPerspective - nastavuje perspektivu kamery, ako bude vypadat priestor z pohladu kamery.
	1. parameter urcuje zorný uhol kamery. 2.param. - gldAspect je pomer výšky a šírky zobrazovanej plochy.
	3. par. udava vzdialenost od ktorej je vidiet vykreslovane objekty
	4. par. udava vzdialenost do ktorej je vidiet vykreslovane objekty */
	glMatrixMode(GL_MODELVIEW);			// Modelview
	glLoadIdentity();
}

void scene::Prepni_do_2D(void)			// rychle prepnutie do 2D
{
//	if(svetlo)glDisable(GL_LIGHTING);					// vypne svetlo
//	if(ciary)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);	// vypne ciarovy rezim
//	if(!texturovanie)glEnable(GL_TEXTURE_2D);			// zapne texturovanie
	glDisable(GL_DEPTH_TEST );

	glMatrixMode(GL_PROJECTION);		// projekcia
	glLoadIdentity();					// Reset pohladu - nacita jednotkovu maticu
	glOrtho(0,screen_x,0,screen_y,-1,1);// nastavuje standardne pravouhle premietanie
	glMatrixMode(GL_MODELVIEW);			// Modelview
	glLoadIdentity();					// Reset The Modelview Matrix
}

void scene::SaveScreen_TGA()
{
	FILE *sub;
	unsigned char* buf = new unsigned char[3*screen_x*screen_y];
	char file[30];
	unsigned char ctmp = 0, type, mode, rb;
	short int width = screen_x, height = screen_y, itmp = 0;
	unsigned char pixelDepth = 24;

	if(buf==NULL )return;

	glPixelStorei( GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, screen_x, screen_y, GL_RGB, GL_UNSIGNED_BYTE, buf);

	for(int c=0;c<99;c++)
	{
		char pom[20];
		strcpy(file,"screen");
		if(c<10)strcat(file,"0");
		strcat(file,itoa(c,pom,10));
		strcat(file,".tga");
		sub = fopen(file,"r");
		if(sub==NULL)c=100;
		if(sub!=NULL)fclose(sub);
	}
	
	sub = fopen(file,"wb");
	
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

	// tu sa musi prehodit R->B - pozri specifikaciu TGA formatu.
	if (mode >= 3) {
		for (int i = 0; i < width * height * mode; i += mode)
		{
			rb = buf[i];
			buf[i] = buf[i+2];
			buf[i+2] = rb;
		}
	}

	// tu sa zapisuje hlavicka
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);

	fwrite(&type, sizeof(unsigned char), 1, sub);

	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);
	fwrite(&itmp, sizeof(short int), 1, sub);

	fwrite(&width, sizeof(short int), 1, sub);
	fwrite(&height, sizeof(short int), 1, sub);
	fwrite(&pixelDepth, sizeof(unsigned char), 1, sub);

	fwrite(&ctmp, sizeof(unsigned char), 1, sub);
	
	fwrite(buf, 3*screen_x*screen_y, 1, sub);

	delete[] buf;
	fclose(sub);
}

void scene::Paint_axes(float size, float r, float g, float b)
{
	GLfloat LightAmbient[4];		//svetlo ktore osvetluje zo vsetkych stran rovnako (okolite svetlo)
	GLfloat LightDiffuse[4];		//smerove, alebo bodove svetlo - svetlo svieti z pozicie zadanej pomocou premennej LightPosition

	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT );
	glEnable  ( GL_LIGHT0 );
	glDisable(GL_TEXTURE_2D);
	glEnable( GL_LIGHTING);
	GLUquadricObj *quad;
	quad = gluNewQuadric();

	LightAmbient[0]=0.2f*r; LightAmbient[1]=0.2f*g; LightAmbient[2]=0.2f*b; LightAmbient[3]=0;
	LightDiffuse[0]=0.8f*r; LightDiffuse[1]=0.8f*g; LightDiffuse[2]=0.8f*b; LightDiffuse[3]=0;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	gluSphere( quad, size*0.03f, 10, 10);
	LightAmbient[0]=0.2f; LightAmbient[1]=0.0f; LightAmbient[2]=0.0f;
	LightDiffuse[0]=0.8f; LightDiffuse[1]=0.0f; LightDiffuse[2]=0.0f;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	
	glPushMatrix();
		glTranslatef(size,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluCylinder(quad, 0, size*0.05f, size*0.2f, 20, 1);
		glRotatef(90.f,0,1,0);
		glTranslatef( -size*0.2f,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluDisk(quad, 0, size*0.05f, 20, 1);
	glPopMatrix();
	LightAmbient[0]=0;
	LightAmbient[1]=0.2f;
	LightDiffuse[0]=0;
	LightDiffuse[1]=0.8f;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	
	glPushMatrix();
		glRotatef(90,0,0,1);
		glTranslatef(size,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluCylinder(quad, 0, size*0.05f, size*0.2f, 20, 1);
		glRotatef(90.f,0,1,0);
		glTranslatef( -size*0.2f,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluDisk(quad, 0, size*0.05f, 20, 1);
	glPopMatrix();
	
	LightAmbient[1]=0;
	LightAmbient[2]=0.2f;
	LightDiffuse[1]=0;
	LightDiffuse[2]=0.8f;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	glPushMatrix();
		glRotatef(90,0,-1,0);
		glTranslatef(size,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluCylinder(quad, 0, size*0.05f, size*0.2f, 20, 1);
		glRotatef(90.f,0,1,0);
		glTranslatef( -size*0.2f,0.0f,0.f);
		glRotatef(-90.f,0,1,0);
		gluDisk(quad, 0, size*0.05f, 20, 1);
	glPopMatrix();

	gluDeleteQuadric(quad);
	glDisable( GL_LIGHTING);

	glLineWidth( size*0.2f+1.0f );
	glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex3f(-0.1f*size,0,0);
		glVertex3f(0.8f*size,0,0);

		glColor3f(0,1,0);
		glVertex3f(0,-0.1f*size,0);
		glVertex3f(0,0.8f*size,0);

		glColor3f(0,0,1);
		glVertex3f(0,0,-0.1f*size);
		glVertex3f(0,0,0.8f*size);
	glEnd();
	glLineWidth( 1.0f );
	glColor3f(1,1,1);

	LightAmbient[0]=0.2f;	LightAmbient[1]=0.2f;	LightAmbient[2]=0.2f;
	LightDiffuse[0]=0.8f;	LightDiffuse[1]=0.8f;	LightDiffuse[2]=0.8f;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla

	glPopAttrib();
}

void scene::Paint_vector(const vec &v, float size, float r, float g, float b)
{
	GLfloat LightAmbient[4];		//svetlo ktore osvetluje zo vsetkych stran rovnako (okolite svetlo)
	GLfloat LightDiffuse[4];		//smerove, alebo bodove svetlo - svetlo svieti z pozicie zadanej pomocou premennej LightPosition

	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT );
	glEnable  ( GL_LIGHT0 );
	glDisable(GL_TEXTURE_2D);
	glEnable( GL_LIGHTING);
	GLUquadricObj *quad;
	quad = gluNewQuadric();

	LightAmbient[0]=0.2f*r; LightAmbient[1]=0.2f*g; LightAmbient[2]=0.2f*b; LightAmbient[3]=0;
	LightDiffuse[0]=0.8f*r; LightDiffuse[1]=0.8f*g; LightDiffuse[2]=0.8f*b; LightDiffuse[3]=0;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	gluSphere( quad, size*0.03f, 10, 10);
	
	glPushMatrix();
		mat4 m;
		vec vr,vu,ve;
		
		ve = v;	ve.Normalize();		// ve vektor k ociam, mapuje sa ma z+
		vr.set( -ve.y, ve.x, 0 );	// vektor vpravo, urobime ho ako kolmy k ve
		if(vr.Length()==0)vr.set(1,0,0);
		vr.Normalize();
		vu = CROSS( ve, vr);		// vu vektor hore, pouzijeme vektorovy sucin
		vu.Normalize();
		m.MakeMatrix( vr, vu, ve );	// vytvorime maticu, ktora transformuje vr na x, vu na y a ve na z
		// my potrebujeme transformovat z suradnicu na ve
		// kuzel nam ukazuje na z+, my potrebujeme aby ukazoval smerom vektora v, t.j. na smer ve
		// potrebujeme transformovat os z na vektor ve, to dosiahneme pouzitim inverznej (=transponovanej) matice
		m = ~m;
		vr = v-ve*size*0.2f;		// vr teraz predstavuje posun zaciatku kuzela, kuzel nam musi koncit v bode v
		m.m14 = vr.x;	m.m24 = vr.y;	m.m34 = vr.z;
		m.glMultMatrix();
		// kuzel sa kresli tak, ze v z=0 ma polomer size*0.05f a v z=size*0.2f ma polomer 0
		gluCylinder(quad, size*0.05f, 0, size*0.2f, 20, 1);
		glRotatef(180.f,0,1,0);
		gluDisk(quad, 0, size*0.05f, 20, 1);
	glPopMatrix();

	glDisable( GL_LIGHTING);
	glLineWidth( size*0.2f+1.0f );
	glBegin(GL_LINES);
		glColor3f(r,g,b);
		glVertex3f(0,0,0);
		glVertex3fv(v.v);

	glEnd();
	glLineWidth( 1.0f );
	glColor3f(1,1,1);

	LightAmbient[0]=0.2f;	LightAmbient[1]=0.2f;	LightAmbient[2]=0.2f;
	LightDiffuse[0]=0.8f;	LightDiffuse[1]=0.8f;	LightDiffuse[2]=0.8f;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla

	glPopAttrib();
}

void scene::Paint_point( const vec &v, float size, float r, float g, float b)
{
	GLfloat LightAmbient[4];		//svetlo ktore osvetluje zo vsetkych stran rovnako (okolite svetlo)
	GLfloat LightDiffuse[4];		//smerove, alebo bodove svetlo - svetlo svieti z pozicie zadanej pomocou premennej LightPosition
	GLfloat LightPosition[4]=	{ 0, 0, 1, 0 };		//smer difuzneho svetla - ak je 4 zlozka 0 - smerove svetlo, ak nie je 0 potom s jedna o bodove svetlo so suradnicami x,y,z

	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT );
	glEnable  ( GL_LIGHT0 );
	glDisable(GL_TEXTURE_2D);
	glEnable( GL_LIGHTING);
	GLUquadricObj *quad;
	quad = gluNewQuadric();

	LightAmbient[0]=0.2f*r; LightAmbient[1]=0.2f*g; LightAmbient[2]=0.2f*b; LightAmbient[3]=0;
	LightDiffuse[0]=0.8f*r; LightDiffuse[1]=0.8f*g; LightDiffuse[2]=0.8f*b; LightDiffuse[3]=0;
	glLightModelfv ( GL_LIGHT_MODEL_AMBIENT, LightAmbient );	// nastavenie okoliteho (Ambient) svetla
	glLightfv ( GL_LIGHT0, GL_DIFFUSE, LightDiffuse );			// nastavenie difuzneho svetla
	glPushMatrix();
	glLoadIdentity();
	glLightfv ( GL_LIGHT0, GL_POSITION, LightPosition  );		// pozicia difuzneho svetla
	glPopMatrix();
	
	glPushMatrix();
		glTranslatef( v.x, v.y, v.z);
		
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// z+
		glTranslatef(0,0,-size*0.05f);
		glRotatef(90,0,1,0);
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// x+
		glTranslatef(0,0,-size*0.05f);
		glRotatef(90,0,1,0);
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// z-
		glTranslatef(0,0,-size*0.05f);
		glRotatef(90,0,1,0);
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// x-
		glTranslatef(0,0,-size*0.05f);
		glRotatef(-270,0,1,0);
		glRotatef(-90,1,0,0);
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// y+
		glTranslatef(0,0,-size*0.05f);
		glRotatef(180,1,0,0);
		glTranslatef(0,0,size*0.05f);
		gluCylinder(quad, size*0.1f, 0, size*0.45f, 20, 1);		// y-
	glPopMatrix();

	glPopAttrib();
}

void scene::Print_mat4(int stlpec, int riadok, const mat4 &m, char* nadpis)
{
	glEnable(GL_TEXTURE_2D);
	font0->Begin();
	if(nadpis!=NULL)
	{
		font0->Print(stlpec, riadok++, nadpis,0);
	}
	for(int i=0;i<16;i++)
	{
		gcvt( (double)m.m[i], 2, text);			//prevod realneho cisla na text
		//	a00  a04  a08  a12 
		//  a01  a05  a09  a13
		//  a02  a06  a10  a14
		//  a03  a07  a11  a15
		font0->Print(stlpec+(i/4)*10,riadok+(i%4),text,0);
	}
	font0->End();
}



void scene::Print_vec(int stlpec, int riadok, const vec &a, char* meno)
{
	glEnable(GL_TEXTURE_2D);
	font0->Begin();
	if(meno!=NULL)
	{
		font0->Print(stlpec, riadok, meno,0);
		stlpec += strlen(meno)+1;
	}
	
	gcvt( (double)a.x, 2, text);
	font0->Print(stlpec,riadok,text,0);
	stlpec+=10;
	gcvt( (double)a.y, 2, text);
	font0->Print(stlpec,riadok,text,0);
	stlpec+=10;
	gcvt( (double)a.z, 2, text);
	font0->Print(stlpec,riadok,text,0);

	font0->End();
}

void scene::Print_float(int stlpec, int riadok, float a, char *meno)
{
	glEnable(GL_TEXTURE_2D);
	font0->Begin();
	if(meno!=NULL)
	{
		font0->Print(stlpec, riadok, meno,0);
		stlpec += strlen(meno)+1;
	}
	gcvt( (double)a, 2, text);
	font0->Print(stlpec,riadok,text,0);
	font0->End();
}

void scene::Print_int(int stlpec, int riadok, int a, char *meno)
{
	glEnable(GL_TEXTURE_2D);
	font0->Begin();
	if(meno!=NULL)
	{
		font0->Print(stlpec, riadok, meno,0);
		stlpec += strlen(meno)+1;
	}
	itoa( a, text, 10);
	font0->Print(stlpec,riadok,text,0);
	font0->End();
}

void scene::SaveScreen_BMP(int x0, int y0, int scr_size_x, int scr_size_y)
{
	BITMAPFILEHEADER fileheader;
	BITMAPINFOHEADER infoheader;

	FILE *sub;
	int line_size = 3*scr_size_x;
	if(line_size%4)line_size += 4 - line_size%4;

	unsigned char* buf = new unsigned char[3*line_size*scr_size_y];
	char file[30];

	if(buf==NULL )return;

	glPixelStorei( GL_PACK_ALIGNMENT, 4);
	if(ext.EXT_bgra)
		glReadPixels(x0, y0, scr_size_x, scr_size_y, GL_BGR_EXT, GL_UNSIGNED_BYTE, buf);
	else
		glReadPixels(x0, y0, scr_size_x, scr_size_y, GL_RGB, GL_UNSIGNED_BYTE, buf);

	for(int c=0;c<999;c++)
	{
		char pom[20];
		strcpy(file,"screen");
		if(c<100)strcat(file,"0");
		if(c<10)strcat(file,"0");
		strcat(file,itoa(c,pom,10));
		strcat(file,".bmp");
		sub = fopen(file,"r");
		if(sub==NULL)break;
		if(sub!=NULL)fclose(sub);
	}
	

	fileheader.bfType = 0x4D42; // Magic identifier   - "BM"	| identifikacia BMP suboru musi byt "BM"
	fileheader.bfSize = 3*scr_size_x*scr_size_y+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// File size in bytes			| velkos suboru v byte
	fileheader.bfReserved1 = 0;
	fileheader.bfReserved2 = 0;
	fileheader.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// Offset to image data, bytes	| posun na zaciatok dat
	
	infoheader.biSize = sizeof(BITMAPINFOHEADER);	// Header size in bytes			| velkost hlavicky BITMAPINFOHEADER
	infoheader.biWidth = scr_size_x;	// Width of image			| sirka obrazka - sizeX
	infoheader.biHeight = scr_size_y;	// Height of image			| vyska obrazka - sizeY
	infoheader.biPlanes = 1;		// Number of colour planes	| pocet farebnych rovin musi bit 1
	infoheader.biBitCount = 24;		// Bits per pixel			| bitov na pixel moze bit 1,4,8,24
	infoheader.biCompression = 0;	// Compression type			| typ compresie , 0 - bez kompresie
	infoheader.biSizeImage = line_size*infoheader.biHeight ;	// Image size in bytes		| velkost obrazka v byte
	infoheader.biXPelsPerMeter = 0;	// Pixels per meter X		| pixelov na meter v smere x
	infoheader.biYPelsPerMeter = 0;	// Pixels per meter Y		| pixelov na meter v smere y
	infoheader.biClrUsed = 0;		// Number of colours		| pocet  farieb v palete, ak 0 vsetky su pouzivane
	infoheader.biClrImportant = 0;	// Important colours		| dolezite farby v palete, ak 0 vsetky su dolezite
	
	sub = fopen(file,"wb");
	fwrite( &fileheader, sizeof(BITMAPFILEHEADER), 1, sub);
	fwrite( &infoheader, sizeof(BITMAPINFOHEADER), 1, sub);
	
	if(!ext.EXT_bgra)
	{
		for(int y=0; y<scr_size_y; y++)
		{
			for(int x=0; x<scr_size_x; x++)
			{
				unsigned char temp = buf[y*line_size+x*3+0];
				buf[y*line_size+x*3+0] = buf[y*line_size+x*3+2];
				buf[y*line_size+x*3+2] = temp;
			}
		}
	}
	fwrite( buf, line_size*scr_size_y, 1, sub);

	delete[] buf;
	fclose(sub);
}


void scene::DrawSkyBox(texture *tex, float size)
{
	vec p,n,s,t;
	// pozerame na +x
	tex[0].glBindTexture2D();
	n.set(-1,0,0);
	s.set(0,0,2*size);
	t.set(0,2*size,0);
	p = vec(size,0,0) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
	// pozerame na -x
	tex[1].glBindTexture2D();
	n.set(1,0,0);
	s.set(0,0,-2*size);
	p = vec(-size,0,0) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
	// pozerame na +y
	tex[2].glBindTexture2D();
	n.set(0,-1,0);
	s.set(2*size,0,0);
	t.set(0,0,2*size);
	p = vec(0,size,0) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
	// pozerame na -y
	tex[3].glBindTexture2D();
	n.set(0,1,0);
	s.set(2*size,0,0);
	t.set(0,0,-2*size);
	p = vec(0,-size,0) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
	// pozerame na +z
	tex[4].glBindTexture2D();
	n.set(0,0,1);
	s.set(-2*size,0,0);
	t.set(0,2*size,0);
	p = vec(0,0,size) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
	// pozerame na -z
	tex[5].glBindTexture2D();
	n.set(0,0,1);
	s.set(2*size,0,0);
	p = vec(0,0,-size) -0.5*s -0.5*t;
	DrawQuad( p, n, s, t);
}

void scene::DrawQuad(vec p, vec n, vec s, vec t)
{
	glBegin(GL_TRIANGLE_STRIP);				// 3  4  poradie bodov
		glNormal3fv(n.v);					// 1  2
		glTexCoord2f(0,0);glVertex3f( p.v[0], p.v[1], p.v[2]);
		glTexCoord2f(1,0);glVertex3f( p.v[0]+s.v[0],p.v[1]+s.v[1],p.v[2]+s.v[2]);
		glTexCoord2f(0,1);glVertex3f( p.v[0]+t.v[0],p.v[1]+t.v[1],p.v[2]+t.v[2]);
		glTexCoord2f(1,1);glVertex3f( p.v[0]+s.v[0]+t.v[0],p.v[1]+s.v[1]+t.v[1],p.v[2]+s.v[2]+t.v[2]);
	glEnd();
}

int scene::LoadTextureSkyBox(char *directory, char *extension, texture *tex, int repeat, int filter, int compression)
{
	// right, left, top, bottom, back, front
	char* filename[6];
	int OK=1;

	for(int j=0; j<6; j++)
	{
		filename[j] = new char[strlen(directory)+strlen(extension)+7];
		strcpy( filename[j], directory);
	}

	strcat( filename[0], "right.");
	strcat( filename[1], "left.");
	strcat( filename[2], "top.");
	strcat( filename[3], "bottom.");
	strcat( filename[4], "back.");
	strcat( filename[5], "front.");
	for( j=0; j<6; j++) strcat( filename[j], extension);

	strlwr(extension);				// Convert a string to lowercase
	
	for( j=0; j<6; j++)
	{
		if(j==3)
		{
			FILE* f;
			f = fopen(filename[3],"rb");
			if(f==NULL)
			{
				strcpy( filename[3], directory);
				strcat( filename[3], "down.");
				strcat( filename[3], extension);
			}
			else fclose(f);
		}
		if(!tex[j].load(filename[j], repeat, filter, compression))OK=0;
	}
	return OK;
}

int scene::ReadInit(char *filename)
{
	FILE* sub=NULL;
	char temp[100];
	char varname[100],varvalue[100];
	int ret=0;

	sub = fopen( filename, "rb");
	if(sub == NULL)return 0;

	filterForTexture = -1;
//	renderableTexture=0;
//	filterForCubeMap=1;
	while(GetVariable( temp, sub, varname, varvalue, 100))
	{
		float fl=(float)atof(varvalue);
		int in=atoi(varvalue);
	//	strlwr(varname);				// Convert a string to lowercase
//		if(!strcmp("renderableTexture",varname))renderableTexture=in;
//		if(!strcmp("filterForCubeMap",varname))filterForCubeMap=in;
		if(!strcmp("filterForTexture",varname))filterForTexture=in;
		if(!strcmp("showMenu",varname))if(in==0||in==1)smenu = in;
	}
	fclose(sub);
	if(filterForTexture<0 || filterForTexture>3)filterForTexture=2;
//	if(renderableTexture==0)renderableTexture=128;
//	if(filterForCubeMap>3||filterForCubeMap<0)filterForCubeMap=1;
	return 1;
}

int scene::GetVariable(char *temp, FILE *sub, char *varname, char *varvalue, int size_strings)
{
	if(fgets(temp, size_strings, sub) == NULL)
	{
		temp[0] = NULL;
		return 0;
	}
	varname[0]=NULL;
	varvalue[0]=NULL;
	char rovnasa=0;
	for(int i=0,j=0,k=0; temp[i]!=NULL&&temp[i]!='\n'&&temp[i]!='/'; i++)		// prejdeme vsetky znaky
	{
		if(temp[i]==' ' || temp[i]=='\t')continue;
		if(temp[i]=='='){ rovnasa=1;continue;}
		if(rovnasa)
		{
			varvalue[k]=temp[i]; k++;
		}
		else
		{
			varname[j]=temp[i]; j++;
		}
	}
	varname[j]=NULL;
	varvalue[k]=NULL;
	return 1;
}

void scene::BuildDisplayLists()
{
/*	// lietad - lietadlo
	if(lietad)glDeleteLists(lietad,1);			// ak je vybudovany zmas
	lietad=glGenLists(1);							// generuj 1 novy prazny Dispay List
	glNewList(lietad,GL_COMPILE);					// start noveho Display Listu
	lietadlo->Render();
	glEndList();			//koniec display listu
*/
}



void scene::CubeMapBegin()
{
	// potrebujeme rotovat zrkadlovy povrch
	// zrkadlovy povrch sa nerotuje podla modelovacej matice, rotuje sa projekcnou maticou
	// preto si musime vytvorit rotacnu maticu, ktora nam bude rotavat povrch
	// projekcnu maticu vynasobime touto maticou z prava (normalne glMultMatrix)
	// aby, tato rotacna matica nemala vplyv na modelovaciu maticu
	// modelovaciu maticu vynasobime z LAVA, inverznou rotacnou maticou
	mat4 m,m1;
	m.glGetMatrix();
	// anulujeme posunutie
	m.m14 = 0;	m.m24 = 0;	m.m34 = 0;	m.m44 = 1;

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glMultMatrixf( m.m);		// zmena projekcnej matice - tymto sa rotuje zrkadlovy odraz na objekte
	glMatrixMode( GL_MODELVIEW );

	m=!m;	// urobime inverznu maticu m

	m1.glGetMatrix();		// ziskame modelovaciu maticu
	m1 = m*m1;				// modelovaciu maticu vynasobime z LAVA, inverznou rotacnou maticou
	m1.glSetMatrix();		// nastavime novu maticu

	// povolime generovanie texturovych suradnic, suradnice sa generuju podla normalovych vektorov
	glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
	glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
	glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
}

void scene::CubeMapEnd()
{
	// vypneme generovanie tex. suradnic
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();						// vratime projekcnu na povodnu
	glMatrixMode( GL_MODELVIEW );
}


vec scene::CheckCollision(vec vp, vec vpold, float radius)
{
	//	if(vpold_vp.Length()==0)return vpold;
	vec vpold_vp = vp-vpold;		// smer pohybu
	vec vpold_vp_povodny = vpold_vp;// smer pohybu
	int iter=0;
	float radius2 = radius*radius;

	vec newmove = vpold_vp;
	vec newClosestPointOnSphere;
	vec newContactPointSphereToTriangle;

	do
	{
		float distanceCenterPointOnTriangle=1.e+15f;
		int smykanie=0;
		for( int i=0; i<collision.size(); i++)
		{
			vec normal = collision[i].normal;
			
			vec vp_move;
			vec center = vpold;
			float distance = PlaneDistance( normal, collision[i].a);		// D = - (Ax+By+Cz)
			// ---------------------------------------------------------
			// najdeme kolidujuci bod na guli
			vec ClosestPointOnSphere;		// najblizsi bod gule k rovine aktualneho trojuholnika
			// najdeme ho ako priesecnik priamky prechadzajucej stredom gule a smerovym vektorom = normalovemu vektoru roviny (trojuholnika)
			// vypocitame ho, ale jednodusie tak, ze pripocitame (opocitame) k stredu vektor normala*radius
			
			if( PlanePointDelta( normal, distance, center) > 0 )
			{
				// center je na strane normaly, blizsi bod je v opacnom smere ako smer normaly
				ClosestPointOnSphere = -radius*normal+center;
			}
			else
			{
				// center je na opacnej strane ako normala, blizsi bod je v smere normaly
				ClosestPointOnSphere = radius*normal+center;
			}

			// ---------------------------------------------------------
			// najdeme kolidujuci bod na trojuholniku
			// najprv najdeme kolidujuci bod vzhladom na rovinu v ktorej lezi trojuholnik
			vec contactPointSphereToPlane;			// kolidujuci bod na rovine trojuholnika s gulou
			float distanceCenterToPlane;			// vzdialenost stredu gule k rovine
			// zistime ci rovina pretina gulu
			if( SpherePlaneCollision( center, 0.9999f*radius, normal, distance, &distanceCenterToPlane)==1 )
			{
				// gula pretina rovinu
				// kolidujuci bod je bod na rovine najblizsi k stredu gule
				// je vzdialeny od roviny na distanceCenterToPlane, pretoze pocitame bod na rovine pouzijeme -
				contactPointSphereToPlane = center-distanceCenterToPlane*normal;
			}
			else
			{
				// nie sme v kolizii z gulov, ak sa pohybujeme v smere od roviny, nemoze nastat ziadna kolizia
				// ak sa pohybujeme v smere kolmom na normalovy vektor roviny, tak isto kolizia nehrozi
				// kvoli nepresnosti vypoctov umoznime pohyb aj ked velmi malou castou smeruje do roviny
			//	if( DOT3( vpold_vp, center-ClosestPointOnSphere) >= 0)
				if( DOT3( vpold_vp, center-ClosestPointOnSphere) > -0.000001f)
				{
					continue;
				}
				// gula nepretina rovinu
				// kolidujuci bod je priesecnik roviny a priamky vedenej z bodu ClosestPointOnSphere
				// v smere pohybu t.j. z vpold do vp

				float t = LinePlaneIntersectionDirParameter( ClosestPointOnSphere, vpold_vp, normal, distance);
				// t > 1.f, priesecnik z rovinou je dalej ako vpold_vp od bodu ClosestPointOnSphere
				if(t>1.f)
					continue;	// za cely krok vpold_vp sa s tymto trojuholnikom nestretneme
				else if( t<-radius/vpold_vp.Length())		// priesecnik je za gulou, v smere pohybu tuto rovinu nestretneme
					continue;
				else 
					contactPointSphereToPlane = ClosestPointOnSphere+t*vpold_vp;
			}
			// najdeme kolidujuci bod na trojuholniku
			vec contactPointSphereToTriangle;
			// ak sa bod contactPointSphereToPlane nenachadza v trojuholniku 
			// najdeme najblizsi bod trojuholnika k bodu contactPointSphereToTriangle
			if( !PointInsidePolygon( contactPointSphereToPlane, collision[i].a, collision[i].b, collision[i].c) )
			{
				// najdeme najblizsi bod k contactPointSphereToPlane na hranach trojuholnika
				// z tychto vyberieme najblizi k contactPointSphereToPlane
				vec closest_ab = ClosestPointOnLine( collision[i].a, collision[i].b, contactPointSphereToPlane);
				vec closest_bc = ClosestPointOnLine( collision[i].b, collision[i].c, contactPointSphereToPlane);
				vec closest_ca = ClosestPointOnLine( collision[i].c, collision[i].a, contactPointSphereToPlane);
				
				float dist_ab = Distance2( closest_ab, contactPointSphereToPlane);
				float dist_bc = Distance2( closest_bc, contactPointSphereToPlane);
				float dist_ca = Distance2( closest_ca, contactPointSphereToPlane);
				
				if( dist_ab<dist_bc)
				{
					if(dist_ab<dist_ca)
						contactPointSphereToTriangle = closest_ab;
					else
						contactPointSphereToTriangle = closest_ca;
				}
				else
				{
					if(dist_bc<dist_ca)
						contactPointSphereToTriangle = closest_bc;
					else
						contactPointSphereToTriangle = closest_ca;
				}

				// kedze kolidujuci bod na trojuholniku je iny ako kolidujuci bod na rovine
				// zmeni sa aj kolidujuci bod na guli - ClosestPointOnSphere
				// vypocitame ho ako priesecnik gule a priamky z bodu contactPointSphereToTriangle
				// v smere pohybu t.j. z vpold do vp
				double t1,t2;

				if( LineSphereIntersectionDir( contactPointSphereToTriangle, vpold_vp, center, radius, &t1, &t2) )
				{
					if( t1<=0 && t2<0)
					{
						// gula je pred trojuholnikom
						// berieme bod s t1, lebo ten je blizsie k stene (t1>t2)
						if( t1<-1.f)continue;		// tento trojuholnik nas nezaujima lebo nekoliduje po cely tento krok
						ClosestPointOnSphere = t1*vpold_vp+contactPointSphereToTriangle;

						// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
						// kolidujuceho bodu na trojuholniku
						vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
					}
					else if( t1>0 && t2<0)
					{
						// gula je v stene, vratime ju von zo steny
						// berieme bod, ktory je blizsie k rovine
						vec t1point = t1*vpold_vp+contactPointSphereToTriangle;
						vec t2point = t2*vpold_vp+contactPointSphereToTriangle;

					/*	if(PlanePointDistance( normal, distance, t1point)<=PlanePointDistance( normal, distance, t2point) )
							ClosestPointOnSphere = t1point;
						else 
							ClosestPointOnSphere = t2point;
					*/
						if( ABS(t1) < ABS(t2) )
							ClosestPointOnSphere = t1point;
						else
							ClosestPointOnSphere = t2point;
					
						// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
						// kolidujuceho bodu na trojuholniku
						vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
					}
					else // if( t1>0 && t2>0)
					{
						// gula je za trojuholnikom, gula nekoliduje s trojuholnikom v tomto smere pohybu
						continue;
					}
				}
				else
				{
					// nie je priesecnik, gula je mimo trojuholnika
					continue;
				}
			}
			else
			{
				// bod je vnutri trojuholnika
				contactPointSphereToTriangle = contactPointSphereToPlane;
				
				// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
				// kolidujuceho bodu na trojuholniku
				vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
			}

			// zistime vzdialenost kontaktneho bodu na trojuholniku ku stredu gule
			float dist = Distance2(contactPointSphereToTriangle,center);
			if(dist<radius2)		// ak je mensi ako polomer, gula je v kolizii z polygonom
			{
				if(dist<distanceCenterPointOnTriangle)	// ak vzdialenost je mensia ako ineho bodu v kolizii, nahradime ho
				{
					distanceCenterPointOnTriangle=dist;
					newmove = vp_move;
					newClosestPointOnSphere = ClosestPointOnSphere;
					newContactPointSphereToTriangle = contactPointSphereToTriangle;
				}
			}
			else
			{
				if(distanceCenterPointOnTriangle>5.e+14f)	// nenasiel sa ziaden bod vnutri gule
				{
					if( vp_move.Length2() < newmove.Length2() )		// berieme kratsi
					{
						newmove = vp_move;
						newClosestPointOnSphere = ClosestPointOnSphere;
						newContactPointSphereToTriangle = contactPointSphereToTriangle;
					}
				}
			}
			smykanie=1;
		}

		if(smykanie)
		{
			vec normal=vpold-newClosestPointOnSphere;
			float distance = PlaneDistance( normal, newContactPointSphereToTriangle);
			vec delta = LinePlaneIntersectionDir( newClosestPointOnSphere+vpold_vp, normal, normal, distance)-newContactPointSphereToTriangle;
		//	vec	newvp = newClosestPointOnSphere+vpold_vp;
		//	float distancepoint = PlanePointDelta( normal, distance, newvp);
		//	vec intersec = -distancepoint*normal+newvp;
		//	vec delta = intersec-newContactPointSphereToTriangle;

			// taky klzavy pohyb, ktory ide proti povodnemu pohybu zamietneme
			if( DOT3(vpold_vp_povodny, delta) < 0)delta.clear();

			vpold += newmove;				// posunieme sa po najblizi kolidujuci bod
			vpold += 0.000001f*normal;
			vp = vpold + delta;				// cielovy bod posunieme o deltu klzanim
			vpold_vp = vp-vpold;			// novy vektor pohybu
			newmove = vpold_vp;
			iter++;
		}
		else
		{
			vpold += newmove;
			vpold_vp.clear();
			iter=1000;
		}
	}
	while( (vpold_vp.Length2()>1.e-8f)&&(iter<10) );
	return vpold;
}

int scene::CheckCollisionGround(vec center, float radius, float angle, float mindist)
{
	float ground_skew = (float)cos(angle*PI180)*radius;
	vec vpold_vp = mindist*vec(0,-1,0);

	for( int i=0; i<collision.size(); i++)
	{
		vec normal = collision[i].normal;
		float distance = PlaneDistance( normal, collision[i].a);		// D = - (Ax+By+Cz)
		// ---------------------------------------------------------
		// najdeme kolidujuci bod na guli
		vec ClosestPointOnSphere;		// najblizsi bod gule k rovine aktualneho trojuholnika
		// najdeme ho ako priesecnik priamky prechadzajucej stredom gule a smerovym vektorom = normalovemu vektoru roviny (trojuholnika)
		// vypocitame ho, ale jednodusie tak, ze pripocitame (opocitame) k stredu vektor normala*radius
		
		if( PlanePointDelta( normal, distance, center) > 0 )
		{
			// center je na strane normaly, blizsi bod je v opacnom smere ako smer normaly
			ClosestPointOnSphere = -radius*normal+center;
		}
		else
		{
			// center je na opacnej strane ako normala, blizsi bod je v smere normaly
			ClosestPointOnSphere = radius*normal+center;
		}

		// ---------------------------------------------------------
		// najdeme kolidujuci bod na trojuholniku
		// najprv najdeme kolidujuci bod vzhladom na rovinu v ktorej lezi trojuholnik
		vec contactPointSphereToPlane;			// kolidujuci bod na rovine trojuholnika s gulou
		float distanceCenterToPlane;			// vzdialenost stredu gule k rovine
		// zistime ci rovina pretina gulu
		if( SpherePlaneCollision( center, 0.9999f*radius, normal, distance, &distanceCenterToPlane)==1 )
		{
			// gula pretina rovinu
			// kolidujuci bod je bod na rovine najblizsi k stredu gule
			// je vzdialeny od roviny na distanceCenterToPlane, pretoze pocitame bod na rovine pouzijeme -
			contactPointSphereToPlane = center-distanceCenterToPlane*normal;
		}
		else
		{
			// nie sme v kolizii z gulov, ak sa pohybujeme v smere od roviny, nemoze nastat ziadna kolizia
			// ak sa pohybujeme v smere kolmom na normalovy vektor roviny, tak isto kolizia nehrozi
			// kvoli nepresnosti vypoctov umoznime pohyb aj ked velmi malou castou smeruje do roviny
		//	if( DOT3( vpold_vp, center-ClosestPointOnSphere) >= 0)
			if( DOT3( vpold_vp, center-ClosestPointOnSphere) > -0.000001f)
			{
				continue;
			}
			// gula nepretina rovinu
			// kolidujuci bod je priesecnik roviny a priamky vedenej z bodu ClosestPointOnSphere
			// v smere pohybu t.j. z vpold do vp

			float t = LinePlaneIntersectionDirParameter( ClosestPointOnSphere, vpold_vp, normal, distance);
			// t > 1.f, priesecnik z rovinou je dalej ako vpold_vp od bodu ClosestPointOnSphere
			if(t>1.f)
				continue;	// za cely krok vpold_vp sa s tymto trojuholnikom nestretneme
			else if( t<-radius/vpold_vp.Length())		// priesecnik je za gulou, v smere pohybu tuto rovinu nestretneme
				continue;
			else 
				contactPointSphereToPlane = ClosestPointOnSphere+t*vpold_vp;
		}
		// najdeme kolidujuci bod na trojuholniku
		vec contactPointSphereToTriangle;
		// ak sa bod contactPointSphereToPlane nenachadza v trojuholniku 
		// najdeme najblizsi bod trojuholnika k bodu contactPointSphereToTriangle
		if( !PointInsidePolygon( contactPointSphereToPlane, collision[i].a, collision[i].b, collision[i].c) )
		{
			// najdeme najblizsi bod k contactPointSphereToPlane na hranach trojuholnika
			// z tychto vyberieme najblizi k contactPointSphereToPlane
			vec closest_ab = ClosestPointOnLine( collision[i].a, collision[i].b, contactPointSphereToPlane);
			vec closest_bc = ClosestPointOnLine( collision[i].b, collision[i].c, contactPointSphereToPlane);
			vec closest_ca = ClosestPointOnLine( collision[i].c, collision[i].a, contactPointSphereToPlane);
			
			float dist_ab = Distance2( closest_ab, contactPointSphereToPlane);
			float dist_bc = Distance2( closest_bc, contactPointSphereToPlane);
			float dist_ca = Distance2( closest_ca, contactPointSphereToPlane);
			
			if( dist_ab<dist_bc)
			{
				if(dist_ab<dist_ca)
					contactPointSphereToTriangle = closest_ab;
				else
					contactPointSphereToTriangle = closest_ca;
			}
			else
			{
				if(dist_bc<dist_ca)
					contactPointSphereToTriangle = closest_bc;
				else
					contactPointSphereToTriangle = closest_ca;
			}

			// kedze kolidujuci bod na trojuholniku je iny ako kolidujuci bod na rovine
			// zmeni sa aj kolidujuci bod na guli - ClosestPointOnSphere
			// vypocitame ho ako priesecnik gule a priamky z bodu contactPointSphereToTriangle
			// v smere pohybu t.j. z vpold do vp
			double t1,t2;

			if( LineSphereIntersectionDir( contactPointSphereToTriangle, vpold_vp, center, radius, &t1, &t2) )
			{
				if( t1<=0 && t2<0)
				{
					// gula je pred trojuholnikom
					// berieme bod s t1, lebo ten je blizsie k stene (t1>t2)
					if( t1<-1.f)continue;		// tento trojuholnik nas nezaujima lebo nekoliduje po cely tento krok
					ClosestPointOnSphere = t1*vpold_vp+contactPointSphereToTriangle;
					if( (center.y-ClosestPointOnSphere.y)>ground_skew )return 1;
					else continue;
					// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
					// kolidujuceho bodu na trojuholniku
				//	vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
				}
				else if( t1>0 && t2<0)
				{
					// gula je v stene, vratime ju von zo steny
					// berieme bod, ktory je blizsie k rovine
					vec t1point = t1*vpold_vp+contactPointSphereToTriangle;
					vec t2point = t2*vpold_vp+contactPointSphereToTriangle;

				/*	if(PlanePointDistance( normal, distance, t1point)<=PlanePointDistance( normal, distance, t2point) )
						ClosestPointOnSphere = t1point;
					else 
						ClosestPointOnSphere = t2point;
				*/
					if( ABS(t1) < ABS(t2) )
						ClosestPointOnSphere = t1point;
					else
						ClosestPointOnSphere = t2point;
				
					if( (center.y-ClosestPointOnSphere.y)>ground_skew )return 1;
					else continue;
					// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
					// kolidujuceho bodu na trojuholniku
				//	vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
				}
				else // if( t1>0 && t2>0)
				{
					// gula je za trojuholnikom, gula nekoliduje s trojuholnikom v tomto smere pohybu
					continue;
				}
			}
			else
			{
				// nie je priesecnik, gula je mimo trojuholnika
				continue;
			}
		}
		else
		{
			if( (center.y-ClosestPointOnSphere.y)>ground_skew )return 1;
			else continue;

			// bod je vnutri trojuholnika
		//	contactPointSphereToTriangle = contactPointSphereToPlane;
			
			// mozeme sa pohnut iba tolko pokial sa colidujuci bod na guli nedotkne 
			// kolidujuceho bodu na trojuholniku
		//	vp_move = contactPointSphereToTriangle - ClosestPointOnSphere;
		}
/*			if( LineSphereIntersectionDir( contactPointSphereToTriangle, vpold_vp, center, radius, &t1, &t2) )
			{
				if( t1<=0 && t2<0)
				{
					// gula je pred trojuholnikom
					// berieme bod s t1, lebo ten je blizsie k stene (t1>t2)
					if( t1<-1.f)continue;		// tento trojuholnik nas nezaujima lebo nekoliduje po cely tento krok
					return 1;
				}
				else if( t1>0 && t2<0)
				{
					return 1;		// gula je v stene
				}
				else // if( t1>0 && t2>0)
				{
					// gula je za trojuholnikom, gula nekoliduje s trojuholnikom v tomto smere pohybu
					continue;
				}
			}
			else
			{
				// nie je priesecnik, gula je mimo trojuholnika
				continue;
			}
		}
		else
		{
			return 1;		// bod je vnutri trojuholnika
		}*/
	}
	return 0;
}
