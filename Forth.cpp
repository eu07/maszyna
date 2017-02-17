/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include <vcl.h>
#pragma hdrstop

#include "Forth.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

/* Ra: Forth jest prostym językiem programowania, funkcjonującym na dosyć
niskim poziomie. Może być zarówno interpretowany jak i kompilowany, co
pozwala szybko działać skompilowanym programom, jak również wykonywać
polecenia wprowadzane z klawiatury. Za pomocą zdefiniowanych kompilatorów
można definiować nowe funkcje, można również tworzyć nowe struktury jezyka,
np. wykraczające poza dotychczasową składnię. Prostota i wydajność są okupione
nieco specyficzną składnią, wynikającą z użycia stosu do obliczeń (tzw. odwrotna
notacja polska).

Forth ma być używany jako:
 1. Alternatywna postać eventów. Dotychczasowe eventy w sceneriach mogą być
traktowane jako funkcje języka Forth. Jednocześnie można uzyskać uelastycznienie
składni, poprzez np. połączenie w jednym wpisie różnych typów eventów wraz z
dodaniem warunków dostępnych tylko dla Multiple. Również można będzie używać
wyrażeń języka Forth, czyli dokonywać obliczeń.
 2. Jako docelowa postać schematów obwodów Ladder Diagram. Można zrobić tak, by
program źródłowy w Forth (przy pewnych ograniczeniach) był wyświetlany graficznie
jako Ladder Diagram. Jednocześnie graficzne modyfikacje schematu można by
przełożyć na postać tekstową Forth i skompilować na kod niskiego poziomu.
 3. Jako algorytm funkcjonowania AI, możliwy do zaprogramowania na zewnątzr EXE.
Każdy użytkownik będzie mógł swój własny styl jazdy.
 4. Jako algorytm sterowania ruchem (zależności na stacji).
 5. Jako AI stacji, zarządzające ruchem na stacji zależnie w czasie rzeczywistym.

Względem pierwotnego języka Forth, użyty tutaj będzie miał pewne odstępstwa:
 1. Operacje na liczbach czterobajtowych float. Należy we własnym zakresie dbać
o zgodność użytych funkcji z typem argumentu na stosie.
 2. Jedynym typem całkowitym jest czterobajtowy int.
 3. Do wyszukiwania obiektów w scenerii po nazwie (torów, eventów), używane jest
drzewo zamiast listy.

Wpisy w scenerii, przetwarzane jako komendy Forth, nie mogą używać średnika jako
separatora słów. Uniemożliwia to używanie średnika jako słowa.
*/
