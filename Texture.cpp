/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "Texture.h"

#include <ddraw.h>
#include <io.h>
#include <fcntl.h>
#include "opengl/glew.h"

#include "Globals.h"
#include "logs.h"
#include "Usefull.h"
#include "TextureDDS.h"

texture_manager TextureManager;

texture_manager::texture_manager() {

    // since index 0 is used to indicate no texture, we put a blank entry in the first texture slot
    m_textures.emplace_back( opengl_texture() );
}

// loads texture data from specified file
// TODO: wrap it in a workitem class, for the job system deferred loading
void
opengl_texture::load() {

    if( name.size() < 3 ) { goto fail; }

    WriteLog( "Loading texture data from \"" + name + "\"" );

    data_state = resource_state::loading;
    {
        std::string const extension = name.substr( name.size() - 3, 3 );

             if( extension == "dds" ) { load_DDS(); }
/*
        else if( extension == "tga" ) { load_TGA(); }
        else if( extension == "tex" ) { load_TEX(); }
        else if( extension == "bmp" ) { load_BMP(); }
*/
        else { goto fail; }
    }

    // data state will be set by called loader, so we're all done here
    if( data_state == resource_state::good ) {

        return;
    }

fail:
    data_state = resource_state::failed;
    ErrorLog( "Failed to load texture \"" + name + "\"" );
    return;
}

void
opengl_texture::load_DDS() {

    std::ifstream file( name, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
    std::size_t filesize = static_cast<size_t>(file.tellg());   // ios::ate already positioned us at the end of the file
    file.seekg( 0, std::ios::beg ); // rewind the caret afterwards

    char filecode[5];
    file.read(filecode, 4);
    filesize -= 4;
    filecode[4] = 0;

    if( filecode != std::string( "DDS " ) )
    {
        data_state = resource_state::failed;
        return;
    }

    DDSURFACEDESC2 ddsd;
    file.read((char *)&ddsd, sizeof(ddsd));
    filesize -= sizeof( ddsd );

    //
    // This .dds loader supports the loading of compressed formats DXT1, DXT3
    // and DXT5.
    //

    switch (ddsd.ddpfPixelFormat.dwFourCC)
    {
    case FOURCC_DXT1:
        // DXT1's compression ratio is 8:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;

    case FOURCC_DXT3:
        // DXT3's compression ratio is 4:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;

    case FOURCC_DXT5:
        // DXT5's compression ratio is 4:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;

    default:
        data_state = resource_state::failed;
        return;
    }

    data_width = ddsd.dwWidth;
    data_height = ddsd.dwHeight;
    data_mapcount = 1;// ddsd.dwMipMapCount;

    int blockSize = ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
    int offset = 0;

    while( ( data_width > Global::iMaxTextureSize ) || ( data_height > Global::iMaxTextureSize ) ) {
        // pomijanie zbyt dużych mipmap, jeśli wymagane jest ograniczenie rozmiaru
        offset += ( ( data_width + 3 ) / 4 ) * ( ( data_height + 3 ) / 4 ) * blockSize;
        data_width /= 2;
        data_height /= 2;
        --data_mapcount;
    };

    if( data_mapcount <= 0 ) {
        // there's a chance we've discarded the provided mipmap(s) as too large
        WriteLog( "Texture \"" + name + "\" has no mipmaps which can fit currently set texture size limits." );
        data_state = resource_state::failed;
        return;
    }

    int datasize = filesize - offset;
//    int datasize = ( ( data_width + 3 ) / 4 ) * ( ( data_height + 3 ) / 4 ) * blockSize;
/*
    // calculate size of accepted data
    // NOTE: this is a fallback, as we should be able to just move the file caret by calculated offset and read the rest
    int datasize = 0;
    int mapcount = data_mapcount,
        width = data_width,
        height = data_height;
    while( mapcount ) {

        datasize += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * blockSize;
        width = std::max( width / 2, 4 );
        height = std::max( height / 2, 4 );
        --mapcount;
    }
*/
    // reserve space and load texture data
    data.resize( datasize );
    if( offset != 0 ) {
        // skip data for mipmaps we don't need
        file.seekg( offset, std::ios_base::cur );
        filesize -= offset;
    }
    file.read((char *)&data[0], datasize);
    filesize -= datasize;

    data_components =
        ( ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1 ?
            GL_RGB :
            GL_RGBA );

    data_state = resource_state::good;

    return;
}

void
opengl_texture::create() {

    if( data_state != resource_state::good ) {
        // don't bother until we have useful texture data
        return;
    }

    glGenTextures( 1, &id );
    glBindTexture( GL_TEXTURE_2D, id );

    // TODO: set wrapping according to supplied parameters
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    set_filtering();

    if( GLEW_VERSION_1_4 ) {

        if( data_mapcount == 1 ) {
            // fill missing mipmaps if needed
            glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
        }
        // upload texture data
        // TBD, TODO: handle mipmaps other than base manually, or let the card take care of it?
        int dataoffset = 0,
            datasize = 0,
            datawidth = data_width,
            dataheight = data_height;
        for( int maplevel = 0; maplevel < data_mapcount; ++maplevel ) {

            if( ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
             || ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT )
             || ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ) ) {
                // compressed dds formats
                if( false == Global::bDecompressDDS ) {
                    // let the openGL handle this
                    int const datablocksize =
                        ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ?
                        8 :
                        16 );

                    datasize = ( ( std::max(datawidth, 4) + 3 ) / 4 ) * ( ( std::max(dataheight, 4) + 3 ) / 4 ) * datablocksize;

                    glCompressedTexImage2D(
                        GL_TEXTURE_2D, maplevel,
                        data_format, datawidth, dataheight, 0, datasize,
                        (GLubyte *)&data[0] + dataoffset );

                    dataoffset += datasize;
                    datawidth = std::max( datawidth / 2, 1 );
                    dataheight = std::max( dataheight / 2, 1 );
                }
            }
        }
    }

    is_ready = true;
    has_alpha = (
        data_components == GL_RGBA ?
        true :
        false );

    data.resize( 0 ); // TBD, TODO: keep the texture data if we start doing some gpu data cleaning down the road
    data_state = resource_state::none;
}

void
opengl_texture::set_filtering() {

    bool hash = ( name.find( '#' ) != std::string::npos );

    if( GLEW_VERSION_1_4 ) {

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        if( true == hash ) {
            // #: sharpen more
            glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -2.0 );
        }
        else {
            // regular texture sharpening
            glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1.0 );
        }
    }
}

void
texture_manager::Init() {
}

/*
TTexturesManager::Names::iterator TTexturesManager::LoadFromFile(std::string fileName, int filter)
{

    std::string message("Loading - texture: ");

    std::string realFileName(fileName);
    std::ifstream file(fileName.c_str());
    // Ra: niby bez tego jest lepiej, ale działa gorzej, więc przywrócone jest oryginalne
    if (!file.is_open())
        realFileName.insert(0, szTexturePath);
    else
        file.close();

    // char* cFileName = const_cast<char*>(fileName.c_str());

    message += realFileName;
    WriteLog(message.c_str()); // Ra: chybaa miało być z komunikatem z przodu, a nie tylko nazwa

    size_t pos = fileName.rfind('.');
    std::string ext(fileName, pos + 1, std::string::npos);

    AlphaValue texinfo;

    if( ext == "dds" )
        texinfo = LoadDDS( realFileName, filter );
    else if( ext == "tga" )
        texinfo = LoadTGA(realFileName, filter);
    else if (ext == "tex")
        texinfo = LoadTEX(realFileName);
    else if (ext == "bmp")
        texinfo = LoadBMP(realFileName);

    _alphas.insert(
        texinfo); // zapamiętanie stanu przezroczystości tekstury - można by tylko przezroczyste
    std::pair<Names::iterator, bool> ret = _names.insert(std::make_pair(fileName, texinfo.first));

    if (!texinfo.first)
    {
        WriteLog("Failed");
        ErrorLog("Missed texture: " + realFileName);
        return _names.end();
    };

    _alphas.insert(texinfo);
    ret = _names.insert(
        std::make_pair(fileName, texinfo.first)); // dodanie tekstury do magazynu (spisu nazw)

    // WriteLog("OK"); //Ra: "OK" nie potrzeba, samo "Failed" wystarczy
    return ret.first;
};
*/
/*
struct ReplaceSlash
{
    const char operator()(const char input)
    {
        return input == '/' ? '\\' : input;
    }
};
*/
// ustalenie numeru tekstury, wczytanie jeśli jeszcze takiej nie było
texture_manager::size_type
texture_manager::GetTextureId( std::string Filename, std::string const &Dir, int const Filter, bool const Loadnow ) {

    if( Filename.find( ':' ) != std::string::npos )
        Filename.erase( Filename.find( ':' ) ); // po dwukropku mogą być podane dodatkowe informacje niebędące nazwą tekstury
    if( Filename.find( '|' ) != std::string::npos )
    Filename.erase( Filename.find( '|' ) ); // po | może być nazwa kolejnej tekstury
    for( char &c : Filename ) {
        // change forward slashes to windows ones. NOTE: probably not strictly necessary, but eh
        c = ( c == '/' ? '\\' : c );
    }
    if( Filename.rfind('.')!= std::string::npos )
        Filename.erase( Filename.find( '.' ) ); // trim extension if there's one
/*
    std::transform(
        Filename.begin(), Filename.end(),
        Filename.begin(),
        []( char Char ){ return Char == '/' ? '\\' : Char; } );
*/
    if( Filename.find( '\\' ) == std::string::npos ) {
        // jeśli bieżaca ścieżka do tekstur nie została dodana to dodajemy domyślną
        Filename = szTexturePath + Filename;
    }

    std::vector<std::string> extensions{ { ".dds" }, { ".tga" }, { ".bmp" }, { ".ext" } };

    // try to locate requested texture in the databank
    auto lookup = find_in_databank( Filename + Global::szDefaultExt );
    if( lookup != npos ) {
        // start with the default extension...
        return lookup;
    }
    else {
        // ...then try recognized file extensions other than default
        for( auto const &extension : extensions ) {

            if( extension == Global::szDefaultExt ) {
                // we already tried this one
                continue;
            }
            lookup = find_in_databank( Filename + extension );
            if( lookup != npos ) {

                return lookup;
            }
        }
    }
    // if we don't have the texture in the databank, check if it's on disk
    std::string filename = find_on_disk( Filename + Global::szDefaultExt );
    if( true == filename.empty() ) {
        // if the default lookup fails, try other known extensions
        for( auto const &extension : extensions ) {

            if( extension == Global::szDefaultExt ) {
                // we already tried this one
                continue;
            }
            filename = find_on_disk( Filename + extension );
            if( false == filename.empty() ) {
                // we found something, don't bother with others
                break;
            }
        }
    }

    if( true == filename.empty() ) {
        // there's nothing matching in the databank nor on the disk, report failure
        return npos;
    }

    opengl_texture texture;
    texture.name = filename;
    texture.attributes = std::to_string( Filter ); // temporary. TODO, TBD: check how it's used and possibly get rid of it
    auto const textureindex = m_textures.size();
    m_textures.emplace_back( texture );
    m_texturemappings.emplace( filename, textureindex );

    WriteLog( "Created texture object for \"" + filename + "\"" );

    if( true == Loadnow ) {

        Texture( textureindex ).load();
        Texture( textureindex ).create();
    }

    return textureindex;
};

void
texture_manager::Bind( texture_manager::size_type const Id ) {

    // TODO: keep track of what's currently bound and don't do it twice
    // TODO: do binding in texture object, add support for other types
    if( Id != 0 ) {

        auto const &texture = Texture( Id );
        if( true == texture.is_ready ) {
            glBindTexture( GL_TEXTURE_2D, texture.id );
            return;
        }
    }

    glBindTexture( GL_TEXTURE_2D, 0 );
}
// checks whether specified texture is in the texture bank. returns texture id, or npos.
texture_manager::size_type
texture_manager::find_in_databank( std::string const &Texturename ) {

    auto lookup = m_texturemappings.find( Texturename );
    if( lookup != m_texturemappings.end() ) {
        return lookup->second;
    }
    // jeszcze próba z dodatkową ścieżką
    lookup = m_texturemappings.find( szTexturePath + Texturename );

    return (
        lookup != m_texturemappings.end() ?
            lookup->second :
            npos );
}

// checks whether specified file exists.
std::string
texture_manager::find_on_disk( std::string const &Texturename ) {

    {
        std::ifstream file( Texturename );
        if( true == file.is_open() ) {
            // success
            return Texturename;
        }

    }
    // if we fail make a last ditch attempt in the default textures directory
    {
        std::ifstream file( szTexturePath + Texturename );
        if( true == file.is_open() ) {
            // success
            return szTexturePath + Texturename;
        }

    }
    // no results either way, report failure
    return "";
}

/*
bool TTexturesManager::GetAlpha(GLuint id)
{ // atrybut przezroczystości dla tekstury o podanym numerze (id)
    Alphas::iterator iter = _alphas.find(id);
    return (iter != _alphas.end() ? iter->second : false);
}
*/
/*
TTexturesManager::AlphaValue TTexturesManager::LoadBMP(std::string const &fileName)
{

    AlphaValue fail(0, false);
    std::ifstream file(fileName, std::ios::binary);

    if (!file.is_open())
    {
        // file.close();
        return fail;
    };

    BITMAPFILEHEADER header;

    file.read((char *)&header, sizeof(BITMAPFILEHEADER));
    if (file.eof())
    {
        return fail;
    }

    // Read in bitmap information structure
    BITMAPINFO info;
    unsigned int infoSize = header.bfOffBits - sizeof(BITMAPFILEHEADER);
    if( infoSize > sizeof( info ) ) {
        WriteLog( "Warning - BMP header is larger than expected, possible format difference." );
    }
    file.read((char *)&info, std::min(infoSize, sizeof(info)));

    if (file.eof())
    {
        return fail;
    };

    GLuint width = info.bmiHeader.biWidth;
    GLuint height = info.bmiHeader.biHeight;
    bool hasalpha = ( info.bmiHeader.biBitCount == 32 );

    if( info.bmiHeader.biCompression != BI_RGB ) {
        ErrorLog( "Compressed BMP textures aren't supported." );
        return fail;
    }

    unsigned long bitSize = info.bmiHeader.biSizeImage;
    if (!bitSize)
        bitSize = (width * info.bmiHeader.biBitCount + 7) / 8 * height;

    std::shared_ptr<GLubyte> data( new GLubyte[ bitSize ], std::default_delete<GLubyte[]>() );
    file.read((char *)data.get(), bitSize);

    if (file.eof())
    {
        return fail;
    };

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    if( GLEW_VERSION_1_4 ) {
        glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
    }
    // This is specific to the binary format of the data read in.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

//    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data.get());
    glTexImage2D(
        GL_TEXTURE_2D, 
        0,
        GL_RGBA8,
        width,
        height,
        0,
        hasalpha ? GL_BGRA : GL_BGR,
        GL_UNSIGNED_BYTE,
        data.get() );

    return std::make_pair(id, hasalpha);
};

TTexturesManager::AlphaValue TTexturesManager::LoadTGA(std::string fileName, int filter)
{
    AlphaValue fail(0, false);
    int writeback = -1; //-1 plik jest OK, >=0 - od którego bajtu zapisać poprawiony plik
    GLubyte TGACompheader[] = {0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // uncompressed TGA header
    GLubyte TGAcompare[12]; // used to compare TGA header
    GLubyte header[6]; // first 6 useful bytes from the header
    std::fstream file(fileName.c_str(), std::ios::binary | std::ios::in);
    file.read((char *)TGAcompare, sizeof(TGAcompare));
    file.read((char *)header, sizeof(header));
    // std::cout << file.tellg() << std::endl;
    if (file.eof())
    {
        file.close();
        return fail;
    };
    bool compressed = (memcmp(TGACompheader, TGAcompare, sizeof(TGACompheader)) == 0);
    GLint width = header[1] * 256 + header[0]; // determine the TGA width (highbyte*256+lowbyte)
    GLint height = header[3] * 256 + header[2]; // determine the TGA height (highbyte*256+lowbyte)
    // check if width, height and bpp is correct
    if (!width || !height || (header[4] != 24 && header[4] != 32))
    {
        WriteLog("Bad texture: " + fileName + " has wrong header or bits per pixel");
        file.close();
        return fail;
    };
    { // sprawdzenie prawidłowości rozmiarów
        int i, j;
        for (i = width, j = 0; i; i >>= 1)
            if (i & 1)
                ++j;
        if (j == 1)
            for (i = height, j = 0; i; i >>= 1)
                if (i & 1)
                    ++j;
        if (j != 1)
            WriteLog("Bad texture: " + fileName + " is " + std::to_string(width) + "×" + std::to_string(height) );
    }
    GLuint bpp = header[4]; // grab the TGA's bits per pixel (24 or 32)
    GLuint bytesPerPixel = bpp / 8; // divide by 8 to get the bytes per pixel
    GLuint imageSize =
        width * height * bytesPerPixel; // calculate the memory required for the TGA data
    GLubyte *imageData = new GLubyte[imageSize]; // reserve memory to hold the TGA data
    if (!compressed)
    { // WriteLog("Not compressed.");
        file.read(reinterpret_cast<char*>(imageData), imageSize);
        if (file.eof())
        {
            delete[] imageData;
            file.close();
            return fail;
        };
    }
    else
    { // skompresowany plik TGA
        GLuint filesize; // current byte
        GLuint colorbuffer[1]; // Storage for 1 pixel
        file.seekg(0, std::ios::end); // na koniec
        filesize = (int)file.tellg() - 18; // rozmiar bez nagłówka
        file.seekg(18, std::ios::beg); // ponownie za nagłówkiem
        GLubyte *copyto = imageData; // gdzie wstawiać w buforze
        GLubyte *copyend = imageData + imageSize; // za ostatnim bajtem bufora
        GLubyte *copyfrom = imageData + imageSize - filesize; // gdzie jest początek
        int chunkheader = 0; // Ra: będziemy wczytywać najmłodszy bajt
        if (filesize < imageSize) // jeśli po kompresji jest mniejszy niż przed
        { // Ra: nowe wczytywanie skompresowanych: czytamy całe od razu, dekompresja w pamięci
            GLuint copybytes;
            file.read(reinterpret_cast<char*>(copyfrom), filesize); // wczytanie reszty po nagłówku
            // najpierw trzeba ustalić, ile skopiowanych pikseli jest na samym końcu
            copyto = copyfrom; // roboczo przelatujemy wczytane dane
            copybytes = 0; // licznik bajtów obrazka
            while (copybytes < imageSize)
            {
                chunkheader = (unsigned char)*copyto; // jeden bajt, pozostałe zawsze zerowe
                copyto += 1 + bytesPerPixel; // bajt licznika oraz jeden piksel jest zawsze
                copybytes += (1 + (chunkheader & 127)) * bytesPerPixel; // ilość pikseli
                if (chunkheader < 128) // jeśli kopiowanie, pikseli jest więcej
                    copyto += (chunkheader)*bytesPerPixel; // rozmiar kopiowanego obszaru (bez
                // jednego piksela)
            }
            if (copybytes > imageSize)
            { // nie ma prawa być większe
                WriteLog("Compression error");
                delete[] imageData;
                file.close();
                return fail;
            }
            // na końcu mogą być śmieci
            int extraend = copyend - copyto; // długość śmieci na końcu
            if (extraend > 0)
            { // przesuwamy bufor do końca obszaru dekompresji
                WriteLog("Extra bytes: " + std::to_string(extraend));
                memmove(copyfrom + extraend, copyfrom, filesize - extraend);
                copyfrom += extraend;
                file.close();
                filesize -= extraend; // to chyba nie ma znaczenia
                if (Global::iModifyTGA & 2) // flaga obcinania śmieci
                { // najlepiej by było obciąć plik, ale fstream tego nie potrafi
                    int handle;
					for( unsigned int i = 0; i < fileName.length(); ++i ) {
						if( fileName[ i ] == '/' ) { fileName[ i ] = '\\'; } // bo to Windows }
					}
                    WriteLog("Truncating extra bytes");
					// NOTE: this posix code is unsafe, and being deprecated in visual c
					// TODO: replace with something up to date
                    handle = open(fileName.c_str(), O_RDWR | O_BINARY);
                    chsize(handle, 18 + filesize); // obcięcie śmieci
                    close(handle);
                    extraend = 0; // skoro obcięty, to się już nie liczy
                }
                file.open(fileName.c_str(), std::ios::binary | std::ios::in);
            }
            if (chunkheader < 128) // jeśli ostatnie piksele są kopiowane
                copyend -= (1 + chunkheader) *
                           bytesPerPixel; // bajty kopiowane na końcu nie podlegające dekompresji
            else
                copyend -= bytesPerPixel; // ostatni piksel i tak się nie zmieni
            copyto = imageData; // teraz będzie wypełnianie od początku obszaru
            while (copyto < copyend)
            {
                chunkheader = (unsigned char)*copyfrom; // jeden bajt, pozostałe zawsze zerowe
                if (copyto > copyfrom)
                { // jeśli piksele mają być kopiowane, to możliwe jest przesunięcie ich o 1 bajt, na
                    // miejsce licznika
                    filesize = (imageData + imageSize - copyto) /
                               bytesPerPixel; // ile pikseli pozostało do końca
                    // WriteLog("Decompression buffer overflow at pixel
                    // "+AnsiString((copyto-imageData)/bytesPerPixel)+"+"+AnsiString(filesize));
                    // pozycję w pliku trzeba by zapamietać i po wczytaniu reszty pikseli starą
                    // metodą
                    // zapisać od niej dane od (copyto), poprzedzone bajtem o wartości (filesize-1)
                    writeback = imageData + imageSize + extraend -
                                copyfrom; // ile bajtów skompresowanych zostało do końca
                    copyfrom = copyto; // adres piksela do zapisania
                    file.seekg(-writeback, std::ios::end); // odległość od końca (ujemna)
                    if ((filesize > 128) ||
                        !(Global::iModifyTGA & 4)) // gdy za dużo pikseli albo wyłączone
                        writeback = -1; // zapis możliwe jeśli ilość problematycznych pikseli nie
                    // przekaracza 128
                    break; // bufor się zatkał, dalej w ten sposób się nie da
                }
                if (chunkheader < 128)
                { // dla nagłówka < 128 mamy podane ile pikseli przekopiować minus 1
                    copybytes = (++chunkheader) * bytesPerPixel; // rozmiar kopiowanego obszaru
                    memcpy(copyto, ++copyfrom, copybytes); // skopiowanie tylu bajtów
                    copyto += copybytes;
                    copyfrom += copybytes;
                }
                else
                { // chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
                    chunkheader -= 127;
                    // copy the color into the image data as many times as dictated
                    if (bytesPerPixel == 4)
                    { // przy czterech bajtach powinno być szybsze używanie int
                        __int32 *ptr = (__int32 *)(copyto); // wskaźnik na int
                        __int32 bgra = *((__int32 *)++copyfrom); // kolor wypełniający (4 bajty)
                        for (int counter = 0; counter < chunkheader; counter++)
                            *ptr++ = bgra;
                        copyto = reinterpret_cast<GLubyte *>(ptr); // rzutowanie, żeby nie dodawać
                        copyfrom += 4;
                    }
                    else
                    {
                        colorbuffer[0] = *((int *)(++copyfrom)); // pobranie koloru (3 bajty)
                        for (int counter = 0; counter < chunkheader; counter++)
                        { // by the header
                            memcpy(copyto, colorbuffer, 3);
                            copyto += 3;
                        }
                        copyfrom += 3;
                    }
                }
            } // while (copyto<copyend)
        }
        else
        {
            WriteLog("Compressed file is larger than uncompressed!");
            if (Global::iModifyTGA & 1)
                writeback = 0; // no zapisać ten krótszy zaczynajac od początku...
        }
        // if (copyto<copyend) WriteLog("Slow loader...");
        while (copyto < copyend)
        { // Ra: stare wczytywanie skompresowanych, z nadużywaniem file.read()
            // również wykonywane, jeśli dekompresja w buforze przekroczy jego rozmiar
            file.read((char *)&chunkheader, 1); // jeden bajt, pozostałe zawsze zerowe
            if (file.eof())
            {
                MessageBox(NULL, "Could not read RLE header", "ERROR", MB_OK); // display error
                delete[] imageData;
                file.close();
                return fail;
            };
            if (chunkheader < 128)
            { // if the header is < 128, it means the that is the number of RAW color packets minus
                // 1
                chunkheader++; // add 1 to get number of following color values
                file.read(reinterpret_cast<char*>(copyto), chunkheader * bytesPerPixel);
                copyto += chunkheader * bytesPerPixel;
            }
            else
            { // chunkheader>128 RLE data, next color reapeated (chunkheader-127) times
                chunkheader -= 127;
                file.read((char *)colorbuffer, bytesPerPixel);
                // copy the color into the image data as many times as dictated
                if (bytesPerPixel == 4)
                { // przy czterech bajtach powinno być szybsze używanie int
                    __int32 *ptr = (__int32 *)(copyto), bgra = *((__int32 *)colorbuffer);
                    for (int counter = 0; counter < chunkheader; counter++)
                        *ptr++ = bgra;
                    copyto = reinterpret_cast<GLubyte*>(ptr); // rzutowanie, żeby nie dodawać
                }
                else
                    for (int counter = 0; counter < chunkheader; counter++)
                    { // by the header
                        memcpy(copyto, colorbuffer, bytesPerPixel);
                        copyto += bytesPerPixel;
                    }
            }
        } // while (copyto<copyend)
        if (writeback >= 0)
        { // zapisanie pliku
            file.close(); // tamten zamykamy, bo był tylko do odczytu
            if (writeback)
            { // zapisanie samej końcówki pliku, która utrudnia dekompresję w buforze
                WriteLog("Rewriting end of file...");
                chunkheader = filesize - 1; // licznik jest o 1 mniejszy
                file.open(fileName.c_str(), std::ios::binary | std::ios::out | std::ios::in);
                file.seekg(-writeback, std::ios::end); // odległość od końca (ujemna)
                file.write((char *)&chunkheader, 1); // zapisanie licznika
                file.write(reinterpret_cast<char*>(copyfrom), filesize * bytesPerPixel); // piksele bez kompresji
            }
            else
            { // zapisywanie całości pliku, będzie krótszy, więc trzeba usunąć go w całości
                WriteLog("Writing uncompressed file...");
                TGAcompare[2] = 2; // bez kompresji
                file.open(fileName.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
                file.write((char *)TGAcompare, sizeof(TGAcompare));
                file.write((char *)header, sizeof(header));
                file.write(reinterpret_cast<char*>(imageData), imageSize);
            }
        }
    };
    file.close(); // plik zamykamy dopiero na samym końcu
    bool alpha = (bpp == 32);
    bool hash = (fileName.find('#') != std::string::npos); // true gdy w nazwie jest "#"
    bool dollar = (fileName.find('$') == std::string::npos); // true gdy w nazwie nie ma "$"
    size_t pos = fileName.rfind('%'); // ostatni % w nazwie
    if (pos != std::string::npos)
        if (pos < fileName.size())
        {
            filter = (int)fileName[pos + 1] - '0'; // zamiana cyfry za % na liczbę
            if ((filter < 0) || (filter > 10))
                filter = -1; // jeśli nie jest cyfrą
        }
    if (!alpha && !hash && dollar && (filter < 0))
        filter = Global::iDefaultFiltering; // dotyczy tekstur TGA bez kanału alfa
    // ewentualne przeskalowanie tekstury do dopuszczalnego rozumiaru
    GLint w = width, h = height;
    if (width > Global::iMaxTextureSize)
        width = Global::iMaxTextureSize; // ogranizczenie wielkości
    if (height > Global::iMaxTextureSize)
        height = Global::iMaxTextureSize;
    if ((w != width) || (h != height))
    { // przeskalowanie tekstury, żeby się nie wyświetlała jako biała
        GLubyte *imgData = new GLubyte[width * height * bytesPerPixel]; // nowy rozmiar
        gluScaleImage(bytesPerPixel == 3 ? GL_RGB : GL_RGBA, w, h, GL_UNSIGNED_BYTE, imageData,
                      width, height, GL_UNSIGNED_BYTE, imgData);
        delete[] imageData; // usunięcie starego
        imageData = imgData;
    }
    GLuint id = CreateTexture(imageData, (alpha ? GL_BGRA : GL_BGR), width, height, alpha, hash,
                              dollar, filter);
    delete[] imageData;
    ++Global::iTextures;
    return std::make_pair(id, alpha);
};

TTexturesManager::AlphaValue TTexturesManager::LoadTEX(std::string fileName)
{

    AlphaValue fail(0, false);

    std::ifstream file(fileName.c_str(), std::ios::binary);

    char head[5];
    file.read(head, 4);
    head[4] = 0;

    bool alpha;

    if (std::string("RGB ") == head)
    {
        alpha = false;
    }
    else if (std::string("RGBA") == head)
    {
        alpha = true;
    }
    else
    {
        std::string message("Unrecognized texture format: ");
        message += head;
        Error(message.c_str());
        return fail;
    };

    GLuint width;
    GLuint height;

    file.read((char *)&width, sizeof(int));
    file.read((char *)&height, sizeof(int));

    GLuint bpp = alpha ? 4 : 3;
    GLuint size = width * height * bpp;

    GLubyte *data = new GLubyte[size];
    file.read(reinterpret_cast<char*>(data), size);

    bool hash = (fileName.find('#') != std::string::npos);

    GLuint id = CreateTexture(data, (alpha ? GL_RGBA : GL_RGB), width, height, alpha, hash);
    delete[] data;

    return std::make_pair(id, alpha);
};

TTexturesManager::AlphaValue TTexturesManager::LoadDDS(std::string fileName, int filter)
{

    AlphaValue fail(0, false);

    std::ifstream file(fileName.c_str(), std::ios::binary);

    char filecode[5];
    file.read(filecode, 4);
    filecode[4] = 0;

    if (std::string("DDS ") != filecode)
    {
        file.close();
        return fail;
    };

    DDSURFACEDESC2 ddsd;
    file.read((char *)&ddsd, sizeof(ddsd));

    DDS_IMAGE_DATA data;

    //
    // This .dds loader supports the loading of compressed formats DXT1, DXT3
    // and DXT5.
    //

    GLuint factor;

    switch (ddsd.ddpfPixelFormat.dwFourCC)
    {
    case FOURCC_DXT1:
        // DXT1's compression ratio is 8:1
        data.format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        factor = 2;
        break;

    case FOURCC_DXT3:
        // DXT3's compression ratio is 4:1
        data.format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        factor = 4;
        break;

    case FOURCC_DXT5:
        // DXT5's compression ratio is 4:1
        data.format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        factor = 4;
        break;

    default:
        file.close();
        return fail;
    }

    GLuint bufferSize = (ddsd.dwMipMapCount > 1 ? ddsd.dwLinearSize * factor : ddsd.dwLinearSize);

    data.pixels = new GLubyte[bufferSize];
    file.read((char *)data.pixels, bufferSize);

    file.close();

    data.width = ddsd.dwWidth;
    data.height = ddsd.dwHeight;
    data.numMipMaps = ddsd.dwMipMapCount;
    { // sprawdzenie prawidłowości rozmiarów
        int i, j;
        for (i = data.width, j = 0; i; i >>= 1)
            if (i & 1)
                ++j;
        if (j == 1)
            for (i = data.height, j = 0; i; i >>= 1)
                if (i & 1)
                    ++j;
        if (j != 1)
            WriteLog( "Bad texture: " + fileName + " is " + std::to_string(data.width) + "×" + std::to_string(data.height) );
    }

    if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1)
        data.components = 3;
    else
        data.components = 4;

    data.blockSize = (data.format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    if (filter >= 0)
        SetFiltering(filter); // cyfra po % w nazwie
    else
        // SetFiltering(bHasAlpha&&bDollar,bHash); //znaki #, $ i kanał alfa w nazwie
        SetFiltering(data.components == 4, fileName.find('#') != std::string::npos);

    GLuint offset = 0;
    int firstMipMap = 0;

    while ((data.width > Global::iMaxTextureSize) || (data.height > Global::iMaxTextureSize))
    { // pomijanie zbyt dużych mipmap, jeśli wymagane jest ograniczenie rozmiaru
        offset += ((data.width + 3) / 4) * ((data.height + 3) / 4) * data.blockSize;
        data.width /= 2;
        data.height /= 2;
        firstMipMap++;
    };

    for (int i = 0; i < data.numMipMaps - firstMipMap; i++)
    { // wczytanie kolejnych poziomów mipmap
        if (!data.width)
            data.width = 1;
        if (!data.height)
            data.height = 1;
        GLuint size = ((data.width + 3) / 4) * ((data.height + 3) / 4) * data.blockSize;
        if (Global::bDecompressDDS)
        { // programowa dekompresja DDS
            // if (i==1) //should be i==0 but then problem with "glBindTexture()"
            {
                GLuint decomp_size = data.width * data.height * 4;
                GLubyte *output = new GLubyte[decomp_size];
                DecompressDXT(data, data.pixels + offset, output);
                glTexImage2D( GL_TEXTURE_2D, i, GL_RGBA8, data.width, data.height, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, output);
                delete[] output;
            }
        }
        else // przetwarzanie DDS przez OpenGL (istnieje odpowiednie rozszerzenie)
            glCompressedTexImage2D(GL_TEXTURE_2D, i, data.format, data.width, data.height, 0, size,
                                   data.pixels + offset);
        offset += size;
        // Half the image size for the next mip-map level...
        data.width /= 2;
        data.height /= 2;
    };

    if( ( data.numMipMaps == 1 )
     && ( GLEW_VERSION_1_4 ) ) {
        // generate missing mipmaps for the updated render path
        // TODO, TBD: skip this for UI images
        glGenerateMipmap( GL_TEXTURE_2D );
        WriteLog( "Warning - generating missing mipmaps for " + fileName );
    }

    delete[] data.pixels;
    return std::make_pair(id, data.components == 4);
};

*/
/*
void TTexturesManager::SetFiltering(int filter)
{
    if (filter < 4) // rozmycie przy powiększeniu
    { // brak rozmycia z bliska - tych jest 4: 0..3, aby nie było przeskoku
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        filter += 4;
    }
    else
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    switch (filter) // rozmycie przy oddaleniu
    {
    case 4: // najbliższy z tekstury
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case 5: //średnia z tekstury
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case 6: // najbliższy z mipmapy
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    case 7: //średnia z mipmapy
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    case 8: // najbliższy z dwóch mipmap
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        break;
    case 9: //średnia z dwóch mipmap
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }
};

void TTexturesManager::SetFiltering(bool alpha, bool hash)
{

    if( GLEW_VERSION_1_4 ) {

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        if( true == hash ) {
            // #: sharpen more
            glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -2.0 );
        }
        else {
            // regular texture sharpening
            glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1.0 );
        }
    }
    else {
        // legacy version, for ancient systems
        if( alpha || hash ) {
            if( alpha ) // przezroczystosc: nie wlaczac mipmapingu
            {
                if( hash ) // #: calkowity brak filtracji - pikseloza
                {
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
                }
                else {
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                }
            }
            else // filtruj ale bez dalekich mipmap - robi artefakty
            {
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            }
        }
        else // $: filtruj wszystko - brzydko się zlewa
        {
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
GLuint TTexturesManager::CreateTexture(GLubyte *buff, GLint bpp, int width, int height, bool bHasAlpha,
                                       bool bHash, bool bDollar, int filter)
{ // Ra: używane tylko dla TGA i TEX
    // Ra: dodana obsługa GL_BGR oraz GL_BGRA dla TGA - szybciej się wczytuje
    GLuint ID;
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (filter >= 0)
        SetFiltering(filter); // cyfra po % w nazwie
    else
        SetFiltering(bHasAlpha && bDollar, bHash); // znaki #, $ i kanał alfa w nazwie

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

    if( GLEW_VERSION_1_4 ) {

        glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, bpp, GL_UNSIGNED_BYTE, buff );
    }
    else {
        // legacy version, for ancient systems
        if( bHasAlpha || bHash || ( filter == 0 ) )
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, bpp, GL_UNSIGNED_BYTE, buff );
        else
            gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, width, height, bpp, GL_UNSIGNED_BYTE, buff );
    }

    return ID;
}
*/
void
texture_manager::Free()
{ 
    for( auto const &texture : m_textures ) {
        // usunięcie wszyskich tekstur (bez usuwania struktury)
        glDeleteTextures( 1, &texture.id );
    }
}
/*
std::string TTexturesManager::GetName(GLuint id)
{ // pobranie nazwy tekstury
	for( auto const &pair : _names ) {
		if( pair.second == id ) { return pair.first; }
	}
    return "";
};
*/