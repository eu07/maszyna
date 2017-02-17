/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef ButtonH
#define ButtonH

#include <string>
#include "Model3d.h"
#include "parser.h"

class TButton
{ // animacja dwustanowa, włącza jeden z dwóch submodeli (jednego
    // z nich może nie być)
  private:
    TSubModel *pModelOn, *pModelOff; // submodel dla stanu załączonego i wyłączonego
    bool bOn;
    bool *bData;
    int iFeedbackBit; // Ra: bit informacji zwrotnej, do wyprowadzenia na pulpit

  public:
    TButton();
    ~TButton();
    void Clear(int i = -1);
    inline void FeedbackBitSet(int i)
    {
        iFeedbackBit = 1 << i;
    };
    inline void Turn(bool to)
    {
        bOn = to;
        Update();
    };
    inline void TurnOn()
    {
        bOn = true;
        Update();
    };
    inline void TurnOff()
    {
        bOn = false;
        Update();
    };
    inline void Switch()
    {
        bOn = !bOn;
        Update();
    };
    inline bool Active()
    {
        return (pModelOn) || (pModelOff);
    };
    void Update();
    void Init(std::string const &asName, TModel3d *pModel, bool bNewOn = false);
    void Load(cParser &Parser, TModel3d *pModel1, TModel3d *pModel2 = NULL);
    void AssignBool(bool *bValue);
};

//---------------------------------------------------------------------------
#endif
