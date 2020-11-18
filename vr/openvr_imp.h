#pragma once
#include "vr_interface.h"
#include <openvr/openvr.h>

class vr_openvr : vr_interface
{
private:
    struct controller_info {
        std::unique_ptr<TModel3d> model;
        std::string rendermodel;
        std::vector<int32_t> pending_components;
        std::vector<TSubModel*> active_components;
        std::vector<std::pair<TSubModel*, vr::TextureID_t>> pending_textures;
        vr::VRInputValueHandle_t inputhandle = 0;
    };

    static const size_t MAX_DEVICES = 16;

    vr::IVRSystem *vr_system = nullptr;
    std::array<std::shared_ptr<controller_info>, MAX_DEVICES> controllers;
    std::unordered_map<vr::TextureID_t, std::shared_ptr<opengl_texture>> textures;
    vr::VRInputValueHandle_t inputhandle_left = 0;
    vr::VRInputValueHandle_t inputhandle_right = 0;

    std::array<std::unique_ptr<TModel3d>, 2> hiddenarea_mesh;

    uint32_t primary_controller;
    glm::mat4 primary_controller_transform;

    user_command command_active = user_command::none;

    bool update_component(const std::string &rendermodel, vr::VRInputValueHandle_t handle, TSubModel *component);
    glm::mat4 get_matrix(const vr::HmdMatrix34_t &src);
    std::unique_ptr<TModel3d> create_hiddenarea_model(eye_e e);

    vr::VRActionSetHandle_t actionset = 0;
    vr::VRActionHandle_t primary_action = 0;
    vr::VRActionHandle_t secondary_action = 0;

    bool should_pause = false;
    bool draw_controllers = true;

    struct button_bindings {
        user_command primary;
        user_command secondary;
    };

    static std::unordered_map<std::string, button_bindings> m_buttonbindings;
    command_relay relay;

public:
    vr_openvr();
    viewport_proj_config get_proj_config(eye_e) override;
    glm::ivec2 get_target_size() override;
    void begin_frame() override;
    void submit(eye_e, opengl_texture*) override;
    std::vector<TModel3d*> get_render_models() override;
    TModel3d* get_hiddenarea_mesh(eye_e) override;
    glm::mat4 get_pick_transform() override;
    void finish_frame() override;
    ~vr_openvr() override;

    static std::unique_ptr<vr_interface> create_func();

private:
    static bool backend_register;
};
