//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Model3d.h"
#include "Usefull.h"
#include "Texture.h"
#include "logs.h"
#include "Globals.h"
#include "Timer.h"

double fSquareDist= 0;

__fastcall TSubModel::TSubModel()
{
    FirstInit();
    eType= smt_Unknown;
    uiDisplayList= 0;
};

__fastcall TSubModel::FirstInit()
{
    Index= -1;
    v_RotateAxis= vector3(0,0,0);
    v_TransVector= vector3(0,0,0);
    v_aRotateAxis= vector3(0,0,0);
    v_aTransVector= vector3(0,0,0);
    f_Angle= 0;
    f_aAngle= 0;
    v_DesiredTransVector= vector3(0,0,0);
    f_TranslateSpeed= 0;
    f_DesiredAngle= 0;
    f_RotateSpeed= 0;
    b_Anim= at_None;
    v_aDesiredTransVector= vector3(0,0,0);
    f_aTranslateSpeed= 0;
    f_aDesiredAngle= 0;
    f_aRotateSpeed= 0;
    b_aAnim= at_None;
    Visible= false;
    Matrix.Identity();
//    Transform.Identity();
    Next= NULL;
    Child= NULL;
    TextureID= 0;
    TexAlpha= false;
//    TexHash= false;
//    Hits= NULL;
  //  CollisionPts= NULL;
//    CollisionPtsCount= 0;

    Transparency= 0;

    bWire= false;
    fWireSize= 0;

    fNearAttenStart= 40;
    fNearAttenEnd= 80;
    bUseNearAtten= false;
    iFarAttenDecay= 0;
    fFarDecayRadius= 100;
    fcosFalloffAngle= 0.5;
    fcosHotspotAngle= 0.3;
    fCosViewAngle= 0;

    fSquareMaxDist= 10000;
    fSquareMinDist= 0;
};

//#define SafeDelete(x) if (x!=NULL) delete (x)

__fastcall TSubModel::~TSubModel()
{
    glDeleteLists(uiDisplayList,1);
//    SafeDeleteArray(Indices);
    SafeDelete(Next);
    SafeDelete(Child);
};

int __fastcall TSubModel::SeekFaceNormal(DWORD *Masks, int f, DWORD dwMask, vector3 pt, GLVERTEX *Vertices, int iNumVerts)
{
    int iNumFaces= iNumVerts/3;

    for (int i=f; i<iNumFaces; i++)
    {
        if ( ( (Masks[i] & dwMask) != 0 ) && ( (Vertices[i*3+0].Point==pt) ||
                                                (Vertices[i*3+1].Point==pt) ||
                                                (Vertices[i*3+2].Point==pt) ) )
            return i;

    }
    return -1;
}

float emm1[]= { 1,1,1,0 };
float emm2[]= { 0,0,0,0 };

inline double readIntAsDouble(cParser& parser, int base = 255)
{
    int value;
    parser.getToken(value);
    return double(value) / base;
};

template <typename ColorT>
inline void readColor(cParser& parser, ColorT& color)
{
    color[0] = readIntAsDouble(parser);
    color[1] = readIntAsDouble(parser);
    color[2] = readIntAsDouble(parser);
    parser.ignoreToken();
};

inline void readMatrix(cParser& parser, matrix4x4& matrix)
{
    for(int x = 0; x <= 3; x++)
        for(int y = 0; y <= 3; y++)
            parser.getToken(matrix(x)[y]);
};

void __fastcall TSubModel::Load(cParser& parser, int NIndex, TModel3d *Model)
{
    GLVERTEX *Vertices= NULL;
    int iNumVerts= -1;
    bool bLight;
//    TMaterialColorf Ambient,Diffuse,Specular;
    float Ambient[] = { 1,1,1,1 };
    float Diffuse[] = { 1,1,1,1 };
    float Specular[] = { 0,0,0,1 };
//    GLuint TextureID;
//    char *extName;

    if(!parser.expectToken("type:"))
        Error("Model type parse failure!");

    {
        std::string type;
        parser.getToken(type);
        
        if(type == "mesh")
            eType = smt_Mesh;
        else if(type == "point")
            eType = smt_Point;
        else if(type == "freespotlight")
            eType = smt_FreeSpotLight;
    };

    parser.ignoreToken(); 
    parser.getToken(Name);

    if(parser.expectToken("anim:"))
        parser.ignoreTokens(2);

    if(eType == smt_Mesh) readColor(parser, Ambient);
    readColor(parser, Diffuse);
    if(eType == smt_Mesh) readColor(parser, Specular);

    bLight = parser.expectToken("true");

    if(eType == smt_FreeSpotLight)
    {

        if(!parser.expectToken("nearattenstart:"))
            Error("Model light parse failure!");

        parser.getToken(fNearAttenStart);

        parser.ignoreToken();
        parser.getToken(fNearAttenEnd);

        parser.ignoreToken();
        bUseNearAtten = parser.expectToken("true");

        parser.ignoreToken();
        parser.getToken(iFarAttenDecay);

        parser.ignoreToken();
        parser.getToken(fFarDecayRadius);

        parser.ignoreToken();
        parser.getToken(fcosFalloffAngle);
        fcosFalloffAngle = cos(fcosFalloffAngle * M_PI / 90);

        parser.ignoreToken();
        parser.getToken(fcosHotspotAngle); 
        fcosHotspotAngle = cos(fcosHotspotAngle * M_PI / 90);

    };

    if (eType == smt_Mesh)
    {

        parser.ignoreToken();
        bWire = parser.expectToken("true");

        parser.ignoreToken();
        parser.getToken(fWireSize);

        parser.ignoreToken();
        Transparency = readIntAsDouble(parser, 100.0f);

        if(!parser.expectToken("map:"))
            Error("Model map parse failure!");

        std::string texture;
        parser.getToken(texture);

        if(texture == "none")
        {
            TextureID= 0;
        }
        else
        {
// McZapkie-060702: zmienialne skory modelu
            if (texture.find("replacableskin") != texture.npos)
            {
                TextureID= -1;
            }
            else
            {
                //jesli tylko nazwa pliku to dawac biezaca sciezke do tekstur
                if(texture.find_first_of("/\\") == texture.npos)
                    texture.insert(0, Global::asCurrentTexturePath.c_str());

                TextureID= TTexturesManager::GetTextureID(texture);
                TexAlpha= TTexturesManager::GetAlpha(TextureID);
            };
        };
    };

    parser.ignoreToken();
    parser.getToken(fSquareMaxDist);
    fSquareMaxDist *= fSquareMaxDist;

    parser.ignoreToken();
    parser.getToken(fSquareMinDist);
    fSquareMinDist*= fSquareMinDist;

    parser.ignoreToken();
    readMatrix(parser, Matrix);

    int iNumFaces;
    DWORD *sg;

    if(eType == smt_Mesh)
    {
        parser.ignoreToken();
        parser.getToken(iNumVerts);

        if(iNumVerts % 3)
        {
            iNumVerts= 0;
            Error("Mesh error, iNumVertices%3!=0");
            return;
        }

        Vertices= new GLVERTEX[iNumVerts];
        iNumFaces= iNumVerts/3;
        sg = new DWORD[iNumFaces];

        for(int i=0; i<iNumVerts; i++)
        {

            if(i % 3 == 0)
                parser.getToken(sg[i/3]);

            parser.getToken(Vertices[i].Point.x);
            parser.getToken(Vertices[i].Point.y);
            parser.getToken(Vertices[i].Point.z);

            Vertices[i].Normal= vector3(0,0,0);

            parser.getToken(Vertices[i].tu);
            parser.getToken(Vertices[i].tv);

            if(i%3 == 2 && (Vertices[i].Point == Vertices[i-1].Point ||
                            Vertices[i-1].Point == Vertices[i-2].Point ||
                            Vertices[i-2].Point == Vertices[i].Point))
                WriteLog("Degenerated triangle found");
        }

        int v=0;
        int f=0;
        int j;

        vector3 norm;

        for (int i=0; i<iNumFaces; i++)
        {
            for (j=0; j<3; j++)
            {
                norm= vector3(0,0,0);

                f= SeekFaceNormal(sg,0,sg[i],Vertices[v].Point,Vertices,iNumVerts);
                norm= vector3(0,0,0);
                while (f>=0)
                {
                    norm += SafeNormalize(CrossProduct( Vertices[f*3].Point-Vertices[f*3+1].Point,
                                               Vertices[f*3].Point-Vertices[f*3+2].Point ));
                    f= SeekFaceNormal(sg,f+1,sg[i],Vertices[v].Point,Vertices,iNumVerts);
                }

                if (norm.Length()==0)
                    norm += SafeNormalize(CrossProduct( Vertices[i*3].Point-Vertices[i*3+1].Point,
                                               Vertices[i*3].Point-Vertices[i*3+2].Point ));

                if (norm.Length()>0)
                    Vertices[v].Normal= Normalize(norm);
                else
                    f=0;
                v++;
            }
        }

        delete[] sg;
    };

    int d;

    if(eType==smt_Mesh)
    {

        // ShaXbee-121209: przekazywanie wierzcholkow hurtem
        glVertexPointer(3, GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Point.x);
        glNormalPointer(GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Normal.x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(GLVERTEX), &Vertices[0].tu);

        uiDisplayList= glGenLists(1);
        glNewList(uiDisplayList,GL_COMPILE);

        glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);   //McZapkie-240702: zamiast ub
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,Diffuse);

        if (bLight)
            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Diffuse);  //zeny swiecilo na kolorowo

        glDrawArrays(GL_TRIANGLES, 0, iNumVerts);

        if (bLight)
            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emm2);

        glEndList();
    }
    else
    {
        uiDisplayList= glGenLists(1);
        glNewList(uiDisplayList,GL_COMPILE);

        glBindTexture(GL_TEXTURE_2D, 0);
//        if (eType==smt_FreeSpotLight)
//         {
//          if (iFarAttenDecay==0)
//            glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
//         }
//         else
//TODO: poprawic zeby dzialalo
         glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);
        glColorMaterial(GL_FRONT_AND_BACK,GL_EMISSION);
        glDisable( GL_LIGHTING );  //Tolaris-030603: bo mu punkty swiecace sie blendowaly
        glBegin(GL_POINTS);
            glVertex3f(0,0,0);
        glEnd();
        glEnable( GL_LIGHTING );
        glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
        glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emm2);

        glEndList();

    }

    SafeDeleteArray(Vertices);
    Visible= true;
};

void __fastcall TSubModel::AddChild(TSubModel *SubModel)
{
    if (Child==NULL)
        Child= SubModel;
    else
        Child->AddNext(SubModel);
};

void __fastcall TSubModel::AddNext(TSubModel *SubModel)
{
    if (Next==NULL)
        Next= SubModel;
    else
        Next->AddNext(SubModel);
};
void __fastcall TSubModel::SetRotate(vector3 vNewRotateAxis, double fNewAngle)
{
    f_RotateSpeed= 0;
    v_RotateAxis= vNewRotateAxis;
    f_Angle= fNewAngle;
    b_Anim= at_Rotate;
    f_aRotateSpeed= 0;
    v_aRotateAxis= vNewRotateAxis;
    f_aAngle= fNewAngle;
    b_aAnim= at_Rotate;
//    bAnim= true;
}

void __fastcall TSubModel::SetRotateXYZ(vector3 vNewAngles)
{
    f_RotateSpeed= 0;
    v_Angles= vNewAngles;
    b_Anim= at_RotateXYZ;
    f_aRotateSpeed= 0;
    v_aAngles= vNewAngles;
    b_aAnim= at_RotateXYZ;
//    bAnim= true;
}

void __fastcall TSubModel::SetTranslate(vector3 vNewTransVector)
{
    f_TranslateSpeed= 0;
    v_TransVector= vNewTransVector;
    b_Anim= true;
    f_aTranslateSpeed= 0;
    v_aTransVector= vNewTransVector;
    b_aAnim= true;
}
/*
void __fastcall TSubModel::SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double fNewRotateSpeed, bool bResetAngle)
{
    fRotateSpeed= fNewRotateSpeed;
    vRotateAxis= Normalize(vNewRotateAxis);
    fDesiredAngle= fNewDesiredAngle;
    if (bResetAngle)
        fAngle= 0;
}


  */

struct ToLower
{
    char operator()(char input) { return tolower(input); }
};

TSubModel* __fastcall TSubModel::GetFromName(std::string search)
{

    TSubModel* result = NULL;

    std::transform(search.begin(), search.end(), search.begin(), ToLower());

    if(search == Name)
        return this;

    if(Next)
    {
        result = Next->GetFromName(search);
        if(result)
            return result;
    };

    if(Child)
    {
        result = Child->GetFromName(search);
        if(result)
            return result;
    };

    return NULL;
    
};

WORD hbIndices[18]= {3,0,1,5,4,2,1,0,4,1,5,3,2,3,5,2,4,0};

void __fastcall TSubModel::Render(GLuint ReplacableSkinId)
{
    float Distdimm=0;
    if (Next!=NULL)
        Next->Render(ReplacableSkinId);

    if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
    {
      glPushMatrix();
      glMultMatrixd(Matrix.getArray());

      if (b_Anim==at_Rotate)   //czy to potrzebne tu czy moze nizej?
      {
          glRotatef(f_Angle,v_RotateAxis.x,v_RotateAxis.y,v_RotateAxis.z);
          glTranslatef(v_TransVector.x,v_TransVector.y,v_TransVector.z);

  //        vRotateAxis= vector3(0,0,0);
    //      vTransVector= vector3(0,0,0);
          f_Angle= 0;
          b_Anim= at_None;
  //        bAnim= false;
      }
      else
      if (b_Anim==at_RotateXYZ)
      {
          glTranslatef(v_TransVector.x,v_TransVector.y,v_TransVector.z);
          glRotatef(v_Angles.y,0.0,1.0,0.0);
          glRotatef(v_Angles.x,1.0,0.0,0.0);
          glRotatef(v_Angles.z,0.0,0.0,1.0);
          v_Angles.x=v_Angles.y=v_Angles.z= 0;
          b_Anim= at_None;
      }

      //zmienialne skory
      if ((TextureID==-1)) // && (ReplacableSkinId!=0))
       {
        glBindTexture(GL_TEXTURE_2D, ReplacableSkinId);
        if (ReplacableSkinId>0)
          TexAlpha= TTexturesManager::GetAlpha(ReplacableSkinId); //malo eleganckie ale narazie niech bedzie
       }
      else
       {
        glBindTexture(GL_TEXTURE_2D, TextureID);
       }
      if (!TexAlpha || !Global::bRenderAlpha)  //rysuj gdy nieprzezroczyste lub # albo gdy zablokowane alpha
      {
        if (eType==smt_FreeSpotLight)
        {
            matrix4x4 mat;
            glGetDoublev(GL_MODELVIEW_MATRIX,mat.getArray());
            fCosViewAngle=DotProduct(Normalize(mat*vector3(0,0,1)-mat*vector3(0,0,0)),vector3(0,0,1));
            if (fCosViewAngle>fcosFalloffAngle)  // kat wiekszy niz max stozek swiatla
            {
/*   TODO: poprawic to zeby dzialalo
              if (iFarAttenDecay>0)
               switch (iFarAttenDecay)
               {
                case 1:
                    Distdimm=fFarDecayRadius/(1+sqrt(fSquareDist));  //dorobic od kata
                break;
                case 2:
                    Distdimm=fFarDecayRadius/(1+fSquareDist);  //dorobic od kata
                break;
               }
              if (Distdimm>1)
               Distdimm=1;
              glColor3f(Diffuse[0]*Distdimm,Diffuse[1]*Distdimm,Diffuse[2]*Distdimm);
*/
              glPopMatrix();
              return;
            }
        }

        glCallList(uiDisplayList);
      }
      if (Child!=NULL)
          Child->Render(ReplacableSkinId);

      glPopMatrix();
    }
};       //Render

void __fastcall TSubModel::RenderAlpha(GLuint ReplacableSkinId)
{
    if (Next!=NULL)
        Next->RenderAlpha(ReplacableSkinId);

    if (eType==smt_FreeSpotLight)
    {
//        if (CosViewAngle>0)  //dorobic od kata
//        {
            return;
//        }
     // dorobic aureole!
    }

    if (Visible && (fSquareDist>=fSquareMinDist) && (fSquareDist<fSquareMaxDist))
    {
      glPushMatrix();
      glMultMatrixd(Matrix.getArray());
      if (b_aAnim==at_Rotate)
      {
          glRotatef(f_aAngle,v_aRotateAxis.x,v_aRotateAxis.y,v_aRotateAxis.z);
          glTranslatef(v_aTransVector.x,v_aTransVector.y,v_aTransVector.z);
          f_aAngle= 0;
          b_aAnim= at_None;
      }
      else
      if (b_aAnim==at_RotateXYZ)
      {
          glTranslatef(v_TransVector.x,v_TransVector.y,v_TransVector.z);
          glRotatef(v_aAngles.y,0.0,1.0,0.0);
          glRotatef(v_aAngles.x,1.0,0.0,0.0);
          glRotatef(v_aAngles.z,0.0,0.0,1.0);
          v_aAngles.x=v_aAngles.y=v_aAngles.z= 0;
          b_aAnim= at_None;
      }
    //zmienialne skory
      if ((TextureID==-1)) // && (ReplacableSkinId!=0))
       {
        glBindTexture(GL_TEXTURE_2D, ReplacableSkinId);
        if (ReplacableSkinId>0)
          TexAlpha= TTexturesManager::GetAlpha(ReplacableSkinId); //malo eleganckie ale narazie niech bedzie
       }
      else
       {
        glBindTexture(GL_TEXTURE_2D, TextureID);
       }
      if (TexAlpha && Global::bRenderAlpha)  //mozna rysowac bo przezroczyste i nie ma #
      {
        glCallList(uiDisplayList);
      }
      if (Child!=NULL)
          Child->RenderAlpha(ReplacableSkinId);
      glPopMatrix();
    }
}; //RenderAlpha


matrix4x4* __fastcall TSubModel::GetTransform()
{
//    Anim= true;
//    return &Transform;
};
//---------------------------------------------------------------------------

__fastcall TModel3d::TModel3d()
{
//    Root= NULL;
//    Materials= NULL;
//    MaterialsCount= 0;
    Root= NULL;
    SubModelsCount= 0;
//    ReplacableSkinID = 0;
};

__fastcall TModel3d::TModel3d(char *FileName)
{
//    Root= NULL;
//    Materials= NULL;
//    MaterialsCount= 0;
    Root= NULL;
    SubModelsCount= 0;
    LoadFromFile(FileName);
};

__fastcall TModel3d::~TModel3d()
{
    SafeDelete(Root);
//    SafeDeleteArray(Materials);

};

bool __fastcall TModel3d::AddTo(const char *Name, TSubModel *SubModel)
{
    TSubModel *tmp= GetFromName(Name);
    if (tmp!=NULL)
    {
        tmp->AddChild(SubModel);
        return true;
    }
    else
    {
        if (Root!=NULL)
            Root->AddNext(SubModel);
        else
            Root= SubModel;

        return true;
    }
};

TSubModel* __fastcall TModel3d::GetFromName(const char *sName)
{
    return Root ? Root->GetFromName(sName) : NULL;
};

/*
TMaterial* __fastcall TModel3d::GetMaterialFromName(char *sName)
{
    AnsiString tmp= AnsiString(sName).Trim();
    for (int i=0; i<MaterialsCount; i++)
        if (strcmp(sName,Materials[i].Name.c_str())==0)
//        if (Trim()==Materials[i].Name.tmp)
            return Materials+i;
    return Materials;
}
*/

bool __fastcall TModel3d::LoadFromTextFile(char *FileName)
{

    WriteLog(AnsiString("Loading - text model: "+AnsiString(FileName)).c_str());

    cParser parser(FileName, cParser::buffer_FILE);

    TSubModel *SubModel= NULL;

    std::string token;
    parser.getToken(token);

    while (token != "" || parser.eof())
    {
        std::string parent;
        parser.getToken(parent);

        if(parent == "")
            break;

        SubModel= new TSubModel();
        SubModel->Load(parser,SubModelsCount,this);
        if (!AddTo(parent.c_str(),SubModel))
            SafeDelete(SubModel);
        SubModelsCount++;
        
        parser.getToken(token);
    }
//    DecimalSeparator= ',';

    matrix4x4 *mat,tmp;
    if (Root)
    {
        mat= Root->GetMatrix();
        tmp.Identity();
        tmp.Rotation(M_PI/2,vector3(1,0,0));
        (*mat)= tmp*(*mat);
        tmp.Identity();
        tmp.Rotation(M_PI,vector3(0,0,1));
        (*mat)= tmp*(*mat);
    }

}


void __fastcall TModel3d::SaveToFile(char *FileName)
{
    Error("Not implemented yet :(");
};

void __fastcall TModel3d::BreakHierarhy()
{
    Error("Not implemented yet :(");
};


void __fastcall TModel3d::Render(vector3 pPosition, double fAngle, GLuint ReplacableSkinId)
{
//    glColor3f(1.0f,1.0f,1.0f);
//    glColor3f(0.0f,0.0f,0.0f);
    glPushMatrix();

    glTranslated(pPosition.x,pPosition.y,pPosition.z);
    if (fAngle!=0)
        glRotatef(fAngle,0,1,0);
/*
    matrix4x4 Identity;
    Identity.Identity();

    matrix4x4 CurrentMatrix;
    glGetdoublev(GL_MODELVIEW_MATRIX,CurrentMatrix.getArray());
    vector3 pos= vector3(0,0,0);
    pos= CurrentMatrix*pos;
    fSquareDist= SquareMagnitude(pos);
  */
    fSquareDist= SquareMagnitude(pPosition-Global::GetCameraPosition());

#ifdef _DEBUG
    if (Root)
        Root->Render(ReplacableSkinId);
#else
    Root->Render(ReplacableSkinId);
#endif
    glPopMatrix();
};

void __fastcall TModel3d::Render(double fSquareDistance, GLuint ReplacableSkinId)
{
    fSquareDist= fSquareDistance;
#ifdef _DEBUG
    if (Root)
        Root->Render(ReplacableSkinId);
#else
    Root->Render(ReplacableSkinId);
#endif
};

void __fastcall TModel3d::RenderAlpha(vector3 pPosition, double fAngle, GLuint ReplacableSkinId)
{
    glPushMatrix();
    glTranslated(pPosition.x,pPosition.y,pPosition.z);
    if (fAngle!=0)
        glRotatef(fAngle,0,1,0);
    fSquareDist= SquareMagnitude(pPosition-Global::GetCameraPosition());
#ifdef _DEBUG
    if (Root)
        Root->RenderAlpha(ReplacableSkinId);
#else
    Root->RenderAlpha(ReplacableSkinId);
#endif
    glPopMatrix();
};

void __fastcall TModel3d::RenderAlpha(double fSquareDistance, GLuint ReplacableSkinId)
{
    fSquareDist= fSquareDistance;
#ifdef _DEBUG
    if (Root)
        Root->RenderAlpha(ReplacableSkinId);
#else
    Root->RenderAlpha(ReplacableSkinId);
#endif
};


//---------------------------------------------------------------------------
#pragma package(smart_init)
