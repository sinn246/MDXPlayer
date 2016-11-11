//
//  speex_MDX.h
//  
//
//  Created by 西村 信一 on 2016/10/08.
//
//

#ifndef speex_MDX_h
#define speex_MDX_h

//#define USE_SPEEX
 // includes speex/resample.c
#define FIXED_POINT
 //use 16-bit int

#ifdef USE_SPEEX
#define USE_SPEEX_FOR_ADPCM_UPSAMPLING
 // for x68pcm8
//#define USE_SPEEX_FOR_DOWNSAMPLING
 // for mxdrvg_core

#define MXDRVG_SPEEX_ADPCM_QUALITY 0
#define MXDRVG_SPEEX_DOWNSAMPLING_QUALITY 0
 // resampler quality 0-10
#endif

//#define _USE_NEON
// won't work...
// ARM64対応ではないみたい

#endif /* speex_MDX_h */
