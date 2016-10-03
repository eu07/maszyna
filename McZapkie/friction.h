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
 

class TFricMat
{
public:
    virtual double GetFC(double N, double Vel); 
  
};


  
class TP10Bg: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TP10Bgu: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TP10yBg: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TP10yBgu: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TP10: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TFR513: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TFR510: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TCosid: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TDisk1: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};


  
class TDisk2: public TFricMat
    
{
public:
    double GetFC(double N, double Vel)/*override*/; 
  
};





#endif//INCLUDED_FRICTION_H
//END
