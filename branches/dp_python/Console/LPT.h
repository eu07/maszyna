//---------------------------------------------------------------------------

#ifndef LPTH
#define LPTH
//---------------------------------------------------------------------------

class TLPT
{
private:
 int address;
public:
 bool __fastcall Connect(int port);
 void __fastcall Out(int x);
};
#endif
