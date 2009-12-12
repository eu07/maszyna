
Witam

Przesylam szkielet tracerouta, sorry ze nie sprawdzony ale juz mi sie chce
spac %), jakby co to jutro jak wroce z roboty poprawie
przerobilem go z Move wiec powinien dzialac

za parametry bierze odleglosc jaka ma przeleciec, kierunek oraz tor z
ktorego ma zaczac. kierunek (fDirection) jest modyfikowane wewnatrz
procedury aby zwrocilo aktualny kierunek po przejechaniu danej
odleglosci wiec nie podstaw tam przypadkiem jakies waznej zmiennej :)
zwraca nam tor do ktorego dotarlismy po przejechaniu fDistance

przerob sobie tak jak tam chcesz, jesli w czasie przelatywania chcesz
sprawdzac jakis warunek to jego sprawdzenie wstaw tam gdzie
przeskakuje sie na inny tor (oznaczone komentarzem)

TTrack* __fastcall TraceRoute(double fDistance, double &fDirection, TTrack *Track)
{
    fDistance*= fDirection;
    double fCurrentDistance= Track->Length()*0.5;
    double s;

    while (true)
    {
        s= fCurrentDistance+fDistance;
        if (s<0)
        {
            if (Track->bPrevSwitchDirection)
            {
                Track= Track->CurrentPrev();
                fCurrentDistance= 0;
                fDistance= -s;
                fDirection= -fDirection;
                if (Track==NULL)
                {
                    Error("NULL track");
                    return NULL;
                }
            }
            else
            {
                Track= Track->CurrentPrev();
                if (Track==NULL)
                {
                    Error("NULL track");
                    return NULL;
                }
                fCurrentDistance= Track->Length();
                fDistance= s;
            }
            // przeskoczylismy na inny tor

            continue;
        }
        else
        if (s>Track->Length())
        {

            if (Track->bNextSwitchDirection)
            {
                fDistance= -(s-Track->Length());
                Track= Track->CurrentNext();
                if (Track==NULL)
                {
                    Error("NULL track");
                    return NULL;
                }
                fCurrentDistance= Track->Length();
                fDirection= -fDirection;
            }
            else
            {
                fDistance= s-Track->Length();
                Track= Track->CurrentNext();
                fCurrentDistance= 0;
                if (Track==NULL)
                {
                    Error("NULL track");
                    return NULL;
                }
            }
            // tu rowniez przeskoczylismy na inny tor

            continue;
        }
        else
        {
            fCurrentDistance= s;
            fDistance= 0;

            //przejechalismy cale fDistance
            return Track;
        }

    }
}


-- 
Pozdrawiam
Marcin
