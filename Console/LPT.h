//---------------------------------------------------------------------------

#ifndef LPTH
#define LPTH
//---------------------------------------------------------------------------

class TLPT
{
  private:
    int address;

  public:
    bool Connect(int port);
    void Out(int x);
};
#endif
