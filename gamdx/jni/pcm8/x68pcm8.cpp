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
        mMask = 0;
        mVolume = 256;
        for (int i=0; i<PCM8_NCH; ++i) {
            mPcm8[i].Init(rate);
        }
        
        mSampleRate = rate;
        
        if(bq0) free(bq0);
        bq0 = BiQuad_new(LPF, 0, 15625.0/2.0, rate, 2.0); // 1.0 でよいのかチェック必要
        if(bq1) free(bq1);
        bq1 = BiQuad_new(LPF, 0, 15625.0/2.0, rate, 2.0); // 1.0 でよいのかチェック必要
        // sinn246: LowPassFilter
        // 中心周波数はADPCMが15625Hzなのでそのナイキスト周波数にしてあるが、これだとそれより低い周波数も少しカットされてしまう。
        // とはいえ、実機でもLPFが7.8KHzに設定してあるという情報を鵜呑みにしてこのとおりにしておく
        // 参考：https://kmkz.jp/mtm/?day=20060502　http://cygx.mydns.jp/blog/?arti=219
        
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
    inline void X68PCM8::pcmsetRAW(Sample* buffer, int ndata, float volume) {
        Sample* limit = buffer + ndata * 2;
        for (Sample* dest = buffer; dest < limit; dest+=2) {
            Sample Out0 = 0,Out1 = 0;
            
            for (int ch=0; ch<PCM8_NCH; ++ch) {
                int pan = mPcm8[ch].GetMode();
                int o = mPcm8[ch].GetPcmRAW();
                if (o != 0x80000000) {
                    if(pan&1) Out0 += o;
                    if(pan&2) Out1 += o;
                }
            }
            Out0 =  (Sample) BiQuad(Out0, bq0);
            Out1 =  (Sample) BiQuad(Out1, bq1);
            
            Out0 = (Out0 * mVolume) >> 8;
            Out1 = (Out1 * mVolume) >> 8;
            
            // -2048*16〜+2048*16 OPMとADPCMの音量バランス調整
            dest[0] += Out0 * volume;  //?
            dest[1] += Out1 * volume;  //?
        }
    }
    
    
    
    // ---------------------------------------------------------------------------
    //	合成 (stereo)
    //
    void X68PCM8::Mix(Sample* buffer, int nsamples)
    {
    }

    void X68PCM8::MixRAW(Sample* buffer, int nsamples, float volume)
    {
        pcmsetRAW(buffer, nsamples, volume);
    }

    // ---------------------------------------------------------------------------
}  // namespace X68K

// [EOF]
