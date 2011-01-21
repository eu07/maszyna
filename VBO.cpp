//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "VBO.h"
#include "opengl/glew.h"
#include "usefull.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

__fastcall CMesh::CMesh()
{
 m_pVNT=NULL;
 m_nVertexCount=-1;
 m_nVBOVertices=0; //nie zarezerwowane

};
__fastcall CMesh::~CMesh()
{
 Release(); //zwolnienie zasobów
};

void __fastcall CMesh::MakeArray(int n)
{//tworzenie tablic
 m_nVertexCount=n;
 m_pVNT=new CVertNormTex[m_nVertexCount]; // przydzielenie pamiêci dla tablicy
};


void __fastcall CMesh::BuildVBOs()
{//tworzenie VBO i kasowanie ju¿ niepotrzebnych tablic
 //pobierz numer VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBOVertices);         //pobierz numer
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVertNormTex),m_pVNT,GL_STATIC_DRAW_ARB);
 WriteLog("Przydzielone VBO: "+AnsiString(m_nVBOVertices)+", punktów: "+AnsiString(m_nVertexCount));
 delete [] m_pVNT;  m_pVNT=NULL;
};

void __fastcall CMesh::Release()
{//zwolnienie zasobów przez sprz¹tacz albo destruktor
 if (m_nVBOVertices) //jeœli by³o coœ rezerwowane
 {
  unsigned int nBuffers[1]={m_nVBOVertices};
  glDeleteBuffersARB(1,nBuffers); // Free The Memory
  WriteLog("Zwolnione VBO: "+AnsiString(m_nVBOVertices));
 }
 m_nVBOVertices=0;
 m_nVertexCount=-1; //do ponownego zliczenia
 //usuwanie tablic, gdy by³y u¿yte do Vertex Array
 delete [] m_pVNT;
 m_pVNT=NULL;
};

bool __fastcall CMesh::StartVBO()
{//pocz¹tek rysowania elementów z VBO w sektorze
 if (m_nVertexCount<=0) return false; //nie ma nic do rysowania w ten sposób
 glEnableClientState(GL_VERTEX_ARRAY);         // W³¹cz strumieñ z pozycjami
 glEnableClientState(GL_NORMAL_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);         // W³¹cz strumieñ ze wspó³rzêdnymi tekstur
 if (m_nVBOVertices)
 {
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  glVertexPointer(3,GL_FLOAT,sizeof(CVertNormTex),NULL);             //pozycje
  glNormalPointer(GL_FLOAT,sizeof(CVertNormTex),((char*)NULL)+12);     //normalne
  glTexCoordPointer(2,GL_FLOAT,sizeof(CVertNormTex),((char*)NULL)+24); //wierzcho³ki
 }
 return true; //mo¿na rysowaæ w ten sposób
};

void __fastcall CMesh::EndVBO()
{//koniec u¿ycia VBO
 // Disable Pointers
 glDisableClientState(GL_VERTEX_ARRAY); // Disable Vertex Arrays
 glDisableClientState(GL_NORMAL_ARRAY);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY); // Disable Texture Coord Arrays
 //glBindBuffer(GL_ARRAY_BUFFER,0); 
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
 glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
};

