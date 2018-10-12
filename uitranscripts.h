
#pragma once

#include <deque>
#include <string>

namespace ui {

// klasa obsługująca linijkę napisu do dźwięku
struct TTranscript {

    float fShow; // czas pokazania
    float fHide; // czas ukrycia/usunięcia
    std::string asText; // tekst gotowy do wyświetlenia (usunięte znaczniki czasu)
    bool bItalic; // czy kursywa (dźwięk nieistotny dla prowadzącego)
};

// klasa obsługująca napisy do dźwięków
class TTranscripts { 

public:
// constructors
    TTranscripts() = default;
// methods
    void AddLine( std::string const &txt, float show, float hide, bool it );
    // dodanie tekstów, długość dźwięku, czy istotne
    void Add( std::string const &txt, bool background = false );
    // usuwanie niepotrzebnych (ok. 10 razy na sekundę)
    void Update();
// members
    std::deque<TTranscript> aLines;

private:
// members
    float fRefreshTime { 360 }; // wartośc zaporowa

};

extern TTranscripts Transcripts;

} // namespace ui