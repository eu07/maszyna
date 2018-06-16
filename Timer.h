/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

namespace Timer {

double GetTime();

double GetDeltaTime();
double GetDeltaRenderTime();

void SetDeltaTime(double v);

bool GetSoundTimer();

double GetFPS();

void ResetTimers();

void UpdateTimers(bool pause);

class stopwatch {

public:
// constructors
    stopwatch() = default;
// methods
    void
        start() {
            m_start = std::chrono::steady_clock::now(); }
    void
        stop() {
            m_accumulator = 0.95f * m_accumulator + std::chrono::duration_cast<std::chrono::microseconds>( ( std::chrono::steady_clock::now() - m_start ) ).count() / 1000.f; }
    float
        average() const {
            return m_accumulator / 20.f;}

private:
// members
    std::chrono::time_point<std::chrono::steady_clock> m_start { std::chrono::steady_clock::now() };
    float m_accumulator { 1000.f / 30.f * 20.f }; // 20 last samples, initial 'neutral' rate of 30 fps
};

struct subsystem_stopwatches {
    stopwatch gfx_total;
    stopwatch gfx_color;
    stopwatch gfx_shadows;
    stopwatch gfx_reflections;
    stopwatch gfx_swap;
    stopwatch sim_total;
    stopwatch sim_dynamics;
    stopwatch sim_events;
    stopwatch sim_ai;
};

extern subsystem_stopwatches subsystem;

};

//---------------------------------------------------------------------------
