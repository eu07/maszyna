#pragma once

class headtrack
{
    int joy_id = -1;

    void find_joy();
    float get_axis(const float *data, int count, int axis, float mul);

public:
    headtrack();

    void update();
};
