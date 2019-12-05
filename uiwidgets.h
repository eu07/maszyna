/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

namespace ImGui {

bool BufferingBar( const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col );
bool Spinner( const char* label, float radius, int thickness, const ImU32& color );

} // namespace ImGui
