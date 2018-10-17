#include "stdafx.h"
#include "map.h"
#include "imgui/imgui.h"
#include "Logs.h"
#include "Train.h"
#include "Camera.h"
#include "simulation.h"

bool map::init()
{
    WriteLog("initializing map gfx...");

    gl::shader vert("map.vert");
    gl::shader frag("map.frag");
    gl::program *prog = new gl::program({vert, frag});
    m_shader = std::unique_ptr<gl::program>(prog);

    m_tex = std::make_unique<opengl_texture>();
    m_tex->alloc_rendertarget(GL_RGB8, GL_RGB, fb_size, fb_size);

    m_fb = std::make_unique<gl::framebuffer>();
    m_fb->attach(*m_tex, GL_COLOR_ATTACHMENT0);

    m_fb->setup_drawing(1);

    if (!m_fb->is_complete())
    {
        ErrorLog("map framebuffer incomplete");
        return false;
    }

    if (Global.iMultisampling)
    {
        m_msaa_rb = std::make_unique<gl::renderbuffer>();
        m_msaa_rb->alloc(GL_RGB8, fb_size, fb_size, 1 << Global.iMultisampling);

        m_msaa_fb = std::make_unique<gl::framebuffer>();
        m_msaa_fb->attach(*m_msaa_rb, GL_COLOR_ATTACHMENT0);

        m_msaa_fb->setup_drawing(1);

        if (!m_msaa_fb->is_complete())
        {
            ErrorLog("map multisampling framebuffer incomplete");
            return false;
        }
    }

    scene_ubo = std::make_unique<gl::ubo>(sizeof(gl::scene_ubs), 0);

    WriteLog("map init ok");

    return true;
}

float map::get_vehicle_rotation()
{
    const TDynamicObject *vehicle = simulation::Train->Dynamic();
    glm::vec3 front = glm::dvec3(vehicle->VectorFront()) * (vehicle->DirectionGet() > 0 ? 1.0 : -1.0);
    glm::vec2 f2(front.x, front.z);
    glm::vec2 north_ptr(0.0f, 1.0f);
    return glm::atan(f2.y, f2.x) - glm::atan(north_ptr.y, north_ptr.x);
}

void map::render(scene::basic_region *Region)
{
    if (!map_opened)
        return;

    if (!scene_ubo)
        if (!init())
        {
            map_opened = false;
            return;
        }

    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(fb_size, fb_size));
    if (ImGui::Begin("Map", &map_opened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        float prevzoom = zoom;

        if (ImGui::Button("-"))
            zoom /= 2;
        ImGui::SameLine();
        if (ImGui::Button("+"))
            zoom *= 2;
        ImGui::SameLine();

        float x = zoom / prevzoom;
        translate *= x;

        glm::mat4 transform;
        transform[0][0] = -1.0f;

        static int mode = 0;
        ImGui::RadioButton("manual", &mode, 0); ImGui::SameLine();
        ImGui::RadioButton("cam", &mode, 1); ImGui::SameLine();
        ImGui::RadioButton("vehicle", &mode, 2);

        if (mode == 2 && simulation::Train && simulation::Train->Dynamic())
        {
            float rot = get_vehicle_rotation();

            transform = glm::rotate(transform, rot, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::dvec3 vpos = simulation::Train->Dynamic()->GetPosition();
            translate = glm::vec2(vpos.x, vpos.z) * -zoom;
        }
        else if (mode == 1)
        {
            float rot;
            if (FreeFlyModeFlag)
                rot = glm::pi<float>() - Global.pCamera.Yaw;
            else
                rot = get_vehicle_rotation() - Global.pCamera.Yaw;

            transform = glm::rotate(transform, rot, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::dvec3 vpos = Global.pCamera.Pos;
            translate = glm::vec2(vpos.x, vpos.z) * -zoom;
        }

        ImVec2 size = ImGui::GetContentRegionAvail();

        transform = glm::translate(transform, glm::vec3(translate.x, 0.0f, translate.y));
        transform = glm::scale(transform, glm::vec3(zoom));

        frustum.calculate(transform, glm::mat4());

        m_section_handles.clear();

        for (int row = 0; row < scene::EU07_REGIONSIDESECTIONCOUNT; row++)
        {
            for (int column = 0; column < scene::EU07_REGIONSIDESECTIONCOUNT; column++)
            {
                scene::basic_section *s = Region->get_section(row * scene::EU07_REGIONSIDESECTIONCOUNT + column);
                if (s && frustum.sphere_inside(s->area().center, s->area().radius) > 0.f)
                {
                    const gfx::geometrybank_handle handle = s->get_map_geometry();
                    if (handle != null_handle)
                        m_section_handles.push_back(handle);
                }
            }
        }

        scene_ubs.projection = transform;
        scene_ubo->update(scene_ubs);
        scene_ubo->bind_uniform();

        glDisable(GL_DEPTH_TEST);
        if (Global.iMultisampling)
        {
            m_msaa_fb->clear(GL_COLOR_BUFFER_BIT);
            m_msaa_fb->bind();
        }
        else
        {
            m_fb->clear(GL_COLOR_BUFFER_BIT);
            m_fb->bind();
        }

        m_shader->bind();
        glLineWidth(1.5f);
        glViewport(0, 0, size.x, size.y);

        GfxRenderer.Draw_Geometry(m_section_handles.begin(), m_section_handles.end());

        if (Global.iMultisampling)
            m_fb->blit_from(*m_msaa_fb, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_COLOR_ATTACHMENT0);

        gl::framebuffer::unbind();
        m_shader->unbind();

        ImGui::ImageButton(reinterpret_cast<void*>(m_tex->id), size, ImVec2(0, size.y / fb_size), ImVec2(size.x / fb_size, 0), 0);

        if (mode == 0 && ImGui::IsItemHovered() && ImGui::IsMouseDragging())
        {
            ImVec2 ivdelta = ImGui::GetMouseDragDelta();
            ImGui::ResetMouseDragDelta();

            glm::vec2 delta(ivdelta.x, ivdelta.y);
            delta.x /= size.x;
            delta.y /= size.y;

            translate -= delta * 2.0f;
        }
    }
    ImGui::End();
}

void map::toggle_window()
{
    map_opened = !map_opened;
}
