#pragma once

#include "windows.h"
#include "opengl/glew.h"
#include "opengl/wglew.h"
#include "dumb3d.h"


//////////////////////////////////////////////////////////////////////////////////////////
// cSun -- class responsible for dynamic calculation of position and intensity of the Sun,
//         given current weather, time and geographic location.

class cSun {

public:
// types:

// methods:
    void init();
    void update();
	void render( Math3D::vector3 const &Origin );
    // returns location of the sun in the 3d scene
    Math3D::vector3 getPosition() { return m_position; }
	// returns vector pointing at the sun
	Math3D::vector3 getDirection();
	// returns current elevation above horizon
	float getAngle();
	// returns current intensity of the sun
	float getIntensity();
	// sets current geographic location
	void setLocation( float const Longitude, float const Latitude );
	// sets ambient temperature in degrees C.
	void setTemperature( float const Temperature );
	// sets surface pressure in milibars
	void setPressure( float const Pressure );

// constructors:
	cSun();

// deconstructor:
	~cSun();

// members:

protected:
// types:

// methods:
	// calculates sun position on the sky given specified time and location
	void move();
	// calculates position adjustment due to refraction
	void refract();
	// calculates light intensity at current moment
	void irradiance();
	// calculates day of year from given date
	int yearday( int Day, int const Month, int const Year );
    // obtains current time for calculations
    void time( SYSTEMTIME *Time );

// members:
	GLUquadricObj *sunsphere;	// temporary handler for sun positioning test

	struct celestialbody {	// main planet parameters

		double dayang;		// day angle (daynum*360/year-length) degrees
		double mnlong;		// mean longitude, degrees
		double mnanom;		// mean anomaly, degrees
		double eclong;		// ecliptic longitude, degrees.
		double ecobli;		// obliquity of ecliptic.
		double declin;		// declination--zenith angle of solar noon at equator, degrees NORTH.
		double rascen;		// right ascension, degrees
		double hrang;		// hour angle--hour of sun from solar noon, degrees WEST
		double zenetr;		// solar zenith angle, no atmospheric correction (= ETR)
		double zenref;		// solar zenith angle, deg. from zenith, refracted
		double coszen;		// cosine of refraction corrected solar zenith angle
		double elevetr;		// solar elevation, no atmospheric correction (= ETR)
		double elevref;		// solar elevation angle, deg. from horizon, refracted.
		double distance;	// distance from earth in AUs
		double erv;			// earth radius vector (multiplied to solar constant)
		double etr;			// extraterrestrial (top-of-atmosphere) W/sq m global horizontal solar irradiance
		double etrn;		// extraterrestrial (top-of-atmosphere) W/sq m direct normal solar irradiance
    };

	struct observer {		// weather, time and position data in observer's location

		double latitude;	// latitude, degrees north (south negative)
		double longitude;	// longitude, degrees east (west negative)
		double utime;		// universal (Greenwich) standard time
		double timezone;	// time zone, east (west negative). USA:  Mountain = -7, Central = -6, etc.
		double gmst;		// Greenwich mean sidereal time, hours
		double lmst;		// local mean sidereal time, degrees
		double temp;		// ambient dry-bulb temperature, degrees C, used for refraction correction
		double press;		// surface pressure, millibars, used for refraction correction and ampress
	};

    celestialbody m_body;
    observer m_observer;
    Math3D::vector3 m_position;
};
