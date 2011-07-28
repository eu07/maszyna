//---------------------------------------------------------------------------

#ifndef PSH
#define PSH

struct vector3ps
{
float x;
float y;
float z;
};

struct drop // struktura przechowuj¹ca
{ // dane o pojedynczej kropli
vector3ps v; // prêdkoœæ kropli
vector3ps p; // po³o¿enie kropli
vector3ps prevp; // poprzednie po³o¿enie kropli
float t; // czas ¿ycia kropli
};


class CRain
{
public:
// konstruktor klasy generuj¹cy deszcz o zadanej
// maks. liczbie kropli, po³o¿eniu pocz¹tkowym
// [x0,y0,z0], czasie ¿ycia timemax, promieniu maxr,
// wysokoœci maxh i prêdkoœci pionowej kropli vstart
__fastcall CRain();
void __fastcall Init(int maxn, float x0, float y0, float z0, float timemax, float maxr, float maxh, float vstart);
__fastcall ~CRain();
void __fastcall Render(); // rysuje opad deszczu
// Update uaktualnia dane o deszczu po up³ywie
// czasu t dla wektora wiatru wind
// i procentowego stopnia zaburzenia d
void __fastcall Update(float t, vector3ps wind);
void __fastcall Generate(int n); // tworzy n nowych kropli deszczu
void __fastcall StopAll(); // zatrzymuje emisjê deszczu i usuwa
// wszystkie krople
// MoveTo symuluje przesuniêcie
// obserwatora do punktu [x,y,z]
void __fastcall MoveTo(float x, float y, float z);
protected:
drop drops[1000]; // lista "kropli" deszczu
int maxnr; // maksymalna liczba kropli
int nr; // aktualna liczba kropli
float r; // promieñ obszaru emisji
float h; // wysokoœæ obszaru emisji
float tmax; // maksymalny czas ¿ycia kropli
vector3ps start; // po³o¿enie punktu startowego
// emisji deszczu
// CreateDrop tworzy now¹ kroplê w losowym punkcie
void __fastcall CreateDrop();
};
//---------------------------------------------------------------------------
#endif
