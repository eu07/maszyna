#include "stdafx.h"
#include "widgets/vehicleparams.h"
#include "simulation.h"
#include "driveruipanels.h"
#include "Driver.h"
#include "Train.h"

ui::vehicleparams_panel::vehicleparams_panel(const std::string &vehicle)
    : ui_panel(std::string(STR("Vehicle parameters")) + ": " + vehicle, false), m_vehicle_name(vehicle)
{
    vehicle_mini = GfxRenderer->Fetch_Texture("vehicle_mini");
}

void screen_window_callback(ImGuiSizeCallbackData *data) {
	auto config = static_cast<const global_settings::pythonviewport_config*>(data->UserData);
	data->DesiredSize.y = data->DesiredSize.x * (float)config->size.y / (float)config->size.x;
}

void ui::vehicleparams_panel::draw_infobutton(const char *str, ImVec2 pos, const ImVec4 color)
{
	if (pos.x != -1.0f) {
		ImVec2 window_size = ImGui::GetWindowSize();

		ImGuiStyle &style = ImGui::GetStyle();
		ImVec2 text_size = ImGui::CalcTextSize(str);
		ImVec2 button_size = ImVec2(
		             text_size.x + style.FramePadding.x * 2.0f,
		             text_size.y + style.FramePadding.y * 2.0f);

		pos.x = pos.x * window_size.x / 512.0f - button_size.x / 2.0f;
		pos.y = pos.y * window_size.y / 118.0f - button_size.y / 2.0f;

		ImGui::SetCursorPos(pos);
	}

	if ((color.x + color.y + color.z) / 3.0f < 0.5f)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	else
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, color);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

	ImGui::Button(str);
	ImGui::SameLine();

	ImGui::PopStyleColor(4);
}

void ui::vehicleparams_panel::draw_mini(const TMoverParameters &mover)
{
	if (vehicle_mini == null_handle)
		return;

    opengl_texture &tex = GfxRenderer->Texture(vehicle_mini);
	tex.create();

	ImVec2 size = ImGui::GetContentRegionAvail();
	float x = size.x;
	float y = x * ((float)tex.height() / tex.width());

	if (ImGui::BeginChild("mini", ImVec2(x, y)))
	{
		ImGui::Image(reinterpret_cast<void*>(tex.id), ImVec2(x, y), ImVec2(0, 1), ImVec2(1, 0));

        if (mover.Pantographs[end::rear].is_active)
			draw_infobutton(u8"╨╨╨", ImVec2(126, 10));
        if (mover.Pantographs[end::front].is_active)
			draw_infobutton(u8"╨╨╨", ImVec2(290, 10));

		if (mover.Battery)
			draw_infobutton(STR_C("bat."), ImVec2(120, 55));
		if (mover.Mains)
			draw_infobutton(STR_C("main."));
		if (mover.ConverterFlag)
			draw_infobutton(STR_C("conv."));
		if (mover.CompressorFlag)
			draw_infobutton(STR_C("comp."));

		if (mover.WarningSignal)
			draw_infobutton(STR_C("horn"), ImVec2(361, 11));

		if (mover.iLights[end::front] & light::redmarker_left)
			draw_infobutton("o", ImVec2(490, 71), ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		else if (mover.iLights[end::front] & light::headlight_left)
			draw_infobutton("O", ImVec2(490, 71), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		if (mover.iLights[end::front] & light::redmarker_right)
			draw_infobutton("o", ImVec2(443, 71), ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		else if (mover.iLights[end::front] & light::headlight_right)
			draw_infobutton("O", ImVec2(443, 71), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		if (mover.iLights[end::front] & light::headlight_upper)
			draw_infobutton("O", ImVec2(467, 18), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
	ImGui::EndChild();
}

void ui::vehicleparams_panel::render_contents()
{
	TDynamicObject *vehicle_ptr = simulation::Vehicles.find(m_vehicle_name);
	if (!vehicle_ptr) {
		is_open = false;
		return;
	}

	TTrain *train_ptr = simulation::Trains.find(m_vehicle_name);
	if (train_ptr) {
		const TTrain::screenentry_sequence &screens = train_ptr->get_screens();

		for (const auto &viewport : Global.python_viewports) {
			for (auto const &entry : screens) {
                if (entry.script != viewport.surface)
					continue;

				std::string window_name = STR("Screen") + "##" + viewport.surface;
				ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(2500, 2500), screen_window_callback,
				                                    const_cast<global_settings::pythonviewport_config*>(&viewport));
				if (ImGui::Begin(window_name.c_str())) {
					float aspect = (float)viewport.size.y / viewport.size.x;

					glm::mat3 proj = glm::translate(glm::scale(glm::mat3(), 1.0f / viewport.scale), viewport.offset);

					glm::vec2 uv0 = glm::vec2(proj * glm::vec3(0.0f, 1.0f, 1.0f));
					glm::vec2 uv1 = glm::vec2(proj * glm::vec3(1.0f, 0.0f, 1.0f));

					ImVec2 size = ImGui::GetContentRegionAvail();

                    ImGui::Image(reinterpret_cast<void*>(entry.rt->shared_tex), size, ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
				}
				ImGui::End();
			}
		}
	}

	TDynamicObject &vehicle = *vehicle_ptr;
	TMoverParameters &mover = *vehicle.MoverParameters;

	std::vector<text_line> lines;
	std::array<char, 1024> buffer;

	auto const isdieselenginepowered { ( mover.EngineType == TEngineType::DieselElectric ) || ( mover.EngineType == TEngineType::DieselEngine ) };
	auto const isdieselinshuntmode { mover.ShuntMode && mover.EngineType == TEngineType::DieselElectric };

	std::snprintf(
	    buffer.data(), buffer.size(),
	    STR_C("Devices: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%s%s\nPower transfers: %.0f@%.0f%s%s%s%.0f@%.0f"),
	    // devices
	    ( mover.Battery ? 'B' : '.' ),
	    ( mover.Mains ? 'M' : '.' ),
	    ( mover.FuseFlag ? '!' : '.' ),
        ( mover.Pantographs[end::rear].is_active ? ( mover.PantRearVolt > 0.0 ? 'O' : 'o' ) : '.' ),
        ( mover.Pantographs[end::front].is_active ? ( mover.PantFrontVolt > 0.0 ? 'P' : 'p' ) : '.' ),
	    ( mover.PantPressLockActive ? '!' : ( mover.PantPressSwitchActive ? '*' : '.' ) ),
	    ( mover.WaterPump.is_active ? 'W' : ( false == mover.WaterPump.breaker ? '-' : ( mover.WaterPump.is_enabled ? 'w' : '.' ) ) ),
	    ( true == mover.WaterHeater.is_damaged ? '!' : ( mover.WaterHeater.is_active ? 'H' : ( false == mover.WaterHeater.breaker ? '-' : ( mover.WaterHeater.is_enabled ? 'h' : '.' ) ) ) ),
	    ( mover.FuelPump.is_active ? 'F' : ( mover.FuelPump.is_enabled ? 'f' : '.' ) ),
	    ( mover.OilPump.is_active ? 'O' : ( mover.OilPump.is_enabled ? 'o' : '.' ) ),
	    ( false == mover.ConverterAllowLocal ? '-' : ( mover.ConverterAllow ? ( mover.ConverterFlag ? 'X' : 'x' ) : '.' ) ),
	    ( mover.ConvOvldFlag ? '!' : '.' ),
	    ( mover.CompressorFlag ? 'C' : ( false == mover.CompressorAllowLocal ? '-' : ( ( mover.CompressorAllow || mover.CompressorStart == start_t::automatic ) ? 'c' : '.' ) ) ),
	    ( mover.CompressorGovernorLock ? '!' : '.' ),
	    "",
	    std::string( isdieselenginepowered ? STR(" oil pressure: ") + to_string( mover.OilPump.pressure, 2 )  : "" ).c_str(),
	    // power transfers
	    mover.Couplers[ end::front ].power_high.voltage,
	    mover.Couplers[ end::front ].power_high.current,
	    std::string( mover.Couplers[ end::front ].power_high.is_local ? "" : "-" ).c_str(),
	    std::string( vehicle.DirectionGet() ? ":<<:" : ":>>:" ).c_str(),
	    std::string( mover.Couplers[ end::rear ].power_high.is_local ? "" : "-" ).c_str(),
	    mover.Couplers[ end::rear ].power_high.voltage,
	    mover.Couplers[ end::rear ].power_high.current );

	ImGui::TextUnformatted(buffer.data());

	std::snprintf(
	    buffer.data(), buffer.size(),
	    STR_C("Controllers:\n master: %d(%d), secondary: %s\nEngine output: %.1f, current: %.0f\nRevolutions:\n engine: %.0f, motors: %.0f\n engine fans: %.0f, motor fans: %.0f+%.0f, cooling fans: %.0f+%.0f"),
	    // controllers
	    mover.MainCtrlPos,
	    mover.MainCtrlActualPos,
	    std::string( isdieselinshuntmode ? to_string( mover.AnPos, 2 ) + STR(" (shunt mode)") : std::to_string( mover.ScndCtrlPos ) + "(" + std::to_string( mover.ScndCtrlActualPos ) + ")" ).c_str(),
	    // engine
	    mover.EnginePower,
	    std::abs( mover.TrainType == dt_EZT ? mover.ShowCurrent( 0 ) : mover.Im ),
	    // revolutions
	    std::abs( mover.enrot ) * 60,
	    std::abs( mover.nrot ) * mover.Transmision.Ratio * 60,
	    mover.RventRot * 60,
	    std::abs( mover.MotorBlowers[end::front].revolutions ),
	    std::abs( mover.MotorBlowers[end::rear].revolutions ),
	    mover.dizel_heat.rpmw,
	    mover.dizel_heat.rpmw2 );

	ImGui::TextUnformatted(buffer.data());

	if( isdieselenginepowered ) {
		std::snprintf(
		    buffer.data(), buffer.size(),
		    STR_C("\nTemperatures:\n engine: %.2f, oil: %.2f, water: %.2f%c%.2f"),
		    mover.dizel_heat.Ts,
		    mover.dizel_heat.To,
		    mover.dizel_heat.temperatura1,
		    ( mover.WaterCircuitsLink ? '-' : '|' ),
		    mover.dizel_heat.temperatura2 );
		ImGui::TextUnformatted(buffer.data());
	}

	std::string brakedelay;
	{
		std::vector<std::pair<int, std::string>> delays {
			{ bdelay_G, "G" },
			{ bdelay_P, "P" },
			{ bdelay_R, "R" },
			{ bdelay_M, "+Mg" } };

		for( auto const &delay : delays ) {
			if( ( mover.BrakeDelayFlag & delay.first ) == delay.first ) {
				brakedelay += delay.second;
			}
		}
	}

	std::snprintf(
	    buffer.data(), buffer.size(),
	    STR_C("Brakes:\n train: %.2f, independent: %.2f, mode: %d, delay: %s, load flag: %d\nBrake cylinder pressures:\n train: %.2f, independent: %.2f, status: 0x%.2x\nPipe pressures:\n brake: %.2f (hat: %.2f), main: %.2f, control: %.2f\nTank pressures:\n auxiliary: %.2f, main: %.2f, control: %.2f"),
	    // brakes
	    mover.fBrakeCtrlPos,
	    mover.LocalBrakePosA,
	    mover.BrakeOpModeFlag,
	    brakedelay.c_str(),
	    mover.LoadFlag,
	    // cylinders
	    mover.BrakePress,
	    mover.LocBrakePress,
	    mover.Hamulec->GetBrakeStatus(),
	    // pipes
	    mover.PipePress,
	    mover.BrakeCtrlPos2,
	    mover.ScndPipePress,
	    mover.CntrlPipePress,
	    // tanks
	    mover.Hamulec->GetBRP(),
	    mover.Compressor,
	    mover.Hamulec->GetCRP() );

	ImGui::TextUnformatted(buffer.data());

	if( mover.EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
		std::snprintf(
		    buffer.data(), buffer.size(),
		    STR_C(" pantograph: %.2f%cMT"),
		    mover.PantPress,
		    ( mover.bPantKurek3 ? '-' : '|' ) );
		ImGui::TextUnformatted(buffer.data());
	}

	std::snprintf(
	    buffer.data(), buffer.size(),
        STR_C("Forces:\n tractive: %.1f, brake: %.1f, friction: %.2f%s\nAcceleration:\n tangential: %.2f, normal: %.2f (path radius: %s)\nVelocity: %.2f, distance traveled: %.2f\nPosition: [%.2f, %.2f, %.2f]"),
	    // forces
        mover.Ft * 0.001f * ( mover.CabActive ? mover.CabActive : vehicle.ctOwner ? vehicle.ctOwner->Controlling()->CabActive : 1 ) + 0.001f,
	    mover.Fb * 0.001f,
	    mover.Adhesive( mover.RunningTrack.friction ),
	    ( mover.SlippingWheels ? " (!)" : "" ),
	    // acceleration
	    mover.AccSVBased,
        mover.AccN + 0.001f,
	    std::string( std::abs( mover.RunningShape.R ) > 10000.0 ? "~0" : to_string( mover.RunningShape.R, 0 ) ).c_str(),
	    // velocity
	    vehicle.GetVelocity(),
	    mover.DistCounter,
	    // position
	    vehicle.GetPosition().x,
	    vehicle.GetPosition().y,
	    vehicle.GetPosition().z );

	ImGui::TextUnformatted(buffer.data());

    std::pair<double, double> TrainsetPowerMeter;

    TDynamicObject *vehicle_iter = vehicle_ptr;
    while (vehicle_iter) {
        if (vehicle_iter->Next())
            vehicle_iter = vehicle_iter->Next();
        else
            break;
    }

    while (vehicle_iter) {
        TrainsetPowerMeter.first += vehicle_iter->MoverParameters->EnergyMeter.first;
        TrainsetPowerMeter.second += vehicle_iter->MoverParameters->EnergyMeter.second;
        vehicle_iter = vehicle_iter->Prev();
    }

    if (TrainsetPowerMeter.first != 0.0 || TrainsetPowerMeter.second != 0.0) {
        std::snprintf(buffer.data(), buffer.size(), STR_C("Electricity usage:\n drawn: %.1f kWh\n returned: %.1f kWh\n balance: %.1f kWh"),
                      TrainsetPowerMeter.first, -TrainsetPowerMeter.second, TrainsetPowerMeter.first + TrainsetPowerMeter.second);

        ImGui::TextUnformatted(buffer.data());
    }

	draw_mini(mover);

	if (ImGui::Button(STR_C("Radiostop")))
        m_relay.post(user_command::radiostopsend, 0.0, 0.0, GLFW_PRESS, 0, vehicle_ptr->GetPosition());
	ImGui::SameLine();

	if (ImGui::Button(STR_C("Stop and repair")))
		m_relay.post(user_command::resetconsist, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(STR_C("Reset position"))) {
		std::string payload = vehicle_ptr->name() + '%' + vehicle_ptr->initial_track->name();
		m_relay.post(user_command::consistteleport, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &payload);
		m_relay.post(user_command::resetconsist, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	}

	if (ImGui::Button(STR_C("Refill main tank")))
		m_relay.post(user_command::fillcompressor, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(STR_C("Rupture main pipe")))
		m_relay.post(user_command::pullalarmchain, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	ImGui::Button(STR_C("independent brake releaser"));
	if (ImGui::IsItemClicked())
		m_relay.post(user_command::consistreleaser, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	if (ImGui::IsItemDeactivated())
		m_relay.post(user_command::consistreleaser, 0.0, 0.0, GLFW_RELEASE, 0, glm::vec3(0.0f), &vehicle_ptr->name());

	if (vehicle_ptr->MoverParameters->V < 0.01) {
		if (ImGui::Button(STR_C("Move +500m")))
			m_relay.post(user_command::dynamicmove, 500.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
		ImGui::SameLine();

		if (ImGui::Button(STR_C("Move -500m")))
			m_relay.post(user_command::dynamicmove, -500.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	}
}
