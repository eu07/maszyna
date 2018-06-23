/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "dumb3d.h"
#include "command.h"

enum TCameraType { // tryby pracy kamery
	TP_FOLLOW, // jazda z pojazdem
	TP_FREE, // stoi na scenerii
	TP_SATELITE // widok z góry (nie używany)
};

class TCamera
{
public: // McZapkie: potrzebuje do kiwania na boki
	void init(Math3D::vector3 nPos, Math3D::vector3 nAngle);
	void reset();
	void onCursorMove(const double x, const double y);
	bool onCommand(const command_data& command);
	void update();
	Math3D::vector3 getDirection();
	bool setMatrix(glm::dmat4& matrix);
	void raLook();
	void stop();

	TCameraType type;
	double pitch;
	double yaw; // w środku: 0=do przodu; na zewnątrz: 0=na południe
	double roll;
	Math3D::vector3 pos; // współrzędne obserwatora
	Math3D::vector3 lookAt; // współrzędne punktu, na który ma patrzeć
	Math3D::vector3 vUp;
	Math3D::vector3 velocity;

private:
	glm::dvec3 moveRate;
	glm::dvec3 rotationOffsets; // requested changes to pitch, yaw and roll
};
