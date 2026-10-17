// SPDX-License-Identifier: GPL-3.0-only
/* Color helpers adapted from pinned raylib5.5 rtextures.c; copyright Ramon
   Santamaria, retained zlib notice docs/licenses/raylib.txt. */
#include "draw.h"
#include <math.h>
DrawColor draw_color_fade(DrawColor color, float alpha)
{
    DrawColor result = color;

    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;

    result.a = (unsigned char)(255.0f*alpha);

    return result;
}
DrawHSV draw_color_hsv(DrawColor color)
{
    DrawHSV hsv = { 0 };
    DrawHSV rgb = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
    float min, max, delta;

    min = rgb.x < rgb.y? rgb.x : rgb.y;
    min = min  < rgb.z? min  : rgb.z;

    max = rgb.x > rgb.y? rgb.x : rgb.y;
    max = max  > rgb.z? max  : rgb.z;

    hsv.z = max;            // Value
    delta = max - min;

    if (delta < 0.00001f)
    {
        hsv.y = 0.0f;
        hsv.x = 0.0f;       // Undefined, maybe NAN?
        return hsv;
    }

    if (max > 0.0f)
    {
        // NOTE: If max is 0, this divide would cause a crash
        hsv.y = (delta/max);    // Saturation
    }
    else
    {
        // NOTE: If max is 0, then r = g = b = 0, s = 0, h is undefined
        hsv.y = 0.0f;
        hsv.x = NAN;        // Undefined
        return hsv;
    }

    // NOTE: Comparing float values could not work properly
    if (rgb.x >= max) hsv.x = (rgb.y - rgb.z)/delta;    // Between yellow & magenta
    else
    {
        if (rgb.y >= max) hsv.x = 2.0f + (rgb.z - rgb.x)/delta;  // Between cyan & yellow
        else hsv.x = 4.0f + (rgb.x - rgb.y)/delta;      // Between magenta & cyan
    }

    hsv.x *= 60.0f;     // Convert to degrees

    if (hsv.x < 0.0f) hsv.x += 360.0f;

    return hsv;
}
DrawColor draw_color_blend(DrawColor dst, DrawColor src, DrawColor tint)
{
    DrawColor out = ((DrawColor){255,255,255,255});

    // Apply color tint to source color
    src.r = (unsigned char)(((unsigned int)src.r*((unsigned int)tint.r+1)) >> 8);
    src.g = (unsigned char)(((unsigned int)src.g*((unsigned int)tint.g+1)) >> 8);
    src.b = (unsigned char)(((unsigned int)src.b*((unsigned int)tint.b+1)) >> 8);
    src.a = (unsigned char)(((unsigned int)src.a*((unsigned int)tint.a+1)) >> 8);

//#define COLORALPHABLEND_FLOAT
#define COLORALPHABLEND_INTEGERS
#if defined(COLORALPHABLEND_INTEGERS)
    if (src.a == 0) out = dst;
    else if (src.a == 255) out = src;
    else
    {
        unsigned int alpha = (unsigned int)src.a + 1;     // We are shifting by 8 (dividing by 256), so we need to take that excess into account
        out.a = (unsigned char)(((unsigned int)alpha*256 + (unsigned int)dst.a*(256 - alpha)) >> 8);

        if (out.a > 0)
        {
            out.r = (unsigned char)((((unsigned int)src.r*alpha*256 + (unsigned int)dst.r*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
            out.g = (unsigned char)((((unsigned int)src.g*alpha*256 + (unsigned int)dst.g*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
            out.b = (unsigned char)((((unsigned int)src.b*alpha*256 + (unsigned int)dst.b*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
        }
    }
#endif
#if defined(COLORALPHABLEND_FLOAT)
    if (src.a == 0) out = dst;
    else if (src.a == 255) out = src;
    else
    {
        Vector4 fdst = ColorNormalize(dst);
        Vector4 fsrc = ColorNormalize(src);
        Vector4 ftint = ColorNormalize(tint);
        Vector4 fout = { 0 };

        fout.w = fsrc.w + fdst.w*(1.0f - fsrc.w);

        if (fout.w > 0.0f)
        {
            fout.x = (fsrc.x*fsrc.w + fdst.x*fdst.w*(1 - fsrc.w))/fout.w;
            fout.y = (fsrc.y*fsrc.w + fdst.y*fdst.w*(1 - fsrc.w))/fout.w;
            fout.z = (fsrc.z*fsrc.w + fdst.z*fdst.w*(1 - fsrc.w))/fout.w;
        }

        out = (Color){ (unsigned char)(fout.x*255.0f), (unsigned char)(fout.y*255.0f), (unsigned char)(fout.z*255.0f), (unsigned char)(fout.w*255.0f) };
    }
#endif

    return out;
}

DrawColor draw_color_from_hsv(float hue, float saturation, float value)
{
    DrawColor color = { 0, 0, 0, 255 };

    // Red channel
    float k = fmodf((5.0f + hue/60.0f), 6);
    float t = 4.0f - k;
    k = (t < k)? t : k;
    k = (k < 1)? k : 1;
    k = (k > 0)? k : 0;
    color.r = (unsigned char)((value - value*saturation*k)*255.0f);

    // Green channel
    k = fmodf((3.0f + hue/60.0f), 6);
    t = 4.0f - k;
    k = (t < k)? t : k;
    k = (k < 1)? k : 1;
    k = (k > 0)? k : 0;
    color.g = (unsigned char)((value - value*saturation*k)*255.0f);

    // Blue channel
    k = fmodf((1.0f + hue/60.0f), 6);
    t = 4.0f - k;
    k = (t < k)? t : k;
    k = (k < 1)? k : 1;
    k = (k > 0)? k : 0;
    color.b = (unsigned char)((value - value*saturation*k)*255.0f);

    return color;
}
