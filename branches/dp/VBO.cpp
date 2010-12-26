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
 Release(); //zwolnienie zasob�w
};

void __fastcall CMesh::MakeArrays(int n)
{//tworzenie tablic
 m_nVertexCount=n;
 m_pVertices=new CVert[m_nVertexCount];       // Przydzielenie pami�ci dla wierzcho�k�w
 m_pNormals=new CVec[m_nVertexCount];         // Przydzielenie pami�ci dla normalnych
 m_pTexCoords=new CTexCoord[m_nVertexCount];  // Przydzielenie pami�ci dla wsp�rz�dnych
};

void __fastcall CMesh::BuildVBOs()
{//tworzenie VBO i kasowanie ju� niepotrzebnych tablic
 // Wygeneruj nazw� dla VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBOVertices);         // Pobierz poprawn� nazw�
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVert),m_pVertices,GL_STATIC_DRAW_ARB);
 // Wygeneruj nazw� dla VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBONormals);         // Pobierz poprawn� nazw�
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBONormals);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVec),m_pNormals,GL_STATIC_DRAW_ARB);
 // Wygeneruj nazw� oraz ustaw jako aktywny VBO dla wsp�rz�dnych tekstur
 glGenBuffersARB(1,&m_nVBOTexCoords);         // Pobierz poprawn� nazw�
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOTexCoords);         // Ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CTexCoord),m_pTexCoords,GL_STATIC_DRAW_ARB);
 // Nasze lokalne kopie danych nie s� d�u�ej potrzebne, wszystko jest ju� w karcie graficznej.
 delete [] m_pVertices;  m_pVertices=NULL;
 delete [] m_pNormals;   m_pNormals=NULL;
 delete [] m_pTexCoords; m_pTexCoords=NULL;
 WriteLog("Przydzielone VBO: "+AnsiString(m_nVBOVertices)+", "+AnsiString(m_nVBONormals)+", "+AnsiString(m_nVBOTexCoords)+", punkt�w: "+AnsiString(m_nVertexCount));
};

void __fastcall CMesh::Release()
{//zwolnienie zasob�w przez sprz�tacz albo destruktor
 if (m_nVBOVertices) //je�li by�o co� rezerwowane
 {
  unsigned int nBuffers[3]={m_nVBOVertices,m_nVBONormals,m_nVBOTexCoords};
  glDeleteBuffersARB(3,nBuffers); // Free The Memory
  WriteLog("Zwolnione VBO: "+AnsiString(m_nVBOVertices)+", "+AnsiString(m_nVBONormals)+", "+AnsiString(m_nVBOTexCoords));
 }
 m_nVBOVertices=m_nVBONormals=m_nVBOTexCoords=0;
 m_nVertexCount=-1; //do ponownego zliczenia
 //usuwanie tablic, gdy by�y u�yte do Vertex Array
 delete [] m_pVertices;  m_pVertices=NULL;
 delete [] m_pNormals;   m_pNormals=NULL;
 delete [] m_pTexCoords; m_pTexCoords=NULL;
};

bool __fastcall CMesh::StartVBO()
{//pocz�tek rysowania element�w z VBO w sektorze
 if (m_nVertexCount<=0) return false; //nie ma nic do rysowania w ten spos�b
 glEnableClientState(GL_VERTEX_ARRAY);         // W��cz strumie� z pozycjami
 glEnableClientState(GL_NORMAL_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);         // W��cz strumie� ze wsp�rz�dnymi tekstur
 // Za�aduj odpowiednie dane
 if (m_nVBOVertices)
 {
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  //glVertexPointer(3,GL_DOUBLE,0,NULL);         // Za�aduj VBO z pozycjami
  glVertexPointer(3,GL_FLOAT,0,NULL);         // Za�aduj VBO z pozycjami
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBONormals);
  glNormalPointer(GL_FLOAT,0,NULL);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOTexCoords);
  glTexCoordPointer(2,GL_FLOAT,0,NULL);         // Za�aduj VBO ze wsp�rz�dnymi tekstur
 }
 else
 {
  //glVertexPointer(3,GL_DOUBLE,0,m_pVertices);         // Za�aduj bufor z pozycjami
  glVertexPointer(3,GL_FLOAT,0,m_pVertices);         // Za�aduj bufor z pozycjami
  glNormalPointer(GL_FLOAT,0,m_pNormals);
  glTexCoordPointer(2,GL_FLOAT,0,m_pTexCoords);         // Za�aduj bufor ze wsp�rz�dnymi tekstur
 }
 return true; //mo�na rysowa� w ten spos�b
};

void __fastcall CMesh::EndVBO()
{//koniec u�ycia VBO
 // Disable Pointers
 glDisableClientState(GL_VERTEX_ARRAY); // Disable Vertex Arrays
 glDisableClientState(GL_NORMAL_ARRAY);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY); // Disable Texture Coord Arrays
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
 glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
};

