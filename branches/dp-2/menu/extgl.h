// These extensions are supported:
#define GL_ARB_multitexture					1
#define GL_ARB_texture_compression			1		// Not all
#define GL_ARB_texture_cube_map				1
#define GL_ARB_texture_env_add				1
#define GL_ARB_texture_env_combine			1
#define GL_ARB_texture_env_dot3				1

#define GL_EXT_bgra							1
#define GL_EXT_separate_specular_color		1
#define GL_EXT_texture_edge_clamp			1
#define GL_EXT_texture_filter_anisotropic	1

#define GL_SGIS_generate_mipmap				1
#define GL_SGIS_texture_edge_clamp			1


#ifndef __EXTGL_H_
#define __EXTGL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APIENTRY
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#define GL_CLAMP_TO_EDGE							0x812F

/* GL_ARB_multitexture - Not all */
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB            0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3
#define GL_TEXTURE4_ARB                     0x84C4
#define GL_TEXTURE5_ARB                     0x84C5
#define GL_TEXTURE6_ARB                     0x84C6
#define GL_TEXTURE7_ARB                     0x84C7

typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FARBPROC) (GLenum texture, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FARBPROC) (GLenum texture, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FARBPROC) (GLenum texture, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC) (GLenum texture, const GLfloat *v);
//typedef void (APIENTRY * PFNGLMULTITEXCOORD4FARBPROC) (GLenum texture, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
//typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC) (GLenum texture, const GLfloat *v);

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;
extern PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB;
extern PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
//extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
//extern PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB;

/* GL_ARB_texture_compression  - Not all */ 
#define GL_COMPRESSED_ALPHA_ARB               0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB           0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB     0x84EB
#define GL_COMPRESSED_INTENSITY_ARB           0x84EC
#define GL_COMPRESSED_RGB_ARB                 0x84ED
#define GL_COMPRESSED_RGBA_ARB                0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB       0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB  0x86A0
#define GL_TEXTURE_COMPRESSED_ARB             0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB     0x86A3

/* GL_ARB_texture_cube_map */
#define GL_NORMAL_MAP_ARB							0x8511
#define GL_REFLECTION_MAP_ARB						0x8512
#define GL_TEXTURE_CUBE_MAP_ARB						0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB				0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB			0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB			0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB			0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB			0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB			0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB			0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB				0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB			0x851C

/* GL_ARB_texture_env_combine */
#define GL_COMBINE_ARB        0x8570
#define GL_COMBINE_RGB_ARB    0x8571
#define GL_COMBINE_ALPHA_ARB  0x8572
#define GL_SOURCE0_RGB_ARB    0x8580
#define GL_SOURCE1_RGB_ARB    0x8581
#define GL_SOURCE2_RGB_ARB    0x8582
#define GL_SOURCE0_ALPHA_ARB  0x8588
#define GL_SOURCE1_ALPHA_ARB  0x8589
#define GL_SOURCE2_ALPHA_ARB  0x858A
#define GL_OPERAND0_RGB_ARB   0x8590
#define GL_OPERAND1_RGB_ARB   0x8591
#define GL_OPERAND2_RGB_ARB   0x8592
#define GL_OPERAND0_ALPHA_ARB 0x8598
#define GL_OPERAND1_ALPHA_ARB 0x8599
#define GL_OPERAND2_ALPHA_ARB 0x859A
#define GL_RGB_SCALE_ARB      0x8573
#define GL_ADD_SIGNED_ARB     0x8574
#define GL_INTERPOLATE_ARB    0x8575
#define GL_CONSTANT_ARB       0x8576
#define GL_PRIMARY_COLOR_ARB  0x8577
#define GL_PREVIOUS_ARB       0x8578
#define GL_SUBTRACT_ARB       0x84E7

/* GL_ARB_texture_env_dot3 */
#define GL_DOT3_RGB_ARB  0x86AE
#define GL_DOT3_RGBA_ARB 0x86AF


/* GL_EXT_separate_specular_color */
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT     0x81F8
#define GL_SINGLE_COLOR_EXT                  0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_EXT       0x81FA

/* GL_EXT_texture_filter_anisotropic */
#define GL_TEXTURE_MAX_ANISOTROPY_EXT        0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT    0x84FF

/* GL_SGIS_generate_mipmap */
#define GL_GENERATE_MIPMAP_SGIS              0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS         0x8192


#ifdef __cplusplus
}
#endif

int extInit();

struct extension
{
	int ARB_multitexture;
	int ARB_texture_compression;
	int ARB_texture_cube_map;
	int ARB_texture_env_add;
	int ARB_texture_env_combine;
	int ARB_texture_env_dot3;
	int EXT_bgra;
	int EXT_separate_specular_color;
	int EXT_texture_edge_clamp;
	int EXT_texture_filter_anisotropic;
	int SGIS_generate_mipmap;
	struct maximum
	{
		int	max_texture_size;
		int max_cube_map_texture_size;
		int max_texture_max_anisotropy;
		int max_texture_units;
	}max;
	int z_error;
};

extern extension ext;
#endif // __EXTGL_H_