#pragma once
#include "Camera.h"
#include "Model3d.h"
#include "Texture.h"

class vr_interface
{
public:
    enum eye_e {
        eye_left,
        eye_right
    };

    virtual viewport_proj_config get_proj_config(eye_e) = 0;
    virtual TModel3d* get_hiddenarea_mesh(eye_e) = 0;
    virtual glm::ivec2 get_target_size() = 0;
    virtual void begin_frame() = 0;
    virtual void submit(eye_e, opengl_texture*) = 0;
    virtual std::vector<TModel3d*> get_render_models() = 0;
    virtual glm::mat4 get_pick_transform() = 0;
    virtual void finish_frame() = 0;
    virtual ~vr_interface() = 0;
};

class vr_interface_factory
{
public:
    using create_method = std::unique_ptr<vr_interface>(*)();

    bool register_backend(const std::string &backend, create_method func);
    std::unique_ptr<vr_interface> create(const std::string &name);

    static vr_interface_factory* get_instance();

private:
    std::unordered_map<std::string, create_method> backends;
    static vr_interface_factory *instance;
};
