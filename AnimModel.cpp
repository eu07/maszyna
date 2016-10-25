/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "AnimModel.h"
#include "usefull.h"
#include "Timer.h"
#include "MdlMngr.h"
// McZapkie:
#include "Texture.h"
#include "Globals.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

TAnimAdvanced::TAnimAdvanced(){};

TAnimAdvanced::~TAnimAdvanced(){
    // delete[] pVocaloidMotionData; //plik zosta³ zmodyfikowany
};

int TAnimAdvanced::SortByBone()
{ // sortowanie pliku animacji w celu optymalniejszego wykonania
    // rekordy zostaj¹ u³o¿one wg kolejnych ramek dla ka¿dej koœci
    // u³o¿enie koœci alfabetycznie nie jest niezbêdne, ale upraszcza sortowanie b¹belkowe
    TAnimVocaloidFrame buf; // bufor roboczy (przyda³o by siê pascalowe Swap()
    int i, j, k, swaps = 0, last = iMovements - 1, e;
    for (i = 0; i < iMovements; ++i)
        for (j = 0; j < 15; ++j)
            if (pMovementData[i].cBone[j] == '\0') // jeœli znacznik koñca
                for (++j; j < 15; ++j)
                    pMovementData[i].cBone[j] = '\0'; // zerowanie bajtów za znacznikiem koñca
    for (i = 0; i < last; ++i) // do przedostatniego
    { // dopóki nie posortowane
        j = i; // z którym porównywaæ
        k = i; // z którym zamieniæ (nie trzeba zamieniaæ, jeœli ==i)
        while (++j < iMovements)
        {
            e = strcmp(pMovementData[k].cBone,
                       pMovementData[j].cBone); // numery trzeba porównywaæ inaczej
            if (e > 0)
                k = j; // trzeba zamieniæ - ten pod j jest mniejszy
            else if (!e)
                if (pMovementData[k].iFrame > pMovementData[j].iFrame)
                    k = j; // numer klatki pod j jest mniejszy
        }
        if (k > i)
        { // jeœli trzeba przestawiæ
            // buf=pMovementData[i];
            // pMovementData[i]=pMovementData[k];
            // pMovementData[k]=buf;
            memcpy(&buf, pMovementData + i, sizeof(TAnimVocaloidFrame));
            memcpy(pMovementData + i, pMovementData + k, sizeof(TAnimVocaloidFrame));
            memcpy(pMovementData + k, &buf, sizeof(TAnimVocaloidFrame));
            ++swaps;
        }
    }
    return swaps;
};

TAnimContainer::TAnimContainer()
{
    pNext = NULL;
    vRotateAngles = vector3(0.0f, 0.0f, 0.0f); // aktualne k¹ty obrotu
    vDesiredAngles = vector3(0.0f, 0.0f, 0.0f); // docelowe k¹ty obrotu
    fRotateSpeed = 0.0;
    vTranslation = vector3(0.0f, 0.0f, 0.0f); // aktualne przesuniêcie
    vTranslateTo = vector3(0.0f, 0.0f, 0.0f); // docelowe przesuniêcie
    fTranslateSpeed = 0.0;
    fAngleSpeed = 0.0;
    pSubModel = NULL;
    iAnim = 0; // po³o¿enie pocz¹tkowe
    pMovementData = NULL; // nie ma zaawansowanej animacji
    mAnim = NULL; // nie ma macierzy obrotu dla submodelu
    evDone = NULL; // powiadamianie o zakoñczeniu animacji
    acAnimNext = NULL; // na razie jest poza list¹
}

TAnimContainer::~TAnimContainer()
{
    SafeDelete(pNext);
    delete mAnim; // AnimContainer jest w³aœcicielem takich macierzy
}

bool TAnimContainer::Init(TSubModel *pNewSubModel)
{
    fRotateSpeed = 0.0f;
    pSubModel = pNewSubModel;
    return (pSubModel != NULL);
}

void TAnimContainer::SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed)
{
    if (!this)
        return; // wywo³ywane z eventu, gdy brak modelu
    vDesiredAngles = vNewRotateAngles;
    fRotateSpeed = fNewRotateSpeed;
    iAnim |= 1;
    /* //Ra 2014-07: jeœli model nie jest renderowany, to obliczyæ czas animacji i dodaæ event
     wewnêtrzny
     //mo¿na by te¿ ustawiæ czas pocz¹tku animacji zamiast pobieraæ czas ramki i liczyæ ró¿nicê
    */
    if (evDone)
    { // do³¹czyæ model do listy aniomowania, ¿eby animacje by³y przeliczane równie¿ bez
        // wyœwietlania
        if (iAnim >= 0)
        { // jeœli nie jest jeszcze na liœcie animacyjnej
            acAnimNext = TAnimModel::acAnimList; // pozosta³e dokliæ sobie jako ogon
            TAnimModel::acAnimList = this; // a wstawiæ siê na pocz¹tek
            iAnim |= 0x80000000; // dodany do listy
        }
    }
}

void TAnimContainer::SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed)
{
    if (!this)
        return; // wywo³ywane z eventu, gdy brak modelu
    vTranslateTo = vNewTranslate;
    fTranslateSpeed = fNewSpeed;
    iAnim |= 2;
    /* //Ra 2014-07: jeœli model nie jest renderowany, to obliczyæ czas animacji i dodaæ event
     wewnêtrzny
     //mo¿na by te¿ ustawiæ czas pocz¹tku animacji zamiast pobieraæ czas ramki i liczyæ ró¿nicê
    */
    if (evDone)
    { // do³¹czyæ model do listy aniomowania, ¿eby animacje by³y przeliczane równie¿ bez
        // wyœwietlania
        if (iAnim >= 0)
        { // jeœli nie jest jeszcze na liœcie animacyjnej
            acAnimNext = TAnimModel::acAnimList; // pozosta³e dokliæ sobie jako ogon
            TAnimModel::acAnimList = this; // a wstawiæ siê na pocz¹tek
            iAnim |= 0x80000000; // dodany do listy
        }
    }
}

void TAnimContainer::AnimSetVMD(double fNewSpeed)
{
    if (!this)
        return; // wywo³ywane z eventu, gdy brak modelu
    // skala do ustalenia, "cal" japoñski (sun) to nieco ponad 3cm
    // X-w lewo, Y-w górê, Z-do ty³u
    // minimalna wysokoœæ to -7.66, a nadal musi byæ ponad pod³og¹
    // if (pMovementData->iFrame>0) return; //tylko pierwsza ramka
    vTranslateTo = vector3(0.1 * pMovementData->f3Vector.x, 0.1 * pMovementData->f3Vector.z,
                           0.1 * pMovementData->f3Vector.y);
    if (LengthSquared3(vTranslateTo) > 0.0 ? true : LengthSquared3(vTranslation) > 0.0)
    { // jeœli ma byæ przesuniête albo jest przesuniêcie
        iAnim |= 2; // wy³¹czy siê samo
        if (fNewSpeed > 0.0)
            fTranslateSpeed = fNewSpeed; // prêdkoœæ jest mno¿nikiem, nie podlega skalowaniu
        else // za póŸno na animacje, trzeba przestawiæ
            vTranslation = vTranslateTo;
    }
    // if ((qCurrent.w<1.0)||(pMovementData->qAngle.w<1.0))
    { // jeœli jest jakiœ obrót
        if (!mAnim)
        {
            mAnim = new float4x4(); // bêdzie potrzebna macierz animacji
            mAnim->Identity(); // jedynkowanie na pocz¹tek
        }
        iAnim |= 4; // animacja kwaternionowa
        qStart = qCurrent; // potrzebna pocz¹tkowa do interpolacji
        //---+ - te¿ niby dobrze, ale nie tak tr¹ca w³osy na pocz¹tku (macha w dó³)
        //-+-+ - d³oñ ma w górze zamiast na pasie w pozycji pocz¹tkowej
        //+--+ - g³owa do ty³u (broda w górê) w pozycji pocz¹tkowej
        //--++ - pozycja pocz¹tkowa dobra, tr¹ca u góry, ale z rêkami jakoœ nie tak, kó³ko w
        // przeciwn¹ stronê
        //++++ - k³adzie siê brzuchem do góry
        //-+++ - rêce w górze na pocz¹tku, zamiast w dó³, ³okieæ jakby w przeciwn¹ stronê
        //+-++ - nie podnosi rêki do g³owy
        //++-+ - d³oñ ma w górze zamiast na pasie
        qDesired = Normalize(float4(-pMovementData->qAngle.x, -pMovementData->qAngle.z,
                                    -pMovementData->qAngle.y,
                                    pMovementData->qAngle.w)); // tu trzeba bêdzie osie zamieniæ
        if (fNewSpeed > 0.0)
        {
            fAngleSpeed = fNewSpeed; // wtedy animowaæ za pomoc¹ interpolacji
            fAngleCurrent = 0.0; // pocz¹tek interpolacji
        }
        else
        { // za póŸno na animacjê, mo¿na tylko przestawiæ w docelowe miejsce
            fAngleSpeed = 0.0;
            fAngleCurrent = 1.0; // interpolacja zakoñczona
            qCurrent = qDesired;
        }
    }
    // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak g³ówna koœæ
    // if (!strcmp(pSubModel->pName,"¶‚Â‚Ü?æ‚h‚j")) //IK lewej stopy
    // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(pMovementData->f3Vector.x)+"
    // "+AnsiString(pMovementData->f3Vector.y)+" "+AnsiString(pMovementData->f3Vector.z));
}

void TAnimContainer::UpdateModel()
{ // przeliczanie animacji wykonaæ tylko raz na model
    if (pSubModel) // pozbyæ siê tego - sprawdzaæ wczeœniej
    {
        if (fTranslateSpeed != 0.0)
        {
            vector3 dif = vTranslateTo - vTranslation; // wektor w kierunku docelowym
            double l = LengthSquared3(dif); // d³ugoœæ wektora potrzebnego przemieszczenia
            if (l >= 0.0001)
            { // jeœli do przemieszczenia jest ponad 1cm
                vector3 s = SafeNormalize(dif); // jednostkowy wektor kierunku
                s = s *
                    (fTranslateSpeed *
                     Timer::GetDeltaTime()); // przemieszczenie w podanym czasie z dan¹ prêdkoœci¹
                if (LengthSquared3(s) < l) //¿eby nie jecha³o na drug¹ stronê
                    vTranslation += s;
                else
                    vTranslation = vTranslateTo; // koniec animacji, "koniec animowania" uruchomi
                // siê w nastêpnej klatce
            }
            else
            { // koniec animowania
                vTranslation = vTranslateTo;
                fTranslateSpeed = 0.0; // wy³¹czenie przeliczania wektora
                if (LengthSquared3(vTranslation) <= 0.0001) // jeœli jest w punkcie pocz¹tkowym
                    iAnim &= ~2; // wy³¹czyæ zmianê pozycji submodelu
                if (evDone)
                    Global::AddToQuery(evDone, NULL); // wykonanie eventu informuj¹cego o
                // zakoñczeniu
            }
        }
        if (fRotateSpeed != 0)
        {

            /*
               double dif= fDesiredAngle-fAngle;
               double s= fRotateSpeed*sign(dif)*Timer::GetDeltaTime();
               if ((abs(s)-abs(dif))>0)
                   fAngle= fDesiredAngle;
               else
                   fAngle+= s;

               while (fAngle>360) fAngle-= 360;
               while (fAngle<-360) fAngle+= 360;
               pSubModel->SetRotate(vRotateAxis,fAngle);
            */

            bool anim = false;
            vector3 dif = vDesiredAngles - vRotateAngles;
            double s;
            s = fRotateSpeed * sign(dif.x) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.x))
                vRotateAngles.x = vDesiredAngles.x;
            else
            {
                vRotateAngles.x += s;
                anim = true;
            }
            s = fRotateSpeed * sign(dif.y) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.y))
                vRotateAngles.y = vDesiredAngles.y;
            else
            {
                vRotateAngles.y += s;
                anim = true;
            }
            s = fRotateSpeed * sign(dif.z) * Timer::GetDeltaTime();
            if (fabs(s) >= fabs(dif.z))
                vRotateAngles.z = vDesiredAngles.z;
            else
            {
                vRotateAngles.z += s;
                anim = true;
            }
            while (vRotateAngles.x >= 360)
                vRotateAngles.x -= 360;
            while (vRotateAngles.x <= -360)
                vRotateAngles.x += 360;
            while (vRotateAngles.y >= 360)
                vRotateAngles.y -= 360;
            while (vRotateAngles.y <= -360)
                vRotateAngles.y += 360;
            while (vRotateAngles.z >= 360)
                vRotateAngles.z -= 360;
            while (vRotateAngles.z <= -360)
                vRotateAngles.z += 360;
            if (vRotateAngles.x == 0.0)
                if (vRotateAngles.y == 0.0)
                    if (vRotateAngles.z == 0.0)
                        iAnim &= ~1; // k¹ty s¹ zerowe
            if (!anim)
            { // nie potrzeba przeliczaæ ju¿
                fRotateSpeed = 0.0;
                if (evDone)
                    Global::AddToQuery(evDone, NULL); // wykonanie eventu informuj¹cego o
                // zakoñczeniu
            }
        }
        if (fAngleSpeed != 0.0)
        { // obrót kwaternionu (interpolacja)
        }
    }
};

void TAnimContainer::PrepareModel()
{ // tutaj zostawiæ tylko ustawienie submodelu, przeliczanie ma byæ w UpdateModel()
    if (pSubModel) // pozbyæ siê tego - sprawdzaæ wczeœniej
    {
        // nanoszenie animacji na wzorzec
        if (iAnim & 1) // zmieniona pozycja wzglêdem pocz¹tkowej
            pSubModel->SetRotateXYZ(vRotateAngles); // ustawia typ animacji
        if (iAnim & 2) // zmieniona pozycja wzglêdem pocz¹tkowej
            pSubModel->SetTranslate(vTranslation);
        if (iAnim & 4) // zmieniona pozycja wzglêdem pocz¹tkowej
        {
            if (fAngleSpeed > 0.0f)
            {
                fAngleCurrent +=
                    fAngleSpeed * Timer::GetDeltaTime(); // aktualny parametr interpolacji
                if (fAngleCurrent >= 1.0f)
                { // interpolacja zakoñczona, ustawienie na pozycjê koñcow¹
                    qCurrent = qDesired;
                    fAngleSpeed = 0.0; // wy³¹czenie przeliczania wektora
                    if (evDone)
                        Global::AddToQuery(evDone,
                                           NULL); // wykonanie eventu informuj¹cego o zakoñczeniu
                }
                else
                { // obliczanie pozycji poœredniej
                    // normalizacja jest wymagana do interpolacji w nastêpnej animacji
                    qCurrent = Normalize(
                        Slerp(qStart, qDesired, fAngleCurrent)); // interpolacja sferyczna k¹ta
                    // qCurrent=Slerp(qStart,qDesired,fAngleCurrent); //interpolacja sferyczna k¹ta
                    if (qCurrent.w ==
                        1.0) // rozpoznaæ brak obrotu i wy³¹czyæ w iAnim w takim przypadku
                        iAnim &= ~4; // k¹ty s¹ zerowe
                }
            }
            mAnim->Quaternion(&qCurrent); // wype³nienie macierzy (wymaga normalizacji?)
            pSubModel->mAnimMatrix = mAnim; // u¿yczenie do submodelu (na czas renderowania!)
        }
    }
    // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak g³ówna koœæ
    // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(iAnim)+"
    // "+AnsiString(vTranslation.x)+" "+AnsiString(vTranslation.y)+" "+AnsiString(vTranslation.z));
}

void TAnimContainer::UpdateModelIK()
{ // odwrotna kinematyka wyliczana dopiero po ustawieniu macierzy w submodelach
    if (pSubModel) // pozbyæ siê tego - sprawdzaæ wczeœniej
    {
        if (pSubModel->b_Anim & at_IK)
        { // odwrotna kinematyka
            float3 d, k;
            TSubModel *ch = pSubModel->ChildGet();
            switch (pSubModel->b_Anim)
            {
            case at_IK11: // stopa: ustawiæ w kierunku czubka (pierwszy potomny)
                d = ch->Translation1Get(); // wektor wzglêdem aktualnego uk³adu (nie uwzglêdnia
                // obrotu)
                k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0,
                           -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
                pSubModel->SetRotateIK1(k);
                // if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak g³ówna koœæ
                // WriteLog("--> "+AnsiString(k.x)+" "+AnsiString(k.y)+" "+AnsiString(k.z));
                // Ra: to ju¿ jest dobrze, mo¿e byæ inna æwiartka i znak
                break;
            case at_IK22: // udo: ustawiæ w kierunku pierwszej potomnej pierwszej potomnej (kostki)
                // pozycjê kostki nale¿y okreœliæ wzglêdem koœci centralnej (+biodro mo¿e byæ
                // pochylone)
                // potem wyliczyæ ewentualne odchylenie w tej i nastêpnej
                // w sumie to proste, jak wyznaczenie k¹tów w trójk¹cie o znanej d³ugoœci boków...
                d = ch->Translation2Get(); // wektor wzglêdem aktualnego uk³adu (nie uwzglêdnia
                // obrotu)
                // if ()
                { // koœæ IK jest dalej ni¿ pozycja spoczynkowa
                    k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0,
                               -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
                    pSubModel->SetRotateIK1(k);
                }
                break;
            }
        }
    }
}

bool TAnimContainer::InMovement()
{ // czy trwa animacja - informacja dla obrotnicy
    return (fRotateSpeed != 0.0) || (fTranslateSpeed != 0.0);
}

void TAnimContainer::EventAssign(TEvent *ev)
{ // przypisanie eventu wykonywanego po zakoñczeniu animacji
    evDone = ev;
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

TAnimModel::TAnimModel()
{
    pRoot = NULL;
    pModel = NULL;
    iNumLights = 0;
    fBlinkTimer = 0;
    ReplacableSkinId[0] = 0;
    ReplacableSkinId[1] = 0;
    ReplacableSkinId[2] = 0;
    ReplacableSkinId[3] = 0;
    ReplacableSkinId[4] = 0;
    for (int i = 0; i < iMaxNumLights; i++)
    {
        LightsOn[i] = LightsOff[i] = NULL; // normalnie nie ma
        lsLights[i] = ls_Off; // a jeœli s¹, to wy³¹czone
    }
    vAngle.x = vAngle.y = vAngle.z = 0.0; // zerowanie obrotów egzemplarza
    pAdvanced = NULL; // nie ma zaawansowanej animacji
    fDark = 0.25; // standardowy próg zaplania
    fOnTime = 0.66;
    fOffTime = fOnTime + 0.66;
}

TAnimModel::~TAnimModel()
{
    delete pAdvanced; // nie ma zaawansowanej animacji
    SafeDelete(pRoot);
}

bool TAnimModel::Init(TModel3d *pNewModel)
{
    fBlinkTimer = double(random(1000 * fOffTime)) / (1000 * fOffTime);
    ;
    pModel = pNewModel;
    return (pModel != NULL);
}

bool TAnimModel::Init(AnsiString asName, AnsiString asReplacableTexture)
{
    if (asReplacableTexture.SubString(1, 1) ==
        "*") // od gwiazdki zaczynaj¹ siê teksty na wyœwietlaczach
        asText = asReplacableTexture.SubString(2, asReplacableTexture.Length() -
                                                      1); // zapamiêtanie tekstu
    else if (asReplacableTexture != "none")
        ReplacableSkinId[1] =
            TTexturesManager::GetTextureID(NULL, NULL, asReplacableTexture.c_str());
    if (TTexturesManager::GetAlpha(ReplacableSkinId[1]))
        iTexAlpha =
            0x31310031; // tekstura z kana³em alfa - nie renderowaæ w cyklu nieprzezroczystych
    else
        iTexAlpha = 0x30300030; // tekstura nieprzezroczysta - nie renderowaæ w cyklu
    // przezroczystych
    return (Init(TModelsManager::GetModel(asName.c_str())));
}

bool TAnimModel::Load(cParser *parser, bool ter)
{ // rozpoznanie wpisu modelu i ustawienie œwiate³
    std::string str;
    std::string token;
    parser->getTokens(); // nazwa modelu
    *parser >> str;
    //str = token;
    parser->getTokens(1, false); // tekstura (zmienia na ma³e)
    *parser >> token;
    if (!Init(AnsiString(str.c_str()), AnsiString(token.c_str())))
    {
        if (str != "notload")
        { // gdy brak modelu
            if (ter) // jeœli teren
            {
                if (str.substr(str.length() - 3, 4) == ".t3d")
                    str[str.length() - 2] = 'e';
                Global::asTerrainModel = str;
                WriteLog("Terrain model \"" + str + "\" will be created.");
            }
            else
                ErrorLog("Missed file: " + str);
        }
    }
    else
    { // wi¹zanie œwiate³, o ile model wczytany
        LightsOn[0] = pModel->GetFromName("Light_On00");
        LightsOn[1] = pModel->GetFromName("Light_On01");
        LightsOn[2] = pModel->GetFromName("Light_On02");
        LightsOn[3] = pModel->GetFromName("Light_On03");
        LightsOn[4] = pModel->GetFromName("Light_On04");
        LightsOn[5] = pModel->GetFromName("Light_On05");
        LightsOn[6] = pModel->GetFromName("Light_On06");
        LightsOn[7] = pModel->GetFromName("Light_On07");
        LightsOff[0] = pModel->GetFromName("Light_Off00");
        LightsOff[1] = pModel->GetFromName("Light_Off01");
        LightsOff[2] = pModel->GetFromName("Light_Off02");
        LightsOff[3] = pModel->GetFromName("Light_Off03");
        LightsOff[4] = pModel->GetFromName("Light_Off04");
        LightsOff[5] = pModel->GetFromName("Light_Off05");
        LightsOff[6] = pModel->GetFromName("Light_Off06");
        LightsOff[7] = pModel->GetFromName("Light_Off07");
    }
    for (int i = 0; i < iMaxNumLights; ++i)
        if (LightsOn[i] || LightsOff[i]) // Ra: zlikwidowa³em wymóg istnienia obu
            iNumLights = i + 1;
    int i = 0;
    int ti;

    parser->getTokens();
    *parser >> token;

    if (token.compare("lights") == 0)
    {
        parser->getTokens();
        *parser >> str;
        //str = AnsiString(token.c_str());
        do
        {
            ti = atof(str.c_str()); // stan œwiat³a jest liczb¹ z u³amkiem
            LightSet(i, ti);
            i++;
            parser->getTokens();
            *parser >> str;
            //str = AnsiString(token.c_str());
        } while (str != "endmodel");
    }
    return true;
}

TAnimContainer * TAnimModel::AddContainer(char *pName)
{ // dodanie sterowania submodelem dla egzemplarza
    if (!pModel)
        return NULL;
    TSubModel *tsb = pModel->GetFromName(pName);
    if (tsb)
    {
        TAnimContainer *tmp = new TAnimContainer();
        tmp->Init(tsb);
        tmp->pNext = pRoot;
        pRoot = tmp;
        return tmp;
    }
    return NULL;
}

TAnimContainer * TAnimModel::GetContainer(char *pName)
{ // szukanie/dodanie sterowania submodelem dla egzemplarza
    if (!pName)
        return pRoot; // pobranie pierwszego (dla obrotnicy)
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
        // if (pCurrent->GetName()==pName)
        if (stricmp(pCurrent->NameGet(), pName) == 0)
            return pCurrent;
    return AddContainer(pName);
}

void TAnimModel::RaAnimate()
{ // przeliczenie animacji - jednorazowo na klatkê
    // Ra 2F1I: to by mo¿na pomijaæ dla modeli bez animacji, których jest wiêkszoœæ
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
        if (!pCurrent->evDone) // jeœli jest bez eventu
            pCurrent->UpdateModel(); // przeliczenie animacji ka¿dego submodelu
    // if () //tylko dla modeli z IK !!!!
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext) // albo osobny ³añcuch
        pCurrent->UpdateModelIK(); // przeliczenie odwrotnej kinematyki
};

void TAnimModel::RaPrepare()
{ // ustawia œwiat³a i animacje we wzorcu modelu przed renderowaniem egzemplarza
    fBlinkTimer -= Timer::GetDeltaTime();
    if (fBlinkTimer <= 0)
        fBlinkTimer = fOffTime;
    bool state; // stan œwiat³a
    for (int i = 0; i < iNumLights; i++)
    {
        switch (lsLights[i])
        {
        case ls_Blink: // migotanie
            state = fBlinkTimer < fOnTime;
            break;
        case ls_Dark: // zapalone, gdy ciemno
            state = Global::fLuminance <= fDark;
            break;
        default: // zapalony albo zgaszony
            state = (lsLights[i] == ls_On);
        }
        if (LightsOn[i])
            LightsOn[i]->iVisible = state;
        if (LightsOff[i])
            LightsOff[i]->iVisible = !state;
    }
    TSubModel::iInstance = (int)this; //¿eby nie robiæ cudzych animacji
    TSubModel::pasText = &asText; // przekazanie tekstu do wyœwietlacza (!!!! do przemyœlenia)
    if (pAdvanced) // jeœli jest zaawansowana animacja
        Advanced(); // wykonaæ co tam trzeba
    TAnimContainer *pCurrent;
    for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
        pCurrent->PrepareModel(); // ustawienie animacji egzemplarza dla ka¿dego submodelu
    // if () //tylko dla modeli z IK !!!!
    // for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext) //albo osobny ³añcuch
    //  pCurrent->UpdateModelIK(); //przeliczenie odwrotnej kinematyki
}

void TAnimModel::RenderVBO(vector3 pPosition, double fAngle)
{ // sprawdza œwiat³a i rekurencyjnie renderuje TModel3d
    RaAnimate(); // jednorazowe przeliczenie animacji
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->RaRender(pPosition, fAngle, ReplacableSkinId, iTexAlpha);
}

void TAnimModel::RenderAlphaVBO(vector3 pPosition, double fAngle)
{
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->RaRenderAlpha(pPosition, fAngle, ReplacableSkinId, iTexAlpha);
};

void TAnimModel::RenderDL(vector3 pPosition, double fAngle)
{
    RaAnimate(); // jednorazowe przeliczenie animacji
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->Render(pPosition, fAngle, ReplacableSkinId, iTexAlpha);
}

void TAnimModel::RenderAlphaDL(vector3 pPosition, double fAngle)
{
    RaPrepare();
    if (pModel)
        pModel->RenderAlpha(pPosition, fAngle, ReplacableSkinId, iTexAlpha);
};

int TAnimModel::Flags()
{ // informacja dla TGround, czy ma byæ w Render, RenderAlpha, czy RenderMixed
    int i = pModel ? pModel->Flags() : 0; // pobranie flag ca³ego modelu
    if (ReplacableSkinId[1] > 0) // jeœli ma wymienn¹ teksturê 0
        i |= (i & 0x01010001) * ((iTexAlpha & 1) ? 0x20 : 0x10);
    // if (ReplacableSkinId[2]>0) //jeœli ma wymienn¹ teksturê 1
    // i|=(i&0x02020002)*((iTexAlpha&1)?0x10:0x08);
    // if (ReplacableSkinId[3]>0) //jeœli ma wymienn¹ teksturê 2
    // i|=(i&0x04040004)*((iTexAlpha&1)?0x08:0x04);
    // if (ReplacableSkinId[4]>0) //jeœli ma wymienn¹ teksturê 3
    // i|=(i&0x08080008)*((iTexAlpha&1)?0x04:0x02);
    return i;
};

//-----------------------------------------------------------------------------
// 2011-03-16 cztery nowe funkcje renderowania z mo¿liwoœci¹ pochylania obiektów
//-----------------------------------------------------------------------------

void TAnimModel::RenderDL(vector3 *vPosition)
{
    RaAnimate(); // jednorazowe przeliczenie animacji
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->Render(vPosition, &vAngle, ReplacableSkinId, iTexAlpha);
};
void TAnimModel::RenderAlphaDL(vector3 *vPosition)
{
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->RenderAlpha(vPosition, &vAngle, ReplacableSkinId, iTexAlpha);
};
void TAnimModel::RenderVBO(vector3 *vPosition)
{
    RaAnimate(); // jednorazowe przeliczenie animacji
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->RaRender(vPosition, &vAngle, ReplacableSkinId, iTexAlpha);
};
void TAnimModel::RenderAlphaVBO(vector3 *vPosition)
{
    RaPrepare();
    if (pModel) // renderowanie rekurencyjne submodeli
        pModel->RaRenderAlpha(vPosition, &vAngle, ReplacableSkinId, iTexAlpha);
};

//---------------------------------------------------------------------------
bool TAnimModel::TerrainLoaded()
{ // zliczanie kwadratów kilometrowych (g³ówna linia po Next) do tworznia tablicy
    return (this ? pModel != NULL : false);
};
int TAnimModel::TerrainCount()
{ // zliczanie kwadratów kilometrowych (g³ówna linia po Next) do tworznia tablicy
    return pModel ? pModel->TerrainCount() : 0;
};
TSubModel * TAnimModel::TerrainSquare(int n)
{ // pobieranie wskaŸników do pierwszego submodelu
    return pModel ? pModel->TerrainSquare(n) : 0;
};
void TAnimModel::TerrainRenderVBO(int n)
{ // renderowanie terenu z VBO
    if (pModel)
        pModel->TerrainRenderVBO(n);
};
//---------------------------------------------------------------------------

void TAnimModel::Advanced()
{ // wykonanie zaawansowanych animacji na submodelach
    pAdvanced->fCurrent +=
        pAdvanced->fFrequency * Timer::GetDeltaTime(); // aktualna ramka zmiennoprzecinkowo
    int frame = floor(pAdvanced->fCurrent); // numer klatki jako int
    TAnimContainer *pCurrent;
    if (pAdvanced->fCurrent >= pAdvanced->fLast)
    { // animacja zosta³a zakoñczona
        delete pAdvanced;
        pAdvanced = NULL; // dalej ju¿ nic
        for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
            if (pCurrent->pMovementData) // jeœli obs³ugiwany tabelk¹ animacji
                pCurrent->pMovementData = NULL; // usuwanie wskaŸników
    }
    else
    { // coœ trzeba poanimowaæ - wszystkie animowane submodele s¹ w tym ³añcuchu
        for (pCurrent = pRoot; pCurrent != NULL; pCurrent = pCurrent->pNext)
            if (pCurrent->pMovementData) // jeœli obs³ugiwany tabelk¹ animacji
                if (frame >= pCurrent->pMovementData->iFrame) // koniec czekania
                    if (!strcmp(pCurrent->pMovementData->cBone,
                                (pCurrent->pMovementData + 1)->cBone))
                    { // jak kolejna ramka dotyczy tego samego submodelu, ustawiæ animacjê do
                        // kolejnej ramki
                        ++pCurrent->pMovementData; // kolejna klatka
                        pCurrent->AnimSetVMD(
                            pAdvanced->fFrequency /
                            (double(pCurrent->pMovementData->iFrame) - pAdvanced->fCurrent));
                    }
                    else
                        pCurrent->pMovementData =
                            NULL; // inna nazwa, animowanie zakoñczone w aktualnym po³o¿eniu
    }
};

void TAnimModel::AnimationVND(void *pData, double a, double b, double c, double d)
{ // rozpoczêcie wykonywania animacji z podanego pliku
    // tabela w pliku musi byæ posortowana wg klatek dla kolejnych koœci!
    // skrócone nagranie ma 3:42 = 222 sekundy, animacja koñczy siê na klatce 6518
    // daje to 29.36 (~=30) klatek na sekundê
    // w opisach jest podawane 24 albo 36 jako standard => powiedzmy, parametr (d) to FPS animacji
    delete pAdvanced; // usuniêcie ewentualnego poprzedniego
    pAdvanced = NULL; // gdyby siê nie uda³o rozpoznaæ pliku
    if (AnsiString((char *)pData) == "Vocaloid Motion Data 0002")
    {
        pAdvanced = new TAnimAdvanced();
        pAdvanced->pVocaloidMotionData = (char *)pData; // podczepienie pliku danych
        pAdvanced->iMovements = *((int *)(((char *)pData) + 50)); // numer ostatniej klatki
        pAdvanced->pMovementData = (TAnimVocaloidFrame *)(((char *)pData) + 54); // rekordy animacji
        // WriteLog(sizeof(TAnimVocaloidFrame));
        pAdvanced->fFrequency = d;
        pAdvanced->fCurrent = 0.0; // aktualna ramka
        pAdvanced->fLast = 0.0; // ostatnia ramka
        /*
          if (0) //jeœli w³¹czone sortowanie plików VMD (trochê siê przeci¹ga)
           if (pAdvanced->SortByBone()) //próba posortowania
           {//zapisaæ posortowany plik, jeœli dokonano zmian
            TFileStream *fs=new TFileStream("models\\1.vmd",fmCreate);
            fs->Write(pData,2198342); //2948728);
            delete fs;
           }
        */

        int i, j, k, idx;
        AnsiString name;
        TAnimContainer *pSub;
        for (i = 0; i < pAdvanced->iMovements; ++i)
        {
            if (strcmp(pAdvanced->pMovementData[i].cBone, name.c_str()))
            { // jeœli pozycja w tabelce nie by³a wyszukiwana w submodelach
                pSub = GetContainer(pAdvanced->pMovementData[i].cBone); // szukanie
                if (pSub) // znaleziony
                {
                    pSub->pMovementData = pAdvanced->pMovementData + i; // gotów do animowania
                    pSub->AnimSetVMD(0.0); // usuawienie pozycji pocz¹tkowej (powinna byæ zerowa,
                    // inaczej bêdzie skok)
                }
                name = AnsiString(pAdvanced->pMovementData[i].cBone); // nowa nazwa do pomijania
            }
            if (pAdvanced->fLast < pAdvanced->pMovementData[i].iFrame)
                pAdvanced->fLast = pAdvanced->pMovementData[i].iFrame;
        }
        /*
          for (i=0;i<pAdvanced->iMovements;++i)
          if
          (AnsiString(pAdvanced->pMovementData[i+1].cBone)!=AnsiString(pAdvanced->pMovementData[i].cBone))
          {//generowane dla ostatniej klatki danej koœci
           name="";
           for (j=0;j<15;j++)
            name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
           WriteLog(name+","
            +AnsiString(pAdvanced->pMovementData[i].cBone)+","
            +AnsiString(idx)+"," //indeks
            +AnsiString(i+1-idx)+"," //ile pozycji animacji
            +AnsiString(k)+"," //pierwsza klatka
            +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //ostatnia klatka
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.x)+","
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.y)+","
            +AnsiString(pAdvanced->pMovementData[i].f3Vector.z)+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[0])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[1])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[2])+","
            +AnsiString(pAdvanced->pMovementData[i].fAngle[3])

            );
           idx=i+1;
           k=pAdvanced->pMovementData[i+1].iFrame; //pierwsza klatka nastêpnego
          }
          else
           if (pAdvanced->pMovementData[i].iFrame>0)
            if ((k>pAdvanced->pMovementData[i].iFrame)||(k==0))
             k=pAdvanced->pMovementData[i].iFrame; //pierwsza niezerowa ramka
        */
        /*
          for (i=0;i<pAdvanced->iMovements;++i)
           if (AnsiString(pAdvanced->pMovementData[i].cBone)=="\x89\x45\x90\x65\x8E\x77\x82\x4F")
           {name="";
            for (j=0;j<15;j++)
             name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
            WriteLog(name+","
             +AnsiString(i)+"," //pozycja w tabeli
             +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //pierwsza klatka
            );
           }
        */
    }
};

//---------------------------------------------------------------------------
void TAnimModel::LightSet(int n, float v)
{ // ustawienie œwiat³a (n) na wartoœæ (v)
    if (n >= iMaxNumLights)
        return; // przekroczony zakres
    lsLights[n] = TLightState(int(v));
    switch (lsLights[n])
    { // interpretacja u³amka zale¿nie od typu
    case 0: // ustalenie czasu migotania, t<1s (f>1Hz), np. 0.1 => t=0.1 (f=10Hz)
        break;
    case 1: // ustalenie wype³nienia u³amkiem, np. 1.25 => zapalony przez 1/4 okresu
        break;
    case 2: // ustalenie czêstotliwoœci migotania, f<1Hz (t>1s), np. 2.2 => f=0.2Hz (t=5s)
        break;
    case 3: // zapalenie œwiate³ zale¿ne od oœwietlenia scenerii
        if (v > 3.0)
            fDark = v - 3.0; // ustawienie indywidualnego progu zapalania
        else
            fDark = 0.25; // standardowy próg zaplania
        break;
    }
};
//---------------------------------------------------------------------------
void TAnimModel::AnimUpdate(double dt)
{ // wykonanie zakolejkowanych animacji, nawet gdy modele nie s¹ aktualnie wyœwietlane
    TAnimContainer *p = TAnimModel::acAnimList;
    while (p)
    { // jeœli w ogóle jest co animowaæ
        // if ((*p)->fTranslateSpeed==0.0)
        // if ((*p)->fRotateSpeed==0.0)
        // {//jak siê naanimowa³, to usun¹æ z listy
        //  *p=(*p)->ListRemove(); //zwraca wskaŸnik do kolejnego z listy
        // }
        p->UpdateModel();
        p = p->acAnimNext; // na razie bez usuwania z listy, bo g³ównie obrotnica na ni¹ wchodzi
    }
};
//---------------------------------------------------------------------------
