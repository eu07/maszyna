unit Unit1;

interface

uses
  ai_driver,mtable,mover,mctools,Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
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
    Label42: TLabel;
    Label43: TLabel;
    Button14: TButton;
    Button15: TButton;
    Label44: TLabel;
    Label45: TLabel;
    Label46: TLabel;
    Label47: TLabel;
    BMains: TButton;
    Label48: TLabel;
    Label49: TLabel;
    Label50: TLabel;
    Panel1: TPanel;
    Label1: TLabel;
    OpenDialog1: TOpenDialog;
    Edit1: TEdit;
    GroupBox1: TGroupBox;
    CheckBox1: TCheckBox;
    BSInc: TButton;
    BSDec: TButton;
    Label51: TLabel;
    Label52: TLabel;
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
    procedure BMainsClick(Sender: TObject);
    procedure CheckBox1Click(Sender: TObject);
    procedure BSIncClick(Sender: TObject);
    procedure BSDecClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

const
mtno=30;
tno:integer=0;

var
Form1: TForm1;
loc:TMoverParameters;
wagony:array[1..mtno] of TMoverParameters;
mechanik: TController;
pociag: TTrainParameters;
couplerflag:byte;
l0,l0p:TLocation;
ll:array[1..mtno] of TLocation;
r0:TRotation;
rr:array[1..mtno] of TRotation;
Shape:TTrackShape; Track:TTrackParam;
ExternalVoltage:real;
ActualTime:real;
fout: text;
OK:boolean;

implementation

{$R *.DFM}

procedure TForm1.Button1Click(Sender: TObject);
var typename,path:string;slashpos:byte;
begin

couplerflag:=strtoint(Edit1.Text);
if tno>0 then
 wagony[tno]:=TMoverParameters.Create;

OpenDialog1.Execute;
path:='';
typename:=OpenDialog1.Filename;
repeat
  slashpos:=pos('\',typename);
  if slashpos>0 then
   typename:=copy(typename,slashpos+1,length(typename));
until slashpos=0;
path:=copy(opendialog1.filename,1,length(opendialog1.filename)-length(typename));
typename:=Copy(typename,1,Pos('.chk',typename)-1);

if tno=0 then
 begin
  loc.Init(l0,r0,0,typename,'lokomotywa',0,'',1);
  OK:=loc.loadchkfile(path) and loc.CheckLocomotiveParameters(Go);
  if OK then
   begin
     Label1.Caption:=loc.TypeName;
     case loc.AutoRelayType of
      2: CheckBox1.Enabled:=True;
      1: CheckBox1.Checked:=True;
     end;
   end;
 end
else
 begin
  wagony[tno].Init(ll[tno],rr[tno],0,typename,IntToStr(tno),0,'',0);
  OK:=OK and (wagony[tno].loadchkfile(path) and wagony[tno].CheckLocomotiveParameters(Go));
  if OK then
   begin
    Label1.Caption:=Label1.Caption+' '+wagony[tno].TypeName;
    if tno=1 then
      ll[tno].x:=loc.Loc.x-loc.Dim.L/2-wagony[tno].Dim.L/2
    else
      ll[tno].x:=wagony[tno-1].Loc.x-wagony[tno-1].Dim.L/2-wagony[tno].Dim.L/2;
    wagony[tno].loc:=ll[tno];
    if tno=1 then
     begin
       OK:=OK and loc.attach(2,@wagony[1],couplerflag);
       OK:=OK and wagony[1].attach(1,@loc,couplerflag);
     end
    else
     begin
       OK:=OK and wagony[tno-1].attach(2,@wagony[tno],couplerflag);
       OK:=OK and wagony[tno].attach(1,@wagony[tno-1],couplerflag);
     end
   end;
 end;

  if OK then
   begin
     BStart.Enabled:=True;
     if tno=mtno then button1.enabled:=false
      else inc(tno);
     Label49.Caption:='ilosc pojazdow '+inttostr(tno);      
   end
  else
   begin
    Label1.Caption:=inttostr(conversionerror)+' '+inttostr(linecount);
    Button1.Enabled:=False;
   end;
end;

procedure TForm1.Button2Click(Sender: TObject);
var iw:integer;
begin
  if Timer1.Enabled then
   begin
     Timer1.Enabled:=False;
     CloseFile(fout);
   end;
  mechanik.CloseLog; 
  mechanik.free;
  loc.Free;
  for iw:=1 to tno do
   wagony[iw].Free;
  Close;
end;

procedure TForm1.Timer1Timer(Sender: TObject);
var dl,dt:real; iw:integer;
begin
  dt:=Timer1.Interval/1000;
  ActualTime:=ActualTime+dt;
  if l0.x>200 then Shape.R:=1000;
  if l0.x>1000 then Shape.R:=-500;
  if l0.x>1500 then Shape.R:=0;
  if l0.x>2000 then Shape.R:=1500;
  if l0.x>2200 then Shape.R:=3000;
  if l0.x>2400 then Shape.R:=0;
  loc.ComputeTotalForce(dt);
  for iw:=1 to tno do
   wagony[iw].ComputeTotalForce(dt);
  dl:=loc.ComputeMovement(dt,Shape,Track,ExternalVoltage,l0,r0);
  l0.x:=l0.x+dl;
  for iw:=1 to tno do
   begin
    dl:=wagony[iw].ComputeMovement(dt,Shape,Track,ExternalVoltage,ll[iw],rr[iw]);
    ll[iw].x:=ll[iw].x+dl;
   end;

  mechanik.UpdateSituation(dt);


  l0p.x:=500; l0p.y:=2; l0.z:=0;
  if (l0.x>-1) and (l0.x<5) then
   loc.PutCommand('SetVelocity',60,-1,l0p);
  if (l0.x>499) and (l0.x<505) then
   loc.PutCommand('SetVelocity',-1,-1,l0);
  l0p.x:=3600;
  if (l0.x>2985) and (l0.x<3005) then
     loc.PutCommand('SetVelocity',-1,0,l0p);
  if (l0.x>3600) and (l0.x<3615) then
   loc.PutCommand('SetVelocity',0,0,l0);
  if (l0.x>3430) and (loc.Vel<0.01) and (mechanik.GetCurrentOrder=Obey_train) then
   loc.PutCommand('SetVelocity',0,0,l0);

  if (loc.power>0) then
   begin
     Label2.Caption:='I0='+r2s(Loc.Im,0)+' A';
     Label3.Caption:=r2s(Loc.Ft/1000,0)+' kN';
     Label13.Caption:=IntToStr(Loc.MainCtrlPos);
     Label18.Caption:=IntToStr(Loc.ScndCtrlPos);
     Label20.Caption:=IntToStr(Loc.MainCtrlActualPos);
     Label21.Caption:=IntToStr(Loc.ScndCtrlActualPos);
   end
  else
  for iw:=1 to tno do
   if wagony[iw].power>0 then
    begin
     Label2.Caption:='I'+IntToStr(iw)+'='+r2s(wagony[iw].Im,0)+' A';
     Label3.Caption:=r2s(wagony[iw].Ft/1000,0)+' kN';
     Label13.Caption:=IntToStr(wagony[iw].MainCtrlPos);
     Label18.Caption:=IntToStr(wagony[iw].ScndCtrlPos);
     Label20.Caption:=IntToStr(wagony[iw].MainCtrlActualPos);
     Label21.Caption:=IntToStr(wagony[iw].ScndCtrlActualPos);
    end;
  Label4.Caption:=r2s(l0.x,0)+' m';
  Label5.Caption:=r2s(Sign(Loc.V)*Loc.Vel,0)+' km/h';
  Label6.Caption:=r2s(Loc.AccS,0)+' m/ss';
  Label9.Caption:=r2s(Loc.Fb/1000,0)+' kN';
  Label10.Caption:=r2s(Loc.Compressor,0)+' MPa';
  Label11.Caption:=r2s(Loc.PipePress,0)+' MPa';
  Label12.Caption:=r2s(Loc.BrakePress,0)+' MPa';


  Label14.Caption:=IntToStr(Loc.BrakeCtrlPos);
  Label15.Caption:=IntToStr(Loc.LocalBrakePos);

  Label16.Caption:=IntToStr(Loc.showcurrent(1));
  Label17.Caption:=IntToStr(Loc.showcurrent(2));

  if loc.SlippingWheels then
   Label19.Caption:='Poslizg!'
  else Label19.Caption:=' ';


  Label22.Caption:=r2s(ll[1].x,0)+' m';
  if tno>=1 then
   begin
    Label23.Caption:=r2s(Sign(wagony[1].V)*wagony[1].Vel,0)+' km/h';
    Label24.Caption:=r2s(Distance(loc.loc,wagony[1].loc,loc.dim,wagony[1].dim),0)+' m';
   end;
  Label25.Caption:=r2s(loc.nrot,0)+' 1/s';

  Label28.Caption:=r2s(ll[2].x,0)+' m';

  if tno>=2 then
   Label29.Caption:=r2s(sign(wagony[2].V)*wagony[2].Vel,0)+' km/h';

  Label30.Caption:=r2s(loc.dpLocalValve/dt,0)+' MPa/s';
  Label31.Caption:=r2s(loc.dpBrake/dt,0)+' MPa/s';
  Label32.Caption:=r2s(loc.dpMainValve/dt,0)+' MPa/s';
  Label33.Caption:=r2s(loc.dpPipe/dt,0)+' MPa/s';

  if tno>0 then Label34.Caption:=r2s(wagony[tno].PipePress,0)+' MPa';

  Label35.Caption:=r2s(ll[3].x,0)+' m';
  if tno>0 then Label36.Caption:=r2s(sign(wagony[tno].V)*wagony[tno].Vel,0)+' km/h';
  if tno>0 then Label37.Caption:=r2s(wagony[tno].couplers[1].cforce/1000,0)+' kN';
  if tno>0 then Label38.Caption:=r2s(wagony[tno].brakepress,0)+' MPa';

  Label39.Caption:=r2s(loc.enginePower/1000,0)+' kW';
  Label40.Caption:=i2s(trunc(loc.Rventrot*60))+' /min';
  Label41.Visible:=loc.SandDose;
  Label42.Caption:=r2s(mechanik.accdesired,0)+' m/ss';
  Label43.Caption:=r2s(mechanik.veldesired,0)+' km/h';
  if tno>0 then Label44.Caption:=r2s(ll[tno].x,0)+' m';
  Label45.Caption:=inttostr(loc.activedir);
  Label46.Caption:=inttostr(ord(mechanik.orderlist[mechanik.orderpos]));
  Label47.Caption:=inttostr(mechanik.orderpos);
  Label48.Caption:=inttostr(ord(loc.mains));
{  if tno>0 then Label51.Caption:=wagony[1].CommandIn.Command;
  if tno>0 then Label52.Caption:=r2s(wagony[1].CommandIn.Value1,0);
}  
  Label51.Caption:=loc.CommandIn.Command;
  Label52.Caption:=r2s(loc.CommandIn.Value1,0);
  Lczas.Caption:=r2s(actualtime,0)+' s';
  writeln(fout,Actualtime,' ',{loc.v-wag.v,}loc.UnitBrakeForce,' ',loc.dpMainValve,{' ',loc.dpLocalValve,}' ',loc.accs,' ',loc.couplers[2].cforce,' ',{,l1.x}' '{,Distance(loc.loc,wagony[1].loc,loc.dim,wagony[1].dim)});
end;

procedure TForm1.FormCreate(Sender: TObject);
var iw:integer;
begin
  Timer1.Enabled:=False;
  tno:=0;
  assignfile(fout,'log.dat');
  rewrite(fout);
  ActualTime:=0;
  loc:=TMoverParameters.Create;
  Shape.R:=0; Shape.Len:=10000; Shape.dHtrack:=0; Shape.dHrail:=0;
  Track.Width:=1435; Track.friction:=0.15; Track.CategoryFlag:=1;
  Track.QualityFlag:=25; Track.DamageFlag:=0;
  Track.VelMax:=120;
  ExternalVoltage:=3000;
  l0.x:=0; l0.y:=0; l0.z:=0;
  r0.rx:=0; r0.ry:=0; r0.rz:=0;
  for iw:=1 to mtno do
   begin
    rr[iw].rx:=0; rr[iw].ry:=0; rr[iw].rz:=0;
    ll[iw].x:=0; ll[iw].y:=0; ll[iw].z:=0;
   end;

end;

procedure TForm1.Button5Click(Sender: TObject);
begin
 loc.incbrakelevel;
end;

procedure TForm1.Button6Click(Sender: TObject);
begin
 loc.decbrakelevel;
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
  tno:=tno-1;
  Button1.Enabled:=False;
  Edit1.Enabled:=False;
  Timer1.Enabled:=True;
  mechanik:=TController.Create;
  pociag:=TTrainParameters.Create;
  pociag.init('Testowy Express',100);
  mechanik.init(l0,r0,HumanDriver,@loc,@pociag,Aggressive);

  BStart.Enabled:=False;
  Button14.enabled:=True;
  Button15.enabled:=True;
end;

procedure TForm1.Button13MouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  loc.SandDoseOn;
end;

procedure TForm1.Button14Click(Sender: TObject);
begin
  mechanik.SetVelocity(30,120);
end;

procedure TForm1.Button15Click(Sender: TObject);
begin
  mechanik.AIControllFlag:=not mechanik.AIControllFlag;
  if not loc.mains then
   begin
     mechanik.ChangeOrder(Prepare_Engine);
     mechanik.JumpToNextOrder;
     mechanik.ChangeOrder(Obey_train);
     mechanik.JumpToNextOrder;
     mechanik.ChangeOrder(Release_engine);
     mechanik.JumpToFirstOrder;
     mechanik.SetVelocity(60,120);
   end;
end;

procedure TForm1.BMainsClick(Sender: TObject);
begin
  loc.MainSwitch;
end;

procedure TForm1.CheckBox1Click(Sender: TObject);
begin
  loc.autorelayswitch;
end;

procedure TForm1.BSIncClick(Sender: TObject);
begin
  loc.incscndctrl(1);
end;

procedure TForm1.BSDecClick(Sender: TObject);
begin
  loc.decscndctrl(1);
end;

end.
