#ifndef AUDIO_VIZ_H
#define AUDIO_VIZ_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "es7210.h"       // 官方 ES7210 驱动 (来自 06_ES7210 示例)
#include "es8311.h"       // 官方 ES8311 驱动 (来自 07_ES8311 示例)
#include <arduinoFFT.h>

// ============================================================
//  音频可视化模块 (频谱 + 分贝计 + 提示音)
//  采集: ES7210 ADC (双麦克风) 经 I2S RX 读入
//  播放: ES8311 DAC 输出到板载扬声器
//  引脚已按官方 pin_config.h 核实, 驱动基于官方示例源码
// ============================================================

#define I2S_CH            I2S_NUM_1      // ES7210 采集用 I2S1 (与官方示例一致)
#define I2S_PLAY_CH       I2S_NUM_0      // ES8311 播放用 I2S0
#define SAMPLE_RATE       16000          // 采样率 16kHz

// 频谱参数
#define SPECTRUM_FFT_SIZE  128           // FFT 点数 (2 的幂)
#define SPECTRUM_BINS      32            // 显示的柱数 (取低频部分)

class AudioViz
{
public:
    // 初始化 ES7210 采集 + I2S RX (参考官方 08_ES7210.ino)
    bool begin()
    {
        Wire.begin(IIC_SDA, IIC_SCL);
        delay(200);

        // 1. 配置并初始化 ES7210 codec
        audio_hal_codec_config_t cfg = {
            .adc_input  = AUDIO_HAL_ADC_INPUT_ALL,
            .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
            .i2s_iface  = {
                .mode    = AUDIO_HAL_MODE_SLAVE,
                .fmt     = AUDIO_HAL_I2S_NORMAL,
                .samples = AUDIO_HAL_16K_SAMPLES,
                .bits    = AUDIO_HAL_BIT_LENGTH_16BITS,
            },
        };
        esp_err_t ret = es7210_adc_init(&Wire, &cfg);
        if (ret != ESP_OK) { Serial.println("es7210 init failed"); return false; }
        es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
        // 双麦克风增益 (MIC1/MIC2 板载两麦)
        es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2),
                            (es7210_gain_value_t)GAIN_0DB);
        es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);

        // 2. 配置并安装 I2S RX 驱动
        i2s_config_t i2s_cfg = {
            .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate          = SAMPLE_RATE,
            .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format       = I2S_CHANNEL_FMT_ALL_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count        = 8,
            .dma_buf_len          = 64,
            .use_apll             = false,
            .tx_desc_auto_clear   = true,
            .fixed_mclk           = 0,
            .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
            .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,
        };
        i2s_pin_config_t pin_cfg = { 0 };
        pin_cfg.bck_io_num   = PIN_ES7210_BCLK;
        pin_cfg.ws_io_num    = PIN_ES7210_LRCK;
        pin_cfg.data_in_num  = PIN_ES7210_DIN;
        pin_cfg.mck_io_num   = PIN_ES7210_MCLK;
        if (i2s_driver_install(I2S_CH, &i2s_cfg, 0, NULL) != ESP_OK) {
            Serial.println("i2s driver install failed"); return false;
        }
        i2s_set_pin(I2S_CH, &pin_cfg);
        i2s_zero_dma_buffer(I2S_CH);

        _initialized = true;
        return true;
    }

    // 读取一帧音频做 FFT, 输出归一化频谱 bins [0..1]
    void updateSpectrum(float* bins, int binCount)
    {
        if (!_initialized) return;
        if (binCount > SPECTRUM_BINS) binCount = SPECTRUM_BINS;

        // 读满 FFT_SIZE 个采样
        size_t bytesRead = 0;
        i2s_read(I2S_CH, (char*)_raw, SPECTRUM_FFT_SIZE * sizeof(int16_t),
                 &bytesRead, portMAX_DELAY);
        int n = bytesRead / sizeof(int16_t);

        // 填入 FFT 实部, 虚部清零
        for (int i = 0; i < SPECTRUM_FFT_SIZE; i++) {
            _vReal[i] = (i < n) ? (double)_raw[i] : 0.0;
            _vImag[i] = 0.0;
        }

        // FFT
        _fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        _fft.Compute(FFT_FORWARD);
        _fft.ComplexToMagnitude();

        // 取低频 bin, 归一化到 [0..1] 并平滑
        for (int i = 0; i < binCount; i++) {
            double mag = _vReal[i + 1];   // 跳过 DC, 从 bin 1 开始
            float norm = (float)(mag / 20000.0);
            if (norm > 1.0f) norm = 1.0f;
            if (norm < 0.0f) norm = 0.0f;
            bins[i] = bins[i] * 0.6f + norm * 0.4f;  // 低通平滑
        }
    }

    // 计算当前分贝值 (声压级近似)
    float readDecibel()
    {
        if (!_initialized) return 0.0f;
        size_t bytesRead = 0;
        i2s_read(I2S_CH, (char*)_raw, SPECTRUM_FFT_SIZE * sizeof(int16_t),
                 &bytesRead, portMAX_DELAY);
        int n = bytesRead / sizeof(int16_t);

        float sum = 0.0f;
        for (int i = 0; i < n; i++) {
            float s = (float)_raw[i];
            sum += s * s;
        }
        float rms = sqrtf(sum / (n > 0 ? n : 1));

        // RMS 映射到声压级 (16bit 满幅 32768)
        float db = 20.0f * log10f(rms / 32768.0f) + 96.0f;
        if (db < 0) db = 0;
        if (db > 120) db = 120;
        return db;
    }

    // 播放提示音: 用 ES8311 播放固定频率正弦
    void playTone(int freqHz, int durationMs)
    {
        if (!_initialized) return;
        if (!_playbackReady) {
            if (!initPlayback()) return;
        }
        int samples = SAMPLE_RATE * durationMs / 1000;
        for (int i = 0; i < samples; i++) {
            int16_t s = (int16_t)(sinf(2.0f * PI * freqHz * i / SAMPLE_RATE) * 6000);
            size_t wr = 0;
            i2s_write(I2S_PLAY_CH, (const char*)&s, sizeof(s), &wr, portMAX_DELAY);
        }
    }

private:
    // 初始化 ES8311 播放链路 (I2S TX), 参考官方 15_ES8311.ino
    bool initPlayback()
    {
        Wire.begin(IIC_SDA, IIC_SCL);
        pinMode(PIN_PA, OUTPUT);
        digitalWrite(PIN_PA, HIGH);   // 使能功放

        es8311_handle_t es_handle = es8311_create(0, ES8311_ADDRRES_0);
        if (!es_handle) return false;
        const es8311_clock_config_t es_clk = {
            .mclk_inverted    = false,
            .sclk_inverted    = false,
            .mclk_from_mclk_pin = true,
            .mclk_frequency   = SAMPLE_RATE * 256,
            .sample_frequency = SAMPLE_RATE,
        };
        if (es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK) return false;
        es8311_sample_frequency_config(es_handle, es_clk.mclk_frequency, es_clk.sample_frequency);
        es8311_microphone_config(es_handle, false);
        es8311_voice_volume_set(es_handle, 85, NULL);
        es8311_microphone_gain_set(es_handle, (es8311_mic_gain_t)3);

        i2s_config_t tx_cfg = {
            .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate          = SAMPLE_RATE,
            .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count        = 4,
            .dma_buf_len          = 128,
            .use_apll             = false,
            .tx_desc_auto_clear   = true,
            .fixed_mclk           = 0,
            .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
            .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,
        };
        i2s_pin_config_t tx_pin = { 0 };
        tx_pin.bck_io_num   = PIN_ES7210_BCLK;   // BCLK 复用 GPIO9
        tx_pin.ws_io_num    = PIN_ES7210_LRCK;   // LRCK 复用 GPIO45
        tx_pin.data_out_num = PIN_ES8311_DOUT;   // DOUT GPIO8
        tx_pin.mck_io_num   = PIN_ES7210_MCLK;   // MCLK 复用 GPIO16
        if (i2s_driver_install(I2S_PLAY_CH, &tx_cfg, 0, NULL) != ESP_OK) return false;
        i2s_set_pin(I2S_PLAY_CH, &tx_pin);

        _playbackReady = true;
        return true;
    }

    bool _initialized = false;
    bool _playbackReady = false;
    int16_t _raw[SPECTRUM_FFT_SIZE];
    double _vReal[SPECTRUM_FFT_SIZE];
    double _vImag[SPECTRUM_FFT_SIZE];
    arduinoFFT _fft = arduinoFFT(_vReal, _vImag, SPECTRUM_FFT_SIZE, SAMPLE_RATE);
};

#endif // AUDIO_VIZ_H
