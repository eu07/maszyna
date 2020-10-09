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
double GetRenderTime();

double GetDeltaTime();
double GetDeltaRenderTime();

void set_delta_override(double v);

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
	std::chrono::duration<float, std::milli>
        stop() {
		    m_last = std::chrono::duration_cast<std::chrono::microseconds>( ( std::chrono::steady_clock::now() - m_start ) );
			m_accumulator = 0.95f * m_accumulator + m_last.count() / 1000.f;
			return m_last; }
    float
        average() const {
            return m_accumulator / 20.f;}
	std::chrono::microseconds
	    last() const {
		    return m_last; }

private:
// members
    std::chrono::time_point<std::chrono::steady_clock> m_start { std::chrono::steady_clock::now() };
    float m_accumulator { 1000.f / 30.f * 20.f }; // 20 last samples, initial 'neutral' rate of 30 fps
    std::chrono::microseconds m_last;
};

struct subsystem_stopwatches {
    stopwatch gfx_total;
    stopwatch gfx_color;
    stopwatch gfx_shadows;
    stopwatch gfx_reflections;
    stopwatch gfx_swap;
    stopwatch gfx_gui;
    stopwatch sim_total;
    stopwatch sim_dynamics;
    stopwatch sim_events;
    stopwatch sim_ai;
    stopwatch mainloop_total;
};

extern subsystem_stopwatches subsystem;

};

//---------------------------------------------------------------------------
