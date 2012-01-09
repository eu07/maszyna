unit Unit1;

interface

uses
  ai_driver,mtable,mover,mctools,Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    Label1: TLabel;
    Button2: TButton;
    Button3: TButton;
    Button4: TButton;
    Button5: TButton;
    Button6: TButton;
    Label2: TLabel;
    Label3: TLabel;
    Timer1: TTimer;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    Label8: TLabel;
    Label9: TLabel;
    Label10: TLabel;
    Label11: TLabel;
    Button7: TButton;
    Button8: TButton;
    Label12: TLabel;
    Label13: TLabel;
    Label14: TLabel;
    Label15: TLabel;
    Button9: TButton;
    Button10: TButton;
    Label16: TLabel;
    Label17: TLabel;
    Label18: TLabel;
    Label19: TLabel;
    Button11: TButton;
    Label20: TLabel;
    Label21: TLabel;
    Label22: TLabel;
    Label23: TLabel;
    Label24: TLabel;
    Label25: TLabel;
    lczas: TLabel;
    Label26: TLabel;
    Label27: TLabel;
    Label28: TLabel;
    Label29: TLabel;
    Label30: TLabel;
    Label31: TLabel;
    Label32: TLabel;
    Label33: TLabel;
    Label34: TLabel;
    Button12: TButton;
    BStart: TButton;
    Label35: TLabel;
    Label36: TLabel;
    Label37: TLabel;
    Label38: TLabel;
    Button13: TButton;
    Label39: TLabel;
    Label40: TLabel;
    Label41: TLabel;
    Button14: TButton;
    Button15: TButton;
    Label42: TLabel;
    Label43: TLabel;
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure Button8Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button9Click(Sender: TObject);
    procedure Button10Click(Sender: TObject);
    procedure Button11Click(Sender: TObject);
    procedure Button12Click(Sender: TObject);
    procedure BStartClick(Sender: TObject);
    procedure Button13MouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure Button14Click(Sender: TObject);
    procedure Button15Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
Form1: TForm1;
loc,wag,wag2,wag3:TMoverParameters;
mechanik: TController;
pociag: TTrainParameters;
l0,l1,l2,l3:TLocation;
r0,r1,r2,r3:TRotation;
Shape:TTrackShape; Track:TTrackParam;
ExternalVoltage:real;
ActualTime:real;
fout: text;

implementation

{$R *.DFM}

procedure TForm1.Button1Click(Sender: TObject);
begin
assignfile(fout,'log.dat');
rewrite(fout);
ActualTime:=0;
loc:=TMoverParameters.Create;
wag:=TMoverParameters.Create;
wag2:=TMoverParameters.Create;
wag3:=TMoverParameters.Create;
mechanik:=TController.Create;
pociag:=TTrainParameters.Create;
Shape.R:=0; Shape.Len:=10000; Shape.dHtrack:=0; Shape.dHrail:=0;
Track.Width:=1435; Track.friction:=0.15; Track.CategoryFlag:=1;
Track.QualityFlag:=25; Track.DamageFlag:=0;
Track.VelMax:=120;
ExternalVoltage:=3000;
  l0.x:=0; l0.y:=0; l0.z:=0;
  l1.x:=-14.0235; l1.y:=0; l1.z:=0;
  l2.x:=-27.3634; l2.y:=0; l2.z:=0;
  l3.x:=-40.7032; l3.y:=0; l3.z:=0;
  r0.rx:=0; r0.ry:=0; r0.rz:=0;
  r1.rx:=0; r1.ry:=0; r1.rz:=0;
  r2.rx:=0; r2.ry:=0; r2.rz:=0;
  r3.rx:=0; r3.ry:=0; r3.rz:=0;
{  loc.Init(l1,r1,0,'en57','456',0,'Human',1); }
  loc.Init(l0,r0,60,'eu07','456',0,'',1);
  wag.Init(l1,r1,60,'Falns_440v','52-51-342027-2',30,'',0);
 wag2.Init(l2,r2,60,'Falns_440v','52-51-322311-5',30,'',0);
 wag3.Init(l3,r3,60,'Falns_440v','52-51-410025-3',30,'',0);
  pociag.init('Testowy Express',100);
  mechanik.init(l1,r1,True,@loc,@pociag,Aggressive);
  if (loc.loadchkfile('') and wag.loadchkfile('') and wag2.loadchkfile('') and wag3.loadchkfile(''))
   and (loc.CheckLocomotiveParameters(Go) and wag.CheckLocomotiveParameters(Go) and wag2.CheckLocomotiveParameters(Go) and wag3.CheckLocomotiveParameters(Go)) then
   begin
     Label1.Caption:='OK';
     loc.attach(2,@wag,3);
     wag.attach(1,@loc,3);
     wag.attach(2,@wag2,3);
     wag2.attach(1,@wag,3);
     wag2.attach(2,@wag3,3);
     wag3.attach(1,@wag2,3);
     Timer1.Enabled:=True;
   end
  else Label1.Caption:=inttostr(conversionerror)+' '+inttostr(linecount);
  Button1.Visible:=False;
end;

procedure TForm1.Button2Click(Sender: TObject);
begin
  if Timer1.Enabled then
   begin
     Timer1.Enabled:=False;
     CloseFile(fout);
   end;
  loc.Free;
  wag.Free;
  wag2.free;
  wag3.free;
  mechanik.free;
  Close;
end;

procedure TForm1.Timer1Timer(Sender: TObject);
var dl,dt:real;
begin
  dt:=Timer1.Interval/1000;
  ActualTime:=ActualTime+dt;
  loc.ComputeTotalForce(dt,Shape,Track,ExternalVoltage);
  wag.ComputeTotalForce(dt,Shape,Track,ExternalVoltage);
  wag2.ComputeTotalForce(dt,Shape,Track,ExternalVoltage);
  wag3.ComputeTotalForce(dt,Shape,Track,ExternalVoltage);
  dl:=loc.ComputeMovement(dt,Shape,Track,ExternalVoltage,l0,r0);
  l0.x:=l0.x+dl;
  dl:=wag.ComputeMovement(dt,Shape,Track,ExternalVoltage,l1,r1);
  l1.x:=l1.x+dl;
  dl:=wag2.ComputeMovement(dt,Shape,Track,ExternalVoltage,l2,r2);
  l2.x:=l2.x+dl;
  dl:=wag3.ComputeMovement(dt,Shape,Track,ExternalVoltage,l3,r3);
  l3.x:=l3.x+dl;

  if l0.x<500 then mechanik.UpdateSituation(500-l0.x,dt)
  else
   if l0.x<2500 then mechanik.UpdateSituation(-1,dt)
    else
     if l0.x<3500 then mechanik.UpdateSituation(3500-l0.x,dt);
  if (l0.x>499) and (l0.x<505) then
   mechanik.SetVelocity(-1,-1,0);
  if (l0.x>2499) and (l0.x<2510) then
   mechanik.SetVelocity(-1,0,0);

  Label2.Caption:=r2s(Loc.Im,0)+' A';
  Label3.Caption:=r2s(Loc.Ft/1000,0)+' kN';
  Label4.Caption:=r2s(l0.x,0)+' m';
  Label5.Caption:=r2s(Sign(Loc.V)*Loc.Vel,0)+' km/h';
  Label6.Caption:=r2s(Loc.AccS,0)+' m/ss';
  Label7.Caption:=r2s(Loc.couplers[1].cforce/1000,0)+' kN';
  Label8.Caption:=r2s(Loc.couplers[2].cforce/1000,0)+' kN';
  Label9.Caption:=r2s(Loc.Fb/1000,0)+' kN';
  Label10.Caption:=r2s(Loc.Compressor,0)+' MPa';
  Label11.Caption:=r2s(Loc.PipePress,0)+' MPa';
  Label12.Caption:=r2s(Loc.BrakePress,0)+' MPa';

  Label13.Caption:=IntToStr(Loc.MainCtrlPos);
  Label14.Caption:=IntToStr(Loc.BrakeCtrlPos);
  Label15.Caption:=IntToStr(Loc.LocalBrakePos);

  Label16.Caption:=IntToStr(Loc.showcurrent(1));
  Label17.Caption:=IntToStr(Loc.showcurrent(2));
  Label18.Caption:=IntToStr(Loc.ScndCtrlPos);
  if loc.SlippingWheels then
   Label19.Caption:='Poslizg!'
  else Label19.Caption:=' ';

  Label20.Caption:=IntToStr(Loc.MainCtrlActualPos);
  Label21.Caption:=IntToStr(Loc.ScndCtrlActualPos);

  Label22.Caption:=r2s(l1.x,0)+' m';
  Label23.Caption:=r2s(Sign(wag.V)*wag.Vel,0)+' km/h';
  Label24.Caption:=r2s(Distance(loc.loc,wag.loc,loc.dim,wag.dim),0)+' m';
  Label25.Caption:=r2s(loc.nrot,0)+' 1/s';

  Label26.Caption:=r2s(wag.couplers[1].cforce/1000,0)+' kN';
  Label27.Caption:=r2s(wag.couplers[2].cforce/1000,0)+' kN';

  Label28.Caption:=r2s(l2.x,0)+' m';
  Label29.Caption:=r2s(sign(wag2.V)*wag2.Vel,0)+' km/h';

  Label30.Caption:=r2s(loc.dpLocalValve/dt,0)+' MPa/s';
  Label31.Caption:=r2s(loc.dpBrake/dt,0)+' MPa/s';
  Label32.Caption:=r2s(loc.dpMainValve/dt,0)+' MPa/s';
  Label33.Caption:=r2s(loc.dpPipe/dt,0)+' MPa/s';

  Label34.Caption:=r2s(wag2.PipePress,0)+' MPa';

  Label35.Caption:=r2s(l3.x,0)+' m';
  Label36.Caption:=r2s(sign(wag3.V)*wag3.Vel,0)+' km/h';
  Label37.Caption:=r2s(wag3.couplers[1].cforce/1000,0)+' kN';
  Label38.Caption:=r2s(wag3.brakepress,0)+' MPa';

  Label39.Caption:=r2s(loc.enginePower/1000,0)+' kW';
  Label40.Caption:=i2s(trunc(loc.Rventrot*60))+' /min';
  Label41.Visible:=loc.SandDose;
  Label42.Caption:=r2s(mechanik.accdesired,0)+' m/ss';
  Label43.Caption:=r2s(mechanik.veldesired,0)+' km/h';
  Lczas.Caption:=r2s(actualtime,0)+' s';
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  Timer1.Enabled:=False;
end;

procedure TForm1.Button5Click(Sender: TObject);
begin
 loc.incbrakelevel(1);
end;

procedure TForm1.Button6Click(Sender: TObject);
begin
 loc.decbrakelevel(1);
end;

procedure TForm1.Button7Click(Sender: TObject);
begin
 loc.inclocalbrakelevel(1);
end;

procedure TForm1.Button8Click(Sender: TObject);
begin
 loc.declocalbrakelevel(1);
end;

procedure TForm1.Button3Click(Sender: TObject);
begin
  loc.incmainctrl(1);
end;

procedure TForm1.Button4Click(Sender: TObject);
begin
  loc.decmainctrl(1);
end;

procedure TForm1.Button9Click(Sender: TObject);
begin
loc.DirectionForward;
end;

procedure TForm1.Button10Click(Sender: TObject);
begin
  loc.DirectionBackward;
end;

procedure TForm1.Button11Click(Sender: TObject);
begin
  loc.FuseOn;
end;

procedure TForm1.Button12Click(Sender: TObject);
begin
  Loc.AntiSlippingBrake;
end;

procedure TForm1.BStartClick(Sender: TObject);
begin
  loc.activedir:=1;
{  if mechanik.OrderDirectionChange(1) then }
   mechanik.SetVelocity(60,0,0);
end;

procedure TForm1.Button13MouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  loc.SandDoseOn;
end;

procedure TForm1.Button14Click(Sender: TObject);
begin
  mechanik.SetVelocity(30,120,0);
end;

procedure TForm1.Button15Click(Sender: TObject);
begin
  mechanik.SetVelocity(0,120,0);
end;

end.
