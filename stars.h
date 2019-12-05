#pragma once

#include "Classes.h"


//////////////////////////////////////////////////////////////////////////////////////////
// cStars -- simple starfield model, simulating appearance of starry sky

class cStars {

    friend opengl_renderer;
    friend opengl33_renderer;

public:
// types:

// methods:
    void init();
// constructors:
    cStars() = default;
// deconstructor:

// members:

private:
// types:

// methods:

// members:
    float m_longitude{ 19.0f }; // geograpic coordinates hardcoded roughly to Poland location, for the time being
    float m_latitude{ 52.0f };
    TModel3d *m_stars { nullptr };
};
