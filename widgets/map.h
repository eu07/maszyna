#pragma once

#include "gl/shader.h"
#include "renderer.h"
#include "Texture.h"
#include "uilayer.h"

namespace ui {

class map_panel : public ui_panel {
	std::unique_ptr<gl::program> m_shader;
    std::unique_ptr<gl::framebuffer> m_msaa_fb;
	std::unique_ptr<gl::renderbuffer> m_msaa_rb;

    std::unique_ptr<gl::framebuffer> m_fb;
    std::unique_ptr<opengl_texture> m_tex;

    std::unique_ptr<gl::ubo> scene_ubo;
    gl::scene_ubs scene_ubs;

    std::vector<gfx::geometrybank_handle> m_section_handles;

    const int fb_size = 1024;

    glm::vec2 translate;
    float zoom = 1.0f / 1000.0f;
    float get_vehicle_rotation();
	void render_map_texture(glm::mat4 transform, glm::vec2 surface_size);
	void render_labels(glm::mat4 transform, ImVec2 origin, glm::vec2 surface_size);

	bool init_done = false;

public:
	map_panel();
	void render_contents() override;
};

}
