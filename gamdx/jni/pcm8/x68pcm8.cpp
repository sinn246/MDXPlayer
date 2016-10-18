#include "../speex/speex_MDX.h"
#include "x68pcm8.h"


namespace X68K
{
#ifdef USE_SPEEX_FOR_ADPCM_UPSAMPLING
    
#define SPEEX_BUFSIZ 256
// このバッファサイズが大きくなると効率は良くなると思いますが、バッファに貯められているデータの分だけ
// ADPCMの音が遅れます。　256なら44KHzで 256/44100 = 0.006s程度。1024だとちょっとわかってしまう
    
#define SHIFTSIZE 1
// ADPCMデータを16Bitに収めるためにシフトしておく　その量
    
//何故か以下の変数（特に配列）をX86PCM8クラスのメンバ変数にするとLLVMがおかしくなる・・・
//関係ない変数が破壊されます
//ということでローカルのstatic変数にしてあります。2つ以上のインスタンスを使うときには困りますが
    static SpeexResamplerState* Resampler = 0;
    static int Before_rest = 0;
    static int Resample_rest = 0;
    static int16_t* Resample_rest_ptr = 0;
    static int16_t Before_buf[SPEEX_BUFSIZ * 2 + 10];
    static int16_t Resample_buf[SPEEX_BUFSIZ * 2 + 10];
    
#endif

    
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
#ifdef USE_SPEEX_FOR_ADPCM_UPSAMPLING
        mMask = 0;
        mVolume = 256;
        for (int i=0; i<PCM8_NCH; ++i) {
            mPcm8[i].Init(15625);
        }
        int err = 0;
        mSampleRate = rate;
        Resampler = speex_resampler_init(2, 15625, rate, MXDRVG_SPEEX_ADPCM_QUALITY, &err);
        Resample_rest = Before_rest = 0;
 
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
#ifdef USE_SPEEX_FOR_ADPCM_UPSAMPLING

    static void memadd16to32(int32_t* dest, int16_t* src, int len){
        for(int i=0;i<len;i++) *dest++ += ((long)*src++) << SHIFTSIZE;
    }
    
    void X68PCM8::pcmsetRAW(Sample* buffer, int ndata) {
        int32_t* p = buffer;
        int rest = ndata;
        const int bytesPerSample = 2*sizeof(int16_t);
        if(Resample_rest){ //前回の残りをコピー
            if(Resample_rest > ndata){// 残りだけで間に合ってしまった
                memadd16to32(buffer, Resample_rest_ptr, ndata * 2);
                Resample_rest_ptr += ndata*2; //*2は左右２チャンネル分
                Resample_rest -= ndata;
                return;;
            }
            memadd16to32(buffer, Resample_rest_ptr, Resample_rest * 2);
            p += 2*Resample_rest; //*2は左右２チャンネル分
            rest -= Resample_rest;
            Resample_rest = 0;
        }
        
        uint32_t _in, _out;
        while(rest > 0){
            for (int i = Before_rest; i < SPEEX_BUFSIZ; i++) {
                Sample Out0 = 0,Out1 = 0;
                for (int ch=0; ch<PCM8_NCH; ++ch) {
                    int pan = mPcm8[ch].GetMode();
                    Sample o = mPcm8[ch].GetPcmRAW();
                    if (o != 0x80000000) {
                        if(pan&1) Out0 += o;
                        if(pan&2) Out1 += o;
                    }
                }
                Before_buf[i*2] = (Out0 >> SHIFTSIZE); // 不本意ながらOverflowすることありシフトしておく
                Before_buf[i*2+1] = (Out1 >> SHIFTSIZE);
            }
            Resample_rest = 0;
            
            _in = SPEEX_BUFSIZ;
            _out = SPEEX_BUFSIZ;
            speex_resampler_process_interleaved_int(Resampler,
                                                    Before_buf, &_in,
                                                    Resample_buf, &_out);
            Before_rest = SPEEX_BUFSIZ - _in;
            if(Before_rest>0){
                memmove(Before_buf,Before_buf+(SPEEX_BUFSIZ - Before_rest)*2, Before_rest * bytesPerSample);
            }
            
            // _out に実際に変換されたサンプル数が入る
            if(_out > rest){// 作りすぎたので次回に残しておく
                memadd16to32(p, Resample_buf, rest * 2);
                Resample_rest = _out - rest;
                Resample_rest_ptr = Resample_buf + rest*2; // *2 は左右２チャンネル分
                break;
            }
            memadd16to32(p, Resample_buf, _out * 2);
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
