unit mtable;

interface uses mctools,sysutils;

const MaxTTableSize= 100;
      hrsd= '.';
Type

   TMTableLine = record
                   km: real;                  {kilometraz linii}
                   vmax: real;                {predkosc rozkladowa}
                   StationName: string[32];   {nazwa stacji ( '_' zamiast spacji}
                   StationWare: string[32];   {typ i wyposazenie stacji, oddz. przecinkami}
                   TrackNo: byte;             {ilosc torow szlakowych}
                   Ah, Am: integer;              {godz. i min. przyjazdu}
                   Dh, Dm: integer;              {godz. i min. odjazdu}
                   tm: real;                  {czas jazdy do tej stacji w min.}
                   WaitTime: integer;         {czas postoju}
                 end;

   TMTable = array[0..MaxTTableSize] of TMTableLine;

   PTrainParameters= ^TTrainParameters;

   TTrainParameters = class(TObject)
                        TrainName:string;
                        TTVmax:real;
                        Relation1,Relation2: string;
                        BrakeRatio: real;
                        LocSeries: string;
                        LocLoad: real;
                        TimeTable: TMTable;
                        StationCount: integer;
                        StationIndex: integer;
                        NextStationName: string;
                        LastStationLatency: real;
                        Direction: integer;        {kierunek jazdy w/g kilometrazu}                        
                        function CheckTrainLatency: real;
                        {todo: str hh:mm to int i z powrotem}
                        function ShowRelation: string;
                        function WatchMTable(DistCounter:real): real;
                        function UpdateMTable(hh,mm:real; NewName: string): boolean;                        
                        constructor Init(NewTrainName:string);
                        function LoadTTfile(scnpath:string): boolean;
                      end;

   TMTableTime = class(TObject)
                   GameTime: real;
                   dd,hh,mm: integer;
                   srh,srm : integer; {wschod slonca}
	           ssh,ssm : integer; {zachod slonca}
                   procedure UpdateMTableTime(deltaT:real);
                   constructor Init(InitH,InitM,InitSRH,InitSRM,InitSSH,InitSSM:integer);
                   private
                   mr:real;
                 end;

var GlobalTime: TMTableTime;

implementation

function CompareTime(t1h,t1m,t2h,t2m:real):real; {roznica czasu w minutach, nie uwzglednia zmiany daty!!!}
begin
  CompareTime:=(t2h-t1h)*60+t2m-t1m;
end;

function TTrainParameters.CheckTrainLatency: real;
begin
  if (LastStationLatency>1.0) or (LastStationLatency<-1.0) then
   CheckTrainLatency:=LastStationLatency {spoznienie + lub do przodu - z tolerancja 1 min}
  else
   CheckTrainLatency:=0
end;

function TTrainParameters.WatchMTable(DistCounter:real): real;
var dist:real;
begin
  if Direction=1 then
   dist:=TimeTable[StationIndex].km-TimeTable[0].km-DistCounter
  else
   dist:=TimeTable[0].km-TimeTable[StationIndex].km-DistCounter;
  WatchMTable:=dist; {jaka odleglosc do najblizszej stacji gdzie sie ma zatrzymac}
end;

function TTrainParameters.UpdateMTable(hh,mm:real; NewName: string): boolean;
var OK: boolean;
begin
  OK:=false;
  if StationIndex<StationCount then {albo <= !!!}
   begin
     if TimeTable[1].km-TimeTable[0].km<0 then
       Direction:=-1
      else
       Direction:=1; {prowizorka bo moze byc zmiana kilometrazu}
     if NewName=TimeTable[StationIndex+1].StationName then
      begin
        LastStationLatency:=CompareTime(hh,mm,TimeTable[StationIndex].dh,TimeTable[StationIndex].dm);
        inc(StationIndex);
        NextStationName:=TimeTable[StationIndex].StationName;
        OK:=true;
      end;
   end;
  UpdateMTable:=OK; {czy jest nastepna stacja}
end;


function TTrainParameters.ShowRelation: string;
begin
  if (Relation1=TimeTable[1].StationName) and (Relation2=TimeTable[StationCount].StationName)
   then ShowRelation:=Relation1+' - '+Relation2
  else ShowRelation:='';
end;

constructor TTrainParameters.Init(NewTrainName:string);
var i:integer;
begin
  TrainName:=NewTrainName;
  StationCount:=0;
  StationIndex:=0;
  NextStationName:='undefined';
  LastStationLatency:=0;
  Direction:=1;
  Relation1:=''; Relation2:='';
  for i:=0 to MaxTTableSize do
   with timeTable[i] do
    begin
      km:=0;  vmax:=-1; StationName:='undefined'; StationWare:='';
      TrackNo:=1;  Ah:=-1; Am:=-1; Dh:=-1; Dm:=-1; tm:=0; WaitTime:=0;
    end;
  TTVmax:=100; {wykasowac}
end;

function TTrainParameters.LoadTTfile(scnpath:string): boolean;
var
  lines,s: string;
  fin: text;
  EndTable:boolean;
  vActual:real;
procedure UpdateVelocity(StationCount: integer; vActual:real);
var i: integer;
begin
  i:=StationCount;
  TTVmax:=vActual;  {PROWIZORKA!!!}
  while (i>0) and (TimeTable[i-1].vmax=-1) do
   begin
     dec(i);
     TimeTable[i].vmax:=vActual;
   end;
end;
begin
  ConversionError:=666;
  s:=scnpath+TrainName+'.txt';
  assignfile(fin,s);
  s:='';
  EndTable:=False;
  vActual:=-1;
{$I-}
  reset(fin);
{$I+}
  if IOresult<>0 then
   begin
     ConversionError:=-8;
   end
  else
   begin
     ConversionError:=0;
     while not (eof(fin) or (ConversionError<>0) or EndTable) do
      begin
        readln(fin,lines);
        if Pos('___________________',lines)>0 then
        if ReadWord(fin)='[' then
         if ReadWord(fin)='Rodzaj' then
          repeat
          until (ReadWord(fin)='|') or (eof(fin));
          s:=ReadWord(fin);
          if LowerCase(s)<>TrainName then
            ConversionError:=-7
          else
           begin  {czytaj naglowek}
             repeat
             until (Pos('_______|',ReadWord(fin))>0) or eof(fin);
             repeat
             until (ReadWord(fin)='[') or (eof(fin));
             repeat
               s:=ReadWord(fin);
             until (s<>'|') or eof(fin);
             if s<>'|' then Relation1:=s
              else ConversionError:=-5;
             repeat
             until (Readword(fin)='Relacja') or (eof(fin));
             repeat
             until (ReadWord(fin)='|') or (eof(fin));
             Relation2:=ReadWord(fin);
             repeat
             until Readword(fin)='Wymagany';
             repeat
             until (ReadWord(fin)='|') or (eoln(fin));
             s:=ReadWord(fin);
             s:=Copy(s,1,Pos('%',s)-1);
             BrakeRatio:=s2rE(s);
             repeat
             until Readword(fin)='Seria';
             repeat
             until (ReadWord(fin)='|') or (eof(fin));
             LocSeries:=ReadWord(fin);
             LocLoad:=s2rE(ReadWord(fin));
             repeat
             until (Pos('[______________',ReadWord(fin))>0) or (eof(fin));
             while not eof(fin) and not EndTable do
              begin
                inc(StationCount);
                repeat
                  s:=ReadWord(fin);
                until (s='[') or (eof(fin));
                with TimeTable[StationCount] do
                 begin
                   if s='[' then
                    s:=ReadWord(fin)
                   else conversionError:=-4;
                   if Pos('|',s)=0 then
                    begin
                      km:=s2rE(s);
                      s:=ReadWord(fin);
                    end;
                   if Pos('|_____|',s)>0 then {zmiana predkosci szlakowej}
                      UpdateVelocity(StationCount,vActual)
                   else
                    begin
                      s:=ReadWord(fin);
                      if Pos('|',s)=0 then
                       vActual:=s2rE(s);
                    end;
                   while Pos('|',s)=0 do
                    s:=Readword(fin);
                   StationName:=ReadWord(fin);
                   repeat
                     s:=ReadWord(fin);
                   until (s='1') or (s='2') or eof(fin);
                   TrackNo:=s2bE(s);
                   s:=ReadWord(fin);
                   if s<>'|' then
                    begin
                      if Pos(hrsd,s)>0 then
                       begin
                         ah:=s2iE(Copy(s,1,Pos(hrsd,s)-1));
                         am:=s2iE(Copy(s,Pos(hrsd,s)+1,Length(s)));
                       end
                      else
                       begin
                         ah:=TimeTable[StationCount-1].ah;
                         am:=s2iE(s); {tylko minuty podane}
                       end;
                    end;
                   repeat
                     s:=ReadWord(fin);
                   until (s<>'|') or (eof(fin));
                   if s<>']' then
                    tm:=s2rE(s);
                   repeat
                    s:=ReadWord(fin);
                   until (s='[') or eof(fin);
                   s:=ReadWord(fin);
                   if Pos('|',s)=0 then
                    begin
{tu s moze byc miejscem zmiany predkosci szlakowej}
                      s:=ReadWord(fin);
                    end;
                   if Pos('|_____|',s)>0 then {zmiana predkosci szlakowej}
                      UpdateVelocity(StationCount,vActual)
                   else
                    begin
                      s:=ReadWord(fin);
                      if Pos('|',s)=0 then
                       vActual:=s2rE(s);
                    end;
                   while Pos('|',s)=0 do
                    s:=Readword(fin);
                   StationWare:=ReadWord(fin);
                   repeat
                     s:=ReadWord(fin);
                   until (s='1') or (s='2') or eof(fin);
                   TrackNo:=s2bE(s);
                   s:=ReadWord(fin);
                   if s<>'|' then
                    begin
                      if Pos(hrsd,s)>0 then
                       begin
                         dh:=s2iE(Copy(s,1,Pos(hrsd,s)-1));
                         dm:=s2iE(Copy(s,Pos(hrsd,s)+1,Length(s)));
                       end
                      else
                       begin
                         dh:=TimeTable[StationCount-1].dh;
                         dm:=s2iE(s); {tylko minuty podane}
                       end;
                      WaitTime:=Trunc(CompareTime(ah,am,dh,dm)+0.1);
                    end;
                   repeat
                     s:=ReadWord(fin);
                   until (s<>'|') or (eof(fin));
                   if s<>']' then
                    tm:=s2rE(s);
                   repeat
                    s:=ReadWord(fin);
                   until (Pos('[',s)>0) or eof(fin);
                   if Pos('_|_',s)=0 then
                    s:=Readword(fin);
                   if Pos('|',s)=0 then
                    begin
{tu s moze byc miejscem zmiany predkosci szlakowej}
                      s:=ReadWord(fin);
                    end;
                   if Pos('|_____|',s)>0 then {zmiana predkosci szlakowej}
                      UpdateVelocity(StationCount,vActual)
                   else
                    begin
                      s:=ReadWord(fin);
                      if Pos('|',s)=0 then
                       vActual:=s2rE(s);
                    end;
{                   while Pos('|',s)=0 do
                    s:=Readword(fin);    }
                   while (Pos(']',s)=0) do
                     s:=ReadWord(fin);
                   if Pos('_|_',s)>0 then EndTable:=True;
                 end; {timetableline}
              end;
           end;
      end; {while eof}
   end;
   if ConversionError=0 then
    begin
      NextStationName:=TimeTable[1].StationName;
{      TTVmax:=TimeTable[1].vmax;  }
    end;
   LoadTTfile:=(ConversionError=0);
end;

procedure TMTableTime.UpdateMTableTime(deltaT:real);
begin
  mr:=mr+deltaT;
  if mr>60.0 then
   begin
     mr:=mr-60.0;
     inc(mm);
   end;
  if mm>59 then
   begin
     mm:=0;
     inc(hh);
   end;
  if hh>23 then
   begin
     hh:=0;
     inc(dd);
   end;
  GameTime:=GameTime+deltaT;
end;

constructor TMTableTime.Init(InitH,InitM,InitSRH,InitSRM,InitSSH,InitSSM:integer);
begin
  GameTime:=0.0;
  dd:=0;
  hh:=InitH;
  mm:=InitM;
  srh:=InitSRH;
  srm:=InitSRM;
  ssh:=InitSSH;
  ssm:=InitSSM;
end;


END.





