#pragma once
#include "vr_interface.h"
#include <openvr/openvr.h>

class vr_openvr : vr_interface
{
private:
    struct controller_info {
        std::unique_ptr<TModel3d> model;
        std::string rendermodel;
        std::vector<uint32_t> pending_components;
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

    bool update_component(const std::string &rendermodel, vr::VRInputValueHandle_t handle, TSubModel *component);
    glm::mat4 get_matrix(vr::HmdMatrix34_t &src);

    vr::VRActionSetHandle_t actionset = 0;
    vr::VRActionHandle_t primary_action = 0;
    bool should_pause = false;
    bool draw_controllers = true;

    command_relay relay;

public:
    vr_openvr();
    viewport_proj_config get_proj_config(eye_e) override;
    glm::ivec2 get_target_size() override;
    void begin_frame() override;
    void submit(eye_e, opengl_texture*) override;
    std::vector<TModel3d*> get_render_models() override;
    void finish_frame() override;
    ~vr_openvr() override;

    static std::unique_ptr<vr_interface> create_func();

private:
    static bool backend_register;
};
