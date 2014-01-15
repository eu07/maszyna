//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Forth.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

/* Ra: Forth jest prostym j�zykiem programowania, funkcjonuj�cym na dosy�
niskim poziomie. Mo�e by� zar�wno interpretowany jak i kompilowany, co
pozwala szybko dzia�a� skompilowanym programom, jak r�wnie� wykonywa�
polecenia wprowadzane z klawiatury. Za pomoc� zdefiniowanych kompilator�w
mo�na definiowa� nowe funkcje, mo�na r�wnie� tworzy� nowe struktury jezyka,
np. wykraczaj�ce poza dotychczasow� sk�adni�. Prostota i wydajno�� s� okupione
nieco specyficzn� sk�adni�, wynikaj�c� z u�ycia stosu do oblicze� (tzw. odwrotna
notacja polska).

Forth ma by� u�ywany jako:
 1. Alternatywna posta� event�w. Dotychczasowe eventy w sceneriach mog� by�
traktowane jako funkcje j�zyka Forth. Jednocze�nie mo�na uzyska� uelastycznienie
sk�adni, poprzez np. po��czenie w jednym wpisie r�nych typ�w event�w wraz z
dodaniem warunk�w dost�pnych tylko dla Multiple. R�wnie� mo�na b�dzie u�ywa�
wyra�e� j�zyka Forth, czyli dokonywa� oblicze�.
 2. Jako docelowa posta� schemat�w obwod�w Ladder Diagram. Mo�na zrobi� tak, by
program �r�d�owy w Forth (przy pewnych ograniczeniach) by� wy�wietlany graficznie
jako Ladder Diagram. Jednocze�nie graficzne modyfikacje schematu mo�na by
prze�o�y� na posta� tekstow� Forth i skompilowa� na kod niskiego poziomu.
 3. Jako algorytm funkcjonowania AI, mo�liwy do zaprogramowania na zewn�tzr EXE.
Ka�dy u�ytkownik b�dzie m�g� sw�j w�asny styl jazdy.
 4. Jako algorytm sterowania ruchem (zale�no�ci na stacji).
 5. Jako AI stacji, zarz�dzaj�ce ruchem na stacji zale�nie w czasie rzeczywistym.

Wzgl�dem pierwotnego j�zyka Forth, u�yty tutaj b�dzie mia� pewne odst�pstwa:
 1. Operacje na liczbach czterobajtowych float. Nale�y we w�asnym zakresie dba�
o zgodno�� u�ytych funkcji z typem argumentu na stosie.
 2. Jedynym typem ca�kowitym jest czterobajtowy int.
 3. Do wyszukiwania obiekt�w w scenerii po nazwie (tor�w, event�w), u�ywane jest
drzewo zamiast listy.

Wpisy w scenerii, przetwarzane jako komendy Forth, nie mog� u�ywa� �rednika jako
separatora s��w. Uniemo�liwia to u�ywanie �rednika jako s�owa.
*/
