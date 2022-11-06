/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author nopsta 2022
 * 
 *  Based on reSID, a MOS6581 SID emulator engine.
 *  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
 * 
 * See sid/notes for more about the SID chip
 * 
 */


#ifndef SID_H
#define SID_H


#define SID_6581           0
#define SID_8580           1
#define SID_8580_DIGIBOOST 2

#define SID_INPUTDIGIBOOST -0x9500

#define SID_ATTACK        1
#define SID_DECAYSUSTAIN  2
#define SID_RELEASE       3

/* registers */
#define SID_FREQ_LO    0
#define SID_FREQ_HI    1
#define SID_PW_LO      2
#define SID_PW_HI      3
#define SID_CTRL       4
#define SID_ATKDEC     5
#define SID_SUSREL     6

#define SID_FC_LO         21
#define SID_FC_HI         22
#define SID_RES_FILT      23
#define SID_MODE_VOL      24
#define SID_POT_X         25
#define SID_POT_Y         26
#define SID_OSC3RAND      27
#define SID_ENV3          28
#define SID_INV_0         29
#define SID_INV_1         30
#define SID_INV_2         31
#define SID_NUM_REGS      32


/* voice control bits */
#define SID_CTRL_GATE     (1<<0)
#define SID_CTRL_SYNC     (1<<1)
#define SID_CTRL_RINGMOD  (1<<2)
#define SID_CTRL_TEST     (1<<3)
#define SID_CTRL_TRIANGLE (1<<4)
#define SID_CTRL_SAWTOOTH (1<<5)
#define SID_CTRL_PULSE    (1<<6)
#define SID_CTRL_NOISE    (1<<7)

/* filter routing bits */
#define SID_FILTER_FILT1  (1<<0)
#define SID_FILTER_FILT2  (1<<1)
#define SID_FILTER_FILT3  (1<<2)
#define SID_FILTER_FILTEX (1<<3)

/* filter mode bits */
#define SID_FILTER_LP     (1<<0)
#define SID_FILTER_BP     (1<<1)
#define SID_FILTER_HP     (1<<2)
#define SID_FILTER_3OFF   (1<<3)


// there are two buffers
// the sid writes to larger buffer (SIDBUFFER), 
// when program wants sample data, SIDAUDIOBUFFERLENGTH samples are copied to the smaller buffer (SIDAUDIOBUFFER)
// and data in lager buffer is shifted forwards
#define SIDAUDIOBUFFERLENGTHMAX 4096
#define SIDBUFFERLENGTH 32768


extern float sid_waveDac[12];

extern float sid_wavetable_samples[11][4096];
extern uint8_t sid_wavetable_digital[11][4096];

extern uint16_t sid_envelope_rate_periods[16];
extern float sid_envDAC[256]; 

struct sid_voice_s {

  // wave
  // the Oscillator is a 24-bit phase-accumulating design of which the lower 16-bits are programmable for pitch control. 
  // a 16-bit register that controls the oscillator frequency. 
  // This 16-bit value gets added into a 24-bit accumulator on every clock cycle, 
  // and the top bits of the accumulator are used as an index into the current waveform.
  int32_t accumulator;        // 8.16 fixed, top 8 bits are used for index in wavetable
  int32_t accumulatorPrev;

  int32_t noiseShiftRegister;      // 24-bit shift register used for fibbonaci 
  int32_t noiseShiftRegisterTTL;   // counter to count down to reset the shift register, set by test bit

  int32_t freq;
  int32_t pw;
  
  float oscDac;
  uint8_t oscDigital;

  int32_t waveform;
  
  bool_t test;
  bool_t ring;
  bool_t sync;  

  // envelope
  bool_t muted;

  // envelopeDigital is converted to envelope using sid_envDAC
  uint8_t envelopeDigital;
  float envelope;

  int32_t attack;
  int32_t decay;
  int32_t sustain;
  int32_t release;

  int32_t state;
  bool_t gate;
  bool_t freezeZero;

  int32_t expoCounter;
  int32_t expoPeriod;

  int32_t rateCounter;
  int32_t rateCounterPeriod;  


};

typedef struct sid_voice_s sid_voice_t;


struct sid_config_s {
  int32_t sampleRate;
  int32_t m64Frequency;

};

typedef struct sid_config_s sid_config_t;


typedef float (*sid_filterClockFunction)(float v1, float v2, float v3, int32_t inp);

struct sid_s {
  // samples written into sid_buffer
  float sid_buffer[SIDBUFFERLENGTH]; 
  int32_t sid_bufferPos;

  // the last sample generated
  float sid_s_cached;

  float sid_s_offset;

  // sid bus is last value sent to write to a register
  // or gets set to 0xff sometimes on register read
  int32_t sid_bus; // could be uint8_t ??

  // sid_bus is set to zero when busTTL reaches zero
  int64_t sid_busTTL;


  // state of the oscillators
  sid_voice_t sid_voice[3];

  uint8_t sid_filter;

  // channel 1 - 3
  uint8_t sid_filt1;
  uint8_t sid_filt2;
  uint8_t sid_filt3;

  // external
  uint8_t sid_filtE;

  bool_t sid_filterEnabled;

  // last time sid clock was called..
  int64_t sid_lastUpdate;

  // digi muted
  int32_t sid_s_muted;



  int32_t sid_extinp;

  // low pass enabled 
  int32_t sid_f_lp;
  // band pass enabled
  int32_t sid_f_bp;
  // high pass enabled
  int32_t sid_f_hp;

  // filter cutoff frequency
  int32_t sid_f_cut;

  // volume divided by 15
  //int32_t sid_f_vol = 0;
  float sid_f_vol;
  // filter resonance
  float sid_f_res;

  float sid_f_vlp;
  float sid_f_vbp;
  float sid_f_vhp;

  sid_filterClockFunction sid_filterClock;

  // mute voice 3
  int32_t sid_voice3off;


  int32_t sid_model;

  // value used to set the bus ttl
  int32_t sid_modelTTL;

  // output volume offset
  int32_t sid_zero;

};

typedef struct sid_s sid_t;

extern sid_t m64_sid;



unsigned char *m64_getAudioBuffer();

void sid_reset();
void sid_init(int model, float cpuCyclesPerSecond);
void sid_updateConfig(sid_config_t *config);
void m64_setSIDModel(uint32_t model);

void sid_enableFilter(bool_t value);
void sid_input(int32_t value);
void sid_mute(uint32_t voice, bool_t value);

void m64_setFrequency(float clock, float freq);
void sid_setNonlinearity(float nonlinearity);
void sid_update();

void sid_resetFilter();

uint8_t sid_read(uint16_t addr);
void sid_write(uint16_t addr, uint8_t value);
                   
float sid_clock6581(float v1, float v2, float v3, int32_t inp);
float sid_clock8580(float v1, float v2, float v3, int32_t inp);
void sid_recalculate();
void sid_updateCenter();
void sid_updateResonance();

void sid_updatePeriod(sid_voice_t *env, int32_t value);
int32_t sid_osc(sid_voice_t *wave, sid_voice_t *modulator);
float sid_output(sid_voice_t *wave, sid_voice_t *modulator);
void sid_writeControl(sid_voice_t *wave, sid_voice_t *modulator, int32_t control);

uint8_t sid_updateOsc(sid_voice_t *wave, sid_voice_t *modulator, int32_t accum);
float sid_kinkedDac(int32_t inp, float nonlinearity, int32_t bits);


void sid_voice_init(sid_voice_t *waveformGenerator);
void sid_voice_clock(sid_voice_t *wave);
void sid_voice_reset(sid_voice_t *waveformGenerator);

void sid_voice_envelope_clock(sid_voice_t *voice);
void sid_envelope_buildDAC(float nonlinearity);

void sid_voice_updateNoise(sid_voice_t *wave, bool_t clock);

float waveformCalculator_makeSample(float *o, float *dac);
int8_t waveformCalculator_makeDigital(float *o);
void waveformCalculator_populate(int32_t v, float *o);
void waveformCalculator_fill(float *o, uint32_t model, uint32_t w, uint32_t a, uint32_t pw);
void waveformCalculator_build(uint32_t model, float nonlinearity);

#endif