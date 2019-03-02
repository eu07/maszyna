#pragma once

#include "gl/shader.h"
#include "renderer.h"
#include "Texture.h"
#include "uilayer.h"
#include "widgets/map_objects.h"
#include "widgets/popup.h"

namespace ui {

class disambiguation_popup : public popup {
	map::sorted_object_list m_list;

public:
	disambiguation_popup(ui_panel &panel, map::sorted_object_list &&list);

	virtual void render_content() override;
};

class semaphore_window : public popup {
	std::shared_ptr<map::semaphore> m_sem;
	command_relay m_relay;

public:
	semaphore_window(ui_panel &panel, std::shared_ptr<map::semaphore> &&sem);

	virtual void render_content() override;
};

class switch_window : public popup {
	std::shared_ptr<map::track_switch> m_switch;
	command_relay m_relay;

public:
	switch_window(ui_panel &panel, std::shared_ptr<map::track_switch> &&sw);

	virtual void render_content() override;
};

class map_panel : public ui_panel {
	std::unique_ptr<gl::program> m_shader;
    std::unique_ptr<gl::framebuffer> m_msaa_fb;
	std::unique_ptr<gl::renderbuffer> m_msaa_rb;

    std::unique_ptr<gl::framebuffer> m_fb;
    std::unique_ptr<opengl_texture> m_tex;

    std::unique_ptr<gl::ubo> scene_ubo;
    gl::scene_ubs scene_ubs;

    std::vector<gfx::geometrybank_handle> m_section_handles;
	std::vector<gfx::geometrybank_handle> m_switch_handles;

    const int fb_size = 1024;

    glm::vec2 translate;
    float zoom = 1.0f / 1000.0f;
    float get_vehicle_rotation();
	void render_map_texture(glm::mat4 transform, glm::vec2 surface_size);
	void render_labels(glm::mat4 transform, ImVec2 origin, glm::vec2 surface_size);

	bool init_done = false;

	std::optional<map::semaphore> active;

public:
	map_panel();
	void render_contents() override;
};

void handle_map_object_click(ui_panel &parent, std::shared_ptr<map::map_object> &obj);
void handle_map_object_hover(std::shared_ptr<map::map_object> &obj);
}
