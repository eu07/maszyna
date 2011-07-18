//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "VBO.h"
#include "opengl/glew.h"
#include "usefull.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

__fastcall CMesh::CMesh()
{//utworzenie pustego obiektu
 m_pVNT=NULL;
 m_nVertexCount=-1;
 m_nVBOVertices=0; //nie zarezerwowane
};

__fastcall CMesh::~CMesh()
{//usuwanie obiektu
 Clear(); //zwolnienie zasob�w
};

void __fastcall CMesh::MakeArray(int n)
{//tworzenie tablic
 m_nVertexCount=n;
 m_pVNT=new CVertNormTex[m_nVertexCount]; // przydzielenie pami�ci dla tablicy
};

void __fastcall CMesh::BuildVBOs(bool del)
{//tworzenie VBO i kasowanie ju� niepotrzebnych tablic
 //pobierz numer VBO oraz ustaw go jako aktywny
 glGenBuffersARB(1,&m_nVBOVertices); //pobierz numer
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices); //ustaw bufor jako aktualny
 glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_nVertexCount*sizeof(CVertNormTex),m_pVNT,GL_STATIC_DRAW_ARB);
 //WriteLog("Assigned VBO number "+AnsiString(m_nVBOVertices)+", vertices: "+AnsiString(m_nVertexCount));
 if (del) SafeDeleteArray(m_pVNT); //wierzcho�ki ju� si� nie przydadz�
};

void __fastcall CMesh::Clear()
{//niewirtualne zwolnienie zasob�w przez sprz�tacz albo destruktor
 //inna nazwa, �eby nie miesza�o si� z funkcj� wirtualn� sprz�tacza
 if (m_nVBOVertices) //je�li by�o co� rezerwowane
 {
  glDeleteBuffersARB(1,&m_nVBOVertices); // Free The Memory
  //WriteLog("Released VBO number "+AnsiString(m_nVBOVertices));
 }
 m_nVBOVertices=0;
 m_nVertexCount=-1; //do ponownego zliczenia
 SafeDeleteArray(m_pVNT); //usuwanie tablic, gdy by�y u�yte do Vertex Array
};

bool __fastcall CMesh::StartVBO()
{//pocz�tek rysowania element�w z VBO
 if (m_nVertexCount<=0) return false; //nie ma nic do rysowania w ten spos�b
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_NORMAL_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 if (m_nVBOVertices)
 {
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  glVertexPointer(3,GL_FLOAT,sizeof(CVertNormTex),((char*)NULL));               //pozycje
  glNormalPointer(GL_FLOAT,sizeof(CVertNormTex),((char*)NULL)+12);     //normalne
  glTexCoordPointer(2,GL_FLOAT,sizeof(CVertNormTex),((char*)NULL)+24); //wierzcho�ki
 }
 return true; //mo�na rysowa� z VBO
};

bool __fastcall CMesh::StartColorVBO()
{//pocz�tek rysowania punkt�w �wiec�cych z VBO
 if (m_nVertexCount<=0) return false; //nie ma nic do rysowania w ten spos�b
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_COLOR_ARRAY);
 if (m_nVBOVertices)
 {
  glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  glVertexPointer(3,GL_FLOAT,sizeof(CVertNormTex),((char*)NULL));               //pozycje
  glColorPointer(3,GL_UNSIGNED_BYTE,sizeof(CVertNormTex),((char*)NULL)+12);     //kolory
 }
 return true; //mo�na rysowa� z VBO
};

void __fastcall CMesh::EndVBO()
{//koniec u�ycia VBO
 glDisableClientState(GL_VERTEX_ARRAY);
 glDisableClientState(GL_NORMAL_ARRAY);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 glDisableClientState(GL_COLOR_ARRAY);
 //glBindBuffer(GL_ARRAY_BUFFER,0); //takie co� psuje, mimo i� polecali u�y�
 glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
 glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
};

