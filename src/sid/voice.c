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

void sid_voice_init(sid_voice_t *voice) {
  voice->noiseShiftRegisterTTL = 0;

  voice->muted = 0;
  sid_voice_reset(voice);
}


void sid_voice_reset(sid_voice_t *voice) {

  // wave
  voice->accumulator        = 0;
  voice->accumulatorPrev    = 0;

  // initial value for Fibonacci lfsr 
  // 1 is injected into the XOR feedback mechanism and in some way all the bits end up high.
  voice->noiseShiftRegister = 0x7ffffc;

  
  voice->freq       = 0;
  voice->pw         = 0;
  voice->oscDac     = 0;
  voice->oscDigital = 0;
  voice->waveform       = 0;
  voice->test       = 0;
  voice->ring       = 0;
  voice->sync       = 0;

  voice->envelopeDigital   = 0;
  voice->envelope = 0;
  voice->attack   = 0;
  voice->decay    = 0;
  voice->sustain  = 0;
  voice->release  = 0;

  voice->state = 3;
  voice->gate  = 0;
  voice->freezeZero  = 0;

  voice->expoCounter = 0;
  voice->expoPeriod  = 1;
  voice->rateCounter = 0;
  voice->rateCounterPeriod  = 8;

  sid_writeControl(voice, voice, 0);
}
  
// called when setting the gate or adsr registers
void sid_updatePeriod(sid_voice_t *voice, int32_t value) {
  int32_t newRateCounterPeriod = sid_envelope_rate_periods[value & 0xf];

  if (newRateCounterPeriod == voice->rateCounterPeriod) {
    return;
  }

  voice->rateCounterPeriod = newRateCounterPeriod;

  /* if the new period exceeds 0x7fff, we need to wrap */
  if ((newRateCounterPeriod - voice->rateCounter) > 0x7fff) {
    voice->rateCounter += 0x7fff;
  }

  if (newRateCounterPeriod <= voice->rateCounter) {
    voice->rateCounter -= 0x7fff;
  }

}



// https://en.wikipedia.org/wiki/MOS_Technology_6581
//The noise generator is implemented as a 23-bit-length Fibonacci LFSR 
// (Feedback polynomial: x^22+x^17+1).[10][11] 
// When using noise waveform simultaneously with any other waveform, the pull-down via waveform selector tends to quickly reduce the XOR shift register to 0 for all bits that are connected to the output DAC. As the zeroes shift in the register when the noise is clocked, and no 1-bits are produced to replace them, a situation can arise where the XOR shift register becomes fully zeroed. Luckily, the situation can be remedied by using the waveform control test bit, which in that condition injects one 1-bit into the XOR shift register. Some musicians are also known to use noise's combined waveforms and test bit to construct unusual sounds.

// The noise waveform is generated taking the output of eight selected bits from a 23 bit Fibonacci LFSR. 
// There are actually 24 bits on chip but the last one is unused. 
// The register is clocked when bit 19 of the oscillator goes high and has taps at bits 17 and 22. 
// The register can be "reset" using the test bit: the c1 gate stays open and the output inverters slowly settle high.

// The output from bits 0, 2, 5, 9, 11, 14, 18 and 20 is sent to the waveform selector. 

// Examination of the noise waveform: https://codebase64.org/doku.php?id=base:noise_waveform
void sid_voice_updateNoise(sid_voice_t *voice, bool_t clock) {
  if (clock) {
    int32_t bit0 = ((voice->noiseShiftRegister >> 22) ^ (voice->noiseShiftRegister >> 17)) & 0x1;
    voice->noiseShiftRegister = (voice->noiseShiftRegister << 1) | bit0;
  }

  if (voice->waveform >= 8) {
    if (voice->waveform > 8) {
      // clear output bits of shift register if noise and other waveforms
      // are selected simultaneously, use bitshift and xor to create the mask
      voice->noiseShiftRegister &= 0x7fffff ^ 1 << 22 ^ 1 << 20 ^ 1 << 16 ^ 1 << 13 ^ 1 << 11 ^ 1 << 7 ^ 1 << 4 ^ 1 << 2;
    }

    // get the zero level
    voice->oscDac = sid_wavetable_samples[0][0];

    // the 8 selected bits..
    // The output from bits 0, 2, 5, 9, 11, 14, 18 and 20 is sent to the waveform selector. 
    voice->oscDigital = 
              (uint8_t) ((voice->noiseShiftRegister & 0x400000) >> 15 |
                        (voice->noiseShiftRegister & 0x100000) >> 14 |
                        (voice->noiseShiftRegister & 0x010000) >> 11 |
                        (voice->noiseShiftRegister & 0x002000) >>  9 |
                        (voice->noiseShiftRegister & 0x000800) >>  8 |
                        (voice->noiseShiftRegister & 0x000080) >>  5 |
                        (voice->noiseShiftRegister & 0x000010) >>  3 |
                        (voice->noiseShiftRegister & 0x000004) >>  2);

    // pass it to the dac to get the sample value
    int32_t i;
    for (i = 0; i < 8; i++) {
      if (voice->oscDigital & (1 << i)) {
        voice->oscDac += sid_waveDac[i + 4];
      }
    }

  }
}

void sid_voice_clock(sid_voice_t *voice) {

  // no digital operation if test bit is set
  if (voice->test) {
    if (voice->noiseShiftRegisterTTL && --voice->noiseShiftRegisterTTL == 0) {
      // reset the noise shift register
      // original is 7ffffc
      voice->noiseShiftRegister |= 0x7ffffc;
      sid_voice_updateNoise(voice, false);
    }
  } else {
    
    // store the prev value as 8580 outputs one cycle behind
    // also store to check when bit 19 goes high to clock the noise shift register
    voice->accumulatorPrev = voice->accumulator;

    // accumulator is a 24 bit number
    // Each clock cycle produces a new N-bit output consisting of the previous output obtained from the register summed with the 
    // frequency control word (FCW) which is constant for a given output frequency.
    voice->accumulator = (voice->accumulator + voice->freq) & 0xffffff;

    // BOB YANNES: The shift register was clocked by one of the intermediate bits of the accumulator to keep 
    //             the frequency content of the noise waveform relatively the same as the pitched waveforms. 

    // The noise shift register is clocked when bit 19 of the oscillator goes high 
    if ( (~voice->accumulatorPrev & voice->accumulator & 0x080000) != 0) {
      sid_voice_updateNoise(voice, true);
    }
  }

  sid_voice_envelope_clock(voice);
}




uint8_t sid_updateOsc(sid_voice_t *voice, sid_voice_t *modulator, int32_t accumulator) {
  if (!voice->waveform || voice->waveform >= 8) {
    return voice->oscDigital;
  }

  int32_t phase = accumulator >> 12;
  int32_t index = (voice->waveform >= 4) && (voice->test || (phase >= voice->pw)) ? 3 : -1;

  phase ^= voice->ring && (modulator->accumulator & 0x800000) ? 0x800 : 0;
  index += voice->waveform;

  return sid_wavetable_digital[index][phase];
}



float sid_output(sid_voice_t *voice, sid_voice_t *modulator) {
  if (!voice->waveform || voice->waveform > 7) {
    return voice->oscDac;
  }

  // top 12 bits of accumulator have the phase
  int32_t phase = voice->accumulator >> 12;

  // first part of wave table (0-6) is pulse level 0x0000, 2nd part (7-10) is pulse level 0x1000 (starting at pulse waveform)
  // wave-form 1 is index 0, so start at -1 if not pulse wave or in first half of pulse.
  // with test bit, oscillator is held at dc level
  // if pulse wave and in second half of pulse or with test bit, use 7-10, so start at 3
  int32_t index = (voice->waveform >= 4) && (voice->test || (phase >= voice->pw)) ? 3 : -1;

  // 0x800000 is bit 23, 0x800 is bit 11
  phase ^= voice->ring && (modulator->accumulator & 0x800000) ? 0x800 : 0;

  index += voice->waveform;
  return sid_wavetable_samples[index][phase];
}

// get the digital value from the oscillator
// used for reading the value for register d41b
int32_t sid_osc(sid_voice_t *voice, sid_voice_t *modulator) {

  if (m64_sid.sid_model == 1) {
    // 8580 is one cycle behind
    return sid_updateOsc(voice, modulator, voice->accumulatorPrev);
  } else {
    return sid_updateOsc(voice, modulator, voice->accumulator);
  }
}


// called by register 04 to 'control' 
void sid_writeControl(sid_voice_t *voice, sid_voice_t *modulator, int32_t control) {
  int32_t waveformNext = (control >> 4) & 0xf;

  if (waveformNext == 0 && voice->waveform >= 1 && voice->waveform <= 7) {
    // no waveform set for next,  current wave is set, but not noise
    // call the 6581 version
    voice->oscDigital = sid_updateOsc(voice, modulator, voice->accumulator);
    voice->oscDac = sid_output(voice, modulator);
  }

  // next is upper 4 bits
  voice->waveform = waveformNext;

  // ring modulation only works with triangle  3 = gate + triangle
  voice->ring = ((control & 0x4) != 0) && ((voice->waveform & 3) == 1);
  voice->sync = (control & 0x2) != 0;

  // is test bit set
  bool_t testNext = (control & 0x8) != 0;

  if (testNext && !voice->test) {
    // test bit can be set to reset the noise waveform
    // it will be reset after ttl counts down
    voice->accumulator = 0;
    voice->accumulatorPrev = 0;

    int32_t bit19 = voice->noiseShiftRegister >> 18 & 2;
    voice->noiseShiftRegister = voice->noiseShiftRegister & 0x7ffffd | bit19 ^ 2;

    voice->noiseShiftRegisterTTL = 200000; 

  } else {
    if(!testNext) {
      sid_voice_updateNoise(voice, voice->test);
    }
  }

  voice->test = testNext;
}

