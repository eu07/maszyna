#include "stdafx.h"
#include "skydome.h"
#include "color.h"
#include "usefull.h"

// sky gradient based on "A practical analytic model for daylight" 
// by A. J. Preetham Peter Shirley Brian Smits (University of Utah)

float CSkyDome::m_distributionluminance[ 5 ][ 2 ] = {	// Perez distributions
		{  0.17872f , -1.46303f },		// a = darkening or brightening of the horizon
		{ -0.35540f ,  0.42749f },		// b = luminance gradient near the horizon,
		{ -0.02266f ,  5.32505f },		// c = relative intensity of the circumsolar region
		{  0.12064f , -2.57705f },		// d = width of the circumsolar region
		{ -0.06696f ,  0.37027f }		// e = relative backscattered light
	};
float CSkyDome::m_distributionxcomp[ 5 ][ 2 ] = {
		{ -0.01925f , -0.25922f },
		{ -0.06651f ,  0.00081f },
		{ -0.00041f ,  0.21247f },
		{ -0.06409f , -0.89887f },
		{ -0.00325f ,  0.04517f }
	};
float CSkyDome::m_distributionycomp[ 5 ][ 2 ] = {
		{ -0.01669f , -0.26078f },
		{ -0.09495f ,  0.00921f },
		{ -0.00792f ,  0.21023f },
		{ -0.04405f , -1.65369f },
		{ -0.01092f ,  0.05291f }	
	};

float CSkyDome::m_zenithxmatrix[ 3 ][ 4 ] = {
		{  0.00165f, -0.00375f,  0.00209f,  0.00000f },
		{ -0.02903f,  0.06377f, -0.03202f,  0.00394f },
		{  0.11693f, -0.21196f,  0.06052f,  0.25886f }
	};
float CSkyDome::m_zenithymatrix[ 3 ][ 4 ] = {
		{  0.00275f, -0.00610f,  0.00317f,  0.00000f },
		{ -0.04214f,  0.08970f, -0.04153f,  0.00516f },
		{  0.15346f, -0.26756f,  0.06670f,  0.26688f }
	};

//******************************************************************************//

CSkyDome::CSkyDome (int const Tesselation) :
               m_tesselation( Tesselation ) {

//	SetSunPosition( Math3D::vector3(75.0f, 0.0f, 0.0f) );
	SetTurbidity( 3.0f );
	SetExposure( true, 20.0f );
	SetOvercastFactor( 0.05f );
	SetGammaCorrection( 2.2f );
    Generate();
}

CSkyDome::~CSkyDome() {
}

//******************************************************************************//

void CSkyDome::Generate() {
	// radius of dome
    float const radius = 1.0f;
    float const offset = 0.1f * radius; // horizontal offset, a cheap way to prevent a gap between ground and horizon

	// create geometry chunk
    int const latitudes = m_tesselation / 2 / 2; // half-sphere only
    int const longitudes = m_tesselation;

    std::uint16_t index = 0;

    for( int i = 0; i <= latitudes; ++i ) {

        float const latitude = M_PI * ( -0.5f + (float)( i ) / latitudes / 2 );  // half-sphere only
        float const z = std::sin( latitude );
        float const zr = std::cos( latitude );

        for( int j = 0; j <= longitudes; ++j ) {

            float const longitude = 2.0 * M_PI * (float)( j ) / longitudes;
            float const x = std::cos( longitude );
            float const y = std::sin( longitude );
/*
            m_vertices.emplace_back( float3( x * zr, y * zr - offset, z ) * radius );
            // we aren't using normals, but the code is left here in case it's ever needed
//            m_normals.emplace_back( float3( x * zr, -y * zr, -z ) );
*/
            // cartesian to opengl swap: -x, -z, -y
            m_vertices.emplace_back( glm::vec3( -x * zr, -z - offset, -y * zr ) * radius );
            m_colours.emplace_back( glm::vec3( 0.75f, 0.75f, 0.75f ) ); // placeholder

            if( (i == 0) || (j == 0) ) {
                // initial edge of the dome, don't start indices yet
                ++index;
            }
            else {
                // indices for two triangles, formed between current and previous latitude
                m_indices.emplace_back( index - 1 - (longitudes + 1) );
                m_indices.emplace_back( index - 1 );
                m_indices.emplace_back( index );
                m_indices.emplace_back( index );
                m_indices.emplace_back( index - ( longitudes + 1 ) );
                m_indices.emplace_back( index - 1 - ( longitudes + 1 ) );
                ++index;
            }
        }
    }
}

void CSkyDome::Update( glm::vec3 const &Sun ) {

    if( true == SetSunPosition( Sun ) ) {
        // build colors if there's a change in sun position
        RebuildColors();
    }
}

// render skydome to screen
void CSkyDome::Render() {

    if( m_vertexbuffer == -1 ) {
        // build the buffers
        ::glGenBuffers( 1, &m_vertexbuffer );
        ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
        ::glBufferData( GL_ARRAY_BUFFER, m_vertices.size() * sizeof( glm::vec3 ), m_vertices.data(), GL_STATIC_DRAW );

        ::glGenBuffers( 1, &m_coloursbuffer );
        ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
        ::glBufferData( GL_ARRAY_BUFFER, m_colours.size() * sizeof( glm::vec3 ), m_colours.data(), GL_DYNAMIC_DRAW );

        ::glGenBuffers( 1, &m_indexbuffer );
        ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
        ::glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof( unsigned short ), m_indices.data(), GL_STATIC_DRAW );
        // NOTE: vertex and index source data is superfluous past this point, but, eh
    }
    // begin
    ::glEnableClientState( GL_VERTEX_ARRAY );
    ::glEnableClientState( GL_COLOR_ARRAY );
    // positions
    ::glBindBuffer( GL_ARRAY_BUFFER, m_vertexbuffer );
    ::glVertexPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    // colours
    ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
    ::glColorPointer( 3, GL_FLOAT, sizeof( glm::vec3 ), reinterpret_cast<void const*>( 0 ) );
    // indices
    ::glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexbuffer );
    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( m_indices.size() ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );
    // cleanup
    ::glDisableClientState( GL_COLOR_ARRAY );
    ::glDisableClientState( GL_VERTEX_ARRAY );
}

bool CSkyDome::SetSunPosition( glm::vec3 const &Direction ) {

    if( Direction == m_sundirection ) {

        return false;
    }

    m_sundirection = Direction;
	m_thetasun = std::acos( m_sundirection.y );
	m_phisun = std::atan2( m_sundirection.z, m_sundirection.x );

    return true;
}


void CSkyDome::SetTurbidity( float const Turbidity ) {

	m_turbidity = clamp( Turbidity, 1.0f, 512.0f );
}

void CSkyDome::SetExposure( bool const Linearexposure, float const Expfactor ) {

	m_linearexpcontrol = Linearexposure;
	m_expfactor = 1.0f / clamp( Expfactor, 1.0f, std::numeric_limits<float>::infinity() );
}

void CSkyDome::SetGammaCorrection( float const Gamma ) {

	m_gammacorrection = 1.0f / clamp( Gamma, std::numeric_limits<float>::epsilon(), std::numeric_limits<float>::infinity() );		
}

void CSkyDome::SetOvercastFactor( float const Overcast ) {

	m_overcast = clamp( Overcast, 0.0f, 1.0f ) * 0.75f; // going above 0.65 makes the model go pretty bad, appearance-wise
}

void CSkyDome::GetPerez( float *Perez, float Distribution[ 5 ][ 2 ], const float Turbidity ) {

	Perez[ 0 ] = Distribution[ 0 ][ 0 ] * Turbidity + Distribution[ 0 ][ 1 ];
	Perez[ 1 ] = Distribution[ 1 ][ 0 ] * Turbidity + Distribution[ 1 ][ 1 ];
	Perez[ 2 ] = Distribution[ 2 ][ 0 ] * Turbidity + Distribution[ 2 ][ 1 ];
	Perez[ 3 ] = Distribution[ 3 ][ 0 ] * Turbidity + Distribution[ 3 ][ 1 ];
	Perez[ 4 ] = Distribution[ 4 ][ 0 ] * Turbidity + Distribution[ 4 ][ 1 ];
}

float CSkyDome::GetZenith( float Zenithmatrix[ 3 ][ 4 ], const float Theta, const float Turbidity ) {

	const float theta2 = Theta*Theta;
	const float theta3 = Theta*theta2;
	
	return	( Zenithmatrix[0][0] * theta3 + Zenithmatrix[0][1] * theta2 + Zenithmatrix[0][2] * Theta + Zenithmatrix[0][3]) * Turbidity * Turbidity +
			( Zenithmatrix[1][0] * theta3 + Zenithmatrix[1][1] * theta2 + Zenithmatrix[1][2] * Theta + Zenithmatrix[1][3]) * Turbidity +
			( Zenithmatrix[2][0] * theta3 + Zenithmatrix[2][1] * theta2 + Zenithmatrix[2][2] * Theta + Zenithmatrix[2][3]);

}

float CSkyDome::PerezFunctionO1( float Perezcoeffs[ 5 ], const float Thetasun, const float Zenithval ) {

	const float val = ( 1.0f + Perezcoeffs[ 0 ] * std::exp( Perezcoeffs[ 1 ] ) ) *
						( 1.0f + Perezcoeffs[ 2 ] * std::exp( Perezcoeffs[ 3 ] * Thetasun ) + Perezcoeffs[ 4 ] * std::pow( std::cos( Thetasun ), 2 ) );

	return Zenithval / val;
}

float CSkyDome::PerezFunctionO2( float Perezcoeffs[ 5 ], const float Icostheta, const float Gamma, const float Cosgamma2, const float Zenithval ) {
	// iCosTheta = 1.0f / cosf(theta)
	// cosGamma2 = SQR( cosf( gamma ) )
	return Zenithval * ( 1.0f + Perezcoeffs[ 0 ] * std::exp( Perezcoeffs[ 1 ] * Icostheta ) ) * 
						( 1.0f + Perezcoeffs[ 2 ] * std::exp( Perezcoeffs[ 3 ] * Gamma ) + Perezcoeffs[ 4 ] * Cosgamma2 );
}

void CSkyDome::RebuildColors() {

	// get zenith luminance
	float const chi = ( (4.0f / 9.0f) - (m_turbidity / 120.0f) ) * ( M_PI - (2.0f * m_thetasun) );
	float zenithluminance = ( (4.0453f * m_turbidity) - 4.9710f ) * std::tan( chi ) - (0.2155f * m_turbidity) + 2.4192f; 

	// get x / y zenith
	float zenithx = GetZenith( m_zenithxmatrix, m_thetasun, m_turbidity );
	float zenithy = GetZenith( m_zenithymatrix, m_thetasun, m_turbidity );

	// get perez function parametrs
	float perezluminance[5], perezx[5], perezy[5];  
	GetPerez( perezluminance, m_distributionluminance, m_turbidity );
	GetPerez( perezx, m_distributionxcomp, m_turbidity );
	GetPerez( perezy, m_distributionycomp, m_turbidity );

	// make some precalculation
	zenithx = PerezFunctionO1( perezx, m_thetasun, zenithx );
	zenithy = PerezFunctionO1( perezy, m_thetasun, zenithy );
	zenithluminance = PerezFunctionO1( perezluminance, m_thetasun, zenithluminance );

    // start with fresh average for the new pass
    glm::vec3 averagecolor { 0.0f, 0.0f, 0.0f };

	// trough all vertices
	glm::vec3 vertex;
	glm::vec3 color, colorconverter, shiftedcolor;

	for ( unsigned int i = 0; i < m_vertices.size(); ++i ) {
		// grab it
		vertex = glm::normalize( m_vertices[ i ] );

		// angle between sun and vertex
		const float gamma = std::acos( glm::dot( vertex, m_sundirection ) );

		// warning : major hack!!! .. i had to do something with values under horizon
		//vertex.y = Clamp<float>( vertex.y, 0.05f, 1.0f );
		if ( vertex.y < 0.05f ) vertex.y = 0.05f;

//		from paper:
//			const float theta = arccos( vertex.y );
//			const float iCosTheta = 1.0f / cosf( theta );
//		optimized:
//			iCosTheta = 
//				= 1.0f / cosf( arccos( vertex.y ) );
//				= 1.0f / vertex.y;
		float const icostheta = 1.0f / vertex.y;
		float const cosgamma2 = std::pow( std::cos( gamma ), 2 );

		// Compute x,y values  
		float const x = PerezFunctionO2( perezx, icostheta, gamma, cosgamma2, zenithx );
		float const y = PerezFunctionO2( perezy, icostheta, gamma, cosgamma2, zenithy );

		// luminance(Y) for clear & overcast sky
		float const yclear = std::max( 0.01f, PerezFunctionO2( perezluminance, icostheta, gamma, cosgamma2, zenithluminance ) );
		float const yover = std::max( 0.01f, zenithluminance * ( 1.0f + 2.0f * vertex.y ) / 3.0f );
		
		float const Y = interpolate( yclear, yover, m_overcast );
		float const X = (x / y) * Y;  
		float const Z = ((1.0f - x - y) / y) * Y;
		
		colorconverter = glm::vec3( X, Y, Z );
		color = XYZtoRGB( colorconverter );

		colorconverter = RGBtoHSV(color);
		if ( m_linearexpcontrol ) {
            // linear scale
			colorconverter.z *= m_expfactor;
		} else {
            // exp scale
			colorconverter.z = 1.0f - std::exp( -m_expfactor * colorconverter.z );  
		}

        // desaturate sky colour, based on overcast level
        if( colorconverter.y > 0.0f ) {
            colorconverter.y *= ( 1.0f - m_overcast );
        }

        // override the hue, based on sun height above the horizon. crude way to deal with model shortcomings
        // correction begins when the sun is higher than 10 degrees above the horizon, and fully in effect at 10+15 degrees
        float const degreesabovehorizon = 90.0f - m_thetasun * ( 180.0f / M_PI );
        auto const sunbasedphase = clamp( (1.0f / 15.0f) * ( degreesabovehorizon - 10.0f ), 0.0f, 1.0f );
        // correction is applied in linear manner from the bottom, becomes fully in effect for vertices with y = 0.50
        auto const heightbasedphase = clamp( vertex.y * 2.0f, 0.0f, 1.0f );
        // this height-based factor is reduced the farther the sky is up in the sky
        float const shiftfactor = clamp( interpolate(heightbasedphase, sunbasedphase, sunbasedphase), 0.0f, 1.0f );
        // h = 210 makes for 'typical' sky tone
        shiftedcolor = glm::vec3( 210.0f, colorconverter.y, colorconverter.z );
        shiftedcolor = HSVtoRGB( shiftedcolor );

		color = HSVtoRGB(colorconverter);

        color = interpolate( color, shiftedcolor, shiftfactor );
/*
		// gamma control
		color.x = std::pow( color.x, m_gammacorrection );
		color.x = std::pow( color.y, m_gammacorrection );
		color.x = std::pow( color.z, m_gammacorrection );
*/
        // crude correction for the times where the model breaks (late night)
        // TODO: use proper night sky calculation for these times instead
        if( ( color.x <= 0.05f )
         && ( color.y <= 0.05f ) ) {
            // darken the sky as the sun goes deeper below the horizon
            // 15:50:75 is picture-based night sky colour. it may not be accurate but looks 'right enough'
            color.z = 0.75f * std::max( color.z + m_sundirection.y, 0.075f );
            color.x = 0.20f * color.z; 
            color.y = 0.65f * color.z;
            color = color * ( 1.15f - vertex.y ); // simple gradient, darkening towards the top
        }
		// save
        m_colours[ i ] = color;
        averagecolor += color * 8.0f; // save for edge cases each vertex goes in 8 triangles
	}
    // NOTE: average reduced to 25% makes nice tint value for clouds lit from behind
    // down the road we could interpolate between it and full strength average, to improve accuracy of cloud appearance
    m_averagecolour = averagecolor / static_cast<float>( m_indices.size() );
    m_averagecolour.r = std::max( m_averagecolour.r, 0.0f );
    m_averagecolour.g = std::max( m_averagecolour.g, 0.0f );
    m_averagecolour.b = std::max( m_averagecolour.b, 0.0f );

    if( m_coloursbuffer != -1 ) {
        // the colour buffer was already initialized, so on this run we update its content
        ::glBindBuffer( GL_ARRAY_BUFFER, m_coloursbuffer );
        ::glBufferSubData( GL_ARRAY_BUFFER, 0, m_colours.size() * sizeof( glm::vec3 ), m_colours.data() );
    }

}

//******************************************************************************//
