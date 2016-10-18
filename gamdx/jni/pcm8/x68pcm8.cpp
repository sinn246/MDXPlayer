#include "../speex/speex_MDX.h"
#include "x68pcm8.h"


namespace X68K
{
    
    
    // ---------------------------------------------------------------------------
    //	構築
    //
    X68PCM8::X68PCM8()
    {
        bq0 = bq1 = 0;
    }
    
    X68PCM8::~X68PCM8()
    {
        if(bq0) free(bq0);
        if(bq1) free(bq1);
    }
 
    // ---------------------------------------------------------------------------
    //	初期化
    //
    bool X68PCM8::Init(uint rate)
    {
#ifdef USE_SPEEX
        mMask = 0;
        mVolume = 256;
        for (int i=0; i<PCM8_NCH; ++i) {
            mPcm8[i].Init(15625);
        }
        int err = 0;
        mSampleRate = rate;
        _Resampler = speex_resampler_init(2, 15625, rate, MXDRVG_SPEEX_QUALITY, &err);
        _Resample_rest = _Before_rest = 0;
 
#else
        mMask = 0;
        mVolume = 256;
        for (int i=0; i<PCM8_NCH; ++i) {
            mPcm8[i].Init(rate);
        }
        
        mSampleRate = rate;
        
        if(bq0) free(bq0);
        bq0 = BiQuad_newQ(LPF, 15625.0/2.0, rate, 1.0/sqrt(2));
        if(bq1) free(bq1);
        bq1 = BiQuad_newQ(LPF, 15625.0/2.0, rate, 1.0/sqrt(2));
        // sinn246: LowPassFilter
#endif
        return true;
    }
    
    // ---------------------------------------------------------------------------
    //	サンプルレート設定
    //
    bool X68PCM8::SetRate(uint rate)
    {
        mSampleRate = rate;
        
        return true;
    }
    
    // ---------------------------------------------------------------------------
    //	リセット
    //
    void X68PCM8::Reset()
    {
        Init(mSampleRate);
    }
    
    // ---------------------------------------------------------------------------
    //	パラメータセット
    //
    int X68PCM8::Out(int ch, void *adrs, int mode, int len)
    {
        return mPcm8[ch & (PCM8_NCH-1)].Out(adrs, mode, len);
    }
    
    // ---------------------------------------------------------------------------
    //	アボート
    //
    void X68PCM8::Abort()
    {
        Reset();
    }
    
    // ---------------------------------------------------------------------------
    //	チャンネルマスクの設定
    //
    void X68PCM8::SetChannelMask(uint mask)
    {
        mMask = mask;
    }
    
    // ---------------------------------------------------------------------------
    //	音量設定
    //
    void X68PCM8::SetVolume(int db)
    {
        db = Min(db, 20);
        if (db > -192)
            mVolume = int(16384.0 * pow(10, db / 40.0));
        else
            mVolume = 0;
    }
    
    
    // ---------------------------------------------------------------------------
    //	ADPCM合成処理(RAW)
    //
#ifdef USE_SPEEX
#define SHIFTSIZE 4
    static void memcpy16to32(int32_t* dest, int16_t* src, int len){
        for(int i=0;i<len;i++) *dest++ = *src++ << SHIFTSIZE;
    }
    
    void X68PCM8::pcmsetRAW(Sample* buffer, int ndata) {
        int32_t* p = buffer;
        int rest = ndata;
        const int bytesPerSample = 2*sizeof(int16_t);
        if(_Resample_rest){ //前回の残りをコピー
            if(_Resample_rest > ndata){// 残りだけで間に合ってしまった
                memcpy16to32(buffer, _Resample_rest_ptr, ndata * 2);
                _Resample_rest_ptr += ndata*2; //*2は左右２チャンネル分
                _Resample_rest -= ndata;
                return;;
            }
            memcpy16to32(buffer, _Resample_rest_ptr, _Resample_rest * 2);
            p += 2*_Resample_rest; //*2は左右２チャンネル分
            rest -= _Resample_rest;
            _Resample_rest = 0;
        }
        
        uint32_t _in, _out;
        while(rest > 0){
            int16_t* src = _Before_buf+ _Before_rest*2;
            for (int i = _Before_rest; i < 1024; i++) {
                Sample Out0 = 0,Out1 = 0;
                for (int ch=0; ch<PCM8_NCH; ++ch) {
                    int pan = 0;//mPcm8[ch].GetMode();
                    Sample o = 0;//mPcm8[ch].GetPcmRAW();
                    if (o != 0x80000000) {
                        if(pan&1) Out0 += o;
                        if(pan&2) Out1 += o;
                    }
                }
                *src++ = Out0 >> SHIFTSIZE; // 不本意ながらOverflowすることありシフトしておく
                *src++ = Out1 >> SHIFTSIZE;
            }
            _Resample_rest = 0;
            
            _in = 1024;
            _out = 1024;
            speex_resampler_process_interleaved_int(_Resampler,
                                                    _Before_buf, &_in,
                                                    _Resample_buf, &_out);
            _Before_rest = 1024 - _in;
            if(_Before_rest>0){
                memcpy(_Before_buf,_Before_buf+(1024 - _Before_rest)*2, _Before_rest * bytesPerSample);
            }
            
            // _out に実際に変換されたサンプル数が入る
            if(_out > rest){// 作りすぎたので次回に残しておく
                memcpy16to32(p, _Resample_buf, rest * 2);
                _Resample_rest = _out - rest;
                _Resample_rest_ptr = _Resample_buf + rest*2; // *2 は左右２チャンネル分
                break;
            }
            memcpy16to32(p, _Resample_buf, _out * 2);
            p+=_out*2;
            rest -= _out;
        }
    }
#else
    void X68PCM8::pcmsetRAW(Sample* buffer, int ndata) {
        Sample* limit = buffer + ndata * 2;
        for (Sample* dest = buffer; dest < limit; dest+=2) {
            Sample Out0 = 0,Out1 = 0;
            for (int ch=0; ch<PCM8_NCH; ++ch) {
                int pan = mPcm8[ch].GetMode();
                Sample o = mPcm8[ch].GetPcmRAW();
                if (o != 0x80000000) {
                    if(pan&1) Out0 += o;
                    if(pan&2) Out1 += o;
                }
            }
            Out0 =  (Sample) BiQuad(Out0, bq0);
            Out1 =  (Sample) BiQuad(Out1, bq1);
            
            dest[0] += (Out0 * mVolume) >> 8;
            dest[1] += (Out1 * mVolume) >> 8;
            
            // -2048*16〜+2048*16 OPMとADPCMの音量バランス調整
        }
    }
#endif
    
    
    
    // ---------------------------------------------------------------------------
    //	合成 (stereo)
    //
    void X68PCM8::Mix(Sample* buffer, int nsamples)
    {
    }

    void X68PCM8::MixRAW(Sample* buffer, int nsamples)
    {
        pcmsetRAW(buffer, nsamples);
    }

    // ---------------------------------------------------------------------------
}  // namespace X68K

// [EOF]
