//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Forth.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

/* Ra: Forth jest prostym jêzykiem programowania, funkcjonuj¹cym na dosyæ
niskim poziomie. Mo¿e byæ zarówno interpretowany jak i kompilowany, co
pozwala szybko dzia³aæ skompilowanym programom, jak równie¿ wykonywaæ
polecenia wprowadzane z klawiatury. Za pomoc¹ zdefiniowanych kompilatorów
mo¿na definiowaæ nowe funkcje, mo¿na równie¿ tworzyæ nowe struktury jezyka,
np. wykraczaj¹ce poza dotychczasow¹ sk³adniê. Prostota i wydajnoœæ s¹ okupione
nieco specyficzn¹ sk³adni¹, wynikaj¹c¹ z u¿ycia stosu do obliczeñ (tzw. odwrotna
notacja polska).

Forth ma byæ u¿ywany jako:
 1. Alternatywna postaæ eventów. Dotychczasowe eventy w sceneriach mog¹ byæ
traktowane jako funkcje jêzyka Forth. Jednoczeœnie mo¿na uzyskaæ uelastycznienie
sk³adni, poprzez np. po³¹czenie w jednym wpisie ró¿nych typów eventów wraz z
dodaniem warunków dostêpnych tylko dla Multiple. Równie¿ mo¿na bêdzie u¿ywaæ
wyra¿eñ jêzyka Forth, czyli dokonywaæ obliczeñ.
 2. Jako docelowa postaæ schematów obwodów Ladder Diagram. Mo¿na zrobiæ tak, by
program Ÿród³owy w Forth (przy pewnych ograniczeniach) by³ wyœwietlany graficznie
jako Ladder Diagram. Jednoczeœnie graficzne modyfikacje schematu mo¿na by
prze³o¿yæ na postaæ tekstow¹ Forth i skompilowaæ na kod niskiego poziomu.
 3. Jako algorytm funkcjonowania AI, mo¿liwy do zaprogramowania na zewn¹tzr EXE.
Ka¿dy u¿ytkownik bêdzie móg³ swój w³asny styl jazdy.
 4. Jako algorytm sterowania ruchem (zale¿noœci na stacji).
 5. Jako AI stacji, zarz¹dzaj¹ce ruchem na stacji zale¿nie w czasie rzeczywistym.

Wzglêdem pierwotnego jêzyka Forth, u¿yty tutaj bêdzie mia³ pewne odstêpstwa:
 1. Operacje na liczbach czterobajtowych float. Nale¿y we w³asnym zakresie dbaæ
o zgodnoœæ u¿ytych funkcji z typem argumentu na stosie.
 2. Jedynym typem ca³kowitym jest czterobajtowy int.
 3. Do wyszukiwania obiektów w scenerii po nazwie (torów, eventów), u¿ywane jest
drzewo zamiast listy.

Wpisy w scenerii, przetwarzane jako komendy Forth, nie mog¹ u¿ywaæ œrednika jako
separatora s³ów. Uniemo¿liwia to u¿ywanie œrednika jako s³owa.
*/
