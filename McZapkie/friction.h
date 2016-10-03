#pragma once
#ifndef INCLUDED_FRICTION_H
#define INCLUDED_FRICTION_H
           /*wspolczynnik tarcia roznych materialow*/

/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 - SPKS
    Friction coefficient.
    Copyright (C) 2007-2013 Maciej Cierniak
*/


/*
(C) youBy
Co brakuje:
- hamulce tarczowe
- kompozyty
*/
/*
Zrobione:
1) zadeklarowane niektore typy
2) wzor jubaja na tarcie wstawek Bg i Bgu z zeliwa P10
3) hamulec tarczowy marki 152A ;)
*/



//uses hamulce;
 
#include <SysUtils.hpp>	// Pascal unit
#include <mctools.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit



  
    
struct/*class*/ TFricMat: public TObject
{
public:
    virtual double GetFC(double N, double Vel); 
  
};


  
struct/*class*/ TP10Bg: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TP10Bgu: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TP10yBg: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TP10yBgu: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TP10: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TFR513: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TFR510: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TCosid: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TDisk1: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
struct/*class*/ TDisk2: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};





#endif//INCLUDED_FRICTION_H
//END
