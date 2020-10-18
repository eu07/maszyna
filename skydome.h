#pragma	once

// sky gradient based on "A practical analytic model for daylight" 
// by A. J. Preetham Peter Shirley Brian Smits (University of Utah)

class CSkyDome {
public:
    CSkyDome( int const Tesselation = 54 );
	~CSkyDome();
	void Generate();
	void RebuildColors();

	bool SetSunPosition( glm::vec3 const &Direction );
	
	void SetTurbidity( const float Turbidity = 5.0f );
	void SetExposure( const bool Linearexposure, const float Expfactor );		
    void SetOvercastFactor( const float Overcast = 0.0f );

	// update skydome
    void Update( glm::vec3 const &Sun );

    // retrieves average colour of the sky dome
    glm::vec3 GetAverageColor() { return m_averagecolour * 8.f / 6.f; }
    glm::vec3 GetAverageHorizonColor() { return m_averagehorizoncolour; }

    std::vector<glm::vec3> const & vertices() const {
        return m_vertices; }
    std::vector<glm::vec3> & colors() {
        return m_colours; }
    std::vector<std::uint16_t> const & indices() const {
        return m_indices; }
    auto const & is_dirty() const { return m_dirty; }
    auto & is_dirty() { return m_dirty; }

private:
	// shading parametrs
    glm::vec3 m_sundirection;
    float m_thetasun, m_phisun;
    float m_turbidity;
    bool m_linearexpcontrol;
    float m_expfactor;
    float m_overcast;
    glm::vec3 m_averagecolour;
    glm::vec3 m_averagehorizoncolour;

	// data
    int const m_tesselation;
    std::vector<glm::vec3> m_vertices;
    std::vector<std::uint16_t> m_indices;
//    std::vector<float3> m_normals;
    std::vector<glm::vec3> m_colours;
    bool m_dirty { true }; // indicates sync state between simulation and gpu sides

	static float m_distributionluminance[ 5 ][ 2 ];
    static float m_distributionxcomp[ 5 ][ 2 ];
    static float m_distributionycomp[ 5 ][ 2 ];

    static float m_zenithxmatrix[ 3 ][ 4 ];
    static float m_zenithymatrix[ 3 ][ 4 ];
	
	// coloring
	void GetPerez( float *Perez, float Distribution[ 5 ][ 2 ], const float Turbidity );
	float GetZenith( float Zenithmatrix[ 3 ][ 4 ], const float Theta, const float Turbidity );		
	float PerezFunctionO1( float Perezcoeffs[ 5 ], const float Thetasun, const float Zenithval );
	float PerezFunctionO2( float Perezcoeffs[ 5 ], const float Icostheta, const float Gamma, const float Cosgamma2, const float Zenithval );
};

