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


#include "../m64.h"
#include <math.h>


#define SID_OUTPUTLEVEL 0.01

//  waveforms are generated digitally and then converted to an analog signal through a 12 bit R–2R Ladder.
// https://en.wikipedia.org/wiki/Resistor_ladder
// In the MOS 6581 the DACs are far from perfect, unbalanced resistors and missing terminator, giving a non-linear conversion while in the 8580 the quality has been improved.
float sid_waveDac[12];



/*
  on a standard C=64 motherboard, there is an RC lowpass filter followed by a BJT based common collector acting as a voltage follower.
  The lowpass filter provides a 3dB cutoff at 16KHz, while the DC-Blocker capacitor acts as a high-pass filter with a cutoff dependent 
  on the attached audio equipment impedance.
  https://sourceforge.net/p/sidplay-residfp/wiki/SID%20internals%20-%20External%20filter/

  Low-pass: R = 10kOhm, C = 1000pF; w0l = 1/RC = 1/(1e4*1e-9) = 100000
  High-pass: R = 1kOhm, C = 10uF; w0h = 1/RC = 1/(1e3*1e-5) = 100

  https://en.wikipedia.org/wiki/Low-pass_filter
  https://en.wikipedia.org/wiki/High-pass_filter
*/

// external highpass cutoff freq
float sid_externalHighPassFilter_w0 = 0;
// external lowpass cutoff freq
float sid_externalLowPassFilter_w0 = 0;

// external highpass voltage
float sid_externalHighPassFilter_v = 0;

// external lowpass voltage
float sid_externalLowPassFilter_v = 0;

uint32_t SIDAUDIOBUFFERLENGTH = 4096;

// the output audio buffer, m64_getAudioBuffer will copy samples into this buffer
float sid_audioBuffer[SIDAUDIOBUFFERLENGTHMAX];

// number of cycles per sample * 1024  (m64 freq/sample freq * 1024)
float sid_cycles    = 0;
float sid_cpuCyclesPerSecond = 0;

// samples per second for audio output
// this value should be overridden on setup
float sid_samplesPerSecond = 48000;

sid_t m64_sid;
uint32_t sidCount = 1;


void sid_init(int model, float cpuCyclesPerSecond) {

  // init to -1 so setModel will detect the change
  m64_sid.sid_model = -1;

  sid_enableFilter(true);


  sid_mute(0, false);
  sid_mute(1, false);
  sid_mute(2, false);
  sid_mute(3, false);

  m64_setSIDModel(model);
  sid_reset();

  sid_cpuCyclesPerSecond = cpuCyclesPerSecond;

/*
		w0hp = (float) (100 / frequency);
		w0lp = (float) (LOWPASS_FREQUENCY * 2.f * Math.PI / frequency);
    for low pass, fcutoff = 1 / (2 * PI * RC)

    Low-pass: R = 10kOhm, C = 1000pF; w0l = 1/RC = 1/(1e4*1e-9) = 100000
    High-pass: R = 1kOhm, C = 10uF; w0h = 1/RC = 1/(1e3*1e-5) = 100

*/
  sid_externalHighPassFilter_w0 = 100 / sid_cpuCyclesPerSecond;
  sid_externalLowPassFilter_w0 = 100000 / sid_cpuCyclesPerSecond; //1000 * sid_externalHighPassFilter_w0;

  sid_recalculate();
  sid_updateCenter();

}
  
void sid_reset() {
  m64_sid.sid_bus    = 0;
  m64_sid.sid_busTTL = 0;

  // external output
  sid_externalHighPassFilter_v = 0;
  sid_externalLowPassFilter_v = 0;

  int32_t i;
  for(i = 0; i < SIDBUFFERLENGTH; i++) {
    m64_sid.sid_buffer[i] = 0;
  }
  m64_sid.sid_bufferPos = 0;
  m64_sid.sid_s_cached  = 0;
  m64_sid.sid_s_offset  = 0;

  
  m64_sid.sid_lastUpdate = clock_getTime(&m64_clock, 1);

  sid_voice_reset(&(m64_sid.sid_voice[0]));
  sid_voice_reset(&(m64_sid.sid_voice[1]));
  sid_voice_reset(&(m64_sid.sid_voice[2]));


  m64_sid.sid_f_bp  = 0;
  m64_sid.sid_f_hp  = 0;
  m64_sid.sid_f_lp  = 0;

  m64_sid.sid_f_vlp = 0;
  m64_sid.sid_f_vbp = 0;
  m64_sid.sid_f_vhp = 0;

  m64_sid.sid_f_cut = 0;
  m64_sid.sid_f_res = 0;
  m64_sid.sid_f_vol = 0;

  m64_sid.sid_voice3off = 1;

  sid_updateCenter();
  sid_updateResonance();

}

void m64_setSampleRate(int32_t samplesPerSecond) {
  uint32_t i;

  sid_samplesPerSecond = samplesPerSecond;

  // sid_cycles used in zero order resampler
  sid_cycles = ((sid_cpuCyclesPerSecond / sid_samplesPerSecond) * 1024);

//  m64_setFrequency(sid_cpuCyclesPerSecond, samplesPerSecond);

  m64_sid.sid_bufferPos = 0;
  for(i = 0; i < SIDBUFFERLENGTH; i++) {
    m64_sid.sid_buffer[i] = 0;
  }
  m64_sid.sid_s_cached  = 0;
  m64_sid.sid_s_offset  = 0;

}
  
void sid_enableFilter(bool_t value) {
  if (value == m64_sid.sid_filterEnabled) { 
    return; 
  }
  
  m64_sid.sid_filterEnabled = value;
  
  if (value)  {
    m64_sid.sid_f_res = (m64_sid.sid_filter >> 4) & 15;
    sid_updateResonance();

    m64_sid.sid_filt1 = m64_sid.sid_filter & 1;
    m64_sid.sid_filt2 = m64_sid.sid_filter & 2;
    m64_sid.sid_filt3 = m64_sid.sid_filter & 4;
    m64_sid.sid_filtE = m64_sid.sid_filter & 8;
  } else {
    m64_sid.sid_filt1 = 0;
    m64_sid.sid_filt2 = 0;
    m64_sid.sid_filt3 = 0;
    m64_sid.sid_filtE = 0;
  }
}
  
void sid_input(int32_t value) {
  m64_sid.sid_extinp = (value << 4) * 3;
}

void sid_mute(uint32_t voiceIndex, bool_t value) {
  if (voiceIndex < 3) {
    m64_sid.sid_voice[voiceIndex].muted = value;
  } else {
    m64_sid.sid_s_muted = value;
  }
}
  
void m64_setSIDModel(uint32_t model) {

  if (model == m64_sid.sid_model) { 
    return; 
  }

  if(model == SID_8580_DIGIBOOST) {
    model = SID_8580;
    sid_input(SID_INPUTDIGIBOOST);
  } else {
    sid_input(0);
  }

  if (model == SID_8580) {
    m64_sid.sid_model    = SID_8580;
    m64_sid.sid_modelTTL = 663552;
    m64_sid.sid_zero     = -65280;
    m64_sid.sid_filterClock = &sid_clock8580;
  } else {
    m64_sid.sid_model    = SID_6581;
    m64_sid.sid_modelTTL = 7424;
    m64_sid.sid_zero     = 522240;
    m64_sid.sid_filterClock = &sid_clock6581;
  }

  sid_resetFilter();
}

   
void sid_setNonlinearity(float nonlinearity) {

  waveformCalculator_build(m64_sid.sid_model, nonlinearity);

  sid_envelope_buildDAC(nonlinearity);

}
  

// run the sid for a certain number of cycles
void sid_clock(uint64_t cycles) {

  float output, externalFilterOutput, v1, v2, v3;
  
  float *sampleBuffer = m64_sid.sid_buffer;

  int32_t sampleBufferPos = m64_sid.sid_bufferPos;

  sid_voice_t *voice0 = &(m64_sid.sid_voice[0]);
  sid_voice_t *voice1 = &(m64_sid.sid_voice[1]);
  sid_voice_t *voice2 = &(m64_sid.sid_voice[2]);

  int32_t i;
  for (i = 0; i < cycles; i++) {

    sid_voice_clock(voice0);
    sid_voice_clock(voice1);
    sid_voice_clock(voice2);

    // sync the voices
    if (voice1->sync 
        && (~voice0->accumulatorPrev & voice0->accumulator & 0x800000) 
        && !(voice0->sync && (~voice2->accumulatorPrev & voice2->accumulator & 0x800000))) { 
      voice1->accumulator = 0; 
    }

    if (voice2->sync 
        && (~voice1->accumulatorPrev & voice1->accumulator & 0x800000) 
        && !(voice1->sync && (~voice0->accumulatorPrev & voice0->accumulator & 0x800000))) { 
      voice2->accumulator = 0; 
    }

    if (voice0->sync 
        && (~voice2->accumulatorPrev & voice2->accumulator & 0x800000) 
        && !(voice2->sync && (~voice1->accumulatorPrev & voice1->accumulator & 0x800000))) { 
      voice0->accumulator = 0; 
    }


    // get output from each of the voices
    v1 = (sid_output(voice0, voice2) * voice0->envelope) + m64_sid.sid_zero;

    v2 = (sid_output(voice1, voice0) * voice1->envelope) + m64_sid.sid_zero;

    v3 = (sid_output(voice2, voice1) * voice2->envelope) + m64_sid.sid_zero;

    // send it through the filter
    output = m64_sid.sid_filterClock(v1, v2, v3, m64_sid.sid_extinp);


    /*
    on a standard C=64 motherboard, there is an RC lowpass filter followed by a BJT based common collector acting as a voltage follower.
    The lowpass filter provides a 3dB cutoff at 16KHz, while the DC-Blocker capacitor acts as a high-pass filter with a cutoff dependent 
    on the attached audio equipment impedance.
    https://sourceforge.net/p/sidplay-residfp/wiki/SID%20internals%20-%20External%20filter/

		final float out = Vlp - Vhp;
		Vhp += w0hp * out;
		Vlp += w0lp * (Vi - Vlp);

    */
    externalFilterOutput = sid_externalLowPassFilter_v - sid_externalHighPassFilter_v;
    sid_externalHighPassFilter_v += (sid_externalHighPassFilter_w0 * externalFilterOutput);
    sid_externalLowPassFilter_v += (sid_externalLowPassFilter_w0 * (output - sid_externalLowPassFilter_v));

    // scale it by the output level
    externalFilterOutput *= SID_OUTPUTLEVEL; 

    // zero order resampler
    // need to resample from the cpu clock freq to the output sample freq
    if (m64_sid.sid_s_offset < 1024) {
      // enters here every (sid_cycles / 1024) cycles

      if(sampleBufferPos >= SIDBUFFERLENGTH) {
        // uh oh, wrap around, maybe m64_getAudioBuffer isnt being called
        // prob should shift backwards by some amount rather than wrap around
        sampleBufferPos = 0;
        m64_sid.sid_s_cached  = 0;
        m64_sid.sid_s_offset  = 0;        
      }
      // last sample plus difference between this and last sample multiply by offset divide by 1024
      // >> 10 is divide by 1024
      sampleBuffer[sampleBufferPos++] = m64_sid.sid_s_cached + ( (int32_t)( m64_sid.sid_s_offset * (externalFilterOutput - m64_sid.sid_s_cached)) >> 10);
      m64_sid.sid_s_offset += sid_cycles;
    }

    m64_sid.sid_s_offset -= 1024;
    m64_sid.sid_s_cached = externalFilterOutput;
  }

  m64_sid.sid_bufferPos = sampleBufferPos;
}


// run the sid for the number of cycles since the last run
void sid_update() {

  // get number of cycles since sid_clock last called
  uint64_t time = clock_getTime(&m64_clock, PHASE_PHI2);
  int32_t deltaCycles = (int32_t)(time - m64_sid.sid_lastUpdate);

  m64_sid.sid_lastUpdate = time;

  if(deltaCycles == 0) {
    return;
  }

  if (m64_sid.sid_busTTL) {
    m64_sid.sid_busTTL -= deltaCycles;

    if (m64_sid.sid_busTTL <= 0) {
      m64_sid.sid_bus = 0;
      m64_sid.sid_busTTL = 0;
    }
  }

  sid_clock(deltaCycles);
}


// should be able to set audio buffer size?
uint32_t m64_getAudioBufferLength() {
  return SIDAUDIOBUFFERLENGTH;
}



// set the buffer length, sample rate, reset buffer position to 0
void m64_audioInit(uint32_t bufferLength, uint32_t sampleRate) {
  if(bufferLength >= 8192) {
    SIDAUDIOBUFFERLENGTH = 8192;
  } else if(bufferLength >= 4096) {
    SIDAUDIOBUFFERLENGTH = 4096;
  } else if(bufferLength >= 2048) {
    SIDAUDIOBUFFERLENGTH = 2048;
  } else if(bufferLength >= 1024) {
    SIDAUDIOBUFFERLENGTH = 1024;
  } else {
    SIDAUDIOBUFFERLENGTH = 512;
  }

  m64_setSampleRate(sampleRate);
}

int32_t m64_getAudioSamplesAvailable() {
  return m64_sid.sid_bufferPos;
}


unsigned char *m64_getAudioBuffer() {
  uint32_t i = 0;

  while(m64_sid.sid_bufferPos < SIDAUDIOBUFFERLENGTH) {
    // dont have enough samples, step the clock until buffer is full
    clock_step(&m64_clock);
    sid_update();
    i++;
    if(i > 1000000) {
      // make sure not getting out of hand
      break;
    }    
  }

  for (i = 0; i < SIDAUDIOBUFFERLENGTH; i++) {
    sid_audioBuffer[i] = m64_sid.sid_buffer[i] * 0.00006;//0.000030517578125;
  }

  // shift data back by SIDAUDIOBUFFERLENGTH, if there are more than SIDAUDIOBUFFERLENGTH in the buffer
  float *buf = m64_sid.sid_buffer;
  int32_t pos = m64_sid.sid_bufferPos;
  if (pos > SIDAUDIOBUFFERLENGTH) {
    // output of audio is lagging calls to get buffer
    memcpy(buf, &(buf[SIDAUDIOBUFFERLENGTH]), sizeof(float) * (pos - SIDAUDIOBUFFERLENGTH));

    pos -= SIDAUDIOBUFFERLENGTH;
  } else {
    pos = 0;
  }

  m64_sid.sid_bufferPos = pos; 

  return (unsigned char *)sid_audioBuffer;
}

uint8_t sid_read(uint16_t addr) {
  // sync sid and cpu clocks
  sid_update();

  switch (addr & 0x1f) {
    // paddle x and y
    case SID_POT_X: //0x19
    case SID_POT_Y: //0x1a
      m64_sid.sid_bus = 0xff;
      m64_sid.sid_busTTL = m64_sid.sid_modelTTL;
      break;
    case SID_OSC3RAND: //0x1b
      // reading output of oscillator voice 3
      m64_sid.sid_bus = sid_osc(&(m64_sid.sid_voice[2]), &(m64_sid.sid_voice[0]));
      m64_sid.sid_busTTL = m64_sid.sid_modelTTL;
      break;
    case SID_ENV3: // 0x1c
      m64_sid.sid_bus = m64_sid.sid_voice[2].envelopeDigital;
      m64_sid.sid_busTTL = m64_sid.sid_modelTTL;
      break;
    default:
      m64_sid.sid_busTTL >>= 1;
      break;
  }
  return m64_sid.sid_bus;
}

void sid_write(uint16_t address, uint8_t value) {
  // sync sid and cpu clocks
  sid_update();

  // is bus shared by all sids?
  m64_sid.sid_bus = value;
  m64_sid.sid_busTTL = m64_sid.sid_modelTTL;

  int32_t reg = address & 0x1f;

  int32_t mod = 1;
  sid_voice_t *voice = m64_sid.sid_voice;

  if (reg >= 7 && reg <= 13) {
    // voice 2
    reg -= 7;

    mod = 2;
    voice = &(m64_sid.sid_voice[1]);
  } else if (reg >= 14 && reg <= 20) {
    // voice 3
    reg -= 14;

    mod = 0;
    voice = &(m64_sid.sid_voice[2]);
  }

  switch (reg) {
    // frequency low byte
    case SID_FREQ_LO:  // 0x00
      voice->freq = (voice->freq & 0xff00) | (value & 0xff);
      break;
    case SID_FREQ_HI: // 0x01
      // frequency high byte
      voice->freq = ((value << 8) & 0xff00) | (voice->freq & 0xff);
      break;
    case SID_PW_LO: // 0x02
      // pulse wave duty cycle low byte
      voice->pw = (voice->pw & 0x0f00) | (value & 0xff);
      break;
    case SID_PW_HI: // 0x03
      voice->pw = ((value << 8) & 0x0f00) | (voice->pw & 0xff);
      break;
    case SID_CTRL: // 0x04
      // control register voice 
      sid_writeControl(voice, &( m64_sid.sid_voice[mod]) , value);

      // update envelope
      bool_t gateNext = (value & 0x1) != 0;

      if (!voice->gate && gateNext) {
        // gate switching from off to on
        voice->state = SID_ATTACK;
        sid_updatePeriod(voice, voice->attack);

        voice->freezeZero = 0;
      } else if (voice->gate && !gateNext) {
        // gate switching from on to off
        voice->state = SID_RELEASE;
        sid_updatePeriod(voice, voice->release); 
      }

      voice->gate = gateNext;
      break;
    case SID_ATKDEC: // 0x05
      // set attack/decay
      voice->attack = (value >> 4) & 0xf;
      voice->decay = value & 0xf;

      if (voice->state == SID_ATTACK) {
        sid_updatePeriod(voice, voice->attack);
      } else if (voice->state == SID_DECAYSUSTAIN) {
        sid_updatePeriod(voice, voice->decay);
      }
      break;
    case SID_SUSREL: // 0x06
      // set sustain/release
      voice->sustain = (value >> 4) & 0xf;
      voice->release = value & 0xf;

      if (voice->state == SID_RELEASE) {
        sid_updatePeriod(voice, voice->release);
      }
      break;


    // --- end voice specific registers  
    case SID_FC_LO: // 0x15
      // filter cutoff frequency low byte
      m64_sid.sid_f_cut = (m64_sid.sid_f_cut & 2040) | (value & 7);
      sid_updateCenter();
      break;
    case SID_FC_HI: // 0x16
      // filter cutoff frequency high byte
      m64_sid.sid_f_cut = ((value << 3) & 2040) | (m64_sid.sid_f_cut & 7);
      sid_updateCenter();
      break;
    case SID_RES_FILT: // 0x17
      m64_sid.sid_filter = value;
      m64_sid.sid_f_res = (value >> 4) & 15;
      sid_updateResonance();

      if (m64_sid.sid_filterEnabled) {
        m64_sid.sid_filt1 = value & 1;
        m64_sid.sid_filt2 = value & 2;
        m64_sid.sid_filt3 = value & 4;
        m64_sid.sid_filtE = value & 8;
      }
      break;
    case SID_MODE_VOL:  // 0x18:
    {
      // filter mode and volume
      int32_t vol = (value & 15);

      // s_mute is to mute digi
      if (!m64_sid.sid_s_muted || ( (float)vol / 15.0) >= m64_sid.sid_f_vol) {
        m64_sid.sid_f_vol = (float)vol / 15.0;

        m64_sid.sid_f_lp = value & 0x10;
        m64_sid.sid_f_bp = value & 0x20;
        m64_sid.sid_f_hp = value & 0x40;

        m64_sid.sid_voice3off = !(value & 0x80);
        }
      }
      break;
  }

}


/**
 * from jsidplay:
 * Estimate DAC nonlinearity. The SID contains R-2R ladder, and some likely
 * errors in the resistor lengths which result in errors depending on the bits
 * chosen.
 * <P>
 * This model was derived by Dag Lem, and is port of the upcoming reSID version.
 * In average, it shows a value higher than the target by a value that depends
 * on the _2R_div_R parameter. It differs from the version written by Antti
 * Lankila chiefly in the emulation of the lacking termination of the 2R ladder,
 * which destroys the output with respect to the low bits of the DAC.
 * <P>
 * Returns the analog value as modeled from the R-2R network.
 *
 * @param dac       digital value to convert to analog
 * @param _2R_div_R nonlinearity parameter, 1.0 for perfect linearity.
 * @param term      is the dac terminated by a 2R resistor? (6581 DACs are not)
 */
float sid_kinkedDac(int32_t inp, float nonlinearity, int32_t bits) {
  float value = 0.0;
  int32_t currentBit = 1;
  float weight = 1.0;
  float dir = 2.0 * nonlinearity;

  // uncomment to turn non linearity off
	//  nonlinearity = 1;

  int32_t i;

  for (i = 0; i < bits; i++) {
    if (inp & currentBit) { 
      value += weight; 
    }

    currentBit <<= 1;
    weight *= dir;
  }

  return (value / (weight / (nonlinearity * nonlinearity))) * (1 << bits);
}

