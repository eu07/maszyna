
#include "stdafx.h"
#include "sun.h"
#include "globals.h"
#include "mtable.h"

//////////////////////////////////////////////////////////////////////////////////////////
// cSun -- class responsible for dynamic calculation of position and intensity of the Sun,

cSun::cSun() {

	setLocation( 19.00f, 52.00f );					// default location roughly in centre of Poland
	m_observer.press = 1013.0;						// surface pressure, millibars
	m_observer.temp = 15.0;							// ambient dry-bulb temperature, degrees C

	TIME_ZONE_INFORMATION timezoneinfo;				// TODO: timezone dependant on geographic location
	::GetTimeZoneInformation( &timezoneinfo );
	m_observer.timezone = -timezoneinfo.Bias / 60.0f;
}

cSun::~cSun() { gluDeleteQuadric( sunsphere ); }

void
cSun::init() {

    sunsphere = gluNewQuadric();
    gluQuadricNormals( sunsphere, GLU_SMOOTH );
}

void
cSun::update() {

    move();
    Math3D::vector3 position( 0.0f, 0.0f, -2000.0f );
    position.RotateX( (float)(  m_body.elevref * ( M_PI / 180.0 ) ) );
    position.RotateY( (float)( -m_body.hrang *   ( M_PI / 180.0 ) ) );

    m_position = position;
}

void
cSun::render() {

/*
	glLightfv(GL_LIGHT0, GL_POSITION, position.getVector() );	// sun

	GLfloat LightPosition[]= { 10.0f, 50.0f, -5.0f, 1.0f };		// ambient
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition );
*/
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glColor4f( 255.0f/255.0f, 242.0f/255.0f, 231.0f/255.0f, 1.f );
	// debug line to locate the sun easier
	Math3D::vector3 position = m_position;
	glBegin( GL_LINES );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( position.x, 0.0f, position.z );
	glEnd();
	glPushMatrix();
	glTranslatef( position.x, position.y, position.z );
	// radius is a result of scaling true distance down to 2km -- it's scaled by equal ratio
	gluSphere( sunsphere, (float)(m_body.distance * 9.359157), 12, 12 );
	glPopMatrix();
	glEnable(GL_FOG);
	glEnable(GL_LIGHTING);
}

Math3D::vector3
cSun::getDirection() {

	Math3D::vector3 position( 0.f, 0.f, -1.f );
	position.RotateX( (float)(  m_body.elevref * (M_PI/180.0)) );
	position.RotateY( (float)( -m_body.hrang *   (M_PI/180.0)) );
	position.Normalize();
	return position;
}

float
cSun::getAngle() {
    
    return (float)m_body.elevref;
}
	
float cSun::getIntensity() {

	irradiance();
	return (float)( m_body.etr/ 1399.0 );	// arbitrary scaling factor taken from etrn value
}

void cSun::setLocation( float const Longitude, float const Latitude ) {

	// convert fraction from geographical base of 6o minutes
	m_observer.longitude = (int)Longitude + (Longitude - (int)(Longitude)) * 100.0 / 60.0;
	m_observer.latitude = (int)Latitude + (Latitude - (int)(Latitude)) * 100.0 / 60.0 ;
}

void cSun::setTemperature( float const Temperature ) {
    
    m_observer.temp = Temperature;
}

void cSun::setPressure( float const Pressure ) {
    
    m_observer.press = Pressure;
}

void cSun::move() {

	static double degrad = 57.295779513;					// converts from radians to degrees
	static double raddeg = 0.0174532925;					// converts from degrees to radians

    SYSTEMTIME localtime; // time for the calculation
    time( &localtime );

    double ut = localtime.wHour
              + localtime.wMinute / 60.0 // too low resolution, noticeable skips
              + localtime.wSecond / 3600.0; // good enough in normal circumstances
/*
			  + localtime.wMilliseconds / 3600000.0; // for really smooth movement
*/
	double daynumber = 367 * localtime.wYear
					 - 7 * ( localtime.wYear + ( localtime.wMonth + 9 ) /12 ) / 4
					 + 275 * localtime.wMonth / 9
					 + localtime.wDay
					 - 730530
					 + (ut / 24.0);

	// Universal Coordinated (Greenwich standard) time
    m_observer.utime = ut * 3600.0;
    m_observer.utime = m_observer.utime / 3600.0 - m_observer.timezone;

	// mean longitude
    m_body.mnlong  = 280.460 + 0.9856474 * daynumber;
	m_body.mnlong -= 360.0 * (int) ( m_body.mnlong / 360.0 );	// clamp the range to 0-360
	if( m_body.mnlong < 0.0 ) m_body.mnlong += 360.0;

	// mean anomaly
    m_body.mnanom  = 357.528 + 0.9856003 * daynumber;
	m_body.mnanom -= 360.0 * (int) ( m_body.mnanom / 360.0 );	// clamp the range to 0-360
	if( m_body.mnanom < 0.0 ) m_body.mnanom += 360.0;

	// ecliptic longitude
    m_body.eclong =	m_body.mnlong
					+ 1.915 * sin( m_body.mnanom * raddeg )
					+ 0.020 * sin ( 2.0 * m_body.mnanom * raddeg );
	m_body.eclong -= 360.0 * (int)( m_body.eclong / 360.0 );
    if( m_body.eclong < 0.0 ) m_body.eclong += 360.0;			// clamp the range to 0-360

    // obliquity of the ecliptic
    m_body.ecobli = 23.439 - 4.0e-07 * daynumber;

	// declination
	m_body.declin = degrad * asin( sin (m_body.ecobli * raddeg) * sin (m_body.eclong * raddeg) );

	// right ascension
	double top = cos ( raddeg * m_body.ecobli ) * sin ( raddeg * m_body.eclong );
	double bottom = cos ( raddeg * m_body.eclong );

	m_body.rascen = degrad * atan2( top, bottom );
    if( m_body.rascen < 0.0 ) m_body.rascen += 360.0;			// (make it a positive angle)

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

	double tdatcd = cos( raddeg * m_body.declin );
	double tdatch = cos( raddeg * m_body.hrang );
	double tdatcl = cos( raddeg * m_observer.latitude );
	double tdatsd = sin( raddeg * m_body.declin );
	double tdatsl = sin( raddeg * m_observer.latitude );

	cz = tdatsd * tdatsl + tdatcd * tdatcl * tdatch;

    // (watch out for the roundoff errors)
	if( fabs (cz) > 1.0 ) {	cz >= 0.0 ? cz = 1.0 : cz = -1.0; }

	m_body.zenetr = acos( cz ) * degrad;
	m_body.elevetr = 90.0 - m_body.zenetr;
	refract();

	// additional calculations for proper object sizing.
	// orbit eccentricity
	double e = 0.016709 - 1.151e-9 * daynumber;
	// eccentric anomaly
	double E = m_body.mnanom + e * degrad * sin(m_body.mnanom) * ( 1.0 + e * cos(m_body.mnanom) );
	double xv = cos(E) - e;
    double yv = sqrt(1.0 - e*e) * sin(E);
	m_body.distance = sqrt( xv*xv + yv*yv );
}

void cSun::refract() {

	static double raddeg = 0.0174532925;					// converts from degrees to radians

	double prestemp;	// temporary pressure/temperature correction
	double refcor;		// temporary refraction correction
	double tanelev;		// tangent of the solar elevation angle

	// if the sun is near zenith, the algorithm bombs; refraction near 0.
	if( m_body.elevetr > 85.0 )
		refcor = 0.0;
	else {

		tanelev = tan( raddeg * m_body.elevetr );
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

void cSun::irradiance() {

	static double degrad = 57.295779513;					// converts from radians to degrees
	static double raddeg = 0.0174532925;					// converts from degrees to radians

    SYSTEMTIME localtime; // time for the calculation
    time( &localtime );

	m_body.dayang = ( yearday( localtime.wDay, localtime.wMonth, localtime.wYear ) - 1 ) * 360.0 / 365.0;
	double sd = sin( raddeg * m_body.dayang );				// sine of the day angle
	double cd = cos( raddeg * m_body.dayang );				// cosine of the day angle or delination
	m_body.erv = 1.000110 + 0.034221*cd + 0.001280*sd;
	double d2 = 2.0 * m_body.dayang;
	double c2 = cos( raddeg * d2 );
	double s2 = sin( raddeg * d2 );
	m_body.erv += 0.000719*c2 + 0.000077*s2;

	double solcon = 1367.0;									// Solar constant, 1367 W/sq m

	m_body.coszen  = cos( raddeg * m_body.zenref );
	if( m_body.coszen > 0.0 ) {
		m_body.etrn = solcon * m_body.erv;
		m_body.etr  = m_body.etrn * m_body.coszen;
	}
	else {
		m_body.etrn = 0.0;
		m_body.etr  = 0.0;
	}
}

int cSun::yearday( int Day, const int Month, const int Year ) {

	char daytab[ 2 ][ 13 ] = {
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};
	int i, leap;

	leap = ( Year%4 == 0 ) && ( Year%100 != 0 ) || ( Year%400 == 0 );
	for( i = 1; i < Month; ++i )
		Day += daytab[ leap ][ i ];

	return Day;
}

void cSun::daymonth( WORD &Day, WORD &Month, WORD const Year, WORD const Yearday ) {

    WORD daytab[ 2 ][ 13 ] = {
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    int leap = ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 );
    WORD idx = 1;
    while( (idx < 13) && ( Yearday <= daytab[ leap ][ idx ] )) {

        ++idx;
    }
    Month = idx + 1;
    Day = Yearday - daytab[ leap ][ idx ];
}

// obtains current time for calculations
void
cSun::time( SYSTEMTIME *Time ) {

    ::GetLocalTime( Time );
    // NOTE: we're currently using local time to determine day/month/year
    if( Global::fMoveLight > 0.0 ) {
        // TODO: enter scenario-defined day/month/year instead.
        daymonth( Time->wDay, Time->wMonth, Time->wYear, static_cast<WORD>(Global::fMoveLight) );
    }
    Time->wHour = GlobalTime->hh;
    Time->wMinute = GlobalTime->mm;
    Time->wSecond = std::floor( GlobalTime->mr );
}
