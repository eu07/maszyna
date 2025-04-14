#ifndef GBUFFER_CONTACT_SHADOWS_HLSLI
#define GBUFFER_CONTACT_SHADOWS_HLSLI
Texture2D<float> g_ContactShadows : register(t12);

float GetContactShadows(in uint2 pixel_position) {
  return g_ContactShadows[pixel_position];
}

#endif

