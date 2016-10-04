#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <AudioToolbox/AudioToolbox.h>

#include "../types.h"
#include "global.h"
#include "pcm8.h"
#include "biquad.h"

namespace X68K
{
	typedef MXDRVG_SAMPLETYPE Sample;
	typedef int32 ISample;

	class X68PCM8
	{
	public:
		X68PCM8();
		~X68PCM8() {}

		bool Init(uint rate);
		bool SetRate(uint rate);
		void Reset();

		int Out(int ch, void *adrs, int mode, int len);
		void Abort();

		void Mix(Sample *buffer, int nsamples);
        void MixRAW(Sample *buffer, int nsamples);
        void SetVolume(int db);
		void SetChannelMask(uint mask);

	private:
		Pcm8 mPcm8[PCM8_NCH];
		uint mMask;
		int mVolume;
		int mSampleRate;
        biquad* bq0;
        biquad* bq1;
//        AudioConverterRef ACRef;

		sint32 OutInpAdpcm[2];
		sint32 OutInpAdpcm_prev[2];
		sint32 OutInpAdpcm_prev2[2];
		sint32 OutOutAdpcm[2];
		sint32 OutOutAdpcm_prev[2];
		sint32 OutOutAdpcm_prev2[2];  // 高音フィルター２用バッファ

		sint32 OutInpOutAdpcm[2];
		sint32 OutInpOutAdpcm_prev[2];
		sint32 OutInpOutAdpcm_prev2[2];
		sint32 OutOutInpAdpcm[2];
		sint32 OutOutInpAdpcm_prev[2];  // 高音フィルター３用バッファ

        inline void pcmsetRAW(Sample* buffer, int ndata);
	};

	inline int Max(int x, int y) { return (x > y) ? x : y; }
	inline int Min(int x, int y) { return (x < y) ? x : y; }
	inline int Abs(int x) { return x >= 0 ? x : -x; }

	inline int Limit(int v, int max, int min) 
	{ 
		return v > max ? max : (v < min ? min : v); 
	}

	inline void StoreSample(Sample& dest, ISample data)
	{
		if (sizeof(Sample) == 2)
			dest = (Sample) Limit(dest + data, 0x7fff, -0x8000);
		else
			dest += data;
	}

}

// [EOF]
