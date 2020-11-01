/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>

#include "Classes.h"
#include "scenenode.h"

// radio-transmitted event launch messages
enum radio_message {
    call3 = -3,
    call1 = -1
};

class TEventLauncher : public scene::basic_node {

public:
// constructor
    explicit TEventLauncher( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}
    // legacy constructor
    TEventLauncher() = default;

// methods
    bool Load( cParser *parser );
	bool check_activation_key();
    bool check_activation();
    // checks conditions associated with the event. returns: true if the conditions are met
    bool check_conditions();
    inline
    auto key() const {
        return iKey; }
    bool IsGlobal() const;
    bool IsRadioActivated() const;
// members
    std::string asEvent1Name;
    std::string asEvent2Name;
    std::string asMemCellName;
    basic_event *Event1 { nullptr };
    basic_event *Event2 { nullptr };
    TMemCell *MemCell { nullptr };
    int iCheckMask { 0 };
    double dRadius { 0.0 };
    bool train_triggered { false };

private:
// methods
    // radius() subclass details, calculates node's bounding radius
    float radius_();
    // serialize() subclass details, sends content of the subclass to provided stream
    void serialize_( std::ostream &Output ) const;
    // deserialize() subclass details, restores content of the subclass from provided stream
    void deserialize_( std::istream &Input );
    // export() subclass details, sends basic content of the class in legacy (text) format to provided stream
    void export_as_text_( std::ostream &Output ) const;

// members
    int iKey { 0 };
    double DeltaTime { -1.0 };
    double UpdatedTime { 0.0 };
    double fVal1 { 0.0 };
    double fVal2 { 0.0 };
    std::string szText;
    int iHour { -1 };
    int iMinute { -1 }; // minuta uruchomienia
};

//---------------------------------------------------------------------------
