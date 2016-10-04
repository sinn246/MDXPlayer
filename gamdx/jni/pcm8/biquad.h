//
//  biquad.h
//  mdxplayer
//
//  Created by 西村信一 on 2016/10/04.
//  Copyright © 2016年 asada. All rights reserved.
//

#ifndef biquad_h
#define biquad_h

#include <stdio.h>

/* whatever sample type you want */
typedef double smp_type;

/* this holds the data required to update samples thru a filter */
typedef struct {
    smp_type a0, a1, a2, a3, a4;
    smp_type x1, x2, y1, y2;
}
biquad;

extern smp_type BiQuad(smp_type sample, biquad * b);
extern biquad *BiQuad_new(int type, smp_type dbGain, /* gain of filter */
                          smp_type freq,             /* center frequency */
                          smp_type srate,            /* sampling rate */
                          smp_type bandwidth);       /* bandwidth in octaves */

/* filter types */
enum {
    LPF, /* low pass filter */
    HPF, /* High pass filter */
    BPF, /* band pass filter */
    NOTCH, /* Notch Filter */
    PEQ, /* Peaking band EQ filter */
    LSH, /* Low shelf filter */
    HSH /* High shelf filter */
};

#endif /* biquad_h */
