/****************************************************************************
 *  @doc INTERNAL
 *  @module dlportio.h |
 *
 *  DriverLINX Port I/O Driver Interface
 *  <cp> Copyright 1996-1999 Scientific Software Tools, Inc.<nl>
 *  All Rights Reserved.<nl>
 *  DriverLINX is a registered trademark of Scientific Software Tools, Inc.
 *
 *  Win32 Prototypes for DriverLINX Port I/O
 *
 *  @comm   
 *  Author: RoyF<nl>
 *  Date:   09/26/96 14:08:58
 *
 *  @group Revision History
 *  @comm
 *  $Revision: 2 $
 *  <nl>
 *  $Log: /DLPortIO/API/DLPORTIO.H $
 * 
 * 2     3/03/99 5:25p Kevind
 * Removed any reference for customer to call us when encountering bugs,
 * also removed our old address info.
 * 
 * 1     9/27/96 2:03p Royf
 * Initial revision.
 *
 ****************************************************************************/

#ifndef DLPORTIO_H
  #define DLPORTIO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IN
  #define IN
#endif

#define DLPORT_API _stdcall

UCHAR DLPORT_API
DlPortReadPortUchar(
    IN ULONG Port
    );

USHORT DLPORT_API
DlPortReadPortUshort(
    IN ULONG Port
    );

ULONG DLPORT_API
DlPortReadPortUlong(
    IN ULONG Port
    );

VOID DLPORT_API
DlPortReadPortBufferUchar(
    IN ULONG Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

VOID DLPORT_API
DlPortReadPortBufferUshort(
    IN ULONG Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID DLPORT_API
DlPortReadPortBufferUlong(
    IN ULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

VOID DLPORT_API
DlPortWritePortUchar(
    IN ULONG Port,
    IN UCHAR Value
    );

VOID DLPORT_API
DlPortWritePortUshort(
    IN ULONG Port,
    IN USHORT Value
    );

VOID DLPORT_API
DlPortWritePortUlong(
    IN ULONG Port,
    IN ULONG Value
    );

VOID DLPORT_API
DlPortWritePortBufferUchar(
    IN ULONG Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

VOID DLPORT_API
DlPortWritePortBufferUshort(
    IN ULONG Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID DLPORT_API
DlPortWritePortBufferUlong(
    IN ULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

#ifdef __cplusplus
}
#endif

#endif // DLPORTIO_H
