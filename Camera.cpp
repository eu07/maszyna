/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#include "stdafx.h"
#include "Camera.h"

#include "Globals.h"
#include "utilities.h"
#include "Console.h"
#include "Timer.h"
#include "MOVER.h"

void TCamera::init(Math3D::vector3 nPos, Math3D::vector3 nAngle)
{
	vUp = Math3D::vector3(0, 1, 0);
	velocity = Math3D::vector3(0, 0, 0);
	pitch = nAngle.x;
	yaw = nAngle.y;
	roll = nAngle.z;
	pos = nPos;
	type = (Global.bFreeFly ? TP_FREE : TP_FOLLOW);
};

void TCamera::reset()
{
	pitch = yaw = roll = 0;
	rotationOffsets = {};
};

void TCamera::onCursorMove(const double x, const double y)
{
	rotationOffsets += glm::dvec3{ y, x, 0.0 };
}

bool TCamera::onCommand(const command_data& command)
{
	auto const walkSpeed{ 1.0 };
	auto const runSpeed{ 10.0 };

	bool isCameraCommand{ true };
	switch (command.command) {
		case user_command::viewturn:
			onCursorMove(reinterpret_cast<double const&>(command.param1) * 0.005 * Global.fMouseXScale / Global.ZoomFactor,
			reinterpret_cast<double const&>(command.param2) * 0.01 * Global.fMouseYScale / Global.ZoomFactor);
			break;
		case user_command::movehorizontal:
		case user_command::movehorizontalfast: {
			auto const moveSpeed = (type == TP_FREE ? runSpeed : (type == TP_FOLLOW ? walkSpeed : 0.0));
			auto const speedMultiplier = (((type == TP_FREE) && (command.command == user_command::movehorizontalfast)) ? 30.0 : 1.0);
			// left-right
			auto const moveXParam{ reinterpret_cast<double const&>(command.param1) };
			// 2/3rd of the stick range enables walk speed, past that we lerp between walk and run speed
			auto const moveX{ walkSpeed + (std::max(0.0, std::abs(moveXParam) - 0.65) / 0.35) * (moveSpeed - walkSpeed) };
			moveRate.x = (moveXParam > 0.0 ? moveX * speedMultiplier : (moveXParam < 0.0 ? -moveX * speedMultiplier : 0.0));
			// forward-back
			double const moveZParam{ reinterpret_cast<double const&>(command.param2) };
			auto const moveZ{ walkSpeed + (std::max(0.0, std::abs(moveZParam) - 0.65) / 0.35) * (moveSpeed - walkSpeed) };
			// NOTE: z-axis is flipped given world coordinate system
			moveRate.z = (moveZParam > 0.0 ? -moveZ * speedMultiplier : (moveZParam < 0.0 ? moveZ * speedMultiplier : 0.0));
			break;
		}
		case user_command::movevertical:
		case user_command::moveverticalfast: {
			auto const moveSpeed = (type == TP_FREE ? runSpeed * 0.5 : type == TP_FOLLOW ? walkSpeed : 0.0);
			auto const speedMultiplier = (((type == TP_FREE) && (command.command == user_command::moveverticalfast)) ? 10.0 : 1.0);
			// up-down
			auto const moveYParam{ reinterpret_cast<double const&>(command.param1) };
			// 2/3rd of the stick range enables walk speed, past that we lerp between walk and run speed
			auto const moveY{ walkSpeed + (std::max(0.0, std::abs(moveYParam) - 0.65) / 0.35) * (moveSpeed - walkSpeed) };
			moveRate.y = (moveYParam > 0.0 ? moveY * speedMultiplier : (moveYParam < 0.0 ? -moveY * speedMultiplier : 0.0));
			break;
		}
		default:
			isCameraCommand = false;
			break;
	}

	return isCameraCommand;
}

void TCamera::update()
{
	if (FreeFlyModeFlag) {
		type = TP_FREE;
	} else {
		type = TP_FOLLOW;
	}

	// check for sent user commands
	// NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
	// ultimately we'll need to track position of camera/driver for all human entities present in the scenario
	command_data command;
	// NOTE: currently we're only storing commands for local entity and there's no id system in place,
	// so we're supplying 'default' entity id of 0
	while (simulation::Commands.pop(command, static_cast<std::size_t>(command_target::entity) | 0)) {
		onCommand(command);
	}

	auto const deltaTime{ Timer::GetDeltaRenderTime() }; // czas bez pauzy

	// update position
	if ((type == TP_FREE) || !Global.ctrlState || DebugCameraFlag) {
		// ctrl is used for mirror view, so we ignore the controls when in vehicle if ctrl is pressed
		// McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
		velocity.x = clamp(velocity.x + moveRate.x * 10.0 * deltaTime, -std::abs(moveRate.x), std::abs(moveRate.x));
		velocity.z = clamp(velocity.z + moveRate.z * 10.0 * deltaTime, -std::abs(moveRate.z), std::abs(moveRate.z));
		velocity.y = clamp(velocity.y + moveRate.y * 10.0 * deltaTime, -std::abs(moveRate.y), std::abs(moveRate.y));
	}

	if ((type == TP_FREE) || DebugCameraFlag) {
		// free movement position update is handled here, movement while in vehicle is handled by train update
		Math3D::vector3 vec = velocity;
		vec.RotateY(yaw);
		pos += vec * 5.0 * deltaTime;
	}
	// update rotation
	auto const rotationFactor{ std::min(1.0, 20 * deltaTime) };

	yaw -= (rotationFactor * rotationOffsets.y);
	rotationOffsets.y *= (1.0 - rotationFactor);
	while (yaw > M_PI) {
		yaw -= 2 * M_PI;
	}
	while (yaw < -M_PI) {
		yaw += 2 * M_PI;
	}

	pitch -= (rotationFactor * rotationOffsets.x);
	rotationOffsets.x *= (1.0 - rotationFactor);
	if (type == TP_FOLLOW) {
		// jeżeli jazda z pojazdem ograniczenie kąta spoglądania w dół i w górę
		pitch = clamp(pitch, -M_PI_4, M_PI_4);
	}
}

Math3D::vector3 TCamera::getDirection()
{
	glm::vec3 v = glm::normalize(glm::rotateY<float>(glm::vec3{ 0.f, 0.f, 1.f }, yaw));
	return Math3D::vector3(v.x, v.y, v.z);
}

bool TCamera::setMatrix(glm::dmat4& matrix)
{
	matrix = glm::rotate(matrix, -roll, glm::dvec3(0.0, 0.0, 1.0)); // po wyłączeniu tego kręci się pojazd, a sceneria nie
	matrix = glm::rotate(matrix, -pitch, glm::dvec3(1.0, 0.0, 0.0));
	matrix = glm::rotate(matrix, -yaw, glm::dvec3(0.0, 1.0, 0.0)); // w zewnętrznym widoku: kierunek patrzenia

	if ((type == TP_FOLLOW) && !DebugCameraFlag) {
		matrix *= glm::lookAt(glm::dvec3{ pos }, glm::dvec3{ lookAt }, glm::dvec3{ vUp });
	} else {
		matrix = glm::translate(matrix, glm::dvec3{ -pos }); // nie zmienia kierunku patrzenia
	}

	return true;
}

void TCamera::raLook()
{ // zmiana kierunku patrzenia - przelicza Yaw
	Math3D::vector3 where = lookAt - pos + Math3D::vector3(0, 3, 0); // trochę w górę od szyn
	if ((where.x != 0.0) || (where.z != 0.0)) {
		yaw = atan2(-where.x, -where.z); // kąt horyzontalny
		rotationOffsets.y = 0.0;
	}
	double l = Math3D::Length3(where);
	if (l > 0.0) {
		pitch = asin(where.y / l); // kąt w pionie
		rotationOffsets.x = 0.0;
	}
};

void TCamera::stop()
{ // wyłącznie bezwładnego ruchu po powrocie do kabiny
	type = TP_FOLLOW;
	velocity = Math3D::vector3(0, 0, 0);
};
