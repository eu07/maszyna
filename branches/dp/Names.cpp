//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Names.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
/*
Modu� zarz�dzaj�cy plikami oraz wyszukiwaniem obiekt�w wg nazw.
1. B�dzie mia� przydzielony z g�ry (EU07.INI) obszar pami�ci (rz�du 64kB).
2. W przypadku przepe�nienia dost�pnej pami�ci wyst�pi b��d wczytywania.
3. Obszar ten b�dzie zu�ywany na rekordy obiekt�w oraz ci�gi tekstowe z nazwami.
4. Rekordy b�d� sortowane w ramach typu (tekstury, d�wi�ki, modele, node, eventy).
5. Pierwszy etap wyszukiwania to 5 bit�w z pierwszego bajtu i 3 z drugiego (256).
6. Dla plik�w istnieje mo�liwo�� wczytania ich w innym terminie.
7. Mo�liwo�� wczytania plik�w w oddzielnym watku (np. tekstur)/

Obs�ugiwane pliki:
1. Tekstury, mo�na wczytywa� p�niej, rekord przechowuje numer podany przez kart� graficzn�.
2. D�wi�ki, mo�na wczyta� p�niej.
3. Modele, mo�na wczyta� p�niej o ile nie maj� animacji eventami i nie dotycz� pojazd�w.

Obiekty sortowane wg nazw, mo�na dodawa� i usuwa� kom�rki:
4. Tory, drogi, rzeki - wyszukiwanie w celu sprawdzenia zajeto�ci.
5. Eventy - wyszukiwane przy zewn�trznym wywo�aniu oraz podczas wczytywania.
6. Pojazdy - wyszukiwane w celu wysy�ania komend.
7. Egzemplarze modeli animowanych - wyszukiwanie w celu po��czenia z animacjami.

*/
