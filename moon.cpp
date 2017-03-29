
#include "stdafx.h"
#include "moon.h"
#include "globals.h"
#include "mtable.h"
#include "usefull.h"
#include "world.h"

//////////////////////////////////////////////////////////////////////////////////////////
// cSun -- class responsible for dynamic calculation of position and intensity of the Sun,

cMoon::cMoon() {

	setLocation( 19.00f, 52.00f );					// default location roughly in centre of Poland
	m_observer.press = 1013.0;						// surface pressure, millibars
	m_observer.temp = 15.0;							// ambient dry-bulb temperature, degrees C

	TIME_ZONE_INFORMATION timezoneinfo;				// TODO: timezone dependant on geographic location
	::GetTimeZoneInformation( &timezoneinfo );
	m_observer.timezone = -timezoneinfo.Bias / 60.0f;
}

cMoon::~cMoon() { gluDeleteQuadric( moonsphere ); }

void
cMoon::init() {

    moonsphere = gluNewQuadric();
    gluQuadricNormals( moonsphere, GLU_SMOOTH );
    // NOTE: we're calculating phase just once, because it's unlikely simulation will last a few days,
    // plus a sudden texture change would be pretty jarring
    phase();
}

void
cMoon::update() {

    move();
    Math3D::vector3 position( 0.0f, 0.0f, -2000.0f );
    position.RotateX( (float)(  m_body.elevref * ( M_PI / 180.0 ) ) );
    position.RotateY( (float)( -m_body.hrang *   ( M_PI / 180.0 ) ) );

    m_position = position;
}

void
cMoon::render() {

	glColor4f( 225.0f/255.0f, 225.0f/255.0f, 255.0f/255.0f, 1.f );
	// debug line to locate the sun easier
	Math3D::vector3 position = m_position;
	glBegin( GL_LINES );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( position.x, 0.0f, position.z );
	glEnd();
	glPushMatrix();
	glTranslatef( position.x, position.y, position.z );
    gluSphere( moonsphere, /* (float)( Global::ScreenHeight / Global::FieldOfView ) * 0.5 * */ ( m_body.distance / 60.2666 ) * 9.037461, 12, 12 );
	glPopMatrix();
}

Math3D::vector3
cMoon::getDirection() {

	Math3D::vector3 position( 0.f, 0.f, -1.f );
	position.RotateX( (float)(  m_body.elevref * (M_PI/180.0)) );
	position.RotateY( (float)( -m_body.hrang *   (M_PI/180.0)) );
	position.Normalize();
	return position;
}

float
cMoon::getAngle() const {
    
    return (float)m_body.elevref;
}
	
float cMoon::getIntensity() {

	irradiance();
    // NOTE: we don't have irradiance model for the moon so we cheat here
    // calculating intensity of the sun instead, and returning 15% of the value,
    // which roughly matches how much sunlight is reflected by the moon
    // We alter the intensity further based on current phase of the moon
    auto const phasefactor = 1.0f - std::abs( m_phase - 29.53f * 0.5f ) / ( 29.53 * 0.5f );
	return (float)( m_body.etr/ 1399.0 ) * phasefactor * 0.15f; // arbitrary scaling factor taken from etrn value
}

void cMoon::setLocation( float const Longitude, float const Latitude ) {

	// convert fraction from geographical base of 6o minutes
	m_observer.longitude = (int)Longitude + (Longitude - (int)(Longitude)) * 100.0 / 60.0;
	m_observer.latitude = (int)Latitude + (Latitude - (int)(Latitude)) * 100.0 / 60.0 ;
}

// sets current time, overriding one acquired from the system clock
void cMoon::setTime( int const Hour, int const Minute, int const Second ) {

    m_observer.hour = clamp( Hour, -1, 23 );
    m_observer.minute = clamp( Minute, -1, 59 );
    m_observer.second = clamp( Second, -1, 59 );
}

void cMoon::setTemperature( float const Temperature ) {
    
    m_observer.temp = Temperature;
}

void cMoon::setPressure( float const Pressure ) {
    
    m_observer.press = Pressure;
}

void cMoon::move() {

	static double radtodeg = 57.295779513; // converts from radians to degrees
	static double degtorad = 0.0174532925; // converts from degrees to radians

    SYSTEMTIME localtime = Simulation::Time.data(); // time for the calculation

    if( m_observer.hour >= 0 ) { localtime.wHour = m_observer.hour; }
    if( m_observer.minute >= 0 ) { localtime.wMinute = m_observer.minute; }
    if( m_observer.second >= 0 ) { localtime.wSecond = m_observer.second; }

    double ut = localtime.wHour
        + localtime.wMinute / 60.0 // too low resolution, noticeable skips
        + localtime.wSecond / 3600.0; // good enough in normal circumstances
    /*
    + localtime.wMilliseconds / 3600000.0; // for really smooth movement
    */
    double daynumber = 367 * localtime.wYear
        - 7 * ( localtime.wYear + ( localtime.wMonth + 9 ) / 12 ) / 4
        + 275 * localtime.wMonth / 9
        + localtime.wDay
        - 730530
        + ( ut / 24.0 );

    // Universal Coordinated (Greenwich standard) time
    m_observer.utime = ut * 3600.0;
    m_observer.utime = m_observer.utime / 3600.0 - m_observer.timezone;

    // obliquity of the ecliptic
    m_body.oblecl = clamp_circular( 23.4393 - 3.563e-7 * daynumber );
    // moon parameters
    double longascnode = clamp_circular( 125.1228 - 0.0529538083  * daynumber ); // N, degrees
    double const inclination = 5.1454; // i, degrees
    double const mndistance = 60.2666; // a, in earth radii
    // argument of perigee
    double const perigeearg = clamp_circular( 318.0634 + 0.1643573223  * daynumber ); // w, degrees
    // mean anomaly
    m_body.mnanom = clamp_circular( 115.3654 + 13.0649929509 * daynumber ); // M, degrees
    // eccentricity
    double const e = 0.054900;
    // eccentric anomaly
    double E0 = m_body.mnanom + radtodeg * e * std::sin( degtorad * m_body.mnanom ) * ( 1.0 + e * std::cos( degtorad * m_body.mnanom ) );
    double E1 = E0 - ( E0 - radtodeg * e * std::sin( degtorad * E0 ) - m_body.mnanom ) / ( 1.0 - e * std::cos( degtorad * E0 ) );
    while( std::abs( E0 - E1 ) > 0.005 ) { // arbitrary precision tolerance threshold
        E0 = E1;
        E1 = E0 - ( E0 - radtodeg * e * std::sin( degtorad * E0 ) - m_body.mnanom ) / ( 1.0 - e * std::cos( degtorad * E0 ) );
    }
    double const E = E1;
    // lunar orbit plane rectangular coordinates
    double const xv = mndistance * ( std::cos( degtorad * E ) - e );
    double const yv = mndistance * std::sin( degtorad * E ) * std::sqrt( 1.0 - e*e );
    // distance
	m_body.distance = std::sqrt( xv*xv + yv*yv ); // r
    // true anomaly
    m_body.tranom = clamp_circular( radtodeg * std::atan2( yv, xv ) ); // v
    // ecliptic rectangular coordinates
    double const vpluswinrad = degtorad * ( m_body.tranom + perigeearg );
    double const xeclip = m_body.distance * ( std::cos( degtorad * longascnode ) * std::cos( vpluswinrad ) - std::sin( degtorad * longascnode ) * std::sin( vpluswinrad ) * std::cos( degtorad * inclination ) );
    double const yeclip = m_body.distance * ( std::sin( degtorad * longascnode ) * std::cos( vpluswinrad ) + std::cos( degtorad * longascnode ) * std::sin( vpluswinrad ) * std::cos( degtorad * inclination ) );
    double const zeclip = m_body.distance * std::sin( vpluswinrad ) * std::sin( degtorad * inclination );
    // ecliptic coordinates
    double ecliplat = radtodeg * std::atan2( zeclip, std::sqrt( xeclip*xeclip + yeclip*yeclip ) );
    m_body.eclong = clamp_circular( radtodeg * std::atan2( yeclip, xeclip ) );
    // distance
    m_body.distance = std::sqrt( xeclip*xeclip + yeclip*yeclip + zeclip*zeclip );
    // perturbations
    // NOTE: perturbation calculation can be safely disabled if we don't mind error of 1-2 degrees
    // Sun's  mean anomaly:          Ms     (already computed)
    double const sunmnanom = clamp_circular( 356.0470 + 0.9856002585 * daynumber ); // M
    // Sun's  mean longitude:        Ls     (already computed)
    double const sunphlong = clamp_circular( 282.9404 + 4.70935e-5 * daynumber );
    double const sunmnlong = clamp_circular( sunphlong + sunmnanom ); // L = w + M
    // Moon's mean anomaly:          Mm     (already computed)
    // Moon's mean longitude:        Lm  =  N + w + M (for the Moon)
    m_body.mnlong = clamp_circular( longascnode + perigeearg + m_body.mnanom );
    // Moon's mean elongation:       D   =  Lm - Ls
    double const mnelong = clamp_circular( m_body.mnlong - sunmnlong );
    // Moon's argument of latitude:  F   =  Lm - N
    double const arglat = clamp_circular( m_body.mnlong - longascnode );
    // longitude perturbations
    double const pertevection = -1.274 * std::sin( degtorad * ( m_body.mnanom - 2.0 * mnelong ) ); // Evection
    double const pertvariation = +0.658 * std::sin( degtorad * ( 2.0 * mnelong ) ); // Variation
    double const pertyearlyeqt = -0.186 * std::sin( degtorad * sunmnanom ); // Yearly equation
    // latitude perturbations
    double const pertlat = -0.173 * std::sin( degtorad * ( arglat - 2.0 * mnelong ) );

    m_body.eclong += pertevection + pertvariation + pertyearlyeqt;
    ecliplat += pertlat;
    // declination
	m_body.declin = radtodeg * std::asin( std::sin (m_body.oblecl * degtorad) * std::sin(m_body.eclong * degtorad) );

    // right ascension
	double top = std::cos( degtorad * m_body.oblecl ) * std::sin( degtorad * m_body.eclong );
	double bottom = std::cos( degtorad * m_body.eclong );
	m_body.rascen = clamp_circular( radtodeg * std::atan2( top, bottom ) );

	// Greenwich mean sidereal time
	m_observer.gmst = 6.697375 + 0.0657098242 * daynumber + m_observer.utime;

	m_observer.gmst -= 24.0 * (int)( m_observer.gmst / 24.0 );
	if( m_observer.gmst < 0.0 ) m_observer.gmst += 24.0;

	// local mean sidereal time
	m_observer.lmst = m_observer.gmst * 15.0 + m_observer.longitude;

	m_observer.lmst -= 360.0 * (int)( m_observer.lmst / 360.0 );
	if( m_observer.lmst < 0.0 ) m_observer.lmst += 360.0;

	// hour angle
	m_body.hrang = m_observer.lmst - m_body.rascen;

		 if( m_body.hrang < -180.0 )	m_body.hrang += 360.0;	// (force it between -180 and 180 degrees)
    else if( m_body.hrang >  180.0 )	m_body.hrang -= 360.0;

	double cz;	// cosine of the solar zenith angle

	double tdatcd = std::cos( degtorad * m_body.declin );
	double tdatch = std::cos( degtorad * m_body.hrang );
	double tdatcl = std::cos( degtorad * m_observer.latitude );
	double tdatsd = std::sin( degtorad * m_body.declin );
	double tdatsl = std::sin( degtorad * m_observer.latitude );

	cz = tdatsd * tdatsl + tdatcd * tdatcl * tdatch;

    // (watch out for the roundoff errors)
	if( fabs (cz) > 1.0 ) {	cz >= 0.0 ? cz = 1.0 : cz = -1.0; }

	m_body.zenetr = std::acos( cz ) * radtodeg;
	m_body.elevetr = 90.0 - m_body.zenetr;
	refract();
}

void cMoon::refract() {

	static double degtorad = 0.0174532925;					// converts from degrees to radians

	double prestemp;	// temporary pressure/temperature correction
	double refcor;		// temporary refraction correction
	double tanelev;		// tangent of the solar elevation angle

	// if the sun is near zenith, the algorithm bombs; refraction near 0.
	if( m_body.elevetr > 85.0 )
		refcor = 0.0;
	else {

		tanelev = tan( degtorad * m_body.elevetr );
		if( m_body.elevetr >= 5.0 )
			refcor	= 58.1 / tanelev
					- 0.07 / pow( tanelev, 3 )
					+ 0.000086 / pow( tanelev, 5 );
		else if( m_body.elevetr >= -0.575 )
			refcor  = 1735.0
					+ m_body.elevetr * ( -518.2 + m_body.elevetr *
					  ( 103.4 + m_body.elevetr * ( -12.79 + m_body.elevetr * 0.711 ) ) );
		else
			refcor  = -20.774 / tanelev;

		prestemp = ( m_observer.press * 283.0 ) / ( 1013.0 * ( 273.0 + m_observer.temp ) );
		refcor *= prestemp / 3600.0;
	}

	// refracted solar elevation angle
	m_body.elevref = m_body.elevetr + refcor;

	// refracted solar zenith angle
	m_body.zenref  = 90.0 - m_body.elevref;
}

void cMoon::irradiance() {

	static double radtodeg = 57.295779513;					// converts from radians to degrees
	static double degtorad = 0.0174532925;					// converts from degrees to radians

	m_body.dayang = ( Simulation::Time.year_day() - 1 ) * 360.0 / 365.0;
	double sd = sin( degtorad * m_body.dayang );				// sine of the day angle
	double cd = cos( degtorad * m_body.dayang );				// cosine of the day angle or delination
	m_body.erv = 1.000110 + 0.034221*cd + 0.001280*sd;
	double d2 = 2.0 * m_body.dayang;
	double c2 = cos( degtorad * d2 );
	double s2 = sin( degtorad * d2 );
	m_body.erv += 0.000719*c2 + 0.000077*s2;

	double solcon = 1367.0;									// Solar constant, 1367 W/sq m

	m_body.coszen  = cos( degtorad * m_body.zenref );
	if( m_body.coszen > 0.0 ) {
		m_body.etrn = solcon * m_body.erv;
		m_body.etr  = m_body.etrn * m_body.coszen;
	}
	else {
		m_body.etrn = 0.0;
		m_body.etr  = 0.0;
	}
}

void
cMoon::phase() {

    // calculate moon's age in days from new moon
    float ip = normalize( ( Simulation::Time.julian_day() - 2451550.1f ) / 29.530588853f );
    m_phase = ip * 29.53f;
}

// normalize values to range 0...1
float
cMoon::normalize( const float Value ) const {

    float value = Value - floor( Value );
    if( value < 0.f ) { ++value; }

    return value;
}
