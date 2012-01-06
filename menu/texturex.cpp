// texture.cpp: implementation of the texture class.
//
//////////////////////////////////////////////////////////////////////

#include "texturex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

texture::texture()
{
	id = 0;
	name = NULL;
	extens = NULL;
}

texture::~texture()
{
	glDeleteTextures( 1, &id);
	if(name!=NULL)delete[]name;
	if(extens!=NULL)delete[]extens;
	if(im.data!=NULL) delete []im.data;
}

/* 
 char *filename, int repeat, int filter, int compression
 repeat : 0-clamp, 1-repeat
 filter :
 0 - nearest
 1 - linear
 2 - bilinear, linear_mipmap_nearest
 3 - trilinear, linear_mipmap_linear
*/
int texture::load(char *filename, int repeat, int filter, int compression)
{
	if(filename==NULL)
	{
		if(name!=NULL)imageLoad( name);
		else return 0;
	}
	else imageLoad( filename);

	if(im.data==NULL)
	{
		if(filename==NULL)MessageBox( hWnd, name, "Nenájdený súbor", MB_OK );
		else MessageBox( hWnd, filename, "Nenájdený súbor", MB_OK );
		error = 1;
		return 0;
	}
	if(!imageResizeOgl())return 0;
	if(!imageLoadToOgl( repeat, filter, compression))return 0;

	if(filename!=NULL)
	{
		if(name!=NULL)delete[]name;
		name = new char[strlen(filename)+1];
		strcpy(name,filename);	
	}
	
	if(im.data) delete [] im.data;
	im.data=NULL;
	return 1;
}

int texture::loadCube(char *directory, char *extension, int filter, int compression)
{
	// right, left, top, bottom, back, front
	char *filename[6],*extension_lowercase;
	IMAGE i[6];
	int copyName=1;
	
	if( !ext.ARB_texture_cube_map) return 0;

	if(directory==NULL)
	{
		copyName=0;
		directory = name;
		extension = extens;
	}
	
	extension_lowercase = new char[strlen(extension)+1];
	if(extension_lowercase==NULL)return 0;
	strcpy(extension_lowercase,extension);
	strlwr(extension_lowercase);				// Convert a string to lowercase
	
	for(int j=0; j<6; j++)
	{
		filename[j] = new char[strlen(directory)+strlen(extension)+8];
		strcpy( filename[j], directory);
	}

	strcat( filename[0], "right.");
	strcat( filename[1], "left.");
	strcat( filename[2], "top.");
	strcat( filename[3], "bottom.");
	strcat( filename[4], "back.");
	strcat( filename[5], "front.");
	for( j=0; j<6; j++) strcat( filename[j], extension);

	for( j=0; j<6; j++)
	{
		if( !strcmp(extension_lowercase,"bmp"))		i[j] = LoadBMP( filename[j]);
		else if( !strcmp(extension_lowercase,"jpg"))	i[j] = LoadJPG( filename[j]);
		else if( !strcmp(extension_lowercase,"jpeg"))	i[j] = LoadJPG( filename[j]);
		else if( !strcmp(extension_lowercase,"tga"))	i[j] = LoadTGA( filename[j]);
	}

	if(i[3].data==NULL)			// ak sa nieje bottom, mozno bude down
	{
		strcpy( filename[3], directory);
		strcat( filename[3], "down.");
		strcat( filename[3], extension);
		if( !strcmp(extension_lowercase,"bmp"))		i[3] = LoadBMP( filename[3]);
		else if( !strcmp(extension_lowercase,"jpg"))	i[3] = LoadJPG( filename[3]);
		else if( !strcmp(extension_lowercase,"jpeg"))	i[3] = LoadJPG( filename[3]);
		else if( !strcmp(extension_lowercase,"tga"))	i[3] = LoadTGA( filename[3]);
	}
	delete [] extension_lowercase;
	int OK=1;
	for( j=0; j<6; j++)
	{
		if(i[j].data==NULL)
		{
			OK=0;
			MessageBox( hWnd, filename[j], "Nenájdený súbor", MB_OK );
		}
	}
	
	for( j=0; j<6; j++)delete [] filename[j];
	
	if(!OK)
	{
		for( j=0; j<6; j++)if(i[j].data!=NULL)delete[] i[j].data;
		error=1;
		return 0;
	}

	for( j=0; j<6; j++)
	{
		// uprava obrazku aby bol 2^x x 2^x
		if( !isPow2(i[j].sizeX) || !isPow2(i[j].sizeY) || i[j].sizeX!=i[j].sizeY )
		{
			int newx=1,newy=1;
			for( int newx2=i[j].sizeX; newx2 ;newx2>>=1)newx<<=1;
			for( int newy2=i[j].sizeY; newy2 ;newy2>>=1)newy<<=1;
			if( isPow2(i[j].sizeX) ) newx = i[j].sizeX;
			if( isPow2(i[j].sizeY) ) newy = i[j].sizeY;
			if(newx!=newy) newx<newy ? newx=newy : newy=newx;

			IMAGE out=resize( i[j], newx, newy);
			if( i[j].data!=NULL)delete [] i[j].data;
			if( out.data==NULL)return 0;
			i[j] = out;
		}

		// uprava obrazku aby velkost nebola vecsia ako maximalna velkost
		while(i[j].sizeX>ext.max.max_cube_map_texture_size)
		{
			IMAGE ret = resizeToHalfXY( i[j]);
			delete [] i[j].data;		
			if(ret.data==NULL)return 0;
			i[j] = ret;
		}
	}

	if(copyName)
	{
		if(extens!=NULL)delete[]extens;
		extens = new char[strlen(extension)+1];
		strcpy(extens,extension);
		if(name!=NULL)delete[]name;
		name = new char[strlen(directory)+1];
		strcpy(name,directory);
	}

	if(id!=0)glDeleteTextures( 1, &id);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, id);

	if(ext.EXT_texture_edge_clamp)
	{	// vyuzijeme rozsirenie GL_SGI_texture_edge_clamp ( GL_EXT_texture_edge_clamp )
		// pri texturavani v blizkosti hrany sa farba hrany nepouziva, texturuje sa iba z textury
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere v (zvislom) neopakuju
	}
	else
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );		// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );		// nastavuje ze textury sa v smere v (zvislom) neopakuju
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch(filter)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:		// bilinear
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 3:		// trilinear
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
	unsigned int format;
	int internalformat;
	unsigned int position;
	for( j=0; j<6; j++)
	{
		switch(j)
		{
		case 0: position=GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB; break;
		case 1: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB; break;
		case 2: position=GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB; break;
		case 3: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB; break;
		case 4: position=GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB; break;
		case 5: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB; break;
		}

		if(i[j].planes==1)
		{
			format = GL_ALPHA;
			if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_ALPHA_ARB;
			else internalformat = GL_ALPHA8;
		}
		else if(i[j].planes==3)
		{
			if(i[j].isBGR&&ext.EXT_bgra) format = GL_BGR_EXT;
			else 
			{
				if(i[j].isBGR)BGRtoRGB(i[j]);
				format = GL_RGB;
			}
			if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_RGB_ARB;
			else internalformat = GL_RGB8;
		}
		else if(i[j].planes==4)
		{
			if(i[j].isBGR&&ext.EXT_bgra) format = GL_BGRA_EXT;
			else 
			{
				if(i[j].isBGR)BGRtoRGB(i[j]);
				format = GL_RGBA;
			}
			if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_RGBA_ARB;
			else internalformat = GL_RGBA8;
		}

		if( position==GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || position==GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB || \
			position==GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB || position==GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB )
		{
			FlipMirror( i[j]);
		}
		if(filter<2)
		{
			glTexImage2D(position, 0, internalformat, i[j].sizeX, i[j].sizeY, 0, format, GL_UNSIGNED_BYTE, i[j].data);
		}
		else
		{			// mipmap
			if(ext.SGIS_generate_mipmap)
			{
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, TRUE);
				glTexImage2D(position, 0, internalformat, i[j].sizeX, i[j].sizeY, 0, format, GL_UNSIGNED_BYTE, i[j].data);
			}
			else
				gluBuild2DMipmaps(position, internalformat, i[j].sizeX, i[j].sizeY, format, GL_UNSIGNED_BYTE, i[j].data);
		}
	}
	
	for( j=0; j<6; j++)if(i[j].data!=NULL)delete[] i[j].data;
	return 1;
}

static unsigned char getSavePoint( IMAGE &i, int x, int y)
{
	x = (x+i.sizeX)%i.sizeX;						// repeat
//	y = (y+i.sizeY)%i.sizeY;						// repeat
//	if(x>=i.sizeX)x=i.sizeX-1; else if(x<0)x=0;		// clamp
	if(y>=i.sizeY)y=i.sizeY-1; else if(y<0)y=0;		// clamp
	return i.data[(i.sizeX*y+x)*i.planes+0];
}

static float evaluateMask( IMAGE &i, int x, int y, \
							float m11, float m12, float m13, \
							float m21, float m22, float m23, \
							float m31, float m32, float m33 )
{
	float r;

	r  = m11*getSavePoint( i, x-1, y-1);
	r += m12*getSavePoint( i, x  , y-1);
	r += m13*getSavePoint( i, x+1, y-1);
	r += m21*getSavePoint( i, x-1, y  );
	r += m22*getSavePoint( i, x  , y  );
	r += m23*getSavePoint( i, x+1, y  );
	r += m31*getSavePoint( i, x-1, y+1);
	r += m32*getSavePoint( i, x  , y+1);
	r += m33*getSavePoint( i, x+1, y+1);
	return r/255.f;
}
int texture::MakeDOT3(char *filename, int repeat, int filter, int compression, float strong, char *savename, int only_make)
{
	if(filename==NULL)
	{
		if(name!=NULL)imageLoad( name);
		else return 0;
	}
	else imageLoad( filename);

	if(im.data==NULL)
	{
		if(filename==NULL)MessageBox( hWnd, name, "Nenájdený súbor", MB_OK );
		else MessageBox( hWnd, filename, "Nenájdený súbor", MB_OK );
		error = 1;
		return 0;
	}

	if(im.isBGR)BGRtoRGB( im);

	IMAGE dot3;

	dot3.isBGR=0;
	dot3.planes=3;
	dot3.sizeX=im.sizeX;
	dot3.sizeY=im.sizeY;
	dot3.data = new unsigned char[dot3.sizeX*dot3.sizeY*dot3.planes];
	if(dot3.data==NULL)
	{
		if(im.data) delete [] im.data;
		im.data=NULL;
		return 0;
	}

	float dx,dy;		// dx derivacia vysky (farby) v smere x, dy derivacia v smere y

	for(int x=0; x<im.sizeX; x++)
	{
		for(int y=0; y<im.sizeY; y++)
		{
		//	dx = evaluateMask( im, x, y,  1,  0, -1,  2,  0, -2,  1,  0, -1 );	// Sobel-Prewitt maska y
		//	dy = evaluateMask( im, x, y,  1,  2,  1,  0,  0,  0, -1, -2, -1 );	// Sobel-Prewitt maska x

		//	dx = evaluateMask( im, x, y,  1,  0, -1,  1,  0, -1,  1,  0, -1 );	// Prewitt maska y
		//	dy = evaluateMask( im, x, y,  1,  1,  1,  0,  0,  0, -1, -1, -1 );	// Prewitt maska x

		//	dx = evaluateMask( im, x, y,  1,  1, -1,  1, -2, -1,  1,  1, -1 );	// Prewitt-Robinson maska y
		//	dy = evaluateMask( im, x, y,  1,  1,  1,  1, -2,  1, -1, -1, -1 );	// Prewitt-Robinson maska x

		//	dx = evaluateMask( im, x, y,  3,  3, -5,  3,  0, -5,  3,  3, -5 );	// Kirsch maska y
		//	dy = evaluateMask( im, x, y,  3,  3,  3,  3,  0,  3, -5, -5, -5 );	// Kirsch maska x

			// Sobel-Prewitt maska y - vyhladava vertikalne ciary = derivaciu x
			dx  = getSavePoint( im, x-1, y-1);					//  1  0 -1
			dx += 2.f*(float)getSavePoint( im, x-1, y);			//  2  0 -2
			dx += getSavePoint( im, x-1, y+1);					//  1  0 -1
			dx -= getSavePoint( im, x+1, y-1);
			dx -= 2.f*(float)getSavePoint( im, x+1, y);
			dx -= getSavePoint( im, x+1, y+1);
			dx /= 255.f;
			// Sobel-Prewitt maska x - vyhladava horizontalne ciary = derivacia y
			dy  = getSavePoint( im, x-1, y-1);					//  1  2  1
			dy += 2.f*(float)getSavePoint( im, x  , y-1);		//  0  0  0
			dy += getSavePoint( im, x+1, y-1);					// -1 -2 -1
			dy -= getSavePoint( im, x-1, y+1);
			dy -= 2.f*(float)getSavePoint( im, x  , y+1);
			dy -= getSavePoint( im, x+1, y+1);
			dy /= 255.f;

			float nX,nY,nZ;
			nX = strong*dx;
			nY = strong*dy;
			nZ = 1.f;
			float normalise = 1.0f/((float) sqrt(nX*nX + nY*nY + nZ*nZ));
			nX *= normalise;
			nY *= normalise;
			nZ *= normalise;
			nX = nX*0.5f+0.5f;
			nY = nY*0.5f+0.5f;
			nZ = nZ*0.5f+0.5f;

			dot3.data[(dot3.sizeX*y+x)*dot3.planes+0] = (unsigned char)(255.f*nX);
			dot3.data[(dot3.sizeX*y+x)*dot3.planes+1] = (unsigned char)(255.f*nY);
			dot3.data[(dot3.sizeX*y+x)*dot3.planes+2] = (unsigned char)(255.f*nZ);
		}
	}

	if(savename!=NULL)SaveBMP( dot3, savename);
	if(im.data!=NULL) delete[]im.data;
	im = dot3;
	if(only_make)return 1;

	if(!imageResizeOgl())return 0;
	if(!imageLoadToOgl( repeat, filter, compression))return 0;

	if(filename!=NULL)
	{
		if(name!=NULL)delete[]name;
		name = new char[strlen(filename)+1];
		strcpy(name,filename);	
	}

	if(im.data) delete [] im.data;
	im.data=NULL;
	return 1;
}

void texture::setForCopyToTexture(int x, int y, int repeat, int filter)
{
	im.sizeX=x;
	im.sizeY=y;
	if(id!=0)glDeleteTextures( 1, &id);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	if(repeat)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}
	else
	{
		if(ext.EXT_texture_edge_clamp)
		{	// vyuzijeme rozsirenie GL_SGI_texture_edge_clamp ( GL_EXT_texture_edge_clamp )
			// pri texturavani v blizkosti hrany sa farba hrany nepouziva, texturuje sa iba z textury
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );		// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );		// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch(filter)
	{
	case 0:		// nearest
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:		// linear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:		// bilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 3:		// trilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	if(filter<2)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	}
	else
	{			// mipmap
		if(ext.SGIS_generate_mipmap)
		{
			glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}
		else
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB8, x, y, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	}
}

void texture::setCubeForCopyToTexture(int x, int y, int repeat, int filter)
{
	// right, left, top, bottom, back, front
	if(id!=0)glDeleteTextures( 1, &id);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, id);

	if(ext.EXT_texture_edge_clamp)
	{	// vyuzijeme rozsirenie GL_SGI_texture_edge_clamp ( GL_EXT_texture_edge_clamp )
		// pri texturavani v blizkosti hrany sa farba hrany nepouziva, texturuje sa iba z textury
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere v (zvislom) neopakuju
	}
	else
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP );		// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP );		// nastavuje ze textury sa v smere v (zvislom) neopakuju
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch(filter)
	{
	case 0:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:		// bilinear
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 3:		// trilinear
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
	unsigned int position;
	for(int j=0; j<6; j++)
	{
		switch(j)
		{
		case 0: position=GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB; break;
		case 1: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB; break;
		case 2: position=GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB; break;
		case 3: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB; break;
		case 4: position=GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB; break;
		case 5: position=GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB; break;
		}

	//	if( position==GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || position==GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB || \
	//		position==GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB || position==GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB )
	//	{
		//	FlipMirror( i[j]);
	//	}
		if(filter<2)
		{
			glTexImage2D(position, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}
		else
		{			// mipmap
			if(ext.SGIS_generate_mipmap)
			{
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
				glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, TRUE);
				glTexImage2D(position, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			}
			else
				gluBuild2DMipmaps(position, GL_RGB8, x, y, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		}
	}
}
void texture::setAnisotropicFiltering(int level)
{
	if(ext.EXT_texture_filter_anisotropic)
	{
		if(level>ext.max.max_texture_max_anisotropy)level=ext.max.max_texture_max_anisotropy;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, level);
	}
}

int texture::imageLoad(char *filename)
{
	char p[5];

	char* pdest = strrchr( filename, '.' );		// find '.'
	if( pdest[0]!=NULL)pdest++;
	strncpy( p, pdest, 4);
	strlwr(p);				// Convert a string to lowercase
	
	if(im.data!=NULL) delete[]im.data;
	im.data = NULL;

	if( !strcmp(p,"bmp"))		im = LoadBMP( filename);
	else if( !strcmp(p,"jpg"))	im = LoadJPG( filename);
	else if( !strcmp(p,"jpeg"))	im = LoadJPG( filename);
	else if( !strcmp(p,"tga"))	im = LoadTGA( filename);
	
	if(im.data==NULL)return 0;
	else return 1;
}

int texture::imageResizeOgl()
{
	if(im.data==NULL)return 0;

	// uprava obrazku aby bol 2^ x 2^
	if( !isPow2(im.sizeX) || !isPow2(im.sizeY) )
	{
		int newx=1,newy=1;
		for( int newx2=im.sizeX; newx2 ;newx2>>=1)newx<<=1;		// newx = 2^n, newx>im.sizeX
		for( int newy2=im.sizeY; newy2 ;newy2>>=1)newy<<=1;		// newy = 2^m, newy>im.sizeY
		if( isPow2(im.sizeX) ) newx = im.sizeX;
		if( isPow2(im.sizeY) ) newy = im.sizeY;

		IMAGE out=resize( im, newx, newy);
		if( im.data!=NULL)delete [] im.data;
		if( out.data==NULL)return 0;
		im = out;
	}

	// uprava obrazku aby velkost nebola vecsia ako maximalna velkost
	while(im.sizeX>ext.max.max_texture_size && im.sizeY>ext.max.max_texture_size)
	{
		IMAGE ret = resizeToHalfXY( im);
		delete [] im.data;		
		if(ret.data==NULL)return 0;
		im = ret;
	}
	while(im.sizeX>ext.max.max_texture_size )
	{
		IMAGE ret = resizeToHalfX( im);
		delete [] im.data;		
		if(ret.data==NULL)return 0;
		im = ret;
	}
	while( im.sizeY>ext.max.max_texture_size)
	{
		IMAGE ret = resizeToHalfY( im);
		delete [] im.data;		
		if(ret.data==NULL)return 0;
		im = ret;
	}
	return 1;
}

/* 
 repeat : 0-clamp, 1-repeat
 filter :
 0 - nearest
 1 - linear
 2 - bilinear, linear_mipmap_nearest
 3 - trilinear, linear_mipmap_linear
*/
int texture::imageLoadToOgl( int repeat, int filter, int compression)
{
	if(im.data==NULL)return 0;
	
	if(id!=0)glDeleteTextures( 1, &id);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	if(repeat)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}
	else
	{
		if(ext.EXT_texture_edge_clamp)
		{	// vyuzijeme rozsirenie GL_SGI_texture_edge_clamp ( GL_EXT_texture_edge_clamp )
			// pri texturavani v blizkosti hrany sa farba hrany nepouziva, texturuje sa iba z textury
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );		// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );		// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch(filter)
	{
	case 0:		// nearest
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:		// linear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:		// bilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 3:		// trilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	unsigned int format;
	int internalformat;
	if(im.planes==1)
	{
		format = GL_ALPHA;
		if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_ALPHA_ARB;
		else internalformat = GL_ALPHA8;
	}
	else if(im.planes==3)
	{
		if(im.isBGR&&ext.EXT_bgra) format = GL_BGR_EXT;
		else 
		{
			if(im.isBGR)BGRtoRGB(im);
			format = GL_RGB;
		}
		if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_RGB_ARB;
		else internalformat = GL_RGB8;
	}
	else if(im.planes==4)
	{
		if(im.isBGR&&ext.EXT_bgra) format = GL_BGRA_EXT;
		else 
		{
			if(im.isBGR)BGRtoRGB(im);
			format = GL_RGBA;
		}
		if(compression && ext.ARB_texture_compression) internalformat = GL_COMPRESSED_RGBA_ARB;
		else internalformat = GL_RGBA8;
	}

	if(filter<2)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, im.sizeX, im.sizeY, 0, format, GL_UNSIGNED_BYTE, im.data);
	}
	else
	{			// mipmap
		if(ext.SGIS_generate_mipmap)
		{
			glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, im.sizeX, im.sizeY, 0, format, GL_UNSIGNED_BYTE, im.data);
		}
		else
		gluBuild2DMipmaps(GL_TEXTURE_2D, internalformat, im.sizeX, im.sizeY, format, GL_UNSIGNED_BYTE, im.data);
	}
	return 1;
}


IMAGE texture::LoadBMP(char *filename)
{
	FILE* f;
	IMAGE i;
	BITMAPFILEHEADER fileheader;
	BITMAPINFOHEADER infoheader;
	unsigned char* data;

	f = fopen(filename,"rb");
	if(f==NULL)return i;

	if(!fread( &fileheader, sizeof(BITMAPFILEHEADER), 1, f))
	{	fclose(f); return i; }

	if(!fread( &infoheader, sizeof(BITMAPINFOHEADER), 1, f))
	{	fclose(f); return i; }

	// kontrola ci je to BMP subor, 'BM'  == 0x4D42
	if(fileheader.bfType!=0x4D42){	fclose(f); return i; }

	// nacitavame iba obrazky bez kompresie
	if(infoheader.biCompression){	fclose(f); return i; }
	
	switch(infoheader.biBitCount)
	{
	case 1: return LoadBMP1bit( f, i, fileheader, infoheader);
	case 4: return LoadBMP4bit( f, i, fileheader, infoheader);
	case 8: return LoadBMP8bit( f, i, fileheader, infoheader);
	case 24: break;
	case 32: break;
	default:
		fclose(f); return i;
	}

	int line_size = infoheader.biWidth*infoheader.biBitCount/8;
	if(line_size%4)line_size += 4 - line_size%4;
	int data_size = line_size*infoheader.biHeight;
	//	data_size = fileheader.bfSize-sizeof(BITMAPFILEHEADER)-infoheader.biSize;
	data = new unsigned char[data_size];
	if(data==NULL){	fclose(f); return i; }
	if(!fread( data, data_size, 1, f))
	{	
		fclose(f);
		delete [] data;
		return i; 
	}
	fclose(f);
	
	i.sizeX=infoheader.biWidth;
	i.sizeY=infoheader.biHeight;
	i.planes=infoheader.biBitCount/8;
	i.isBGR=1;

	if( (i.sizeX*i.planes)%4==0 ) 
	{
		i.data = data;
		return i;
	}
	
	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL){	fclose(f); delete [] data; return i; }
	
	line_size = i.sizeX*i.planes;
	if(line_size%4)line_size += 4 - line_size%4;	

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(i.sizeX*y+x)*i.planes+2]=data[line_size*y+x*i.planes+0];
			i.data[(i.sizeX*y+x)*i.planes+1]=data[line_size*y+x*i.planes+1];
			i.data[(i.sizeX*y+x)*i.planes+0]=data[line_size*y+x*i.planes+2];
			if(i.planes==4)i.data[(i.sizeX*y+x)*i.planes+3]=data[line_size*y+x*i.planes+3];
		}
	}
	i.isBGR = 0;
	delete [] data;

	return i;
}

void texture::SaveBMP(IMAGE &imag, char *filename)
{
	BITMAPFILEHEADER fileheader;
	BITMAPINFOHEADER infoheader;

	FILE *sub;
	
	int line_size = imag.planes*imag.sizeX;
	if(line_size%4)line_size += 4 - line_size%4;
	
	fileheader.bfType = 0x4D42; // Magic identifier   - "BM"	| identifikacia BMP suboru musi byt "BM"
	fileheader.bfSize = imag.planes*imag.sizeX*imag.sizeY+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// File size in bytes			| velkos suboru v byte
	fileheader.bfReserved1 = 0;
	fileheader.bfReserved2 = 0;
	fileheader.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// Offset to image data, bytes	| posun na zaciatok dat
	
	infoheader.biSize = sizeof(BITMAPINFOHEADER);	// Header size in bytes			| velkost hlavicky BITMAPINFOHEADER
	infoheader.biWidth = imag.sizeX;	// Width of image			| sirka obrazka - sizeX
	infoheader.biHeight = imag.sizeY;	// Height of image			| vyska obrazka - sizeY
	infoheader.biPlanes = 1;		// Number of colour planes	| pocet farebnych rovin musi bit 1
	infoheader.biBitCount = 24;		// Bits per pixel			| bitov na pixel moze bit 1,4,8,24
	infoheader.biCompression = 0;	// Compression type			| typ compresie , 0 - bez kompresie
	infoheader.biSizeImage = line_size*infoheader.biHeight ;	// Image size in bytes		| velkost obrazka v byte
	infoheader.biXPelsPerMeter = 0;	// Pixels per meter X		| pixelov na meter v smere x
	infoheader.biYPelsPerMeter = 0;	// Pixels per meter Y		| pixelov na meter v smere y
	infoheader.biClrUsed = 0;		// Number of colours		| pocet  farieb v palete, ak 0 vsetky su pouzivane
	infoheader.biClrImportant = 0;	// Important colours		| dolezite farby v palete, ak 0 vsetky su dolezite
	
	sub = fopen(filename,"wb");
	if(sub==NULL)return;
	fwrite( &fileheader, sizeof(BITMAPFILEHEADER), 1, sub);
	fwrite( &infoheader, sizeof(BITMAPINFOHEADER), 1, sub);
	
	unsigned char* p = new unsigned char[line_size*imag.sizeY];
	for(int y=0; y<imag.sizeY; y++)
	{
		for(int x=0; x<imag.sizeX; x++)
		{
			if(imag.isBGR)
			{
				p[y*line_size+x*3+0] = imag.data[(y*imag.sizeX+x)*3+0];
				p[y*line_size+x*3+1] = imag.data[(y*imag.sizeX+x)*3+1];
				p[y*line_size+x*3+2] = imag.data[(y*imag.sizeX+x)*3+2];
			}
			else
			{
				p[y*line_size+x*3+0] = imag.data[(y*imag.sizeX+x)*3+2];
				p[y*line_size+x*3+1] = imag.data[(y*imag.sizeX+x)*3+1];
				p[y*line_size+x*3+2] = imag.data[(y*imag.sizeX+x)*3+0];
			}
		}
	}
	fwrite( p, line_size*imag.sizeY, 1, sub);
	delete[] p;
	fclose(sub);
}


void texture::BGRtoRGB( IMAGE &i)
{
	if(!i.isBGR || i.planes<3)return;
	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			unsigned char pom = i.data[(i.sizeX*y+x)*i.planes+2];
			i.data[(i.sizeX*y+x)*i.planes+2]=i.data[(i.sizeX*y+x)*i.planes+0];
			i.data[(i.sizeX*y+x)*i.planes+0]=pom;
		}
	}
	i.isBGR=0;
}

IMAGE texture::LoadBMP1bit(FILE *f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader)
{
	RGBQUAD pal[2];
	unsigned int countpal;

	if(infoheader.biBitCount!=1)return i;
	if(infoheader.biClrUsed==0)countpal=2;
	else countpal=infoheader.biClrUsed;
	if(fread( pal, sizeof(RGBQUAD), countpal, f)!=countpal)
	{
		fclose(f);
		return i;
	}

	unsigned char* data;
	int line_size = infoheader.biWidth/8;
	if(infoheader.biWidth%8)line_size++;
	if(line_size%4)line_size += 4 - line_size%4;
	data = new unsigned char[line_size*infoheader.biHeight];
	if(data==NULL){	fclose(f); return i; }
	if(!fread( data, line_size*infoheader.biHeight, 1, f))
	{	
		fclose(f);
		delete [] data;
		return i; 
	}
	fclose(f);

	i.sizeX=infoheader.biWidth;
	i.sizeY=infoheader.biHeight;
	i.planes=3;
	i.isBGR=0;

	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL){ delete [] data; return i; }

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			unsigned char index=data[y*line_size+x/8];
			index >>= (7-x%8);
			index &= 0x01;
			i.data[(y*i.sizeX+x)*i.planes+0] = pal[index].rgbRed;
			i.data[(y*i.sizeX+x)*i.planes+1] = pal[index].rgbGreen;
			i.data[(y*i.sizeX+x)*i.planes+2] = pal[index].rgbBlue;
		}
	}
	if(data!=NULL)delete []data;

	return i;
}

IMAGE texture::LoadBMP4bit(FILE *f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader)
{
	RGBQUAD pal[16];
	unsigned int countpal;

	if(infoheader.biBitCount!=4)return i;
	if(infoheader.biClrUsed==0)countpal=4;
	else countpal=infoheader.biClrUsed;
	if(fread( pal, sizeof(RGBQUAD), countpal, f)!=countpal)
	{
		fclose(f);
		return i;
	}

	unsigned char* data;
	int line_size = infoheader.biWidth/2+infoheader.biWidth%2;
	if(line_size%4)line_size += 4 - line_size%4;
	data = new unsigned char[line_size*infoheader.biHeight];
	if(data==NULL){	fclose(f); return i; }
	if(!fread( data, line_size*infoheader.biHeight, 1, f))
	{	
		fclose(f);
		delete [] data;
		return i; 
	}
	fclose(f);

	i.sizeX=infoheader.biWidth;
	i.sizeY=infoheader.biHeight;
	i.planes=3;
	i.isBGR=0;

	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL){ delete [] data; return i; }

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			int index;
			if(x%2) index = data[y*line_size+x/2]&0x0F;
			else index = (data[y*line_size+x/2]&0x0F0)>>4;
			i.data[(y*i.sizeX+x)*i.planes+0] = pal[index].rgbRed;
			i.data[(y*i.sizeX+x)*i.planes+1] = pal[index].rgbGreen;
			i.data[(y*i.sizeX+x)*i.planes+2] = pal[index].rgbBlue;
		}
	}
	if(data!=NULL)delete []data;

	return i;
}

IMAGE texture::LoadBMP8bit(FILE *f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader)
{
	RGBQUAD pal[256];
	unsigned int countpal;

	if(infoheader.biBitCount!=8)return i;
	if(infoheader.biClrUsed==0)countpal=256;
	else countpal=infoheader.biClrUsed;
	if(fread( pal, sizeof(RGBQUAD), countpal, f)!=countpal)
	{
		fclose(f);
		return i;
	}

	unsigned char* data;
	int line_size = infoheader.biWidth;
	if(line_size%4)line_size += 4 - line_size%4;
	data = new unsigned char[line_size*infoheader.biHeight];
	if(data==NULL){	fclose(f); return i; }
	if(!fread( data, line_size*infoheader.biHeight, 1, f))
	{	
		fclose(f);
		delete [] data;
		return i; 
	}
	fclose(f);

	i.sizeX=infoheader.biWidth;
	i.sizeY=infoheader.biHeight;
	i.planes=3;
	i.isBGR=0;

	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL){ delete [] data; return i; }

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = pal[data[y*line_size+x]].rgbRed;
			i.data[(y*i.sizeX+x)*i.planes+1] = pal[data[y*line_size+x]].rgbGreen;
			i.data[(y*i.sizeX+x)*i.planes+2] = pal[data[y*line_size+x]].rgbBlue;
		}
	}
	if(data!=NULL)delete []data;
	return i;
}

IMAGE texture::LoadJPG(char *filename)
{
	IMAGE i;
	i = ::LoadJPG( filename);
	Flip( i);
	return i;
}

// These defines are used to tell us about the type of TARGA file it is
#define TGA_RGB		 2		// This tells us it's a normal RGB (really BGR) file
#define TGA_A		 3		// This tells us it's a ALPHA file
#define TGA_RLE		10		// This tells us that the targa is Run-Length Encoded (RLE)

IMAGE texture::LoadTGA(char *filename)
{
	IMAGE i;

	// code from www.gametutorials.com
	WORD width = 0, height = 0;			// The dimensions of the image
	byte length = 0;					// The length in bytes to the pixels
	byte imageType = 0;					// The image type (RLE, RGB, Alpha...)
	byte bits = 0;						// The bits per pixel for the image (16, 24, 32)
	FILE *pFile = NULL;					// The file pointer
	int channels = 0;					// The channels of the image (3 = RGA : 4 = RGBA)
	int stride = 0;						// The stride (channels * width)
		
	// Open a file pointer to the targa file and check if it was found and opened 
	if((pFile = fopen(filename, "rb")) == NULL) return i;
		
	fread(&length, sizeof(byte), 1, pFile);
	fseek(pFile,1,SEEK_CUR); 
	fread(&imageType, sizeof(byte), 1, pFile);
	fseek(pFile, 9, SEEK_CUR); 

	// Read the width, height and bits per pixel (16, 24 or 32)
	fread(&width,  sizeof(WORD), 1, pFile);
	fread(&height, sizeof(WORD), 1, pFile);
	fread(&bits,   sizeof(byte), 1, pFile);
	
	// Now we move the file pointer to the pixel data
	fseek(pFile, length + 1, SEEK_CUR); 

	// Check if the image is RLE compressed or not
	if(imageType != TGA_RLE)
	{
		// Check if the image is a 24 or 32-bit image
		if(bits == 24 || bits == 32)
		{
			// Calculate the channels (3 or 4) - (use bits >> 3 for more speed).
			// Next, we calculate the stride and allocate enough memory for the pixels.
			channels = bits / 8;
			stride = channels * width;
			i.data = new unsigned char[stride * height];

			// Load in all the pixel data line by line
			for(int y = 0; y < height; y++)
			{
				// Store a pointer to the current line of pixels
				unsigned char *pLine = &(i.data[stride * y]);

				// Read in the current line of pixels
				fread(pLine, stride, 1, pFile);
			}
			i.isBGR=1;
		}
		// Check if the image is a 16 bit image (RGB stored in 1 unsigned short)
		else if(bits == 16)
		{
			unsigned short pixels = 0;
			int r=0, g=0, b=0;

			// Since we convert 16-bit images to 24 bit, we hardcode the channels to 3.
			// We then calculate the stride and allocate memory for the pixels.
			channels = 3;
			stride = channels * width;
			i.data = new unsigned char[stride * height];

			// Load in all the pixel data pixel by pixel
			for(int j = 0; j < width*height; j++)
			{
				// Read in the current pixel
				fread(&pixels, sizeof(unsigned short), 1, pFile);
				
				// To convert a 16-bit pixel into an R, G, B, we need to
				// do some masking and such to isolate each color value.
				// 0x1f = 11111 in binary, so since 5 bits are reserved in
				// each unsigned short for the R, G and B, we bit shift and mask
				// to find each value.  We then bit shift up by 3 to get the full color.
				b = (pixels & 0x1f) << 3;
				g = ((pixels >> 5) & 0x1f) << 3;
				r = ((pixels >> 10) & 0x1f) << 3;
				
				// This essentially assigns the color to our array and swaps the
				// B and R values at the same time.
				i.data[j * 3 + 0] = r;
				i.data[j * 3 + 1] = g;
				i.data[j * 3 + 2] = b;
			}
		}	
		else
			return i;
	}
	// Else, it must be Run-Length Encoded (RLE)
	else
	{
		int j=0;
		byte rleID = 0;
		int colorsRead = 0;
		channels = bits / 8;
		stride = channels * width;

		i.data = new unsigned char[stride * height];
		byte *pColors = new byte [channels];

		while(j < width*height)
		{
			fread(&rleID, sizeof(byte), 1, pFile);
			
			if(rleID < 128)
			{
				rleID++;
				while(rleID)
				{
					fread(pColors, sizeof(byte) * channels, 1, pFile);
					i.data[colorsRead + 0] = pColors[2];
					i.data[colorsRead + 1] = pColors[1];
					i.data[colorsRead + 2] = pColors[0];

					if(bits == 32)i.data[colorsRead + 3] = pColors[3];
					j++;
					rleID--;
					colorsRead += channels;
				}
			}
			else
			{
				rleID -= 127;
				fread(pColors, sizeof(byte) * channels, 1, pFile);

				while(rleID)
				{
					i.data[colorsRead + 0] = pColors[2];
					i.data[colorsRead + 1] = pColors[1];
					i.data[colorsRead + 2] = pColors[0];

					if(bits == 32)i.data[colorsRead + 3] = pColors[3];
					j++;
					rleID--;
					colorsRead += channels;
				}
				
			}
				
		}
	}
	fclose(pFile);

	i.planes = channels;
	i.sizeX    = width;
	i.sizeY    = height;

	return i;
}

IMAGE texture::shift(const IMAGE &in)		// jas * 0.5
{
	IMAGE i=in;

	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL)return i;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = in.data[(y*i.sizeX+x)*i.planes+0]>>1;
			i.data[(y*i.sizeX+x)*i.planes+1] = in.data[(y*i.sizeX+x)*i.planes+1]>>1;
			i.data[(y*i.sizeX+x)*i.planes+2] = in.data[(y*i.sizeX+x)*i.planes+2]>>1;
		}
	}
	return i;
}

IMAGE texture::invShift(const IMAGE &in)	// invertovana a potom jas*0.5
{
	IMAGE i=in;

	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL)return i;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = (255-in.data[(y*i.sizeX+x)*i.planes+0])>>1;
			i.data[(y*i.sizeX+x)*i.planes+1] = (255-in.data[(y*i.sizeX+x)*i.planes+1])>>1;
			i.data[(y*i.sizeX+x)*i.planes+2] = (255-in.data[(y*i.sizeX+x)*i.planes+2])>>1;
		}
	}
	return i;
}

IMAGE texture::resize(const IMAGE &in, int newx, int newy)
{
	IMAGE i;
	float *d;

	i.sizeX = newx;
	i.sizeY = newy;
	i.planes = in.planes;
	i.isBGR = in.isBGR;
	i.data = new unsigned char[i.sizeX*i.sizeY*i.planes];
	if(!i.data)return i;

	d = new float[in.sizeX*in.sizeY*in.planes];
	if(!d)
	{
		delete [] i.data;
		i.data = NULL;
		return i;
	}

	for(int j=0; j<in.sizeX*in.sizeY*in.planes; j++)d[j] = (float)in.data[j];

	float xf,yf;
	int ix,iy;		// index do vstupneho obrazka
	for(int y=0; y<newy; y++)
	{
		yf = y/(float)(newy-1);
		yf *= (float) in.sizeY;
		iy = (int) floor(yf);		// vrati mensiu celu cast 2.8 -> 2.0
		yf = (float)fmod(yf,1.0);			// zvysok po deleni 1
		for(int x=0; x<newx; x++)
		{
			xf = x/(float)(newx-1);
			xf *= (float) in.sizeX;
			ix = (int) floor(xf);		// vrati mensiu celu cast 2.8 -> 2.0
			xf = (float)fmod(xf,1.0);			// zvysok po deleni 1
		
			for(int c=0; c<in.planes; c++)
			{
				float a,b;
				if(ix<in.sizeX-1 && iy<in.sizeY-1)
				{
					a = (1.f-xf)*d[(iy*in.sizeX+ix)*in.planes+c]+xf*d[(iy*in.sizeX+ix+1)*in.planes+c];
					b = (1.f-xf)*d[((iy+1)*in.sizeX+ix)*in.planes+c]+xf*d[((iy+1)*in.sizeX+ix+1)*in.planes+c];
					a = (1.f-yf)*a+yf*b;
				}
				else
				{
					if(ix<in.sizeX-1)		// iy >= in.sizeY-1
					{
						iy = in.sizeY-1;
						a = (1.f-xf)*d[(iy*in.sizeX+ix)*in.planes+c]+xf*d[(iy*in.sizeX+ix+1)*in.planes+c];
					}
					else if(iy<in.sizeY-1)	// ix >= in.sizeX-1 && iy<in.sizeY-1
					{
						ix = in.sizeX-1;
						a = (1.f-yf)*d[(iy*in.sizeX+ix)*in.planes+c]+yf*d[((iy+1)*in.sizeX+ix)*in.planes+c];
					}
					else
					{
						ix = in.sizeX-1;
						iy = in.sizeY-1;
						a = d[(iy*in.sizeX+ix)*in.planes+c];
					}
				}
				i.data[(y*i.sizeX+x)*i.planes+c] = (unsigned char) a;
			}
		}
	}

	delete [] d;
	return i;
}

IMAGE texture::resizeToHalfX(const IMAGE &in)
{
	IMAGE i=in;
	i.data = NULL;

	if(in.sizeX<=1)return i;

	i.sizeX >>=1;		// /2
	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL)return i;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = ((unsigned int)in.data[(y*in.sizeX+2*x)*in.planes+0]+(unsigned int)in.data[(y*in.sizeX+2*x+1)*in.planes+0])>>1;
			if(i.planes>1)i.data[(y*i.sizeX+x)*i.planes+1] = ((unsigned int)in.data[(y*in.sizeX+2*x)*in.planes+1]+(unsigned int)in.data[(y*in.sizeX+2*x+1)*in.planes+1])>>1;
			if(i.planes>2)i.data[(y*i.sizeX+x)*i.planes+2] = ((unsigned int)in.data[(y*in.sizeX+2*x)*in.planes+2]+(unsigned int)in.data[(y*in.sizeX+2*x+1)*in.planes+2])>>1;
			if(i.planes>3)i.data[(y*i.sizeX+x)*i.planes+3] = ((unsigned int)in.data[(y*in.sizeX+2*x)*in.planes+3]+(unsigned int)in.data[(y*in.sizeX+2*x+1)*in.planes+3])>>1;
		}
	}
	return i;
}

IMAGE texture::resizeToHalfY(const IMAGE &in)
{
	IMAGE i=in;
	i.data = NULL;
	if(in.sizeY<=1)return i;

	i.sizeY >>=1;		// /2
	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL)return i;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = ((unsigned int)in.data[(2*y*in.sizeX+x)*in.planes+0]+(unsigned int)in.data[((2*y+1)*in.sizeX+x)*in.planes+0])>>1;
			if(i.planes>1)i.data[(y*i.sizeX+x)*i.planes+1] = ((unsigned int)in.data[(2*y*in.sizeX+x)*in.planes+1]+(unsigned int)in.data[((2*y+1)*in.sizeX+x)*in.planes+1])>>1;
			if(i.planes>2)i.data[(y*i.sizeX+x)*i.planes+2] = ((unsigned int)in.data[(2*y*in.sizeX+x)*in.planes+2]+(unsigned int)in.data[((2*y+1)*in.sizeX+x)*in.planes+2])>>1;
			if(i.planes>3)i.data[(y*i.sizeX+x)*i.planes+3] = ((unsigned int)in.data[(2*y*in.sizeX+x)*in.planes+3]+(unsigned int)in.data[((2*y+1)*in.sizeX+x)*in.planes+3])>>1;
		}
	}
	return i;
}

IMAGE texture::resizeToHalfXY(const IMAGE &in)
{
	IMAGE i=in;
	i.data = NULL;
	if(in.sizeX<=1 && in.sizeY<=1)return i;

	i.sizeX >>=1;		// /2
	i.sizeY >>=1;		// /2
	i.data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(i.data==NULL)return i;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			i.data[(y*i.sizeX+x)*i.planes+0] = (
							(unsigned int)in.data[(2*y*in.sizeX+2*x)*in.planes+0]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x)*in.planes+0]+
							(unsigned int)in.data[(2*y*in.sizeX+2*x+1)*in.planes+0]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x+1)*in.planes+0])>>2;
			if(i.planes>1)i.data[(y*i.sizeX+x)*i.planes+1] = (
							(unsigned int)in.data[(2*y*in.sizeX+2*x)*in.planes+1]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x)*in.planes+1]+
							(unsigned int)in.data[(2*y*in.sizeX+2*x+1)*in.planes+1]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x+1)*in.planes+1])>>2;
			if(i.planes>2)i.data[(y*i.sizeX+x)*i.planes+2] = (
							(unsigned int)in.data[(2*y*in.sizeX+2*x)*in.planes+2]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x)*in.planes+2]+
							(unsigned int)in.data[(2*y*in.sizeX+2*x+1)*in.planes+2]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x+1)*in.planes+2])>>2;
			if(i.planes>3)i.data[(y*i.sizeX+x)*i.planes+3] = (
							(unsigned int)in.data[(2*y*in.sizeX+2*x)*in.planes+3]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x)*in.planes+3]+
							(unsigned int)in.data[(2*y*in.sizeX+2*x+1)*in.planes+3]+
							(unsigned int)in.data[((2*y+1)*in.sizeX+2*x+1)*in.planes+3])>>2;
		}
	}
	return i;
}

void texture::Flip( IMAGE &i)		// hore - dole
{
	if(i.data==NULL)return;
	unsigned char *data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(data==NULL)return;

	for(int y=0; y<i.sizeY; y++)
	{
		memcpy( &data[i.planes*i.sizeX*y], &i.data[i.planes*i.sizeX*(i.sizeY-y-1)], i.planes*i.sizeX );
	}
	delete [] i.data;
	i.data = data;
}

void texture::FlipMirror(IMAGE &i)		// up - down, right - left
{
	if(i.planes<3 || i.data==NULL)return;
	unsigned char *data = new unsigned char[i.planes*i.sizeX*i.sizeY];
	if(data==NULL)return;

	for(int y=0; y<i.sizeY; y++)
	{
		for(int x=0; x<i.sizeX; x++)
		{
			data[(y*i.sizeX+x)*i.planes+0]=i.data[( (i.sizeY-y-1)*i.sizeX+i.sizeX-x-1)*i.planes+0];
			data[(y*i.sizeX+x)*i.planes+1]=i.data[( (i.sizeY-y-1)*i.sizeX+i.sizeX-x-1)*i.planes+1];
			data[(y*i.sizeX+x)*i.planes+2]=i.data[( (i.sizeY-y-1)*i.sizeX+i.sizeX-x-1)*i.planes+2];
		}
	}
	delete [] i.data;
	i.data = data;
}

int texture::loadDUDV(char *filename, int repeat, int filter, int compression)
{
	imageLoad( filename);

	if(im.data==NULL)
	{
		MessageBox( hWnd, filename, "Nenájdený súbor", MB_OK );
		error = 1;
		return 0;
	}
	if(!imageResizeOgl())return 0;
	
	if(id!=0)glDeleteTextures( 1, &id);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	if(repeat)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}
	else
	{
		if(ext.EXT_texture_edge_clamp)
		{	// vyuzijeme rozsirenie GL_SGI_texture_edge_clamp ( GL_EXT_texture_edge_clamp )
			// pri texturavani v blizkosti hrany sa farba hrany nepouziva, texturuje sa iba z textury
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );	// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );		// nastavuje ze textury sa v smere u (vodorovnom) neopakuju
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );		// nastavuje ze textury sa v smere v (zvislom) neopakuju
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	switch(filter)
	{
	case 0:		// nearest
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		break;
	case 1:		// linear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		break;
	case 2:		// bilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		break;
	case 3:		// trilinear
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		break;
	default:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

#ifndef GL_DUDV_ATI
#define GL_DUDV_ATI                 0x8779
#endif
#ifndef GL_DU8DV8_ATI
#define GL_DU8DV8_ATI               0x877A
#endif

	unsigned int format=GL_DUDV_ATI;
	int internalformat=GL_DU8DV8_ATI;	//GL_DU8DV8_ATI;
	
	if(im.planes==1)
	{
		if(im.data) delete [] im.data;
		im.data=NULL;
		return 0;
	}
	else if(im.planes==3)
	{
		if(im.isBGR)BGRtoRGB(im);
	}
	else if(im.planes==4)
	{
		if(im.data) delete [] im.data;
		im.data=NULL;
		return 0;
	}
	// orezeme 3 (blue) kanal
	for(int y=0; y<im.sizeY; y++)
	{
		for(int x=0; x<im.sizeX; x++)
		{
			im.data[(y*im.sizeX+x)*2+0] = im.data[(y*im.sizeX+x)*im.planes+0];
			im.data[(y*im.sizeX+x)*2+1] = im.data[(y*im.sizeX+x)*im.planes+1];
		}
	}

	if(filter<2)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, im.sizeX, im.sizeY, 0, format, GL_UNSIGNED_BYTE, im.data);
	}
	else
	{			// mipmap
		if(ext.SGIS_generate_mipmap)
		{
			glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, im.sizeX, im.sizeY, 0, format, GL_UNSIGNED_BYTE, im.data);
		}
		else
		gluBuild2DMipmaps(GL_TEXTURE_2D, internalformat, im.sizeX, im.sizeY, format, GL_UNSIGNED_BYTE, im.data);
	}

	if(name!=NULL)delete[]name;
	name = new char[strlen(filename)+1];
	strcpy(name,filename);	
	
	if(im.data) delete [] im.data;
	im.data=NULL;
	return 1;
}


int texture::MakeSphereMap(char *directory, char *extension, int filter, int compression)
{
	// right, left, top, bottom, back, front
	char *filename[6],*extension_lowercase;
	IMAGE i[6];
	int copyName=1;

	if(directory==NULL)
	{
		copyName=0;
		directory = name;
		extension = extens;
	}
	
	extension_lowercase = new char[strlen(extension)+1];
	if(extension_lowercase==NULL)return 0;
	strcpy(extension_lowercase,extension);
	strlwr(extension_lowercase);				// Convert a string to lowercase
	
	for(int j=0; j<6; j++)
	{
		filename[j] = new char[strlen(directory)+strlen(extension)+8];
		strcpy( filename[j], directory);
	}

	strcat( filename[0], "right.");
	strcat( filename[1], "left.");
	strcat( filename[2], "top.");
	strcat( filename[3], "bottom.");
	strcat( filename[4], "back.");
	strcat( filename[5], "front.");
	for( j=0; j<6; j++) strcat( filename[j], extension);

	for( j=0; j<6; j++)
	{
		if( !strcmp(extension_lowercase,"bmp"))		i[j] = LoadBMP( filename[j]);
		else if( !strcmp(extension_lowercase,"jpg"))	i[j] = LoadJPG( filename[j]);
		else if( !strcmp(extension_lowercase,"jpeg"))	i[j] = LoadJPG( filename[j]);
		else if( !strcmp(extension_lowercase,"tga"))	i[j] = LoadTGA( filename[j]);
	}

	if(i[3].data==NULL)			// ak sa nieje bottom, mozno bude down
	{
		strcpy( filename[3], directory);
		strcat( filename[3], "down.");
		strcat( filename[3], extension);
		if( !strcmp(extension_lowercase,"bmp"))		i[3] = LoadBMP( filename[3]);
		else if( !strcmp(extension_lowercase,"jpg"))	i[3] = LoadJPG( filename[3]);
		else if( !strcmp(extension_lowercase,"jpeg"))	i[3] = LoadJPG( filename[3]);
		else if( !strcmp(extension_lowercase,"tga"))	i[3] = LoadTGA( filename[3]);
	}
	delete [] extension_lowercase;
	int OK=1;
	for( j=0; j<6; j++)
	{
		if(i[j].data==NULL)
		{
			OK=0;
			MessageBox( hWnd, filename[j], "Nenájdený súbor", MB_OK );
		}
	}
	
	for( j=0; j<6; j++)delete [] filename[j];
	
	if(!OK)
	{
		for( j=0; j<6; j++)if(i[j].data!=NULL)delete[] i[j].data;
		error=1;
		return 0;
	}

	for( j=0; j<6; j++)
	{
		// uprava obrazku aby bol 2^x x 2^x
		if( !isPow2(i[j].sizeX) || !isPow2(i[j].sizeY) || i[j].sizeX!=i[j].sizeY )
		{
			int newx=1,newy=1;
			for( int newx2=i[j].sizeX; newx2 ;newx2>>=1)newx<<=1;
			for( int newy2=i[j].sizeY; newy2 ;newy2>>=1)newy<<=1;
			if( isPow2(i[j].sizeX) ) newx = i[j].sizeX;
			if( isPow2(i[j].sizeY) ) newy = i[j].sizeY;
			if(newx!=newy) newx<newy ? newx=newy : newy=newx;

			IMAGE out=resize( i[j], newx, newy);
			if( i[j].data!=NULL)delete [] i[j].data;
			if( out.data==NULL)return 0;
			i[j] = out;
		}
	}

	if(copyName)
	{
		if(extens!=NULL)delete[]extens;
		extens = new char[strlen(extension)+1];
		strcpy(extens,extension);
		if(name!=NULL)delete[]name;
		name = new char[strlen(directory)+1];
		strcpy(name,directory);
	}
	for( j=0; j<6; j++)if(i[j].isBGR)BGRtoRGB( i[j]);
	
	im.sizeX = 2*i[0].sizeX;
	im.sizeY = im.sizeX;
	im.isBGR = 0;
	im.planes = 3;
	im.data = new unsigned char[im.planes*im.sizeX*im.sizeY];
	if(im.data==NULL)
	{
		for( j=0; j<6; j++)if(i[j].data!=NULL)delete[] i[j].data;
		return 0;
	}
	
	for( j=0; j<im.planes*im.sizeX*im.sizeY; j++) im.data[j] = 0;
	
	for( int y=0; y<im.sizeY; y++)
	{
		float t = (float)y/(float)(im.sizeY-1);		// t is <0,1>
		t = 2.f*(t-0.5f);							// t is <-1,1>
		for(int x=0; x<im.sizeX; x++)
		{
			float s = (float)x/(float)(im.sizeX-1);	// s is <0,1>
			s = 2.f*(s-0.5f);						// s is <-1,1>
			if( s*s + t*t > 1.0f) continue;

			vec r;
			r.x = s;
			r.y = t;
			r.z = (float) sqrt( 1.0f - s*s - t*t);
			
			r.x *= 2*r.z;
			r.y *= 2*r.z;
			r.z  = 2*r.z*r.z - 1;

			vec ra(r);		// absolute value
			if(ra.x<0)ra.x=-ra.x;
			if(ra.y<0)ra.y=-ra.y;
			if(ra.z<0)ra.z=-ra.z;
/*
my_id my_name  major axis
               direction    target                             sc     tc    ma
----- -------  ----------   -------------------------------    ---    ---   ---
  0   right     +rx         TEXTURE_CUBE_MAP_POSITIVE_X_ARB    -rz    -ry   rx
  1   left      -rx         TEXTURE_CUBE_MAP_NEGATIVE_X_ARB    +rz    -ry   rx
  2   top       +ry         TEXTURE_CUBE_MAP_POSITIVE_Y_ARB    +rx    +rz   ry
  3   bottom    -ry         TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB    +rx    -rz   ry
  4   back      +rz         TEXTURE_CUBE_MAP_POSITIVE_Z_ARB    +rx    -ry   rz
  5   front     -rz         TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB    -rx    -ry   rz
  +rx,-ry,+rz,-rz must be Fliped and Mirrored
*/
			int tex;
			float sc,tc,ma;
			if( ra.x > ra.y)
			{
				// y isn't max
				if(ra.x > ra.z)
				{
					// x is max
					if(r.x>0)
					{
						tex = 0;	// right
						sc = -r.z;
						tc = -r.y;
						ma =  ra.x;
					}
					else 
					{
						tex = 1;		// left
						sc = +r.z;
						tc = -r.y;
						ma =  ra.x;
					}
				}
				else
				{
					// z is max
					if(r.z>0)
					{
						tex = 4;	// back
						sc =  r.x;
						tc = -r.y;
						ma =  ra.z;
					}
					else
					{
						tex = 5;		// front
						sc = -r.x;
						tc = -r.y;
						ma =  ra.z;
					}
				}
			}
			else
			{
				// x isn't max
				if(ra.y > ra.z)
				{
					// y is max
					if(r.y>0)
					{
						tex = 2;	// top
						sc =  r.x;
						tc =  r.z;
						ma =  ra.y;
					}
					else
					{
						tex = 3;		// bottom
						sc =  r.x;
						tc = -r.z;
						ma =  ra.y;
					}
				}
				else
				{
					// z is max
					if(r.z>0)
					{
						tex = 4;	// back
						sc =  r.x;
						tc = -r.y;
						ma =  ra.z;
					}
					else
					{
						tex = 5;		// front
						sc = -r.x;
						tc = -r.y;
						ma =  ra.z;
					}
				}
			}
			if(tex!=2 && tex!=3)	// Flip and Mirror for: right, left, top, bottom
			{
				sc = -sc;
				tc = -tc;
			}

			float sf = (sc/ma + 1)*0.5f;
			float tf = (tc/ma + 1)*0.5f;
			int si = (int)(sf*(float)i[tex].sizeX);	if(si>i[tex].sizeX-1)si-=i[tex].sizeX-1;
			int ti = (int)(tf*(float)i[tex].sizeY);	if(ti>i[tex].sizeY-1)ti-=i[tex].sizeY-1;
			im.data[ (y*im.sizeX+x)*im.planes + 0] = i[tex].data[ (ti*i[tex].sizeX+si)*im.planes + 0];
			im.data[ (y*im.sizeX+x)*im.planes + 1] = i[tex].data[ (ti*i[tex].sizeX+si)*im.planes + 1];
			im.data[ (y*im.sizeX+x)*im.planes + 2] = i[tex].data[ (ti*i[tex].sizeX+si)*im.planes + 2];
		}
	}
	for( j=0; j<6; j++)if(i[j].data!=NULL)delete[] i[j].data;
//	SaveBMP( im, "spheremap.bmp");

	if(!imageResizeOgl())return 0;
	if(!imageLoadToOgl( 1, filter, compression))return 0;

	if(im.data) delete [] im.data;
	im.data=NULL;
	return 1;
}
