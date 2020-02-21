#if ENVMAP_ENABLED
uniform samplerCube envmap;
#endif

vec3 envmap_color( vec3 normal )
{
#if ENVMAP_ENABLED
	vec3 refvec = reflect(f_pos.xyz, normal); // view space
	refvec = vec3(inv_view * vec4(refvec, 0.0)); // world space
	vec3 envcolor = texture(envmap, refvec).rgb;
#else
	vec3 envcolor = vec3(0.5);
#endif
	return envcolor;
}
