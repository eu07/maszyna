//---------------------------------------------------------------------------

#ifndef FeedbackH
#define FeedbackH
//---------------------------------------------------------------------------
class TFeedback
{//Ra: Obiekt generuj¹cy informacje zwrotne o stanie symulacji
private:
 int iBits; //podstawowy zestaw lampek
 void __fastcall BitsUpdate();
public:
 __fastcall TFeedback();
 __fastcall ~TFeedback();
 void __fastcall BitsSet(int mask);
 void __fastcall BitsClear(int mask);
};
#endif
