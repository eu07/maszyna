unit mctools;
{rozne takie duperele do operacji na stringach w paszczalu, pewnie w delfi sa lepsze}
{konwersja zmiennych na stringi, funkcje matematyczne, logiczne, lancuchowe, I/O etc}

interface


const
{Ra: te sta³e nie s¹ u¿ywane...
        _FileName = ['a'..'z','A'..'Z',':','\','.','*','?','0'..'9','_','-'];
        _RealNum  = ['0'..'9','-','+','.','E','e'];
        _Integer  = ['0'..'9','-']; //Ra: to siê gryzie z STLport w Builder 6
        _Plus_Int = ['0'..'9'];
        _All      = [' '..'þ'];
        _Delimiter= [',',';']+_EOL;
        _Delimiter_Space=_Delimiter+[' '];
}
        _EOL      = [#13,#10];
        _SPACE    = ' ';
        _Spacesigns=[' ']+[#9]+_EOL;
        CutLeft=-1; CutRight=1; CutBoth=0;  {Cut_Space}

var
        ConversionError: integer=0;
        LineCount: integer=0;
        DebugModeFlag: boolean=False;
        FreeFlyModeFlag: boolean=False;

type
        TableChar = Set Of Char;  {MCTUTIL}

{konwersje}
function  b2s(b:byte):string;
function  i2s(i:integer):string;
function  l2s(l:longint):string;
function  r2s(r:real; SWidth:byte):string;
function  s2b(s:string):byte;
function  s2i(s:string):integer;
function  s2l(s:string):longint;
function  s2r(s:string):real;
function  s2bE(s:string):byte;
function  s2iE(s:string):integer;
function  s2lE(s:string):longint;
function  s2rE(s:string):real;

{funkcje matematyczne}
function  Max0(x1,x2:byte) : byte;
function  Min0(x1,x2:byte) : byte;

function  Max0R(x1,x2:real) : real;
function  Min0R(x1,x2:real) : real;

function  Sign(x:real) : integer;

{funkcje logiczne}
function TestFlag(Flag:integer; Value:integer):boolean;
function  SetFlag(var Flag:byte; Value:integer):boolean;
function  iSetFlag(var Flag:integer; Value:integer):boolean;

function  FuzzyLogic(Test,Threshold,Probability:real):boolean;
{jesli Test>Threshold to losowanie}
function FuzzyLogicAI(Test,Threshold,Probability:real):boolean;
{to samo ale zawsze niezaleznie od DebugFlag}

{operacje na stringach}
Function  ReadWord(var infile:text):string; {czyta slowo z wiersza pliku tekstowego}
Function  Ups (s:string ): string ;
Function  Cut_Space(s:string; Just:integer) : string;
Function  ExtractKeyWord(InS:String; KeyWord:string) : String;   {wyciaga slowo kluczowe i lancuch do pierwszej spacji}
Function  DUE(S:String) : String;  {Delete Until Equal sign}
Function  DWE(S:String) : String;  {Delete While Equal sign}
Function  Ld2Sp(S:String): String; {Low dash to Space sign}
Function  Tab2Sp(S:String): String; {Tab to Space sign}

{procedury, zmienne i funkcje graficzne}
procedure ComputeArc(X0,Y0,Xn,Yn,R,L:real;dL:real; var phi,Xout,Yout:real);
{wylicza polozenie Xout Yout i orientacje phi punktu na elemencie dL luku}
procedure ComputeALine(X0,Y0,Xn,Yn,L,R:real; var Xout,Yout:real);


var
 Xmin,Ymin,Xmax,Ymax:real;
 Xaspect,Yaspect:real;
 Hstep,Vstep: real;
 Vsize,Hsize:integer;

function Xhor( h:real) :real;
       { Converts horizontal screen coordinate into real X-coordinate. }

function Yver( v:real) :real;
       { Converts vertical screen coordinate into real Y-coordinate. }

function Horiz(X:real):longint;

function Vert(Y:real):longint;

procedure ClearPendingExceptions;

{------------------------------------------------}
Implementation
{================================================}

function Ups (s:string ): string ;
var
  jatka : integer; swy:string;
begin
 swy:='';
 for jatka:=1 to length(s) do swy:=swy+UpCase(s[jatka]);
 Ups:=swy;
end;   {=Ups=}

function b2s(b:byte):string;
var
  s:string;
begin
  Str(b:2,s);
  b2s:=s;
end;    {=b2s=}

function i2s(i:integer):string;
var
  s:string;
begin
  Str(i:3,s);
  i2s:=s;
end;    {=i2s=}

function l2s(l:longint):string;
var
  s:string;
begin
  Str(l:4,s);
  l2s:=s;
end;    {=l2s=}

function  r2s(r:real; SWidth:byte):string;
var
  s:string;
  absr:real;                      {rr:integer;}
begin
  absr:=Abs(r);
  if SWidth<8 then SWidth:=8;
  if absr > 0.0001 then
   begin                          {rr:=(Round(ln(1/absr)/ln(10)-1+Swidth/2));}
     if absr < 100 then
       Str(r:Swidth:(Round(ln(1/absr)/ln(10)-1+Swidth/2)),s)
      else Str(r:Swidth:1,s)
   end
    else Str(r:Swidth,s);
{  if r>0 then s:=insert('+'s,3);}
  r2s:=s;
end;    {=r2s=}

function  s2b(s:string):byte;
var e:integer;
    b:byte;
begin
  Val(s,b,e);
   s2b:=b;
end;

function  s2i(s:string):integer;
var e:integer;
    i:integer;
begin
  Val(s,i,e);
   s2i:=i;
end;

function  s2l(s:string):longint;
var e:integer;
    l:longint;
begin
  Val(s,l,e);
  s2l:=l;
end;

function  s2r(s:string):real;
var e:integer;
    r:real;
begin
  Val(s,r,e);
  s2r:=r;
end;


function  s2bE(s:string):byte;
var e:integer;
    b:byte;
begin
  s2bE:=0;
  Val(s,b,e);
  if e>0 then
   ConversionError:=e
  else s2bE:=b;
end;

function  s2iE(s:string):integer;
var e:integer;
    i:integer;
begin
  s2iE:=0;
  Val(s,i,e);
  if e>0 then
   ConversionError:=e
  else s2iE:=i;
end;

function  s2lE(s:string):longint;
var e:integer;
    l:longint;
begin
  s2lE:=0;
  Val(s,l,e);
  if e>0 then
   ConversionError:=e
  else s2lE:=l;
end;

function  s2rE(s:string):real;
var e:integer;
    r:real;
begin
  s2rE:=0;
  Val(s,r,e);
  if e>0 then
   ConversionError:=e
  else s2rE:=r;
end;


function Max0(x1,x2:byte) : byte;
begin
if x1>x2 then
 Max0:=x1
else
 Max0:=x2;
end;

function Min0(x1,x2:byte) : byte;
begin
if x1<x2 then
 Min0:=x1
else
 Min0:=x2;
end;

function Max0R(x1,x2:real) : real;
begin
if x1>x2 then
 Max0R:=x1
else
 Max0R:=x2;
end;

function Min0R(x1,x2:real) : real;
begin
if x1<x2 then
 Min0R:=x1
else
 Min0R:=x2;
end;

function Sign(x:real) : integer;
begin
  Sign:=0;
  if x>0 then Sign:=1;
  if x<0 then Sign:=-1;
end;

function TestFlag(Flag:integer; Value:integer):boolean;
begin
  if (Flag and Value)=Value then
   TestFlag:=True
  else TestFlag:=False;
end;

function SetFlag(var Flag:byte; Value:integer):boolean;
begin
  SetFlag:=False;
  if Value>0 then
   if (Flag and Value)=0 then
    begin
      Flag:=Flag+Value;
      SetFlag:=True
    end;
  if Value<0 then
   if (Flag and Abs(Value))=Abs(Value) then
    begin
      Flag:=Flag+Value;
      SetFlag:=True
    end;
end;

function iSetFlag(var Flag:integer; Value:integer):boolean;
begin
  iSetFlag:=False;
  if Value>0 then
   if (Flag and Value)=0 then
    begin
      Flag:=Flag+Value;
      iSetFlag:=True; //true, gdy by³o wczeœniej 0 i zosta³o ustawione
    end;
  if Value<0 then
   if (Flag and Abs(Value))=Abs(Value) then
    begin
      Flag:=Flag+Value; //Value jest ujemne, czyli zerowanie flagi
      iSetFlag:=True; //true, gdy by³o wczeœniej 1 i zosta³o wyzerowane
    end;
end;


function FuzzyLogic(Test,Threshold,Probability:real):boolean;
begin
  if (Test>Threshold) and (not DebugModeFlag) then
   FuzzyLogic:=(Random<Probability*Threshold/Test) {im wiekszy Test tym wieksza szansa}
  else FuzzyLogic:=False;
end;

function FuzzyLogicAI(Test,Threshold,Probability:real):boolean;
begin
  if (Test>Threshold) then
   FuzzyLogicAI:=(Random<Probability*Threshold/Test) {im wiekszy Test tym wieksza szansa}
  else FuzzyLogicAI:=False;
end;

Function ReadWord(var infile:text):string;
var s:string; c:char; nextword:boolean;
begin
  s:='';
  nextword:=False;
  while (not eof(infile)) and (not nextword) do
   begin
     read(infile,c);
     if c in _SpaceSigns then
      if s<>'' then
       nextword:=True;
     if not (c in _Spacesigns) then
      s:=s+c;
   end;
  ReadWord:=s;
end;


Function  Cut_Space(s:string; Just:integer) : string;
var ii:byte;
begin
  case Just of
    CutLeft : begin
                ii:=0;
                while (ii<length(s)) and (s[ii+1]=_SPACE) do
                 inc(ii);
                S:=Copy(s,ii+1,length(s)-ii);
              end;
    CutRight: begin
                ii:=length(s);
                while(ii>0) and (s[ii]=_SPACE) do
                 dec(ii);
                S:=Copy(s,1,ii);
              end;
    CutBoth : begin
                S:=Cut_Space(S,CutLeft);
                S:=Cut_Space(S,CutRight);
              end;
  end;
  Cut_Space:=S;
end;
{Cut_Space}

Function  ExtractKeyWord(InS:String; KeyWord:string) : String;
var s:string; kwp:byte;
begin
  Ins:=Ins+' ';
  kwp:=Pos(KeyWord,InS);
  if kwp>0 then
   begin
    s:=Copy(InS,kwp,Length(Ins));
    s:=Copy(s,1,Pos(' ',s)-1);
   end
    else s:='';
  ExtractKeyWord:=s;
end;

Function  DUE(S:String) : String;  {Delete Until Equal sign}
begin
  DUE:=Copy(S,Pos('=',S)+1,Length(S));
end;

Function  DWE(S:String) : String;  {Delete While Equal sign}
var ep:byte;
begin
  ep:=Pos('=',S);
  if ep>0 then
   DWE:=Copy(S,1,ep-1)
  else DWE:=S;
end;

Function  Ld2Sp(S:String): String; {Low dash to Space sign}
var b:byte; s2:string;
begin
  s2:='';
  for b:=1 to length(S) do
   if S[b]='_' then
     s2:=s2+' '
    else
     s2:=s2+S[b];
  Ld2Sp:=s2;
end;

Function  Tab2Sp(S:String): String; {Tab to Space sign}
var b:byte; s2:string;
begin
  s2:='';
  for b:=1 to length(S) do
   if S[b]=#9 then
     s2:=s2+' '
    else
     s2:=s2+S[b];
  Tab2Sp:=s2;
end;


procedure ComputeArc(X0,Y0,Xn,Yn,R,L:real;dL:real; var phi,Xout,Yout:real);
{wylicza polozenie Xout Yout i orientacje phi punktu na elemencie dL luku}
var dX,dY,Xc,Yc,gamma,alfa,AbsR:real;
begin
  if (R<>0) and (L<>0) then
   begin
     AbsR:=Abs(R);
     dX:=Xn-X0;
     dY:=Yn-Y0;
     if dX<>0 then gamma:=arctan(dY/dX)
      else if dY>0 then gamma:=Pi/2
            else gamma:=3*Pi/2;
     alfa:=L/R;
     phi:=gamma-(alfa+Pi*Round(R/AbsR))/2;
     Xc:=X0-AbsR*cos(phi);
     Yc:=Y0-AbsR*sin(phi);
     phi:=phi+alfa*dL/L;
     Xout:=AbsR*cos(phi)+Xc;
     Yout:=AbsR*sin(phi)+Yc;
   end;
end;

procedure ComputeALine(X0,Y0,Xn,Yn,L,R:real; var Xout,Yout:real);
var dX,dY,gamma,alfa:real;
{   pX,pY : real;}
begin
  alfa:=0; //ABu: bo nie bylo zainicjowane
  dX:=Xn-X0;
  dY:=Yn-Y0;
  if dX<>0 then gamma:=arctan(dY/dX)
   else if dY>0 then gamma:=Pi/2
    else gamma:=3*Pi/2;
  if R<>0 then alfa:=L/R;
  Xout:=X0+L*cos(alfa/2-gamma);
  Yout:=Y0+L*sin(alfa/2-gamma);
end;

{graficzne:}


function Xhor( h:real) :real;
  begin
    Xhor:= h*Hstep + Xmin;
  end;

function Yver( v:real) :real;
  begin
    Yver:= (Vsize-v)*Vstep + Ymin;
  end;

function Horiz(X:real):longint;
begin
     x:= (x-Xmin) / Hstep;
     if x > -MaxInt then
                      if x < MaxInt then Horiz:=Round(x)
                         else Horiz:= MaxInt
        else Horiz:= -MaxInt;
end;

function Vert(Y:real):longint;
begin
     y:= (y-Ymin) / Vstep;
     if y > -MaxInt then
                      if y < MaxInt then Vert:=Vsize-Round(y)
                         else Vert:= MaxInt
        else Vert:= -MaxInt
end;

procedure ClearPendingExceptions;
//resetuje b³êdy FPU, wymagane dla Trunc()
asm
  FNCLEX
end;

end.