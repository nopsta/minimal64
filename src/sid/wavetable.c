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
 * and https://bel.fi/alankila/c64-sw/combined-waveforms/
 * 
 * See sid/notes for more about the SID chip
 * 
 */
#include "../m64.h"

// digital versions of the waves
// digital version is only used when reading register d41b for oscillator 3
uint8_t sid_wavetable_digital[11][4096];

// waves converted to samples by the wave dac, each value in wavedac is multiplied by wave amount and summed to make the sample
float sid_wavetable_samples[11][4096];


float waveformCalculatorConfig[2][5][5] = 
{
  {
    {0.880815,  0,        0,        0.3279614,  0.5999545},
    {0.8924618, 2.014781, 1.003332, 0.02992322, 0},
    {0.8646501, 1.712586, 1.137704, 0.02845423, 0},
    {0.9527834, 1.794777, 0,        0.09806272, 0.7752482},
    {0.5,       0,        1,        0,          0}
  },
  {
    {0.9781665, 0,        0.9899469, 8.087667,  0.8226412},
    {0.9097769, 2.039997, 0.9584096, 0.1765447, 0},
    {0.9231212, 2.084788, 0.9493895, 0.1712518, 0},
    {0.9845552, 1.415612, 0.9703883, 3.68829,   0.8265008},
    {0.5,       0,        1,         0,         0}
  }
};

float waveformCalculator_makeSample(float *o, float *dac) {
  float out = 0;
  uint32_t i = 0;

  for(i = 0; i < 12; i++) {
    out += o[i] * dac[i];
  }

  return out;
}

// convert bitarray of length 12 to an 8-bit number (drop 4 lowest bits)
int8_t waveformCalculator_makeDigital(float *bitarray) {
  int8_t out;
  uint32_t i = 0;

  for (i = 11; i >= 4; i--) {
    out <<= 1;

    if (bitarray[i] > 0.5) {
      out |= 1;
    }
  }

  return out;
}

// popuplate array of bits bitarray based on 12 bit number v
void waveformCalculator_populate(int32_t v, float *bitarray) {
  int32_t b = 1;
  uint32_t i = 0;

  for (i = 0; i < 12; i++) {
    bitarray[i] = (v & b) ? 1 : 0;
    b <<= 1;
  }
}

void waveformCalculator_fill(float *bitarray, uint32_t model, uint32_t w, uint32_t a, uint32_t pw) {
  int32_t top, i, j;

  if (w == 4) {
    // The Pulse waveform was created by sending the upper 12-bits of the accumulator to a 12-bit digital comparator. 
    // The output of the comparator was either a one or a zero. 
    // This single output was then sent to all 12 bits of the Waveform D/A.
    // (http://sid.kubarth.com/articles/interview_bob_yannes.html)

    waveformCalculator_populate((a >= pw ? 0xfff : 0), bitarray);
    return;
  }

  float *config = waveformCalculatorConfig[model][w == 3 ? 0 : w == 5 ? 1 : w == 6 ? 2 : w == 7 ? 3 : 4];

  // put the accumulator into the bit array
  // BOB YANNES: The Sawtooth waveform was created by sending the upper 12-bits of the accumulator to the 12-bit Waveform D/A.
  waveformCalculator_populate(a, bitarray);

  if ((w & 0x3) == 1) {
    // Triangle or Sawtooth + Triangle

    // BOB YANNES:  The Triangle waveform was created by using the MSB of the accumulator to invert the remaining upper 11 accumulator bits using EXOR gates. 
    //              These 11 bits were then left-shifted (throwing away the MSB) and sent to the Waveform D/A

    // check msb bit 12
    top = (a & 0x800) != 0;

    for (i = 11; i > 0; i--) {
      if (top) {
        // if msb is set, invert bit and shift it
        bitarray[i] = 1 - bitarray[i - 1];
      } else {
        // msb not set, just shift it
        bitarray[i] = bitarray[i - 1];
      }
    }

    bitarray[0] = 0;
  }

  if ((w & 3) == 3) {
    // Sawtooth + Triangle
    bitarray[0] *= config[4];

    for(i = 1; i < 12; i++) {
      bitarray[i] = (bitarray[i - 1] * (1 - config[4])) + (bitarray[i] * config[4]);
    }
  }

  bitarray[11] *= config[2];

  if (w == 3 || w > 4) {
    // Sawtooth + Triangle
    // or Pulse Combined with Sawtooth and/or Triangle
    float dist[25];
    float temp[12];

    for (i = 0; i <= 12; i++) {
      dist[12 + i] = dist[12 - i] = 1 / (1 + (i * i * config[3]));
    }

    float pulse = (a >= pw ? 1 : -1) * config[1];

    for (i = 0; i < 12; i++) {
      float a = 0;
      float n = 0;

      for (j = 0; j < 12; j++) {
        float weight = dist[i - j + 12];
        a += bitarray[j] * weight;
        n += weight;
      }

      if (w > 4) {
        float weight = dist[i];
        a += pulse * weight;
        n += weight;
      }

      temp[i] = (bitarray[i] + (a / n)) * 0.5;
    }

    for (i = 0; i < 12; i++) {
      bitarray[i] = temp[i];
    }
  }

  /*
    * Use the environment around bias value to set/clear dac bit. Measurements
    * indicate the threshold is very sharp.
    */
  for (i = 0; i < 12; i++) {
    bitarray[i] = ((bitarray[i] - config[0]) * 512) + 0.5;

    if (bitarray[i] < 0) {
      bitarray[i] = 0;
    } else if (bitarray[i] > 1) {
      bitarray[i] = 1;
    }
  }
}



void waveformCalculator_build(uint32_t model, float nonlinearity) {
  uint32_t i = 0;
  uint32_t w, a;

  for(i = 0; i < 12; i++) {
    sid_waveDac[i] = sid_kinkedDac((1 << i), nonlinearity, 12);
  }

  float bitarray[12];
  int32_t z = (model == 0) ? -896 : -2048;

  // waveforms 1-7, noise waveform is treated separately
  for (w = 1; w < 8; w++) {
    for (a = 0; a < 4096; a++) {

      // make bitarray for waveforms 1-3 and waveform 4 where accumulator is greater than pulse width (all zero)
      // pass 12 bit bitarray through dac to make samples
      // convert 12 bit bitarray to 8 bit number for form digital values 
      waveformCalculator_fill(bitarray, model, w, a, 0x1000);

      sid_wavetable_samples[w - 1][a] = waveformCalculator_makeSample(bitarray, sid_waveDac) + z;
      sid_wavetable_digital[w - 1][a] = waveformCalculator_makeDigital(bitarray);

      if (w >= 4) {
        // make a bit array for waveform 4 where accumulator is less than pulse width (all bits are 1)/
        // and combinations of pulse and other waveforms
        waveformCalculator_fill(bitarray, model, w, a, 0);
        sid_wavetable_samples[w + 3][a] = waveformCalculator_makeSample(bitarray, sid_waveDac) + z;
        sid_wavetable_digital[w + 3][a] = waveformCalculator_makeDigital(bitarray);
      }
    }
  }
}

