

#include "stdafx.h"
#include "uitranscripts.h"

#include "globals.h"
#include "parser.h"
#include "utilities.h"

namespace ui {

TTranscripts Transcripts;

// dodanie linii do tabeli, (show) i (hide) w [s] od aktualnego czasu
void
TTranscripts::AddLine( std::string const &txt, float show, float hide, bool it ) {

    if( show == hide ) { return; } // komentarz jest ignorowany

    show = Global.fTimeAngleDeg + show / 240.0; // jeśli doba to 360, to 1s będzie równe 1/240
    hide = Global.fTimeAngleDeg + hide / 240.0;

    TTranscript transcript;
    transcript.asText = txt;
    transcript.fShow = show;
    transcript.fHide = hide;
    transcript.bItalic = it;
    aLines.emplace_back( transcript );
    // set the next refresh time while at it
    // TODO, TBD: sort the transcript lines? in theory, they should be coming arranged in the right order anyway
    // short of cases with multiple sounds overleaping
    fRefreshTime = aLines.front().fHide;
}

// dodanie tekstów, długość dźwięku, czy istotne
void
TTranscripts::Add( std::string const &txt, bool backgorund ) {

    if( true == txt.empty() ) { return; }

    std::string asciitext{ txt }; win1250_to_ascii( asciitext ); // TODO: launch relevant conversion table based on language
    cParser parser( asciitext );
    while( true == parser.getTokens( 3, false, "[]\n" ) ) {

        float begin, end;
        std::string transcript;
        parser
            >> begin
            >> end
            >> transcript;
        AddLine( transcript, 0.10 * begin, 0.12 * end, false );
    }
    // try to handle malformed(?) cases with no show/hide times
    std::string transcript; parser >> transcript;
    while( false == transcript.empty() ) {

        //        WriteLog( "Transcript text with no display/hide times: \"" + transcript + "\"" );
        AddLine( transcript, 0.0, 0.12 * transcript.size(), false );
        transcript = ""; parser >> transcript;
    }
}

// usuwanie niepotrzebnych (nie częściej niż 10 razy na sekundę)
void
TTranscripts::Update() {

    if( Global.fTimeAngleDeg < fRefreshTime ) { return; } // nie czas jeszcze na zmiany

    while( ( false == aLines.empty() )
        && ( Global.fTimeAngleDeg >= aLines.front().fHide ) ) {
        // remove expired lines
        aLines.pop_front();
    }
    // update next refresh time
    if( false == aLines.empty() ) { fRefreshTime = aLines.front().fHide; }
    else { fRefreshTime = 360.0f; }
}

} // namespace ui
