#pragma once
struct _global
{
        bool gfx_shadowmap_enabled;
        bool gfx_envmap_enabled;
        bool gfx_postfx_motionblur_enabled;
        bool gfx_skippipeline;
        bool gfx_extraeffects;
        bool gfx_shadergamma;
        bool gfx_usegles;
};

extern _global Global;

