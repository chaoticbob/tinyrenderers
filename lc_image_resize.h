/*

Copyright (c) 2016, Libertus Code
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

/*

Adapted from Cinder: libcinder.org

Copyright (c) 2010, The Barbarian Group
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <stdbool.h>
#include <string.h>

typedef enum lc_filter {
    LC_FILTER_UNDEFINED = 0,
    LC_FILTER_BOX,
    LC_FILTER_TRIANGLE,
    LC_FILTER_QUADRATIC,
    LC_FILTER_CUBIC,
    LC_FILTER_CATMUL_ROM,
    LC_FILTER_MITCHELL,
    LC_FILTER_SINC_BLACKMAN,
    LC_FILTER_GAUSSIAN,
    LC_FILTER_BESSEL_BLACKMAN,
    LC_FILTER_MAX
} lc_filter;

struct lc_filter_args {
    float support;  /* support radius */
    float b;        /* b value */
    float c;        /* c value */

    /* consider these private */
    float q0, q1, q2, q3;
    float p0, p2, p3;
};

void lc_image_resize_uint8(int src_width, int src_height, int src_row_stride, const unsigned char* p_src_data,
                           int dst_wdith, int dst_height, int dst_row_stride, unsigned char* p_dst_data,
                           unsigned int channel_count, lc_filter filter, const lc_filter_args* p_filter_args);

void lc_image_resize_float(int src_width, int src_height, int src_row_stride, const float* p_src_data,
                           int dst_wdith, int dst_height, int dst_row_stride, float* p_dst_data,
                           unsigned int channel_count, lc_filter filter, const lc_filter_args* p_filter_args);



#if defined(LC_IMAGE_RESIZE_IMPLEMENTATION)

#if defined(__cplusplus)  
    #define LC_DECLARE_ZERO(type, var) \
            type var = {};                        
#else
    #define LC_DECLARE_ZERO(type, var) \
            type var = {0};                        
#endif

#define LC_SAFE_FREE(p_var) \
    if (NULL != p_var) {    \
       free((void*)p_var);  \
       p_var = NULL;        \
    }


#define LC_MATH_MIN(a, b) \
    (a < b ? a : b)

#define LC_MATH_MAX(a, b) \
    (a > b ? a : b)

typedef unsigned int    lc_uint8_sum_t;
typedef unsigned char   lc_uint8_data_t;
typedef float           lc_float_sum_t;
typedef float           lc_float_data_t;

const int k_lc_uint8_weight_bits        = 14;                                   /* # bits in filter coefficients */
const int k_lc_uint8_final_shift        = 2 * k_lc_uint8_weight_bits - 8;       /* shift after x & y filter passes */
const int k_lc_uint8_half_final_shift   = 1 << (k_lc_uint8_final_shift - 1);
const int k_lc_uint8_weight_one         = 1 << k_lc_uint8_weight_bits;          /* filter weight of one */

const float k_lc_float_weight_one       = 1.0f;                                 /* filter weight of one */

inline lc_uint8_data_t lc_uint8_accum_to_channel(const int in) 
{
    int result = (in + k_lc_uint8_half_final_shift) >> k_lc_uint8_final_shift;
    if (result < 0) {
        result = 0;
    }
    else if (result > 255) {
        result = 255;
    }
    return (lc_uint8_data_t)result;
}

inline lc_float_data_t lc_float_accum_to_channel(const float in) 
{
    return in;
}

inline int lc_uint8_channel_to_buffer(const int in) 
{ 
    return in >> 8; 
}

inline float lc_float_channel_to_buffer(const float in) 
{ 
    return in;
}

/* SAMPLED FILTER WEIGHT TABLE */
typedef struct lc_uint8_weight_table {
    int             start, end; /* range of samples is [start..end-1] */
    lc_uint8_sum_t* weight;     /* weight[i] goes with pixel at start+i */
} lc_uint8_weight_table;

typedef struct lc_float_weight_table {
    int             start, end; /* range of samples is [start..end-1] */
    lc_float_sum_t* weight;     /* weight[i] goes with pixel at start+i */
} lc_float_weight_table;

/* Mapping from discrete dest coordinates b to continuous source coordinates */
#define LC_MAP(b, scale, offset) \
    (((b)+(offset))/(scale))

/* ZOOM-SPECIFIC FILTER PARAMETERS */
typedef struct {
    float   scale;      /* filter scale (spacing between centers in a space) */
    float   support;    /* scaled filter support radius */
    int     width;      /* filter width: max number of nonzero samples */
} lc_filter_params;

/* SOURCE TO DEST COORDINATE MAPPING */
typedef struct {
    float   sx, sy;     /* x and y scales */
    float   tx, ty;     /* x and y translations */
    float   ux, uy;     /* x and y offset used by MAP, private fields */
} lc_mapping;

typedef struct lc_rect {
    float x1, y1;
    float x2, y2;
} lc_rect;

typedef struct lc_uint8_line_buffer {
    int             first;
    lc_uint8_sum_t* second;
} lc_uint8_line_buffer;

typedef struct lc_float_line_buffer {
    int             first;
    lc_float_sum_t* second;
} lc_float_line_buffer;

/**************************************************************************************************/
/* Filters                                                                                        */
/**************************************************************************************************/
typedef float(*lc_filter_fn)(float x, const lc_filter_args* p_params);

/* Box, pulse, nearest-neighbor, Fourier window, 1st order (constant) b-spline  */
void  lc_filter_box_init(lc_filter_args* p_params)
{
    p_params->support = 0.5f;
}

float lc_filter_box(float x, const lc_filter_args* p_params)
{
    if ( x < -0.5f ) return 0.0f;
    else if ( x < 0.5f ) return 1.0f;
    return 0.0f;
}

/* Triangle, Bartlett window, 2nd order (linear) b-spline */
void  lc_filter_triangle_init(lc_filter_args* p_params)
{
    p_params->support = 1.0f;
}

float lc_filter_triangle(float x, const lc_filter_args* p_params)
{
    if (x < -1.0f) return 0.0f;
    else if (x < 0.0f) return 1.0f + x;
    else if (x < 1.0f) return 1.0f - x;
    return 0.0f;
}

/* 3rd order (quadratic) b-spline */
void  lc_filter_quadratic_init(lc_filter_args* p_params)
{
    p_params->support = 1.5f;
}

float lc_filter_quadratic(float x, const lc_filter_args* p_params)
{
    float t;

    if ( x < -1.5f ) return 0.0f;
    else if ( x < -0.5f ) { t = x + 1.5f; return 0.5f * t * t; }
    else if ( x < 0.5f ) return 0.75f - x * x;
    else if ( x < 1.5f ) { t = x - 1.5f; return 0.5f * t * t; }
    return 0.0f;
}

/* 4th order (cubic) b-spline */
void  lc_filter_cubic_init(lc_filter_args* p_params)
{
    p_params->support = 2.0f;
}

float lc_filter_cubic(float x, const lc_filter_args* p_params)
{
    float t;

    if ( x < -2.0f ) return 0.0f;
    else if ( x < -1.0f ) { t = 2.0f + x; return t * t * t / 6.0f; }
    else if ( x < 0.0f ) return ( 4.0f + x * x * ( -6.0f + x * -3.0f ) ) / 6.0f;
    else if ( x < 1.0f ) return ( 4.0f + x * x * ( -6.0f + x * 3.0f ) ) / 6.0f;
    else if ( x < 2.0f ) { t = 2.0f - x; return t * t * t / 6.0f; }
    return 0.0f;
}

/* Catmull-Rom spline, Overhauser spline */
void  lc_filter_catmull_rom_init(lc_filter_args* p_params)
{
    p_params->support = 2.0f;
}

float lc_filter_catmull_rom(float x, const lc_filter_args* p_params)
{
    if ( x < -2.0f ) return 0.0f;
    else if ( x < -1.0f ) return 0.5f * ( 4.0f + x * ( 8.0f + x * ( 5.0f + x ) ) );
    else if ( x < 0.0f ) return 0.5f * ( 2.0f + x * x * ( -5.0f + x * -3.0f ) );
    else if ( x < 1.0f ) return 0.5f * ( 2.0f + x * x * ( -5.0f + x * 3.0f ) );
    else if ( x < 2.0f ) return 0.5f * ( 4.0f + x * ( -8.0f + x * ( 5.0f - x ) ) );
    return 0.0f;
}

/*
 Mitchell & Netravali's two-parameter cubic
 see Mitchell&Netravali, "Reconstruction Filters in Computer Graphics", SIGGRAPH 88
*/
void  lc_filter_mitchell_init(lc_filter_args* p_params)
{
    p_params->support = 2.0f;
    p_params->b = 0.3333333333f;
    p_params->c = 0.3333333333f;
    float b = p_params->b;
    float c = p_params->c;

    p_params->p0 = ( 6.0f - 2.0f * b ) / 6.0f;
    p_params->p2 = ( -18.0f + 12.0f * b + 6.0f * c ) / 6.0f;
    p_params->p3 = ( 12.0f - 9.0f * b - 6.0f * c ) / 6.0f;
    p_params->q0 = ( 8.0f * b + 24.0f * c ) / 6.0f;
    p_params->q1 = ( - 12.0f * b - 48.0f * c ) / 6.0f;
    p_params->q2 = ( 6.0f * b + 30.0f * c ) / 6.0f;
    p_params->q3 = ( -b - 6.0f * c ) / 6.0f;
}

float lc_filter_mitchell(float x, const lc_filter_args* p_params)
{
    if ( x < -2.0f ) return 0.;
    else if ( x < -1.0f ) return p_params->q0 - x * ( p_params->q1 - x * ( p_params->q2 - x * p_params->q3 ) );
    else if ( x < 0.0f ) return p_params->p0 + x * x * ( p_params->p2 - x * p_params->p3 );
    else if ( x < 1.0f ) return p_params->p0 + x * x * ( p_params->p2 + x * p_params->p3 );
    else if ( x < 2.0f ) return p_params->q0 + x * ( p_params->q1 + x * ( p_params->q2 + x * p_params->q3 ) );
    return 0.0f;
}

/* Sinc filter, windowed by Blackman */
void  lc_filter_sinc_blackman_init(lc_filter_args* p_params)
{
    p_params->support = 4.0f;
}

float lc_filter_sinc_blackman(float x, const lc_filter_args* p_params)
{
    float v( ( x == 0.0f ) ? 1.0f : sin( 3.14159265358979323846f * x ) / ( 3.14159265358979323846f * x ) );
    /* Blackman */
    x /= p_params->support;
    return v * ( 0.42f + 0.50f * cos( 3.14159265358979323846f * x ) + 0.08f * cos( 6.2831853071795862f * x ) );
}

/* Gaussian */
void  lc_filter_gassian_init(lc_filter_args* p_params)
{
    p_params->support = 1.25f;
}

float lc_filter_gassian(float x, const lc_filter_args* p_params)
{
    return ( exp( -2.0f * x * x ) * sqrt( 2.0f / 3.14159265358979323846f ) );
}

/* Bessel Blackman */
void  lc_filter_bessel_blackman_init(lc_filter_args* p_params)
{
    p_params->support = 3.2383f;
}

float lc_filter_bessel_blackman(float x, const lc_filter_args* p_params)
{
#if defined( WIN32 )
    /* According to VS.Net 2K5, j1 was deprecated, use _j1 instead. */
    float v( ( x == 0.0f ) ? ( 3.14159265358979323846f / 4.0f ) : (float)( _j1( 3.14159265358979323846f * x ) ) / ( 2.0f * x ) );
#else 
    float v( ( x == 0.0f ) ? ( 3.14159265358979323846f / 4.0f ) : (float)( j1( 3.14159265358979323846f * x ) ) / ( 2.0f * x ) );
#endif 
    /* always bet on Blackman */
    x /= p_params->support;
    return v * ( 0.42f + 0.50f * cos( 3.14159265358979323846f * x ) + 0.08f * cos( 6.2831853071795862f * x ) );
}

/**************************************************************************************************/
/* uint8                                                                                          */
/**************************************************************************************************/
void lc_uint8_scanline_accumulate(lc_uint8_sum_t weight, lc_uint8_sum_t* line_buffer, 
                                  int width, lc_uint8_sum_t* accum)
{
    lc_uint8_sum_t *dest = accum;
    for (int x = 0; x < width; ++x) {
        *dest++ += *line_buffer++ * weight;
    }
}

void lc_uint8_scanline_shift_accum_to_channel(lc_uint8_sum_t* accum, 
                                              int x1, int y, int width, int pixel_stride, int row_stride, 
                                              int channel, lc_uint8_data_t* p_data)
{
    lc_uint8_sum_t result;
    lc_uint8_data_t* dst = p_data + (y * row_stride) + (x1 * pixel_stride) + channel;
    for(int32_t i = 0; i < width; i++) {
        result = lc_uint8_accum_to_channel(*accum++);
        *dst = (lc_uint8_data_t)result;
        dst += pixel_stride;
    }
}

void lc_uint8_scanline_filter_channel_to_buffer(lc_uint8_weight_table* weights, 
                                                int x, int y, int pixel_stride, int row_stride, 
                                                int channel, const lc_uint8_data_t* p_data, 
                                                lc_uint8_sum_t* line_buffer, int width)
{
    int b, af;
    lc_uint8_sum_t sum;
    lc_uint8_sum_t *wp;
    const lc_uint8_data_t *src_line;

    src_line = p_data + (y * row_stride) + (x * pixel_stride) + channel;
    for (b = 0; b < width; b++) {
        sum = 1 << 7;
        const lc_uint8_data_t* src = src_line + weights->start * pixel_stride;
        wp = weights->weight;
        for (af = weights->start; af < weights->end; af++) {
            sum += *wp++ * *src;
            src += pixel_stride;
        }
        *line_buffer++ = lc_uint8_channel_to_buffer(sum);
        weights++;
    }   
}

void lc_uint8_make_weight_table(int b, float cen, 
                                lc_filter_fn filter, const lc_filter_args* p_filter_args, 
                                const lc_filter_params *params, 
                                int len, bool trimzeros, lc_uint8_weight_table* wtab)
{
    int start, end, i, stillzero, lastnonzero, nz;
    lc_uint8_sum_t *wp, t, sum;
    float den, sc, tr;

    /* find the source coord range of this positioned filter: [start..end-1] */
    start = (int)(cen - params->support + 0.5f);
    end = (int)(cen + params->support + 0.5f);
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;

    /* the range of source samples to buffer: */
    wtab->start = start;
    wtab->end = end;

    /* find scale factor sc to normalize the filter */
    for (den = 0, i=start; i < end; i++)
        den += filter((i + 0.5f - cen) / params->scale, p_filter_args);

    /* set sc so that sum of sc*func() is approximately WEIGHTONE */
    sc = (den == 0.0f) ? (k_lc_uint8_weight_one) : (k_lc_uint8_weight_one / den);

    /* compute the discrete, sampled filter coefficients */
    stillzero = trimzeros;
    for (sum = 0, wp = wtab->weight, i = start; i < end; i++) {
        /* evaluate the filter function: */
        tr = sc * filter((i + 0.5f - cen) / params->scale, p_filter_args);

        t = (lc_uint8_sum_t)floor(tr + 0.5f);
        if (stillzero && (t == 0)) {
            /* find first nonzero */
            start++;
        }
        else {
            stillzero = 0;
            /* add weight to table */
            *wp++ = t;
            sum += t;
            if (t != 0) {
                /* find last nonzero */
                lastnonzero = i;
            }
        }
    }
        
    if (sum == 0) {
        nz = wtab->end-wtab->start;
        wtab->start = (wtab->start+wtab->end) >> 1;
        wtab->end = wtab->start+1;
        wtab->weight[0] = k_lc_uint8_weight_one;
    }
    else {
        if (trimzeros) {
            /* skip leading and trailing zeros */
            /* set wtab->start and ->end to the nonzero support of the filter */
            nz = wtab->end-wtab->start-(lastnonzero-start+1);
            wtab->start = start;
            wtab->end = end = lastnonzero+1;
        }
        else {
            /* keep leading and trailing zeros */
            nz = 0;
        }

        if (sum != k_lc_uint8_weight_one) {
            /*
                * Fudge the center slightly to make sum=WEIGHTONE exactly.
                * Is this the best way to normalize a discretely sampled
                * continuous filter?
            */
            i = (int32_t)(cen + 0.5f);
            if (i < start) {
                i = start;
            }
            else if (i >= end) {
                i = end - 1;
            }
            t = k_lc_uint8_weight_one - sum;
            /* fudge center sample */
            wtab->weight[i - start] += t;
        }
    }   
}

void lc_image_resize_uint8(int src_width, int src_height, int src_row_stride, const unsigned char* p_src_data,
                           int dst_width, int dst_height, int dst_row_stride, unsigned char* p_dst_data,
                           unsigned int channel_count, lc_filter filter, const lc_filter_args* p_filter_args)
{
    LC_DECLARE_ZERO(lc_filter_args, filter_args);
    lc_filter_fn filter_fn = NULL;
    switch (filter) {
        case LC_FILTER_BOX: {
            filter_fn = lc_filter_box;
            if (NULL == p_filter_args) {lc_filter_box_init(&filter_args);}
        }
        break;
        case LC_FILTER_TRIANGLE: {
            filter_fn = lc_filter_triangle;
            if (NULL == p_filter_args) {lc_filter_triangle_init(&filter_args);}
        }
        break;
        case LC_FILTER_QUADRATIC: {
            filter_fn = lc_filter_quadratic;
            if (NULL == p_filter_args) {lc_filter_quadratic_init(&filter_args);}
        }
        break;
        case LC_FILTER_CUBIC: {
            filter_fn = lc_filter_cubic;
            if (NULL == p_filter_args) {lc_filter_cubic_init(&filter_args);}
        }
        break;
        case LC_FILTER_CATMUL_ROM: {
            filter_fn = lc_filter_catmull_rom;
            if (NULL == p_filter_args) {lc_filter_catmull_rom_init(&filter_args);}
        }
        break;
        case LC_FILTER_MITCHELL: {
            filter_fn = lc_filter_mitchell;
            if (NULL == p_filter_args) {lc_filter_mitchell_init(&filter_args);}
        }
        break;
        case LC_FILTER_SINC_BLACKMAN: {
            filter_fn = lc_filter_sinc_blackman;
            if (NULL == p_filter_args) {lc_filter_sinc_blackman_init(&filter_args);}
        }
        break;
        case LC_FILTER_GAUSSIAN: {
            filter_fn = lc_filter_gassian;
            if (NULL == p_filter_args) {lc_filter_gassian_init(&filter_args);}
        }
        break;
        case LC_FILTER_BESSEL_BLACKMAN: {
            filter_fn = lc_filter_bessel_blackman;
            if (NULL == p_filter_args) {lc_filter_bessel_blackman_init(&filter_args);}
        }
        break;

        default: break;
    }
    assert(NULL != filter);

    if (NULL != p_filter_args) {
        memcpy(&filter_args, p_filter_args, sizeof(*p_filter_args));
    }

    int src_offset_x = 0;
    int src_offset_y = 0;

    LC_DECLARE_ZERO(lc_rect, clipped_src_rect);
    clipped_src_rect.x1 = 0;
    clipped_src_rect.y1 = 0;
    clipped_src_rect.x2 = (float)src_width;
    clipped_src_rect.y2 = (float)src_height;
    
    LC_DECLARE_ZERO(lc_rect, clipped_dst_area);
    clipped_dst_area.x1 = 0;
    clipped_dst_area.y1 = 0;
    clipped_dst_area.x2 = (float)dst_width;
    clipped_dst_area.y2 = (float)dst_height;
    
    LC_DECLARE_ZERO(lc_mapping, m);
    m.sx = dst_width / (float)src_width;
    m.sy = dst_height / (float)src_height;
    m.tx = clipped_dst_area.x1 - 0.5f - m.sx * (clipped_src_rect.x1 - 0.5f);
    m.ty = clipped_dst_area.y1 - 0.5f - m.sy * (clipped_src_rect.y1 - 0.5f);
    m.ux = clipped_dst_area.x1 - m.sx * (clipped_src_rect.x1 - 0.5f) - m.tx;
    m.uy = clipped_dst_area.y1 - m.sy * (clipped_src_rect.y1 - 0.5f) - m.ty;

    LC_DECLARE_ZERO(lc_filter_params, filter_params_x);
    filter_params_x.scale   = LC_MATH_MAX(1.0f, 1.0f / m.sx);
    filter_params_x.support = LC_MATH_MAX(0.5f, filter_params_x.scale * filter_args.support);
    filter_params_x.width   = (int)ceil(2.0f * filter_params_x.support);

    LC_DECLARE_ZERO(lc_filter_params, filter_params_y);
    filter_params_y.scale   = LC_MATH_MAX(1.0f, 1.0f / m.sy);
    filter_params_y.support = LC_MATH_MAX(0.5f, filter_params_y.scale * filter_args.support);
    filter_params_y.width   = (int)ceil(2.0f * filter_params_y.support);

    lc_uint8_line_buffer* lines_buffer = (lc_uint8_line_buffer*)calloc(filter_params_y.width, sizeof(*lines_buffer));
    assert(NULL != lines_buffer);

    for(int i = 0; i < filter_params_y.width; i++) {
        lines_buffer[i].first  = -1;
        lines_buffer[i].second = (lc_uint8_sum_t*)calloc(dst_width, sizeof(lines_buffer[i].second));
        assert(NULL != lines_buffer[i].second);
    }

    lc_uint8_weight_table* x_weights = (lc_uint8_weight_table*)calloc(dst_width, sizeof(*x_weights));
    assert(NULL != x_weights);

    lc_uint8_sum_t* x_weight_buffer = (lc_uint8_sum_t*)calloc(dst_width * filter_params_x.width, sizeof(*x_weight_buffer));
    assert(NULL != x_weight_buffer);

    LC_DECLARE_ZERO(lc_uint8_weight_table, y_weights);
    y_weights.weight = (lc_uint8_sum_t*)calloc(filter_params_y.width, sizeof(*y_weights.weight));
    assert(NULL != y_weights.weight);

    lc_uint8_sum_t* accum = (lc_uint8_sum_t*)calloc(dst_width, sizeof(*accum));
    assert(NULL != accum);

    lc_uint8_sum_t* xWeightPtr = x_weight_buffer;
    for (int bx = 0; bx < dst_width; ++bx, xWeightPtr += filter_params_x.width) {
        x_weights[bx].weight = xWeightPtr;
        lc_uint8_make_weight_table(bx, LC_MAP(bx, m.sx, m.ux), filter_fn, &filter_args, &filter_params_x, src_width, true, &x_weights[bx]);
    }

    int pixel_stride = channel_count * sizeof(lc_uint8_data_t);
    for (unsigned int channel = 0; channel < channel_count; ++channel) {
        /* loop over dest scanlines */
        for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
            /* prepare a weight table for dest y position by */
            lc_uint8_make_weight_table(dst_y, LC_MAP(dst_y, m.sy, m.uy), filter_fn, &filter_args, &filter_params_y, src_height, false, &y_weights);
            memset(accum, 0, sizeof(*accum) * dst_width);
            /* loop over source scanlines that influence this dest scanline */
            for (int ayf = y_weights.start; ayf < y_weights.end; ++ayf) {
                lc_uint8_sum_t* line = lines_buffer[ayf % filter_params_y.width].second;
                if (lines_buffer[ayf % filter_params_y.width].first != ayf) {
                    lc_uint8_scanline_filter_channel_to_buffer(x_weights, src_offset_x, src_offset_y + ayf, pixel_stride, src_row_stride, channel, p_src_data, line, dst_width);
                    lines_buffer[ayf % filter_params_y.width].first = ayf;
                }
                lc_uint8_scanline_accumulate(y_weights.weight[ayf - y_weights.start], line, dst_width, accum);
            }
            lc_uint8_scanline_shift_accum_to_channel(accum, (int)clipped_dst_area.x1, (int)(clipped_dst_area.y1 + dst_y), dst_width, pixel_stride, dst_row_stride, channel, p_dst_data);
        }
    }

    LC_SAFE_FREE(accum);
    LC_SAFE_FREE(y_weights.weight );
    LC_SAFE_FREE(x_weight_buffer);
    LC_SAFE_FREE(x_weights);
    for(int i = 0; i < filter_params_y.width; i++) {
        LC_SAFE_FREE(lines_buffer[i].second);
    }
    LC_SAFE_FREE(lines_buffer);
}

/**************************************************************************************************/
/* float                                                                                          */
/**************************************************************************************************/
void lc_float_scanline_accumulate(lc_float_sum_t weight, lc_float_sum_t* line_buffer, 
                                  int width, lc_float_sum_t* accum)
{
    lc_float_sum_t *dest = accum;
    for (int x = 0; x < width; ++x) {
        *dest++ += *line_buffer++ * weight;
    }
}

void lc_float_scanline_shift_accum_to_channel(lc_float_sum_t* accum, 
                                              int x1, int y, int width, int pixel_stride, int row_stride, 
                                              int channel, lc_float_data_t* p_data)
{
    lc_float_sum_t result;
    unsigned char* dst = (unsigned char*)p_data + (y * row_stride) + (x1 * pixel_stride) + (channel * sizeof(*p_data));
    for(int32_t i = 0; i < width; i++) {
        result = lc_float_accum_to_channel(*accum++);
        *((float *)dst) = (lc_float_data_t)result;
        dst += pixel_stride;
    }
}

void lc_float_scanline_filter_channel_to_buffer(lc_float_weight_table* weights, 
                                                int x, int y, int pixel_stride, int row_stride, 
                                                int channel, const lc_float_data_t* p_data, 
                                                lc_float_sum_t* line_buffer, int width)
{
    int b, af;
    lc_float_sum_t sum;
    lc_float_sum_t *wp;
    const unsigned char *src_line;

    src_line = (const unsigned char*)p_data + (y * row_stride) + (x * pixel_stride) + (channel * sizeof(*p_data));
    for (b = 0; b < width; b++) {
        sum = 0.0f;
        const unsigned char* src = src_line + weights->start * pixel_stride;
        wp = weights->weight;
        for (af = weights->start; af < weights->end; af++) {
            sum += *wp++ * *((const float*)src);
            src += pixel_stride;
        }
        *line_buffer++ = lc_float_channel_to_buffer(sum);
        weights++;
    }   
}

void lc_float_make_weight_table(int b, float cen, 
                                lc_filter_fn filter, const lc_filter_args* p_filter_args, 
                                const lc_filter_params *params, 
                                int len, bool trimzeros, lc_float_weight_table* wtab)
{
    int start, end, i, stillzero, lastnonzero, nz;
    lc_float_sum_t *wp, t, sum;
    float den, sc, tr;

    /* find the source coord range of this positioned filter: [start..end-1] */
    start = (int)(cen - params->support + 0.5f);
    end = (int)(cen + params->support + 0.5f);
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;

    /* the range of source samples to buffer: */
    wtab->start = start;
    wtab->end = end;

    /* find scale factor sc to normalize the filter */
    for (den = 0, i=start; i < end; i++)
        den += filter((i + 0.5f - cen) / params->scale, p_filter_args);

    /* set sc so that sum of sc*func() is approximately WEIGHTONE */
    sc = (den == 0.0f) ? (k_lc_float_weight_one) : (k_lc_float_weight_one / den);

    /* compute the discrete, sampled filter coefficients */
    stillzero = trimzeros;
    for (sum = 0, wp = wtab->weight, i = start; i < end; i++) {
        /* evaluate the filter function: */
        tr = sc * filter((i + 0.5f - cen) / params->scale, p_filter_args);

        t = (lc_float_sum_t)tr;
        if (stillzero && (t == 0)) {
            /* find first nonzero */
            start++;
        }
        else {
            stillzero = 0;
            /* add weight to table */
            *wp++ = t;
            sum += t;
            if (t != 0) {
                /* find last nonzero */
                lastnonzero = i;
            }
        }
    }
        
    if (sum == 0) {
        nz = wtab->end-wtab->start;
        wtab->start = (wtab->start+wtab->end) >> 1;
        wtab->end = wtab->start+1;
        wtab->weight[0] = k_lc_float_weight_one;
    }
    else {
        if (trimzeros) {
            /* skip leading and trailing zeros */
            /* set wtab->start and ->end to the nonzero support of the filter */
            nz = wtab->end-wtab->start-(lastnonzero-start+1);
            wtab->start = start;
            wtab->end = end = lastnonzero+1;
        }
        else {
            /* keep leading and trailing zeros */
            nz = 0;
        }

        if (sum != k_lc_float_weight_one) {
            /*
                * Fudge the center slightly to make sum=WEIGHTONE exactly.
                * Is this the best way to normalize a discretely sampled
                * continuous filter?
            */
            i = (int32_t)(cen + 0.5f);
            if (i < start) {
                i = start;
            }
            else if (i >= end) {
                i = end - 1;
            }
            t = k_lc_float_weight_one - sum;
            /* fudge center sample */
            wtab->weight[i - start] += t;
        }
    }   
}

void lc_image_resize_float(int src_width, int src_height, int src_row_stride, const float* p_src_data,
                           int dst_width, int dst_height, int dst_row_stride, float* p_dst_data,
                           unsigned int channel_count, lc_filter filter, const lc_filter_args* p_filter_args)
{
    LC_DECLARE_ZERO(lc_filter_args, filter_args);
    lc_filter_fn filter_fn = NULL;
    switch (filter) {
        case LC_FILTER_BOX: {
            filter_fn = lc_filter_box;
            if (NULL == p_filter_args) {lc_filter_box_init(&filter_args);}
        }
        break;
        case LC_FILTER_TRIANGLE: {
            filter_fn = lc_filter_triangle;
            if (NULL == p_filter_args) {lc_filter_triangle_init(&filter_args);}
        }
        break;
        case LC_FILTER_QUADRATIC: {
            filter_fn = lc_filter_quadratic;
            if (NULL == p_filter_args) {lc_filter_quadratic_init(&filter_args);}
        }
        break;
        case LC_FILTER_CUBIC: {
            filter_fn = lc_filter_cubic;
            if (NULL == p_filter_args) {lc_filter_cubic_init(&filter_args);}
        }
        break;
        case LC_FILTER_CATMUL_ROM: {
            filter_fn = lc_filter_catmull_rom;
            if (NULL == p_filter_args) {lc_filter_catmull_rom_init(&filter_args);}
        }
        break;
        case LC_FILTER_MITCHELL: {
            filter_fn = lc_filter_mitchell;
            if (NULL == p_filter_args) {lc_filter_mitchell_init(&filter_args);}
        }
        break;
        case LC_FILTER_SINC_BLACKMAN: {
            filter_fn = lc_filter_sinc_blackman;
            if (NULL == p_filter_args) {lc_filter_sinc_blackman_init(&filter_args);}
        }
        break;
        case LC_FILTER_GAUSSIAN: {
            filter_fn = lc_filter_gassian;
            if (NULL == p_filter_args) {lc_filter_gassian_init(&filter_args);}
        }
        break;
        case LC_FILTER_BESSEL_BLACKMAN: {
            filter_fn = lc_filter_bessel_blackman;
            if (NULL == p_filter_args) {lc_filter_bessel_blackman_init(&filter_args);}
        }
        break;

        default: break;
    }
    assert(NULL != filter);

    if (NULL != p_filter_args) {
        memcpy(&filter_args, p_filter_args, sizeof(*p_filter_args));
    }

    int src_offset_x = 0;
    int src_offset_y = 0;

    LC_DECLARE_ZERO(lc_rect, clipped_src_rect);
    clipped_src_rect.x1 = 0;
    clipped_src_rect.y1 = 0;
    clipped_src_rect.x2 = (float)src_width;
    clipped_src_rect.y2 = (float)src_height;
    
    LC_DECLARE_ZERO(lc_rect, clipped_dst_area);
    clipped_dst_area.x1 = 0;
    clipped_dst_area.y1 = 0;
    clipped_dst_area.x2 = (float)dst_width;
    clipped_dst_area.y2 = (float)dst_height;
    
    LC_DECLARE_ZERO(lc_mapping, m);
    m.sx = dst_width / (float)src_width;
    m.sy = dst_height / (float)src_height;
    m.tx = clipped_dst_area.x1 - 0.5f - m.sx * (clipped_src_rect.x1 - 0.5f);
    m.ty = clipped_dst_area.y1 - 0.5f - m.sy * (clipped_src_rect.y1 - 0.5f);
    m.ux = clipped_dst_area.x1 - m.sx * (clipped_src_rect.x1 - 0.5f) - m.tx;
    m.uy = clipped_dst_area.y1 - m.sy * (clipped_src_rect.y1 - 0.5f) - m.ty;

    LC_DECLARE_ZERO(lc_filter_params, filter_params_x);
    filter_params_x.scale   = LC_MATH_MAX(1.0f, 1.0f / m.sx);
    filter_params_x.support = LC_MATH_MAX(0.5f, filter_params_x.scale * filter_args.support);
    filter_params_x.width   = (int)ceil(2.0f * filter_params_x.support);

    LC_DECLARE_ZERO(lc_filter_params, filter_params_y);
    filter_params_y.scale   = LC_MATH_MAX(1.0f, 1.0f / m.sy);
    filter_params_y.support = LC_MATH_MAX(0.5f, filter_params_y.scale * filter_args.support);
    filter_params_y.width   = (int)ceil(2.0f * filter_params_y.support);

    lc_float_line_buffer* lines_buffer = (lc_float_line_buffer*)calloc(filter_params_y.width, sizeof(*lines_buffer));
    assert(NULL != lines_buffer);

    for(int i = 0; i < filter_params_y.width; i++) {
        lines_buffer[i].first  = -1;
        lines_buffer[i].second = (lc_float_sum_t*)calloc(dst_width, sizeof(lines_buffer[i].second));
        assert(NULL != lines_buffer[i].second);
    }

    lc_float_weight_table* x_weights = (lc_float_weight_table*)calloc(dst_width, sizeof(*x_weights));
    assert(NULL != x_weights);

    lc_float_sum_t* x_weight_buffer = (lc_float_sum_t*)calloc(dst_width * filter_params_x.width, sizeof(*x_weight_buffer));
    assert(NULL != x_weight_buffer);

    LC_DECLARE_ZERO(lc_float_weight_table, y_weights);
    y_weights.weight = (lc_float_sum_t*)calloc(filter_params_y.width, sizeof(*y_weights.weight));
    assert(NULL != y_weights.weight);

    lc_float_sum_t* accum = (lc_float_sum_t*)calloc(dst_width, sizeof(*accum));
    assert(NULL != accum);

    lc_float_sum_t* xWeightPtr = x_weight_buffer;
    for (int bx = 0; bx < dst_width; ++bx, xWeightPtr += filter_params_x.width) {
        x_weights[bx].weight = xWeightPtr;
        lc_float_make_weight_table(bx, LC_MAP(bx, m.sx, m.ux), filter_fn, &filter_args, &filter_params_x, src_width, true, &x_weights[bx]);
    }

    int pixel_stride = channel_count * sizeof(lc_float_data_t);
    for (unsigned int channel = 0; channel < channel_count; ++channel) {
        /* loop over dest scanlines */
        for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
            /* prepare a weight table for dest y position by */
            lc_float_make_weight_table(dst_y, LC_MAP(dst_y, m.sy, m.uy), filter_fn, &filter_args, &filter_params_y, src_height, false, &y_weights);
            memset(accum, 0, sizeof(*accum) * dst_width);
            /* loop over source scanlines that influence this dest scanline */
            for (int ayf = y_weights.start; ayf < y_weights.end; ++ayf) {
                lc_float_sum_t* line = lines_buffer[ayf % filter_params_y.width].second;
                if (lines_buffer[ayf % filter_params_y.width].first != ayf) {
                    lc_float_scanline_filter_channel_to_buffer(x_weights, src_offset_x, src_offset_y + ayf, pixel_stride, src_row_stride, channel, p_src_data, line, dst_width);
                    lines_buffer[ayf % filter_params_y.width].first = ayf;
                }
                lc_float_scanline_accumulate(y_weights.weight[ayf - y_weights.start], line, dst_width, accum);
            }
            lc_float_scanline_shift_accum_to_channel(accum, (int)clipped_dst_area.x1, (int)(clipped_dst_area.y1 + dst_y), dst_width, pixel_stride, dst_row_stride, channel, p_dst_data);
        }
    }

    LC_SAFE_FREE(accum);
    LC_SAFE_FREE(y_weights.weight );
    LC_SAFE_FREE(x_weight_buffer);
    LC_SAFE_FREE(x_weights);
    for(int i = 0; i < filter_params_y.width; i++) {
        LC_SAFE_FREE(lines_buffer[i].second);
    }
    LC_SAFE_FREE(lines_buffer);
}

#endif /* defined(LC_IMAGE_RESIZE_IMPLEMENTATION) */