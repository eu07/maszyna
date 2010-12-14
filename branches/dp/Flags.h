#ifndef FlagsH
#define FlagsH

#define Nothing	    0x00000000
#define bit01	    0x00000001
#define bit02	    0x00000002
#define bit03	    0x00000004
#define bit04	    0x00000008
#define bit05	    0x00000010
#define bit06	    0x00000020
#define bit07   	0x00000040
#define bit08	    0x00000080
#define bit09	    0x00000100
#define bit10	    0x00000200
#define bit11	    0x00000400
#define bit12	    0x00000800
#define bit13	    0x00001000
#define bit14	    0x00002000
#define bit15   	0x00004000
#define bit16	    0x00008000
#define bit17	    0x00010000
#define bit18	    0x00020000
#define bit19	    0x00040000
#define bit20	    0x00080000
#define bit21	    0x00100000
#define bit22	    0x00200000
#define bit23   	0x00400000
#define bit24	    0x00800000
#define bit25	    0x01000000
#define bit26	    0x02000000
#define bit27	    0x04000000
#define bit28	    0x08000000
#define bit29	    0x10000000
#define bit30	    0x20000000
#define bit31   	0x40000000
#define bit32	    0x80000000

#define GetFlag(flags,bit) (flags & bit)

#define FlagOn(flags,bit) (flags|= bit)
#define FlagOff(flags,bit) (flags&= ~bit)

//typedef int TFlags32;
class TFlags32
{
private:
    int Flags;
public:
    inline void __fastcall Reset(NFlags=0) { Flags= NFlags; };
    inline bool __fastcall Get(int bit) { return(GetFlag(Flags,bit)); };
    inline void __fastcall TurnOn(int bit) { FlagOn(Flags,bit); };
    inline void __fastcall TurnOff(int bit) { FlagOff(Flags,bit); };
    AnsiString __fastcall AsString() { return IntToHex(Flags,8); };
};

#endif
