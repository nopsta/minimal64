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

//see notes/04-envelope.txt

// Digital to Analog converter used to convert envelopeDigital to envelope
float sid_envDAC[256]; 

// number of cycles between increments of envelope rate counter
// see envelope rates in programmers reference guide

// BOB YANNES: A programmable frequency divider was used to set the various rates
//             A small look-up table translated the 16 (4bit) register-programmable values to the appropriate number to load into the frequency divider. 
//             The actual numbers in the look-up table were arrived at subjectively by setting up typical patches on a Sequential Circuits Pro-1 and measuring 
//             the envelope times by ear

// The rate counter period is the number of cycles between each increment of the envelope counter.
uint16_t sid_envelope_rate_periods[16] = { 
  8, // 2ms*1.0MHz/256 = 7.81
  31, // 8ms*1.0MHz/256 = 31.25
  62, // 16ms*1.0MHz/256 = 62.50
  94, // 24ms*1.0MHz/256 = 93.75
  148, // 38ms*1.0MHz/256 = 148.44
  219, // 56ms*1.0MHz/256 = 218.75
  266, // 68ms*1.0MHz/256 = 265.63
  312, // 80ms*1.0MHz/256 = 312.50
  391, // 100ms*1.0MHz/256 = 390.63
  976, // 250ms*1.0MHz/256 = 976.56
  1953, // 500ms*1.0MHz/256 = 1953.13
  3125, // 800ms*1.0MHz/256 = 3125.00
  3906, // 1 s*1.0MHz/256 = 3906.25
  11719, // 3 s*1.0MHz/256 = 11718.75
  19531, // 5 s*1.0MHz/256 = 19531.25
  31250 // 8 s*1.0MHz/256 = 31250.00  
};


// digital envelope goes from 0 to 0xff
// BOB YANNES: The 8-bit output of the Envelope Generator was then sent to the Multiplying D/A converter to modulate the amplitude of  the selected Oscillator Waveform
// use sid_envDAC to convert from voice->envelopeDigital to voice->envelope
void sid_envelope_buildDAC(float nonlinearity) {
  uint32_t i;

  for (i = 0; i < 256; i++) {
    sid_envDAC[i] = sid_kinkedDac(i, nonlinearity, 8);
  }
}


// clock the envelope
void sid_voice_envelope_clock(sid_voice_t *voice) {

  // envelope
  // BOB YANNES:  The Envelope Generator was simply an 8-bit up/down counter which, when triggered by the Gate bit, counted from 0 to 255 at the Attack rate, 
  //              from 255 down to the programmed Sustain value at the Decay rate, remained at the Sustain value until the Gate bit was cleared then counted down 
  //              from the Sustain value to 0 at the Release rate.

  // rate period is set to values in the envelope rate period table
  if (++voice->rateCounter != voice->rateCounterPeriod) {
    return;
  }

  // reset the rate counter
  voice->rateCounter = 0;


  // SID_ATTACK is linear, so always want to enter if attack
  // other states use the exponential period table for timing, only enter if the expoCounter has reached the expo period
  if (voice->state == SID_ATTACK || (++voice->expoCounter == voice->expoPeriod)) {
    voice->expoCounter = 0;

    if (!voice->freezeZero) {

      // When the gate goes high the volume raises up to $ff linearly (attack is linear) then it falls down exponentially to the sustain level. 
      // Once the gate is released the volume goes down exponentially to zero.

      switch(voice->state) {
        case SID_ATTACK:

          if (++voice->envelopeDigital == 0xff) {
            // when envelope reaches $ff, state switches to DECAYSUSTAIN
            voice->state = SID_DECAYSUSTAIN;
            voice->rateCounterPeriod = sid_envelope_rate_periods[voice->decay];
          }
        break;
        case SID_DECAYSUSTAIN:
          // BOB YANNES: A digital comparator was used for the Sustain function. 
          //             The upper four bits of the Up/Down counter were compared to the programmed 
          //             Sustain value and would stop the clock to the Envelope Generator when the counter counted down to the Sustain value. 
          //             This created 16 linearly spaced sustain levels without having to go through a look-up table translation between 
          //             the 4-bit register value and the 8-bit Envelope Generator output. 
          //             It also meant that sustain levels were adjustable in steps of 16. Again, more register bits would have provided higher resolution.

          if (voice->envelopeDigital != (voice->sustain << 4 | voice->sustain)) {            
            voice->envelopeDigital = (--voice->envelopeDigital) & 0xff;
          }
        break;
        case SID_RELEASE:
          voice->envelopeDigital = (--voice->envelopeDigital) & 0xff;
        break;
      }
      // exponential timing,   for DECAY and RELEASE these are the periods they fall
      // BOB YANNES: In order to more closely model the exponential decay of sounds, another look-up table on the 
      //             output of the Envelope Generator would sequentially 
      //             divide the clock to the Envelope Generator by two at specific counts in the Decay and Release cycles. 
      //             This created a piece-wise linear approximation of an exponential. 
      //             I was particularly happy how well this worked considering the simplicity of the circuitry. 
      //             The Attack, however, was linear, but this sounded fine.
      switch (voice->envelopeDigital) {
        case 255:
          voice->expoPeriod = 1;
          break;
        case 93:
          voice->expoPeriod = 2;
          break;
        case 54:
          voice->expoPeriod = 4;
          break;
        case 26:
          voice->expoPeriod = 8;
          break;
        case 14:
          voice->expoPeriod = 16;
          break;
        case 6:
          voice->expoPeriod = 30;
          break;
        case 0:
          voice->expoPeriod = 1;

          // When the envelope counter is changed to zero, it is frozen at
          // zero.
          voice->freezeZero = 1;
          break;
      }

      // convert it through the dac
      voice->envelope = voice->muted ? 0 : sid_envDAC[voice->envelopeDigital & 0xff];
    }
  }
}
