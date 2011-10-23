//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Names.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
/*
Modu³ zarz¹dzaj¹cy plikami oraz wyszukiwaniem obiektów wg nazw.
1. Bêdzie mia³ przydzielony z góry (EU07.INI) obszar pamiêci (rzêdu 64kB).
2. W przypadku przepe³nienia dostêpnej pamiêci wyst¹pi b³¹d wczytywania.
3. Obszar ten bêdzie zu¿ywany na rekordy obiektów oraz ci¹gi tekstowe z nazwami.
4. Rekordy bêd¹ sortowane w ramach typu (tekstury, dŸwiêki, modele, node, eventy).
5. Pierwszy etap wyszukiwania to 5 bitów z pierwszego bajtu i 3 z drugiego (256).
6. Dla plików istnieje mo¿liwoœæ wczytania ich w innym terminie.
7. Mo¿liwoœæ wczytania plików w oddzielnym watku (np. tekstur)/

Obs³ugiwane pliki:
1. Tekstury, mo¿na wczytywaæ póŸniej, rekord przechowuje numer podany przez kartê graficzn¹.
2. DŸwiêki, mo¿na wczytaæ póŸniej.
3. Modele, mo¿na wczytaæ póŸniej o ile nie maj¹ animacji eventami i nie dotycz¹ pojazdów.

Obiekty sortowane wg nazw, mo¿na dodawaæ i usuwaæ komórki:
4. Tory, drogi, rzeki - wyszukiwanie w celu sprawdzenia zajetoœci.
5. Eventy - wyszukiwane przy zewnêtrznym wywo³aniu oraz podczas wczytywania.
6. Pojazdy - wyszukiwane w celu wysy³ania komend.
7. Egzemplarze modeli animowanych - wyszukiwanie w celu po³¹czenia z animacjami.

*/
