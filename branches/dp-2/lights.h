//---------------------------------------------------------------------------

#ifndef LightsH
#define LightsH


#include "Usefull.h"

#include "Texture.h"
#include "Camera.h"


#define NUM_LIGHTS 5

#define TWO_PI	(2*M_PI)




typedef struct lightRec {
  float amb[4];
  float diff[4];
  float spec[4];
  float pos[4];
  float spotDir[3];
  float spotExp;
  float spotCutoff;
  float atten[3];

  float trans[3];
  float rot[3];
  float swing[3];
  float arc[3];
  float arcIncr[3];
} Light;




static Light spots[] =
{
  {
    {1.0, 0.3, 0.0, 1.0},  /* ambient */                  // RED        [0]
    {0.8, 0.0, 0.0, 1.0},  /* diffuse */
    {0.4, 0.0, 0.0, 1.0},  /* specular */
    {5.0, 2.0, 0.0, 1.0},  /* position */
    {9.0, 1.0, 13.0},   /* direction */
    {13.0},
    {20.0},             // PROMIEN STOZKA
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 110.0, 0.0, TWO_PI / 70.0}  /* arc increment */
  },
  {
    {0.5, 0.5, 0.6, 1.0},  /* ambient */                  // LAMP L     [1]
    {0.5, 0.8, 0.5, 1.0},  /* diffuse */
    {0.5, 0.5, 0.5, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {20.0},
    {13.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 120.0, 0.0, TWO_PI / 60.0}  /* arc increment */
  },
  {
    {0.5, 0.3, 0.1, 1.0},  /* ambient */                  // CAB LIGHT  [2]
    {0.5, 0.3, 0.1, 1.0},  /* diffuse */
    {1.0, 1.0, 0.9, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {30.0},
    {12.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 50.0, 0.0, TWO_PI / 100.0}  /* arc increment */
  },
  {
    {0.9, 0.5, 0.2, 1.0},  /* ambient */                  // LAMP R     [3]
    {0.9, 0.5, 0.2, 1.0},  /* diffuse */
    {0.9, 0.9, 0.9, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {7.0},
    {15.0},             /* exponent, cutoff */
    {0.6, 0.0, 0.0},    /* attenuation */
    {0.0, 0.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 50.0, 0.0, TWO_PI / 100.0}  /* arc increment */
  },
  {
    {0.5, 0.5, 0.6, 1.0},  /* ambient */                  // LAMP U     [4]
    {0.5, 0.5, 0.5, 1.0},  /* diffuse */
    {0.5, 0.5, 0.5, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {20.0},
    {13.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 120.0, 0.0, TWO_PI / 60.0}  /* arc increment */
  },
  {
    {0.5, 0.5, 0.6, 1.0},  /* ambient */                  // LAMP     [5]
    {0.5, 0.5, 0.5, 1.0},  /* diffuse */
    {0.5, 0.5, 0.5, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {20.0},
    {13.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 120.0, 0.0, TWO_PI / 60.0}  /* arc increment */
  },
  {
    {1.0, 1.0, 0.9, 1.0},  /* ambient */                  // LAMP LIGHT 3 [6]
    {1.0, 1.0, 0.9, 1.0},  /* diffuse */
    {1.0, 1.0, 0.9, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {20.0},
    {13.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 50.0, 0.0, TWO_PI / 100.0}  /* arc increment */
  },
  {
    {1.0, 1.0, 0.9, 1.0},  /* ambient */                  // LAMP LIGHT 3 [7]
    {1.0, 1.0, 0.9, 1.0},  /* diffuse */
    {1.0, 1.0, 0.9, 1.0},  /* specular */
    {0.0, 0.0, 0.0, 1.0},  /* position */
    {2.0, 0.0, 35.0},   /* direction */
    {20.0},
    {13.0},             /* exponent, cutoff */
    {1.0, 0.0, 0.0},    /* attenuation */
    {0.0, 1.25, 0.0},   /* translation */
    {0.0, 0.0, 0.0},    /* rotation */
    {20.0, 0.0, 40.0},  /* swing */
    {0.0, 0.0, 0.0},    /* arc */
    {TWO_PI / 50.0, 0.0, TWO_PI / 100.0}  /* arc increment */
  }

};






class TLights
{
public:

    __fastcall loadlights();
    __fastcall cablight_0();
    __fastcall InitLights();
    __fastcall AimLights();
    __fastcall DrawLights();
    __fastcall SetLights(float lx, float ly, float lz);
    __fastcall setcablights(AnsiString ltype, int cabnum);
    __fastcall setcablightA(int cabnum, vector3 pos, vector3 dir);
    __fastcall setcablightB(int cabnum, vector3 pos, vector3 dir);

    __fastcall setheadlightL(int cabnum);
    __fastcall setheadlightR(int cabnum);
    __fastcall setheadlightU(int cabnum);

    __fastcall setlightposA();
    
    __fastcall TLights();
    __fastcall ~TLights();


    static double ltxp;
    static double ltyp;
    static double ltzp;
    
    static double ltsdxp;
    static double ltsdyp;
    static double ltsdzp;

    static double sltxp;
    static double sltyp;
    static double sltzp;
    
    static double sltsdxp;
    static double sltsdyp;
    static double sltsdzp;

    static bool bcablight1;
    static bool bcablight2;
    static bool breflightL;
    static bool breflightR;
    static bool breflightU;

    static int useSAME_AMB_SPEC;
    static int currentcab;

    static float CAB1Ar;
    static float CAB1Ag;
    static float CAB1Ab;
    static float CAB1Dr;
    static float CAB1Dg;
    static float CAB1Db;
    static float CAB1Sr;
    static float CAB1Sg;
    static float CAB1Sb;

private:


};
//---------------------------------------------------------------------------
#endif
