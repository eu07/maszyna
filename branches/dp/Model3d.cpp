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

void __fastcall TSubModel::Load(TQueryParserComp *Parser, int NIndex, TModel3d *Model)
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
    AnsiString str;

    str= Parser->GetNextSymbol().LowerCase();
    if (str!=AnsiString("type:"))
     Error("Model type parse failure!");

    str= Parser->GetNextSymbol().LowerCase();

    if (str=="mesh")
        eType= smt_Mesh;
    else
    if (str=="point")
        eType= smt_Point;
    else
    if (str=="freespotlight")
        eType= smt_FreeSpotLight;  //McZapkie-050702: kierunkowy punkt samoswiecacy

    str= Parser->GetNextSymbol().LowerCase();
    str= Parser->GetNextSymbol().LowerCase();

    Name= str;

    str= Parser->GetNextSymbol().LowerCase();

    if (str=="anim:")
    {
        str= Parser->GetNextSymbol().LowerCase();
//        bAnim= (str=="true");
        str= Parser->GetNextSymbol().LowerCase();
    }
    double test;
    if (eType==smt_Mesh)
    {
        Ambient[0]= Parser->GetNextSymbol().ToDouble()/255;
        Ambient[1]= Parser->GetNextSymbol().ToDouble()/255;
        Ambient[2]= Parser->GetNextSymbol().ToDouble()/255;
        str= Parser->GetNextSymbol().LowerCase();
    }
    Diffuse[0]= Parser->GetNextSymbol().ToDouble()/255;  //McZapkie-240702: kolor w zakresie 0...1
    Diffuse[1]= Parser->GetNextSymbol().ToDouble()/255;
    Diffuse[2]= Parser->GetNextSymbol().ToDouble()/255;
    str= Parser->GetNextSymbol().LowerCase();
    if (eType==smt_Mesh)
    {
        Specular[0]= Parser->GetNextSymbol().ToDouble()/255;
        Specular[1]= Parser->GetNextSymbol().ToDouble()/255;
        Specular[2]= Parser->GetNextSymbol().ToDouble()/255;
        str= Parser->GetNextSymbol().LowerCase();
    }
    str= Parser->GetNextSymbol().LowerCase();
    bLight= (str==AnsiString("true"));

    if (eType==smt_FreeSpotLight)
    {
        str= Parser->GetNextSymbol().LowerCase();
        if (str!=AnsiString("nearattenstart:"))
          Error("Model light parse failure!");
        fNearAttenStart= Parser->GetNextSymbol().ToDouble();
        str= Parser->GetNextSymbol().LowerCase();
        fNearAttenEnd= Parser->GetNextSymbol().ToDouble();
        str= Parser->GetNextSymbol().LowerCase();
        str= Parser->GetNextSymbol().LowerCase();
        bUseNearAtten= (str==AnsiString("true"));
        str= Parser->GetNextSymbol().LowerCase();
        iFarAttenDecay= Parser->GetNextSymbol().ToInt();
        str= Parser->GetNextSymbol().LowerCase();
        fFarDecayRadius= Parser->GetNextSymbol().ToDouble();
        str= Parser->GetNextSymbol().LowerCase();
        fcosFalloffAngle= cos(Parser->GetNextSymbol().ToDouble()*M_PI/90);
        str= Parser->GetNextSymbol().LowerCase();
        fcosHotspotAngle= cos(Parser->GetNextSymbol().ToDouble()*M_PI/90);
    }

    if (eType==smt_Mesh)
    {
        str= Parser->GetNextSymbol().LowerCase();
        str= Parser->GetNextSymbol().LowerCase();
        bWire= (str=="true");

        str= Parser->GetNextSymbol().LowerCase();
        fWireSize= Parser->GetNextSymbol().ToDouble();

        str= Parser->GetNextSymbol().LowerCase();
        Transparency= Parser->GetNextSymbol().ToDouble()/100.0f;

        str= Parser->GetNextSymbol().LowerCase();
        if (str!=AnsiString("map:"))
         Error("Model map parse failure!");
        str= Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("none"))
        {
            TextureID= 0;
        }
        else
        {
// McZapkie-060702: zmienialne skory modelu
          if (StrPos(str.c_str(),AnsiString("replacableskin").c_str()))
           {
//            extName= strchr(str.c_str(),'.');
            TextureID= -1;
           }
          else
           {
            char buf[255]= "";
//            str= ExtractFileName(str);
            if (strchr(str.c_str(),'/')==NULL)                 //jesli tylko nazwa pliku to dawac biezaca sciezke do tekstur
                 {
                    strcat(buf,Global::asCurrentTexturePath.c_str());
                 }
            strcat(buf,str.c_str());
            TextureID= TTexturesManager::GetTextureID(buf);
            TexAlpha= TTexturesManager::GetAlpha(TextureID);
//            TexHash= TTexturesManager::GetHash(TextureID);
           }
        }
    }

    str= Parser->GetNextSymbol().LowerCase();
    fSquareMaxDist= Parser->GetNextSymbol().ToDouble();
    fSquareMaxDist*= fSquareMaxDist;

    str= Parser->GetNextSymbol().LowerCase();
    fSquareMinDist= Parser->GetNextSymbol().ToDouble();
    fSquareMinDist*= fSquareMinDist;

    str= Parser->GetNextSymbol().LowerCase();

    Matrix(0)[0]=Parser->GetNextSymbol().ToDouble();
    Matrix(0)[1]=Parser->GetNextSymbol().ToDouble();
    Matrix(0)[2]=Parser->GetNextSymbol().ToDouble();
    Matrix(0)[3]=Parser->GetNextSymbol().ToDouble();
    Matrix(1)[0]=Parser->GetNextSymbol().ToDouble();
    Matrix(1)[1]=Parser->GetNextSymbol().ToDouble();
    Matrix(1)[2]=Parser->GetNextSymbol().ToDouble();
    Matrix(1)[3]=Parser->GetNextSymbol().ToDouble();
    Matrix(2)[0]=Parser->GetNextSymbol().ToDouble();
    Matrix(2)[1]=Parser->GetNextSymbol().ToDouble();
    Matrix(2)[2]=Parser->GetNextSymbol().ToDouble();
    Matrix(2)[3]=Parser->GetNextSymbol().ToDouble();
    Matrix(3)[0]=Parser->GetNextSymbol().ToDouble();
    Matrix(3)[1]=Parser->GetNextSymbol().ToDouble();
    Matrix(3)[2]=Parser->GetNextSymbol().ToDouble();
    Matrix(3)[3]=Parser->GetNextSymbol().ToDouble();

    if (eType==smt_Mesh)
    {

    str= Parser->GetNextSymbol().LowerCase();
    iNumVerts= Parser->GetNextSymbol().ToDouble();
    if (iNumVerts%3==0)
        Vertices= new GLVERTEX[iNumVerts];
    else
    {
        iNumVerts= 0;
        Error("Mesh error, iNumVertices%3!=0");
        return;
    }

    int iNumFaces= iNumVerts/3;

    DWORD *sg= NULL;
    sg= new DWORD[iNumFaces];

    for (int i=0; i<iNumVerts; i++)
    {
        if (i%3==0)
        {
            sg[i/3]= Parser->GetNextSymbol().ToDouble();
        }
        Vertices[i].Point.x= Parser->GetNextSymbol().ToDouble();
        Vertices[i].Point.y= Parser->GetNextSymbol().ToDouble();
        Vertices[i].Point.z= Parser->GetNextSymbol().ToDouble();

    //    Vertices[i].Normal.x= Parser->GetNextSymbol().ToDouble();
  //      Vertices[i].Normal.y= Parser->GetNextSymbol().ToDouble();
//        Vertices[i].Normal.z= Parser->GetNextSymbol().ToDouble();

        Vertices[i].Normal= vector3(0,0,0);

        Vertices[i].tu= Parser->GetNextSymbol().ToDouble();
        Vertices[i].tv= 1.0f-Parser->GetNextSymbol().ToDouble();
        if (i%3==2)
        {
            if ( (Vertices[i].Point==Vertices[i-1].Point) ||
                 (Vertices[i-1].Point==Vertices[i-2].Point) ||
                 (Vertices[i-2].Point==Vertices[i].Point) )
            {
                WriteLog("Degenerated triangle found");
//                i-= 3;
  //              iNumVerts-= 3;

            }
        }
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
//*
            f= SeekFaceNormal(sg,0,sg[i],Vertices[v].Point,Vertices,iNumVerts);
            norm= vector3(0,0,0);
            while (f>=0)
            {
                norm+= SafeNormalize(CrossProduct( Vertices[f*3].Point-Vertices[f*3+1].Point,
                                               Vertices[f*3].Point-Vertices[f*3+2].Point ));
                f= SeekFaceNormal(sg,f+1,sg[i],Vertices[v].Point,Vertices,iNumVerts);
            }
//  */
//            if (sg[i]==0)
            if (norm.Length()==0)
                norm+= SafeNormalize(CrossProduct( Vertices[i*3].Point-Vertices[i*3+1].Point,
                                               Vertices[i*3].Point-Vertices[i*3+2].Point ));

            if (norm.Length()>0)
                Vertices[v].Normal= Normalize(norm);
            else
                f=0;
            v++;
        }
    }
    delete[] sg;
    }

    int d;

    if (eType==smt_Mesh)
    {

        uiDisplayList= glGenLists(1);
        glNewList(uiDisplayList,GL_COMPILE);

//McZapkie-221002: zwykla tekstura tu, zmienialna w render:
//        if ((TextureID!=-1))
//         glBindTexture(GL_TEXTURE_2D, TextureID);
//        glColorMaterial(GL_FRONT, GL_DIFFUSE);
//        glEnable(GL_COLOR_MATERIAL);
        glColor3f(Diffuse[0],Diffuse[1],Diffuse[2]);   //McZapkie-240702: zamiast ub
//        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,Ambient);
//        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,Diffuse);
//        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,Specular);   //McZapkie-240702: definicje wlasnosci materialu
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,Diffuse);
//        glMaterialfv(GL_FRONT, GL_SHININESS, low_shininess);

        if (bLight)
            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Diffuse);  //zeny swiecilo na kolorowo
//            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emm1);

        glBegin(GL_TRIANGLES);
            for (int i=0; i<iNumVerts; i++)
            {
                glNormal3f(Vertices[i].Normal.x,Vertices[i].Normal.y,Vertices[i].Normal.z);
//            glTexCoord2f(1-Vertices[i].tu,Vertices[i].tv);
                glTexCoord2f(Vertices[i].tu,1-Vertices[i].tv);
                glVertex3f(Vertices[i].Point.x,Vertices[i].Point.y,Vertices[i].Point.z);
            }
        glEnd();

        if (bLight)
            glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emm2);
//        glDisable(GL_COLOR_MATERIAL); //MC
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

TSubModel* __fastcall TSubModel::GetFromName(char *sName)
{
/*    for (int i=0; i<SubModelsCount; i++)
        if (strcmp(SubModels[i].GetName(),Name)==0)
            return(SubModels+i);*/
    TSubModel *ret;
    strlwr(sName);
    if (strcmp(Name.c_str(),sName)==0)
        return this;
    if (Next!=NULL)
    {
        ret= Next->GetFromName(sName);
        if (ret)
            return (ret);
    }
    if (Child!=NULL)
        return(Child->GetFromName(sName));
    return(NULL);
//    return(SubModels[0]);

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

bool __fastcall TModel3d::AddTo(char *Name, TSubModel *SubModel)
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

TSubModel* __fastcall TModel3d::GetFromName(char *sName)
{
    if (Root!=NULL)
        return(Root->GetFromName(sName));
    return(NULL);

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
//    std::string trtest = FileName;
//     if (trtest.find("tr/")==7)
//      return false;

    WriteLog(AnsiString("Loading - text model: "+AnsiString(FileName)).c_str());
    WIN32_FIND_DATA FindFileData;
    HANDLE handle= FindFirstFile(AnsiString(AnsiString(FileName)).c_str(), &FindFileData);
    if (handle==INVALID_HANDLE_VALUE)
    {
        Error("Cannot load model");
        FindClose(handle);
        return false;
    }
    FindClose(handle);


    TFileStream *fs;
    fs= new TFileStream(FileName , fmOpenRead	| fmShareCompat	);
    AnsiString str= "";
    int size= fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+= "";
    delete fs;
    TQueryParserComp *Parser;
    Parser= new TQueryParserComp(NULL);
    Parser->TextToParse= str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();
    AnsiString Token,asFileName;

//    MaterialsCount= 0;

    TSubModel *SubModel= NULL;

    str= Parser->GetNextSymbol().LowerCase();
    while (!Parser->EndOfFile)
    {
        str= Parser->GetNextSymbol().LowerCase();
        SubModel= new TSubModel();
        SubModel->Load(Parser,SubModelsCount,this);
        if (!AddTo(str.c_str(),SubModel))
            SafeDelete(SubModel);
        SubModelsCount++;

        str= Parser->GetNextSymbol().LowerCase();
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
