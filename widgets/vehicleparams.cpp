#include "stdafx.h"
#include "widgets/vehicleparams.h"
#include "simulation.h"
#include "driveruipanels.h"
#include "Driver.h"
#include "Train.h"

ui::vehicleparams_panel::vehicleparams_panel(const std::string &vehicle)
    : ui_panel(std::string(locale::strings[locale::string::vehicleparams_window]) + ": " + vehicle, false), m_vehicle_name(vehicle)
{

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
		const TTrain::screen_map &screens = train_ptr->get_screens();

		for (const auto &viewport : Global.python_viewports) {
			for (auto const &entry : screens) {
				if (std::get<std::string>(entry) != viewport.surface)
					continue;

				float aspect = (float)viewport.size.y / viewport.size.x;

				glm::mat3 proj = glm::translate(glm::scale(glm::mat3(), 1.0f / viewport.scale), viewport.offset);

				glm::vec2 uv0 = glm::vec2(proj * glm::vec3(0.0f, 1.0f, 1.0f));
				glm::vec2 uv1 = glm::vec2(proj * glm::vec3(1.0f, 0.0f, 1.0f));

				glm::vec2 size = glm::vec2(500.0f, 500.0f * aspect) * Global.gui_screensscale;

				ImGui::Image(reinterpret_cast<void*>(std::get<1>(entry)->shared_tex), ImVec2(size.x, size.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
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
	    locale::strings[ locale::string::debug_vehicle_devicespower ].c_str(),
	    // devices
	    ( mover.Battery ? 'B' : '.' ),
	    ( mover.Mains ? 'M' : '.' ),
	    ( mover.FuseFlag ? '!' : '.' ),
	    ( mover.PantRearUp ? ( mover.PantRearVolt > 0.0 ? 'O' : 'o' ) : '.' ),
	    ( mover.PantFrontUp ? ( mover.PantFrontVolt > 0.0 ? 'P' : 'p' ) : '.' ),
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
	    std::string( isdieselenginepowered ? locale::strings[ locale::string::debug_vehicle_oilpressure ] + to_string( mover.OilPump.pressure, 2 )  : "" ).c_str(),
	    // power transfers
	    mover.Couplers[ end::front ].power_high.voltage,
	    mover.Couplers[ end::front ].power_high.current,
	    std::string( mover.Couplers[ end::front ].power_high.local ? "" : "-" ).c_str(),
	    std::string( vehicle.DirectionGet() ? ":<<:" : ":>>:" ).c_str(),
	    std::string( mover.Couplers[ end::rear ].power_high.local ? "" : "-" ).c_str(),
	    mover.Couplers[ end::rear ].power_high.voltage,
	    mover.Couplers[ end::rear ].power_high.current );

	ImGui::TextUnformatted(buffer.data());

	std::snprintf(
	    buffer.data(), buffer.size(),
	    locale::strings[ locale::string::debug_vehicle_controllersenginerevolutions ].c_str(),
	    // controllers
	    mover.MainCtrlPos,
	    mover.MainCtrlActualPos,
	    std::string( isdieselinshuntmode ? to_string( mover.AnPos, 2 ) + locale::strings[ locale::string::debug_vehicle_shuntmode ] : std::to_string( mover.ScndCtrlPos ) + "(" + std::to_string( mover.ScndCtrlActualPos ) + ")" ).c_str(),
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
		    locale::strings[ locale::string::debug_vehicle_temperatures ].c_str(),
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
	    locale::strings[ locale::string::debug_vehicle_brakespressures ].c_str(),
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
		    locale::strings[ locale::string::debug_vehicle_pantograph ].c_str(),
		    mover.PantPress,
		    ( mover.bPantKurek3 ? '-' : '|' ) );
		ImGui::TextUnformatted(buffer.data());
	}

	std::snprintf(
	    buffer.data(), buffer.size(),
	    locale::strings[ locale::string::debug_vehicle_forcesaccelerationvelocityposition ].c_str(),
	    // forces
	    mover.Ft * 0.001f * ( mover.ActiveCab ? mover.ActiveCab : vehicle.ctOwner ? vehicle.ctOwner->Controlling()->ActiveCab : 1 ) + 0.001f,
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

	if (ImGui::Button(LOC_STR(vehicleparams_radiostop)))
		m_relay.post(user_command::radiostop, 0.0, 0.0, GLFW_PRESS, 0, vehicle_ptr->GetPosition());
	ImGui::SameLine();

	if (ImGui::Button(LOC_STR(vehicleparams_reset)))
		m_relay.post(user_command::resetconsist, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(LOC_STR(vehicleparams_resetposition))) {
		std::string payload = vehicle_ptr->name() + '%' + vehicle_ptr->initial_track->name();
		m_relay.post(user_command::consistteleport, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &payload);
		m_relay.post(user_command::resetconsist, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	}

	if (ImGui::Button(LOC_STR(vehicleparams_resetpipe)))
		m_relay.post(user_command::fillcompressor, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(LOC_STR(vehicleparams_rupturepipe)))
		m_relay.post(user_command::pullalarmchain, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	ImGui::Button(LOC_STR(cab_releaser_bt));
	if (ImGui::IsItemClicked())
		m_relay.post(user_command::consistreleaser, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	if (ImGui::IsItemDeactivated())
		m_relay.post(user_command::consistreleaser, 0.0, 0.0, GLFW_RELEASE, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(LOC_STR(vehicleparams_move500f)))
		m_relay.post(user_command::dynamicmove, 500.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();

	if (ImGui::Button(LOC_STR(vehicleparams_move500b)))
		m_relay.post(user_command::dynamicmove, -500.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &vehicle_ptr->name());
	ImGui::SameLine();
}
