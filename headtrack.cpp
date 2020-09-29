#include "stdafx.h"
#include "headtrack.h"
#include "Globals.h"

headtrack::headtrack()
{

}

void headtrack::find_joy() {
    for (size_t i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++) {
        if (!glfwJoystickPresent(i))
            continue;

        std::string name(glfwGetJoystickName(i));
        if (name == Global.headtrack_conf.joy) {
            joy_id = i;
            return;
        }
    }

    joy_id = -1;
}

float headtrack::get_axis(const float *data, int count, int axis, float mul) {
    if (axis < 0)
        return 0.0f;
    if (axis >= count)
        return 0.0f;

    return data[axis] * mul;
}

void headtrack::update()
{
    if (joy_id == -1 || !glfwJoystickPresent(joy_id)) {
        Global.viewport_move = glm::vec3();
        Global.viewport_rotate = glm::mat3();
        find_joy();
        return;
    }

    int count;
    const float* axes = glfwGetJoystickAxes(joy_id, &count);

    auto const &move_axes = Global.headtrack_conf.move_axes;
    auto const &rot_axes = Global.headtrack_conf.rot_axes;
    auto const &move_mul = Global.headtrack_conf.move_mul;
    auto const &rot_mul = Global.headtrack_conf.rot_mul;

    glm::vec3 move;
    move.x = get_axis(axes, count, move_axes.x, move_mul.x);
    move.y = get_axis(axes, count, move_axes.y, move_mul.y);
    move.z = get_axis(axes, count, move_axes.z, move_mul.z);

    glm::vec3 rotate;
    rotate.x = get_axis(axes, count, rot_axes.x, rot_mul.x);
    rotate.y = get_axis(axes, count, rot_axes.y, rot_mul.y);
    rotate.z = get_axis(axes, count, rot_axes.z, rot_mul.z);

    Global.viewport_move = move;
    Global.viewport_rotate = glm::orientate3(rotate);
}
