//-----------------------------------------------------------------------------
// File: WavRead.h
//
// Desc: Support for loading and playing Wave files using DirectSound sound
//       buffers.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef WAVE_READ_H
#define WAVE_READ_H


#include <mmreg.h>
#include <mmsystem.h>


HRESULT WaveOpenFile( CHAR* strFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
                  MMCKINFO* pckInRIFF );
HRESULT WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn,
                           MMCKINFO* pckInRIFF );
HRESULT WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
                      MMCKINFO* pckIn, UINT* cbActualRead );

//-----------------------------------------------------------------------------
// Name: class CWaveSoundRead
// Desc: A class to read in sound data from a Wave file
//-----------------------------------------------------------------------------
class CWaveSoundRead
{
public:
    WAVEFORMATEX* m_pwfx;        // Pointer to WAVEFORMATEX structure
    HMMIO         m_hmmioIn;     // MM I/O handle for the WAVE
    MMCKINFO      m_ckIn;        // Multimedia RIFF chunk
    MMCKINFO      m_ckInRiff;    // Use in opening a WAVE file

public:
    CWaveSoundRead();
    ~CWaveSoundRead();

    HRESULT Open( CHAR* strFilename );
    HRESULT Reset();
    HRESULT Read( UINT nSizeToRead, BYTE* pbData, UINT* pnSizeRead );
    HRESULT Close();

};


#endif WAVE_READ_H



