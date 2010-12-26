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
 m_pVertices=NULL;
 m_pNormals=NULL;
 m_pTexCoords=NULL;
 m_nVertexCount=-1;
 m_nVBOVertices=m_nVBONormals=m_nVBOTexCoords=0; //nie zarezerwowane

};
__fastcall CMesh::~CMesh()
{
 Release(); //zwolnienie zasobów
};

void __fastcall CMesh::MakeArrays(int n)
{//tworzenie tablic
 m_nVertexCount=n;
 m_pVertices=new CVert[m_nVertexCount];       // Przydzielenie pamiêci dla wierzcho³ków
 m_pNormals=new CVec[m_nVertexCount];         // Przydzielenie pamiêci dla normalnych
 m_pTexCoords=new CTexCoord[m_nVertexCount];  // Przydzielenie pamiêci dla wspó³rzêdnych
};

void __fastcall CMesh::BuildVBOs()
{//tworzenie VBO i kasowanie ju¿ niepotrzebnych tablic
 // Wygeneruj nazwê dla VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBOVertices);         // Pobierz poprawn¹ nazwê
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVert),m_pVertices,GL_STATIC_DRAW_ARB);
 // Wygeneruj nazwê dla VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBONormals);         // Pobierz poprawn¹ nazwê
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBONormals);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVec),m_pNormals,GL_STATIC_DRAW_ARB);
 // Wygeneruj nazwê oraz ustaw jako aktywny VBO dla wspó³rzêdnych tekstur
 glGenBuffersARB(1,&m_nVBOTexCoords);         // Pobierz poprawn¹ nazwê
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOTexCoords);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CTexCoord),m_pTexCoords,GL_STATIC_DRAW_ARB);
 // Nasze lokalne kopie danych nie s¹ d³u¿ej potrzebne, wszystko jest ju¿ w karcie graficznej.
 delete [] m_pVertices;  m_pVertices=NULL;
 delete [] m_pNormals;   m_pNormals=NULL;
 delete [] m_pTexCoords; m_pTexCoords=NULL;
 WriteLog("Przydzielone VBO: "+AnsiString(m_nVBOVertices)+", "+AnsiString(m_nVBONormals)+", "+AnsiString(m_nVBOTexCoords)+", punktów: "+AnsiString(m_nVertexCount));
};

void __fastcall CMesh::Release()
{//zwolnienie zasobów przez sprz¹tacz albo destruktor
 if (m_nVBOVertices) //jeœli by³o coœ rezerwowane
 {
  unsigned int nBuffers[3]={m_nVBOVertices,m_nVBONormals,m_nVBOTexCoords};
  glDeleteBuffersARB(3,nBuffers); // Free The Memory
  WriteLog("Zwolnione VBO: "+AnsiString(m_nVBOVertices)+", "+AnsiString(m_nVBONormals)+", "+AnsiString(m_nVBOTexCoords));
 }
 m_nVBOVertices=m_nVBONormals=m_nVBOTexCoords=0;
 m_nVertexCount=-1; //do ponownego zliczenia
 //usuwanie tablic, gdy by³y u¿yte do Vertex Array
 delete [] m_pVertices;  m_pVertices=NULL;
 delete [] m_pNormals;   m_pNormals=NULL;
 delete [] m_pTexCoords; m_pTexCoords=NULL;
};

bool __fastcall CMesh::StartVBO()
{//pocz¹tek rysowania elementów z VBO w sektorze
 if (m_nVertexCount<=0) return false; //nie ma nic do rysowania w ten sposób
 glEnableClientState(GL_VERTEX_ARRAY);         // W³¹cz strumieñ z pozycjami
 glEnableClientState(GL_NORMAL_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);         // W³¹cz strumieñ ze wspó³rzêdnymi tekstur
 // Za³aduj odpowiednie dane
 if (m_nVBOVertices)
 {
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  //glVertexPointer(3,GL_DOUBLE,0,NULL);         // Za³aduj VBO z pozycjami
  glVertexPointer(3,GL_FLOAT,0,NULL);         // Za³aduj VBO z pozycjami
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBONormals);
  glNormalPointer(GL_FLOAT,0,NULL);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOTexCoords);
  glTexCoordPointer(2,GL_FLOAT,0,NULL);         // Za³aduj VBO ze wspó³rzêdnymi tekstur
 }
 else
 {
  //glVertexPointer(3,GL_DOUBLE,0,m_pVertices);         // Za³aduj bufor z pozycjami
  glVertexPointer(3,GL_FLOAT,0,m_pVertices);         // Za³aduj bufor z pozycjami
  glNormalPointer(GL_FLOAT,0,m_pNormals);
  glTexCoordPointer(2,GL_FLOAT,0,m_pTexCoords);         // Za³aduj bufor ze wspó³rzêdnymi tekstur
 }
 return true; //mo¿na rysowaæ w ten sposób
};

void __fastcall CMesh::EndVBO()
{//koniec u¿ycia VBO
 // Disable Pointers
 glDisableClientState(GL_VERTEX_ARRAY); // Disable Vertex Arrays
 glDisableClientState(GL_NORMAL_ARRAY);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY); // Disable Texture Coord Arrays
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
 glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
};

