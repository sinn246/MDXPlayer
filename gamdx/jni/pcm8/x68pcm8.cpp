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
        
        OutInpAdpcm[0] = OutInpAdpcm[1] =
        OutInpAdpcm_prev[0] = OutInpAdpcm_prev[1] =
        OutInpAdpcm_prev2[0] = OutInpAdpcm_prev2[1] =
        OutOutAdpcm[0] = OutOutAdpcm[1] =
        OutOutAdpcm_prev[0] = OutOutAdpcm_prev[1] =
        OutOutAdpcm_prev2[0] = OutOutAdpcm_prev2[1] =
        0;
        OutInpOutAdpcm[0] = OutInpOutAdpcm[1] =
        OutInpOutAdpcm_prev[0] = OutInpOutAdpcm_prev[1] =
        OutInpOutAdpcm_prev2[0] = OutInpOutAdpcm_prev2[1] =
        OutOutInpAdpcm[0] = OutOutInpAdpcm[1] =
        OutOutInpAdpcm_prev[0] = OutOutInpAdpcm_prev[1] =
        0;
        
        mSampleRate = rate;
        
        if(bq0) free(bq0);
        bq0 = BiQuad_new(LPF, 0, 15625.0/2.0, rate, 1.0); // 1.0 でよいのかチェック必要
        if(bq1) free(bq1);
        bq1 = BiQuad_new(LPF, 0, 15625.0/2.0, rate, 1.0); // 1.0 でよいのかチェック必要
        
#if 0
        SetRate(rate);
        AudioStreamBasicDescription inASBD;
        inASBD.mSampleRate         = 15625;
        inASBD.mFormatID           = kAudioFormatLinearPCM;
        inASBD.mFormatFlags        = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        inASBD.mFramesPerPacket    = 1;
        inASBD.mChannelsPerFrame   = 2;
        inASBD.mBitsPerChannel     = 16;
        inASBD.mBytesPerPacket     = 4;
        inASBD.mBytesPerFrame      = 4;
        inASBD.mReserved           = 0;

        AudioStreamBasicDescription outASBD;
        outASBD.mSampleRate         = rate;
        outASBD.mFormatID           = kAudioFormatLinearPCM;
        outASBD.mFormatFlags        = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        outASBD.mFramesPerPacket    = 1;
        outASBD.mChannelsPerFrame   = 2;
        outASBD.mBitsPerChannel     = 16;
        outASBD.mBytesPerPacket     = 4;
        outASBD.mBytesPerFrame      = 4;
        outASBD.mReserved           = 0;
        
        AudioConverterNew(&inASBD, &outASBD, &ACRef);
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
    inline void X68PCM8::pcmsetRAW(Sample* buffer, int ndata) {
        Sample* limit = buffer + ndata * 2;
        for (Sample* dest = buffer; dest < limit; dest+=2) {
            OutInpAdpcm[0] = OutInpAdpcm[1] = 0;
            
            for (int ch=0; ch<PCM8_NCH; ++ch) {
                int pan = mPcm8[ch].GetMode();
                int o = mPcm8[ch].GetPcmRAW();
                if (o != 0x80000000) {
                    OutInpAdpcm[0] += (-(pan&1)) & o;
                    OutInpAdpcm[1] += (-((pan>>1)&1)) & o;
                }
            }
            OutInpAdpcm[0] =  (typeof(OutInpAdpcm[0])) BiQuad(OutInpAdpcm[0], bq0);
            OutInpAdpcm[1] =  (typeof(OutInpAdpcm[1])) BiQuad(OutInpAdpcm[1], bq1);
            
            OutInpAdpcm[0] = (OutInpAdpcm[0] * mVolume) >> 8;
            OutInpAdpcm[1] = (OutInpAdpcm[1] * mVolume) >> 8;
            
            // -2048*16〜+2048*16 OPMとADPCMの音量バランス調整
            StoreSample(dest[0], (OutInpAdpcm[0]*16));  //?
            StoreSample(dest[1], (OutInpAdpcm[1]*16));  //?
        }
    }
    
    
    
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
