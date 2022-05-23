#include "stdafx.h"
#include "openvr_imp.h"
#include "Logs.h"
#include "Globals.h"
#include "renderer.h"
#include "simulation.h"
#include "application.h"

vr_openvr::vr_openvr()
{
    vr::EVRInitError vr_error;
    vr_system = vr::VR_Init(&vr_error, vr::VRApplication_Scene);
    if (vr_error != vr::VRInitError_None) {
        ErrorLog("failed to init openvr!");
        return;
    }

    vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseSeated);
    vr::VRInput()->GetInputSourceHandle("/user/hand/left", &inputhandle_left);
    vr::VRInput()->GetInputSourceHandle("/user/hand/right", &inputhandle_right);

    std::string action_path = std::filesystem::current_path().string() + "/openvrconfig/openvr_actions.json";
    vr::VRInput()->SetActionManifestPath(action_path.c_str());

    vr::VRInput()->GetActionSetHandle("/actions/main", &actionset);
    vr::VRInput()->GetActionHandle("/actions/main/in/PrimaryAction", &primary_action);
    vr::VRInput()->GetActionHandle("/actions/main/in/SecondaryAction", &secondary_action);

    hiddenarea_mesh[(size_t)vr_interface::eye_left] = create_hiddenarea_model(vr_interface::eye_left);
    hiddenarea_mesh[(size_t)vr_interface::eye_right] = create_hiddenarea_model(vr_interface::eye_right);
}

std::unique_ptr<TModel3d> vr_openvr::create_hiddenarea_model(vr_interface::eye_e e)
{
    vr::HiddenAreaMesh_t mesh = vr_system->GetHiddenAreaMesh((e == vr_interface::eye_left) ? vr::Eye_Left : vr::Eye_Right, vr::k_eHiddenAreaMesh_Standard);
    if (!mesh.unTriangleCount)
        return nullptr;

    gfx::vertex_array vertices;
    for (size_t v = 0; v < mesh.unTriangleCount * 3; v++) {
        const vr::HmdVector2_t vertex = mesh.pVertexData[v];
        vertices.push_back(gfx::basic_vertex(
                               glm::vec3(vertex.v[0], vertex.v[1], 0.0f),
                               glm::vec3(0.0f),
                               glm::vec2(0.0f)));
    }


    std::unique_ptr<TModel3d> model = std::make_unique<TModel3d>();
    model->AppendChildFromGeometry("__root", "none", vertices, gfx::index_array());
    model->Init();

    return model;
}

TModel3d* vr_openvr::get_hiddenarea_mesh(eye_e eye)
{
    return hiddenarea_mesh[(size_t)eye].get();
};

glm::ivec2 vr_openvr::get_target_size()
{
    uint32_t vr_w, vr_h;
    vr_system->GetRecommendedRenderTargetSize(&vr_w, &vr_h);
    return glm::ivec2(vr_w, vr_h);
}

viewport_proj_config vr_openvr::get_proj_config(eye_e e)
{
    vr::EVREye eye = (e == vr_interface::eye_left) ? vr::Eye_Left : vr::Eye_Right;

    float left, right, top, bottom; // tangents of half-angles from center view axis
    vr_system->GetProjectionRaw(eye, &left, &right, &top, &bottom);

    glm::mat4 eye_mat = get_matrix(vr_system->GetEyeToHeadTransform(eye));

    viewport_proj_config proj;

    proj.pe = eye_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    proj.pa = eye_mat * glm::vec4(left, top, -1.0f, 1.0f);
    proj.pc = eye_mat * glm::vec4(left, bottom, -1.0f, 1.0f);
    proj.pb = eye_mat * glm::vec4(right, top, -1.0f, 1.0f);

    return proj;
}

glm::mat4 vr_openvr::get_matrix(const vr::HmdMatrix34_t &src)
{
    return glm::mat4(glm::transpose(glm::make_mat3x4((float*)(src.m))));
}

void vr_openvr::begin_frame()
{
    vr::TrackedDevicePose_t devicePose[MAX_DEVICES];
    vr::VRCompositor()->WaitGetPoses(devicePose, MAX_DEVICES, nullptr, 0);

    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (!vr_system->IsTrackedDeviceConnected(i)) {
            if (controllers[i])
                controllers[i].reset();

            continue;
        }

        glm::mat4 device_pose = get_matrix(devicePose[i].mDeviceToAbsoluteTracking);

        vr::ETrackedDeviceClass device_class = vr_system->GetTrackedDeviceClass(i);

        if (device_class == vr::TrackedDeviceClass_HMD) {
            Global.viewport_move = glm::vec3(device_pose[3]);
            Global.viewport_rotate = glm::mat3(device_pose);
        }

        if (device_class == vr::TrackedDeviceClass_Controller) {
            if (!controllers[i]) {
                controllers[i] = std::make_shared<controller_info>();

                vr::ETrackedControllerRole role = (vr::ETrackedControllerRole)vr_system->GetInt32TrackedDeviceProperty(i, vr::Prop_ControllerRoleHint_Int32);
                if (role == vr::TrackedControllerRole_LeftHand)
                    controllers[i]->inputhandle = inputhandle_left;
                else if (role == vr::TrackedControllerRole_RightHand)
                    controllers[i]->inputhandle = inputhandle_right;

                char model_name[128];
                size_t ret = vr_system->GetStringTrackedDeviceProperty(i, vr::Prop_RenderModelName_String, model_name, sizeof(model_name));

                if (ret != 0) {
                    controllers[i]->rendermodel = std::string(model_name);

                    uint32_t component_count = vr::VRRenderModels()->GetComponentCount(model_name);
                    if (component_count == 0 || !controllers[i]->inputhandle) {
                        controllers[i]->pending_components.push_back(-1);
                    } else {
                        for (size_t c = 0; c < component_count; c++)
                            controllers[i]->pending_components.push_back(c);
                    }

                    controllers[i]->model = std::make_unique<TModel3d>();
                    gfx::vertex_array vertices {
                        gfx::basic_vertex{ glm::vec3(0.005f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
                        gfx::basic_vertex{ glm::vec3(-0.005f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
                        gfx::basic_vertex{ glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },

                        gfx::basic_vertex{ glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
                        gfx::basic_vertex{ glm::vec3(-0.005f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
                        gfx::basic_vertex{ glm::vec3(0.005f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) } };
                    controllers[i]->model->AppendChildFromGeometry("__root", "none", gfx::vertex_array(), gfx::index_array());

                    controllers[i]->model->AppendChildFromGeometry("__pointer", "__root", vertices, gfx::index_array());
                }
            }

            for (size_t c = controllers[i]->pending_components.size(); c-- != 0;) {
                int32_t component = controllers[i]->pending_components[c];
                const char *rendermodel = controllers[i]->rendermodel.c_str();

                char rendermodel_name[256];
                char component_name[128];

                gfx::vertex_array vertices;
                gfx::index_array indices;

                std::string submodel_name;
                vr::RenderModel_t *model;
                vr::EVRRenderModelError ret;
                TSubModel *sm;

                if (component == -1) {
                    const char *name = controllers[i]->rendermodel.c_str();
                    strncpy(rendermodel_name, name, sizeof(rendermodel_name) - 1);
                    rendermodel_name[255] = 0;
                }
                else {
                    vr::VRRenderModels()->GetComponentName(rendermodel, component, component_name, sizeof(component_name));
                    if (!vr::VRRenderModels()->GetComponentRenderModelName(rendermodel, component_name, rendermodel_name, sizeof(rendermodel_name)))
                        goto component_done;
                }

                ret = vr::VRRenderModels()->LoadRenderModel_Async(rendermodel_name, &model);

                if (ret == vr::VRRenderModelError_Loading)
                    continue;
                else if (ret != vr::VRRenderModelError_None)
                    goto component_done;

                indices.reserve(model->unTriangleCount * 3);
                for (size_t v = 0; v < model->unTriangleCount * 3; v++)
                    indices.push_back(model->rIndexData[v]);

                vertices.reserve(model->unVertexCount);
                for (size_t v = 0; v < model->unVertexCount; v++) {
                    const vr::RenderModel_Vertex_t *vertex = &model->rVertexData[v];
                    vertices.push_back(gfx::basic_vertex(
                                           glm::vec3(vertex->vPosition.v[0], vertex->vPosition.v[1], vertex->vPosition.v[2]),
                                           glm::vec3(vertex->vNormal.v[0], vertex->vNormal.v[1], vertex->vNormal.v[2]),
                                           glm::vec2(vertex->rfTextureCoord[0], vertex->rfTextureCoord[1])));
                }

                gfx::calculate_tangents(vertices, indices, GL_TRIANGLES);

                submodel_name = std::string(component == -1 ? rendermodel_name : component_name);
                sm = controllers[i]->model->AppendChildFromGeometry(submodel_name, "__root", vertices, indices);

                if (model->diffuseTextureId >= 0)
                    controllers[i]->pending_textures.push_back(std::make_pair(sm, model->diffuseTextureId));

                vr::VRRenderModels()->FreeRenderModel(model);

                if (component != -1 && update_component(controllers[i]->rendermodel, controllers[i]->inputhandle, sm))
                    controllers[i]->active_components.push_back(sm);

component_done:
                controllers[i]->pending_components.erase(controllers[i]->pending_components.begin() + c);

                if (controllers[i]->pending_components.empty())
                    controllers[i]->model->Init();
            }

            for (size_t t = controllers[i]->pending_textures.size(); t-- != 0;) {
                TSubModel *sm = controllers[i]->pending_textures[t].first;
                const vr::TextureID_t texture_id = controllers[i]->pending_textures[t].second;

                if (GfxRenderer->Material(sm->GetMaterial()).textures[0] == null_handle)
                    sm->ReplaceMaterial("internal_src:OPENVR" + std::to_string(texture_id));
                opengl_texture &tex = GfxRenderer->Texture(GfxRenderer->Material(sm->GetMaterial()).textures[0]);

                if (tex.is_stub()) {
                    vr::RenderModel_TextureMap_t *texturemap;
                    auto ret = vr::VRRenderModels()->LoadTexture_Async(texture_id, &texturemap);

                    if (ret == vr::VRRenderModelError_Loading)
                        continue;
                    else if (ret != vr::VRRenderModelError_None)
                        goto texture_done;

                    if (texturemap->format == vr::VRRenderModelTextureFormat_RGBA8_SRGB)
                        tex.make_from_memory(texturemap->unWidth, texturemap->unHeight, texturemap->rubTextureMapData);
                    else
                        ErrorLog("openvr texture format " + std::to_string(texturemap->format) + " not supported");

                    vr::VRRenderModels()->FreeTexture(texturemap);
                }

texture_done:
                controllers[i]->pending_textures.erase(controllers[i]->pending_textures.begin() + t);
            }

            if (controllers[i]->model) {
                glm::vec3 trans = glm::vec3(device_pose[3]);
                glm::mat3 rot = glm::mat3(device_pose);
                trans -= Global.viewport_move;

                controllers[i]->model->GetSMRoot()->ReplaceMatrix(glm::translate(glm::mat4(), trans) * glm::mat4(rot));

                for (TSubModel *component : controllers[i]->active_components)
                    update_component(controllers[i]->rendermodel, controllers[i]->inputhandle, component);
            }

            if (i == primary_controller)
                primary_controller_transform = device_pose;
        }
    }

    vr::VRActiveActionSet_t active_actionset;
    active_actionset.ulActionSet = actionset;
    active_actionset.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
    active_actionset.nPriority = 0;

    vr::VRInput()->UpdateActionState(&active_actionset, sizeof(active_actionset), 1);

    vr::InputDigitalActionData_t actiondata_pri;
    vr::VRInput()->GetDigitalActionData(primary_action, &actiondata_pri, sizeof(actiondata_pri), vr::k_ulInvalidInputValueHandle);

    vr::InputDigitalActionData_t actiondata_sec;
    vr::VRInput()->GetDigitalActionData(secondary_action, &actiondata_sec, sizeof(actiondata_sec), vr::k_ulInvalidInputValueHandle);

    bool any_changed = (actiondata_pri.bChanged || actiondata_sec.bChanged);
    bool any_active = (actiondata_pri.bState || actiondata_sec.bState);

    if (command_active == user_command::none && any_changed) {
        bool primary = actiondata_pri.bState;

        vr::InputOriginInfo_t origin;
        vr::VRInput()->GetOriginTrackedDeviceInfo(primary ? actiondata_pri.activeOrigin : actiondata_sec.activeOrigin, &origin, sizeof(origin));
        primary_controller = origin.trackedDeviceIndex;

        GfxRenderer->Pick_Control_Callback([this, primary](TSubModel const *control, const glm::vec2 pos) {
            if (command_active != user_command::none)
                return;

            if (control != nullptr) {
                if (control->screen_touch_list) {
                    control->screen_touch_list->emplace_back(pos);
                }

                auto it = m_buttonbindings.find(simulation::Train->GetLabel(control));
                if (it == m_buttonbindings.end())
                    return;

                if (primary)
                    command_active = it->second.primary;
                else
                    command_active = it->second.secondary;
                command_relay relay;
                relay.post(command_active, 0, 0, GLFW_PRESS, 0);
            }
        });
    } else if (command_active != user_command::none && !any_active) {
        command_relay relay;
        relay.post(command_active, 0, 0, GLFW_RELEASE, 0);

        command_active = user_command::none;
    }
/*
    bool prev_should_pause = should_pause;
    should_pause = vr_system->ShouldApplicationPause();
    if (prev_should_pause != should_pause)
        relay.post(user_command::focuspauseset, should_pause ? 1.0 : 0.0, 0.0, GLFW_PRESS, 0);*/

    draw_controllers = !vr_system->IsSteamVRDrawingControllers();

    vr::VREvent_t event;
    while (vr_system->PollNextEvent(&event, sizeof(event)))
    {
        if (event.eventType == vr::VREvent_Quit) {
            vr_system->AcknowledgeQuit_Exiting();
            Application.queue_quit(true);
        }
    }
}

glm::mat4 vr_openvr::get_pick_transform()
{
    return primary_controller_transform;
}

bool vr_openvr::update_component(const std::string &rendermodel, vr::VRInputValueHandle_t handle, TSubModel *component)
{
    vr::RenderModel_ControllerMode_State_t state;
    state.bScrollWheelVisible = false;

    vr::RenderModel_ComponentState_t component_state;

    vr::VRRenderModels()->GetComponentStateForDevicePath(rendermodel.c_str(), component->pName.c_str(), handle, &state, &component_state);

    glm::mat4 component_pose = get_matrix(component_state.mTrackingToComponentRenderModel);
    bool visible = (component_state.uProperties & vr::VRComponentProperty_IsVisible);

    component->ReplaceMatrix(component_pose);
    component->iVisible = (visible ? 1 : 0);

    return !(component_state.uProperties & vr::VRComponentProperty_IsStatic);
}

void vr_openvr::submit(vr_openvr::eye_e eye, opengl_texture* tex)
{
    vr::Texture_t hmd_tex =
        { (void*)(uint64_t)tex->id,
          vr::TextureType_OpenGL,
          vr::ColorSpace_Gamma };

    vr::VRCompositor()->Submit(eye == vr_interface::eye_left ? vr::Eye_Left : vr::Eye_Right, &hmd_tex);
}

std::vector<TModel3d*> vr_openvr::get_render_models()
{
    std::vector<TModel3d*> list;

    if (!draw_controllers)
        return list;

    for (auto entry : controllers)
    {
        if (entry && entry->model && entry->pending_components.empty())
            list.push_back(entry->model.get());
    }

    return list;
}

void vr_openvr::finish_frame()
{
    vr::VRCompositor()->PostPresentHandoff();
}

vr_openvr::~vr_openvr()
{
    vr::VR_Shutdown();
}

std::unique_ptr<vr_interface> vr_openvr::create_func()
{
    return std::unique_ptr<vr_interface>(new vr_openvr());
}

std::unordered_map<std::string, vr_openvr::button_bindings> vr_openvr::m_buttonbindings =
{
    /*{ "jointctrl:", {
        user_command::jointcontrollerset,
        user_command::none } },*/
    { "mainctrl:", {
        user_command::mastercontrollerincrease,
        user_command::mastercontrollerdecrease } },
    { "scndctrl:", {
        user_command::secondcontrollerincrease,
        user_command::secondcontrollerdecrease } },
    { "shuntmodepower:", {
        user_command::secondcontrollerincrease,
        user_command::secondcontrollerdecrease } },
    { "tempomat_sw:", {
        user_command::tempomattoggle,
        user_command::none } },
    { "dirkey:", {
        user_command::reverserincrease,
        user_command::reverserdecrease } },
    { "dirforward_bt:", {
        user_command::reverserforward,
        user_command::none } },
    { "dirneutral_bt:", {
        user_command::reverserneutral,
        user_command::none } },
    { "dirbackward_bt:", {
        user_command::reverserbackward,
        user_command::none } },
    { "brakectrl:", {
        user_command::trainbrakeincrease,
        user_command::trainbrakedecrease } },
    { "localbrake:", {
        user_command::independentbrakeincrease,
        user_command::independentbrakedecrease } },
    { "manualbrake:", {
        user_command::manualbrakeincrease,
        user_command::manualbrakedecrease } },
    { "alarmchain:", {
        user_command::alarmchaintoggle,
        user_command::none } },
    { "alarmchainon:", {
        user_command::alarmchainenable,
        user_command::none} },
    { "alarmchainoff:", {
        user_command::alarmchainenable,
        user_command::none} },
    { "brakeprofile_sw:", {
        user_command::brakeactingspeedincrease,
        user_command::brakeactingspeeddecrease } },
    // TODO: dedicated methods for braking speed switches
    { "brakeprofileg_sw:", {
        user_command::brakeactingspeedsetcargo,
        user_command::brakeactingspeedsetpassenger } },
    { "brakeprofiler_sw:", {
        user_command::brakeactingspeedsetrapid,
        user_command::brakeactingspeedsetpassenger } },
    { "brakeopmode_sw:", {
        user_command::trainbrakeoperationmodeincrease,
        user_command::trainbrakeoperationmodedecrease } },
    { "maxcurrent_sw:", {
        user_command::motoroverloadrelaythresholdtoggle,
        user_command::none } },
    { "waterpumpbreaker_sw:", {
        user_command::waterpumpbreakertoggle,
        user_command::none } },
    { "waterpump_sw:", {
        user_command::waterpumptoggle,
        user_command::none } },
    { "waterheaterbreaker_sw:", {
        user_command::waterheaterbreakertoggle,
        user_command::none } },
    { "waterheater_sw:", {
        user_command::waterheatertoggle,
        user_command::none } },
    { "fuelpump_sw:", {
        user_command::fuelpumptoggle,
        user_command::none } },
    { "oilpump_sw:", {
        user_command::oilpumptoggle,
        user_command::none } },
    { "motorblowersfront_sw:", {
        user_command::motorblowerstogglefront,
        user_command::none } },
    { "motorblowersrear_sw:", {
        user_command::motorblowerstogglerear,
        user_command::none } },
    { "motorblowersalloff_sw:", {
        user_command::motorblowersdisableall,
        user_command::none } },
    { "coolingfans_sw:", {
        user_command::coolingfanstoggle,
        user_command::none } },
    { "main_off_bt:", {
        user_command::linebreakeropen,
        user_command::none } },
    { "main_on_bt:",{
        user_command::linebreakerclose,
        user_command::none } },
    { "security_reset_bt:", {
        user_command::alerteracknowledge,
        user_command::none } },
    { "shp_reset_bt:", {
        user_command::cabsignalacknowledge,
        user_command::none } },
    { "releaser_bt:", {
        user_command::independentbrakebailoff,
        user_command::none } },
    { "springbraketoggle_bt:",{
        user_command::springbraketoggle,
        user_command::none } },
    { "springbrakeon_bt:",{
        user_command::springbrakeenable,
        user_command::none } },
    { "springbrakeoff_bt:",{
        user_command::springbrakedisable,
        user_command::none } },
    { "universalbrake1_bt:",{
        user_command::universalbrakebutton1,
        user_command::none } },
    { "universalbrake2_bt:",{
        user_command::universalbrakebutton2,
        user_command::none } },
    { "universalbrake3_bt:",{
        user_command::universalbrakebutton3,
        user_command::none } },
    { "epbrake_bt:",{
        user_command::epbrakecontroltoggle,
        user_command::none } },
    { "sand_bt:", {
        user_command::sandboxactivate,
        user_command::none } },
    { "antislip_bt:", {
        user_command::wheelspinbrakeactivate,
        user_command::none } },
    { "horn_bt:", {
        user_command::hornhighactivate,
        user_command::hornlowactivate } },
    { "hornlow_bt:", {
        user_command::hornlowactivate,
        user_command::none } },
    { "hornhigh_bt:", {
        user_command::hornhighactivate,
        user_command::none } },
    { "whistle_bt:", {
        user_command::whistleactivate,
        user_command::none } },
    { "fuse_bt:", {
        user_command::motoroverloadrelayreset,
        user_command::none } },
    { "converterfuse_bt:", {
        user_command::converteroverloadrelayreset,
        user_command::none } },
    { "relayreset1_bt:", {
        user_command::universalrelayreset1,
        user_command::none } },
    { "relayreset2_bt:", {
        user_command::universalrelayreset2,
        user_command::none } },
    { "relayreset3_bt:", {
        user_command::universalrelayreset3,
        user_command::none } },
    { "stlinoff_bt:", {
        user_command::motorconnectorsopen,
        user_command::none } },
    { "doorleftpermit_sw:", {
        user_command::doorpermitleft,
        user_command::none } },
    { "doorrightpermit_sw:", {
        user_command::doorpermitright,
        user_command::none } },
    { "doorpermitpreset_sw:", {
        user_command::doorpermitpresetactivatenext,
        user_command::doorpermitpresetactivateprevious } },
    { "door_left_sw:", {
        user_command::doortoggleleft,
        user_command::none } },
    { "door_right_sw:", {
        user_command::doortoggleright,
        user_command::none } },
    { "doorlefton_sw:", {
        user_command::dooropenleft,
        user_command::none } },
    { "doorrighton_sw:", {
        user_command::dooropenright,
        user_command::none } },
    { "doorleftoff_sw:", {
        user_command::doorcloseleft,
        user_command::none } },
    { "doorrightoff_sw:", {
        user_command::doorcloseright,
        user_command::none } },
    { "doorallon_sw:", {
        user_command::dooropenall,
        user_command::none } },
    { "dooralloff_sw:", {
        user_command::doorcloseall,
        user_command::none } },
    { "doorstep_sw:", {
        user_command::doorsteptoggle,
        user_command::none } },
    { "doormode_sw:", {
        user_command::doormodetoggle,
        user_command::none } },
    { "departure_signal_bt:", {
        user_command::departureannounce,
        user_command::none } },
    { "upperlight_sw:", {
        user_command::headlighttoggleupper,
        user_command::none } },
    { "leftlight_sw:", {
        user_command::headlighttoggleleft,
        user_command::none } },
    { "rightlight_sw:", {
        user_command::headlighttoggleright,
        user_command::none } },
    { "dimheadlights_sw:", {
        user_command::headlightsdimtoggle,
        user_command::none } },
    { "leftend_sw:", {
        user_command::redmarkertoggleleft,
        user_command::none } },
    { "rightend_sw:", {
        user_command::redmarkertoggleright,
        user_command::none } },
    { "lights_sw:", {
        user_command::lightspresetactivatenext,
        user_command::lightspresetactivateprevious } },
    { "rearupperlight_sw:", {
        user_command::headlighttogglerearupper,
        user_command::none } },
    { "rearleftlight_sw:", {
        user_command::headlighttogglerearleft,
        user_command::none } },
    { "rearrightlight_sw:", {
        user_command::headlighttogglerearright,
        user_command::none } },
    { "rearleftend_sw:", {
        user_command::redmarkertogglerearleft,
        user_command::none } },
    { "rearrightend_sw:", {
        user_command::redmarkertogglerearright,
        user_command::none } },
    { "compressor_sw:", {
        user_command::compressortoggle,
        user_command::none } },
    { "compressorlocal_sw:", {
        user_command::compressortogglelocal,
        user_command::none } },
    { "compressorlist_sw:", {
        user_command::compressorpresetactivatenext,
        user_command::compressorpresetactivateprevious } },
    { "converter_sw:", {
        user_command::convertertoggle,
        user_command::none } },
    { "converterlocal_sw:", {
        user_command::convertertogglelocal,
        user_command::none } },
    { "converteroff_sw:", {
        user_command::convertertoggle,
        user_command::none } }, // TODO: dedicated converter shutdown command
    { "main_sw:", {
        user_command::linebreakertoggle,
        user_command::none } },
    { "radio_sw:", {
        user_command::radiotoggle,
        user_command::none } },
    { "radioon_sw:", {
        user_command::radioenable,
        user_command::none } },
    { "radiooff_sw:", {
        user_command::radiodisable,
        user_command::none } },
    { "radiochannel_sw:", {
        user_command::radiochannelincrease,
        user_command::radiochanneldecrease } },
    { "radiochannelprev_sw:", {
        user_command::radiochanneldecrease,
        user_command::none } },
    { "radiochannelnext_sw:", {
        user_command::radiochannelincrease,
        user_command::none } },
    { "radiostop_sw:", {
        user_command::radiostopsend,
        user_command::none } },
    { "radiostopon_sw:", {
        user_command::radiostopenable,
        user_command::none } },
    { "radiostopoff_sw:", {
        user_command::radiostopdisable,
        user_command::none } },
    { "radiotest_sw:", {
        user_command::radiostoptest,
        user_command::none } },
    { "radiocall3_sw:", {
        user_command::radiocall3send,
        user_command::none } },
    { "radiovolume_sw:",{
        user_command::radiovolumeincrease,
        user_command::radiovolumedecrease } },
    { "radiovolumeprev_sw:",{
        user_command::radiovolumedecrease,
        user_command::none } },
    { "radiovolumenext_sw:",{
        user_command::radiovolumeincrease,
        user_command::none } },
    { "pantfront_sw:", {
        user_command::pantographtogglefront,
        user_command::none } },
    { "pantrear_sw:", {
        user_command::pantographtogglerear,
        user_command::none } },
    { "pantfrontoff_sw:", {
        user_command::pantographlowerfront,
        user_command::none } },
    { "pantrearoff_sw:", {
        user_command::pantographlowerrear,
        user_command::none } },
    { "pantalloff_sw:", {
        user_command::pantographlowerall,
        user_command::none } },
    { "pantselected_sw:", {
        user_command::pantographtoggleselected,
        user_command::none } }, // TBD: bind lowerselected in case of toggle switch
    { "pantselectedoff_sw:", {
        user_command::pantographlowerselected,
        user_command::none } },
    { "pantselect_sw:", {
        user_command::pantographselectnext,
        user_command::pantographselectprevious } },
    { "pantcompressor_sw:", {
        user_command::pantographcompressoractivate,
        user_command::none } },
    { "pantcompressorvalve_sw:", {
        user_command::pantographcompressorvalvetoggle,
        user_command::none } },
    { "trainheating_sw:", {
        user_command::heatingtoggle,
        user_command::none } },
    { "signalling_sw:", {
        user_command::mubrakingindicatortoggle,
        user_command::none } },
    { "door_signalling_sw:", {
        user_command::doorlocktoggle,
        user_command::none } },
    { "nextcurrent_sw:", {
        user_command::mucurrentindicatorothersourceactivate,
        user_command::none } },
    { "distancecounter_sw:", {
        user_command::distancecounteractivate,
        user_command::none } },
    { "instrumentlight_sw:", {
        user_command::instrumentlighttoggle,
        user_command::none } },
    { "dashboardlight_sw:", {
        user_command::dashboardlighttoggle,
        user_command::none } },
    { "dashboardlighton_sw:", {
        user_command::dashboardlightenable,
        user_command::none } },
    { "dashboardlightoff_sw:", {
        user_command::dashboardlightdisable,
        user_command::none } },
    { "timetablelight_sw:", {
        user_command::timetablelighttoggle,
        user_command::none } },
    { "timetablelighton_sw:", {
        user_command::timetablelightenable,
        user_command::none } },
    { "timetablelightoff_sw:", {
        user_command::timetablelightdisable,
        user_command::none } },
    { "cablight_sw:", {
        user_command::interiorlighttoggle,
        user_command::none } },
    { "cablightdim_sw:", {
        user_command::interiorlightdimtoggle,
        user_command::none } },
    { "compartmentlights_sw:", {
        user_command::compartmentlightstoggle,
        user_command::none } },
    { "compartmentlightson_sw:", {
        user_command::compartmentlightsenable,
        user_command::none } },
    { "compartmentlightsoff_sw:", {
        user_command::compartmentlightsdisable,
        user_command::none } },
    { "battery_sw:", {
        user_command::batterytoggle,
        user_command::none } },
    { "batteryon_sw:", {
        user_command::batteryenable,
        user_command::none } },
    { "batteryoff_sw:", {
        user_command::batterydisable,
        user_command::none } },
    { "couplingdisconnect_sw:",{
        user_command::occupiedcarcouplingdisconnect,
        user_command::none } },
    { "universal0:", {
        user_command::generictoggle0,
        user_command::none } },
    { "universal1:", {
        user_command::generictoggle1,
        user_command::none } },
    { "universal2:", {
        user_command::generictoggle2,
        user_command::none } },
    { "universal3:", {
        user_command::generictoggle3,
        user_command::none } },
    { "universal4:", {
        user_command::generictoggle4,
        user_command::none } },
    { "universal5:", {
        user_command::generictoggle5,
        user_command::none } },
    { "universal6:", {
        user_command::generictoggle6,
        user_command::none } },
    { "universal7:", {
        user_command::generictoggle7,
        user_command::none } },
    { "universal8:", {
        user_command::generictoggle8,
        user_command::none } },
    { "universal9:", {
        user_command::generictoggle9,
        user_command::none } },
    { "speedinc_bt:",{
        user_command::speedcontrolincrease,
        user_command::none } },
    { "speeddec_bt:",{
        user_command::speedcontroldecrease,
        user_command::none } },
    { "speedctrlpowerinc_bt:",{
        user_command::speedcontrolpowerincrease,
        user_command::none } },
    { "speedctrlpowerdec_bt:",{
        user_command::speedcontrolpowerdecrease,
        user_command::none } },
    { "speedbutton0:",{
        user_command::speedcontrolbutton0,
        user_command::none } },
    { "speedbutton1:",{
        user_command::speedcontrolbutton1,
        user_command::none } },
    { "speedbutton2:",{
        user_command::speedcontrolbutton2,
        user_command::none } },
    { "speedbutton3:",{
        user_command::speedcontrolbutton3,
        user_command::none } },
    { "speedbutton4:",{
        user_command::speedcontrolbutton4,
        user_command::none } },
    { "speedbutton5:",{
        user_command::speedcontrolbutton5,
        user_command::none } },
    { "speedbutton6:",{
        user_command::speedcontrolbutton6,
        user_command::none } },
    { "speedbutton7:",{
        user_command::speedcontrolbutton7,
        user_command::none } },
    { "speedbutton8:",{
        user_command::speedcontrolbutton8,
        user_command::none } },
    { "speedbutton9:",{
        user_command::speedcontrolbutton9,
        user_command::none } },
    { "inverterenable1_bt:",{
        user_command::inverterenable1,
        user_command::none } },
    { "inverterenable2_bt:",{
        user_command::inverterenable2,
        user_command::none } },
    { "inverterenable3_bt:",{
        user_command::inverterenable3,
        user_command::none } },
    { "inverterenable4_bt:",{
        user_command::inverterenable4,
        user_command::none } },
    { "inverterenable5_bt:",{
        user_command::inverterenable5,
        user_command::none } },
    { "inverterenable6_bt:",{
        user_command::inverterenable6,
        user_command::none } },
    { "inverterenable7_bt:",{
        user_command::inverterenable7,
        user_command::none } },
    { "inverterenable8_bt:",{
        user_command::inverterenable8,
        user_command::none } },
    { "inverterenable9_bt:",{
        user_command::inverterenable9,
        user_command::none } },
    { "inverterenable10_bt:",{
        user_command::inverterenable10,
        user_command::none } },
    { "inverterenable11_bt:",{
        user_command::inverterenable11,
        user_command::none } },
    { "inverterenable12_bt:",{
        user_command::inverterenable12,
        user_command::none } },
    { "inverterdisable1_bt:",{
        user_command::inverterdisable1,
        user_command::none } },
    { "inverterdisable2_bt:",{
        user_command::inverterdisable2,
        user_command::none } },
    { "inverterdisable3_bt:",{
        user_command::inverterdisable3,
        user_command::none } },
    { "inverterdisable4_bt:",{
        user_command::inverterdisable4,
        user_command::none } },
    { "inverterdisable5_bt:",{
        user_command::inverterdisable5,
        user_command::none } },
    { "inverterdisable6_bt:",{
        user_command::inverterdisable6,
        user_command::none } },
    { "inverterdisable7_bt:",{
        user_command::inverterdisable7,
        user_command::none } },
    { "inverterdisable8_bt:",{
        user_command::inverterdisable8,
        user_command::none } },
    { "inverterdisable9_bt:",{
        user_command::inverterdisable9,
        user_command::none } },
    { "inverterdisable10_bt:",{
        user_command::inverterdisable10,
        user_command::none } },
    { "inverterdisable11_bt:",{
        user_command::inverterdisable11,
        user_command::none } },
    { "inverterdisable12_bt:",{
        user_command::inverterdisable12,
        user_command::none } },
    { "invertertoggle1_bt:",{
        user_command::invertertoggle1,
        user_command::none } },
    { "invertertoggle2_bt:",{
        user_command::invertertoggle2,
        user_command::none } },
    { "invertertoggle3_bt:",{
        user_command::invertertoggle3,
        user_command::none } },
    { "invertertoggle4_bt:",{
        user_command::invertertoggle4,
        user_command::none } },
    { "invertertoggle5_bt:",{
        user_command::invertertoggle5,
        user_command::none } },
    { "invertertoggle6_bt:",{
        user_command::invertertoggle6,
        user_command::none } },
    { "invertertoggle7_bt:",{
        user_command::invertertoggle7,
        user_command::none } },
    { "invertertoggle8_bt:",{
        user_command::invertertoggle8,
        user_command::none } },
    { "invertertoggle9_bt:",{
        user_command::invertertoggle9,
        user_command::none } },
    { "invertertoggle10_bt:",{
        user_command::invertertoggle10,
        user_command::none } },
    { "invertertoggle11_bt:",{
        user_command::invertertoggle11,
        user_command::none } },
    { "invertertoggle12_bt:",{
        user_command::invertertoggle12,
        user_command::none } },
};

bool vr_openvr::backend_register = vr_interface_factory::get_instance()->register_backend("openvr", vr_openvr::create_func);
