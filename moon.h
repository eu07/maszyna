#pragma once

#include "winheaders.h"

// TODO: sun and moon share code as celestial bodies, we could make a base class out of it

class cMoon {

public:
// types:

// methods:
    void init();
    void update( bool const Includephase = false );
	void render();
	// returns vector pointing at the sun
	glm::vec3 getDirection();
	// returns current elevation above horizon
	float getAngle() const;
	// returns current intensity of the sun
	float getIntensity();
    // returns current phase of the moon
    float getPhase() const { return m_phase; }
    // sets current time, overriding one acquired from the system clock
    void setTime( int const Hour, int const Minute, int const Second );
	// sets current geographic location
	void setLocation( float const Longitude, float const Latitude );
    // sets ambient temperature in degrees C.
    void setTemperature( float const Temperature );
    // sets surface pressure in milibars
    void setPressure( float const Pressure );

// constructors:
	cMoon();

// deconstructor:
	~cMoon();

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
    void phase();
    // helper, normalize values to range 0...1
    float normalize( const float Value ) const;

// members:

	struct celestialbody {	// main planet parameters

		double dayang;		// day angle (daynum*360/year-length) degrees
        double phlong;      // longitude of perihelion
        double mnlong;		// mean longitude, degrees
		double mnanom;		// mean anomaly, degrees
        double tranom;      // true anomaly, degrees
		double eclong;		// ecliptic longitude, degrees.
		double oblecl;		// obliquity of ecliptic.
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
        int    hour{ -1 };  // current time, used for calculation of utime. if set to -1, time for
        int    minute{ -1 };// calculation will be obtained from the local clock
        int    second{ -1 };
        double utime;		// universal (Greenwich) standard time
		double timezone;	// time zone, east (west negative). USA:  Mountain = -7, Central = -6, etc.
		double gmst;		// Greenwich mean sidereal time, hours
		double lmst;		// local mean sidereal time, degrees
		double temp;		// ambient dry-bulb temperature, degrees C, used for refraction correction
		double press;		// surface pressure, millibars, used for refraction correction and ampress
	};

    celestialbody m_body;
    observer m_observer;
    glm::vec3 m_position;
    float m_phase;
};
