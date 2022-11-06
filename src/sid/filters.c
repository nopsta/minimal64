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


extern sid_t m64_sid;
extern float sid_cpuCyclesPerSecond;

// 6581
float sid_nonlinearity = 3.3e6;

float low_pass;	// previous low pass output
float band_pass;	// previous band pass output
	
float cutoff_ratio_8580;
float cutoff_ratio_6581;
float cutoff_bias_6581;

float cutoff;
float resonance; 	// convenience calculated from the above

	// is set in resetFilter..
float sid_nlvoice = 0;// 0.96;

// settings for type 4
float sid_type4k       = 5.7;
float sid_type4b       = 20;
float sid_type4cache = 0;
float sid_oneDivQ4   = 0;

// both
float sid_resfactor    = 1.0;

void sid_resetFilter() {

  int32_t i = 0;

  low_pass = 0;
  band_pass = 0;

  cutoff_ratio_8580 = 0;
  cutoff_ratio_6581 = 0;
  cutoff_bias_6581 = 0;

  cutoff = 0;
  resonance = 0;

  // put this into sid set model?
  float nonlinearity = sid_nlvoice;
  if (m64_sid.sid_model == SID_8580) {
    sid_nlvoice = 1.0;
  } else {
    sid_nlvoice = 0.96;
  }
  if (sid_nlvoice != nonlinearity) {
    sid_setNonlinearity(sid_nlvoice);
  }

  sid_recalculate();
  sid_updateCenter();
  sid_updateResonance();

}

float sid_clock6581(float v1, float v2, float v3, int32_t inp) {
  
  float di = 0;
  float output = 0;  // final
  float filterInput = 0;  // input to filter

  if (m64_sid.sid_filt1) { 
    filterInput += v1;
  } else { 
    output += v1; 
  }

  if (m64_sid.sid_filt2) { 
    filterInput += v2; 
  } else { 
    output += v2; 
  }

  if (m64_sid.sid_filt3) {
    // voice 3 not silenced by voice 3 off if routed through filter
    filterInput += v3;
  } else if (m64_sid.sid_voice3off) {
    output += v3;
  }

  if (m64_sid.sid_filtE) { 
    filterInput += inp; 
  } else { 
    output += inp; 
  }


/*
https://bel.fi/alankila/c64-sw/index-cpp.html
Vlp -= w0 * Vbp
Vbp -= w0 * Vhp
Vhp = Vbp / Q - Vlp - Vi
*/

  float tmp = filterInput + band_pass * resonance + low_pass;
		
	if (m64_sid.sid_f_hp) { 
    output -= tmp;
  } 
		
  tmp = band_pass - tmp * cutoff;
  band_pass = tmp;
		
	if (m64_sid.sid_f_bp) { 
    output += tmp; 
  }	// make it look like reSID (even if it might be wrong) fixme: check if 6581 and 8580 do this differently
		
	tmp = low_pass + tmp * cutoff;
	low_pass = tmp;
		
	if (m64_sid.sid_f_lp) { 
    output += tmp; 
  }


  output *= m64_sid.sid_f_vol;

  if (output > sid_nonlinearity) {
    output -= ((output - sid_nonlinearity) * 0.5);
  }

  return output;
}

float sid_clock8580(float v1, float v2, float v3, int32_t inp) {
  float output = 0;  // final
  float filterInput = 0;  // input to the filter


  if (m64_sid.sid_filt1) { 
    filterInput += v1;
  } else { 
    output += v1; 

  }

  if (m64_sid.sid_filt2) { 
    filterInput += v2; 

  } else { 
    output += v2; 

  }

  if (m64_sid.sid_filt3) {
    // voice 3 not silenced by voice 3 off if routed through filter
    filterInput += v3;
  } else if (m64_sid.sid_voice3off) {
    output += v3;
  }


  if (m64_sid.sid_filtE) { 
    filterInput += inp; 
  } else { 
    output += inp; 
  }



/*
https://bel.fi/alankila/m64-sw/index-cpp.html
Vlp -= w0 * Vbp
Vbp -= w0 * Vhp
Vhp = Vbp / Q - Vlp - Vi
*/

  m64_sid.sid_f_vlp += (m64_sid.sid_f_vbp * sid_type4cache);
  m64_sid.sid_f_vbp += (m64_sid.sid_f_vhp * sid_type4cache);
  m64_sid.sid_f_vhp = ((-m64_sid.sid_f_vbp * sid_oneDivQ4) - m64_sid.sid_f_vlp - filterInput);

  if (m64_sid.sid_f_lp) { 
    output += m64_sid.sid_f_vlp; 
  }

  if (m64_sid.sid_f_bp) { 
    output += m64_sid.sid_f_vbp; 
  }

  if (m64_sid.sid_f_hp) { 
    output += m64_sid.sid_f_vhp; 
  }


  return output * m64_sid.sid_f_vol;
}

void sid_recalculate() {
  
  // 8580 
  cutoff_ratio_8580 = ((double) -2.0) * 3.1415926535897932385 * (12500.0 / 2048) / sid_cpuCyclesPerSecond;

  // 6581: old cSID impl
  cutoff_ratio_6581 = ((double)-2.0) * 3.1415926535897932385 * (20000.0 / 2048) / sid_cpuCyclesPerSecond;
  cutoff_bias_6581 = 1 - exp(-2 * 3.14 * 220 / sid_cpuCyclesPerSecond); //around 220Hz below threshold
}

void sid_updateCenter() {

  if (m64_sid.sid_model == SID_6581) {
    cutoff = m64_sid.sid_f_cut + 1;
    cutoff = (cutoff_bias_6581 + ((cutoff < 192) ? 0 : 1 - exp((cutoff - 192) * cutoff_ratio_6581)));
    
  } else {
    // +1 is meant to model that even a 0 cutoff will still let through some signal..
    cutoff = m64_sid.sid_f_cut + 1;		
    cutoff = 1.0 - exp(cutoff * cutoff_ratio_8580);

    sid_type4cache = (6.283185307179586 * ((sid_type4k * (float)m64_sid.sid_f_cut) + sid_type4b)) / sid_cpuCyclesPerSecond;    
  }

}
  
void sid_updateResonance() {

  if(m64_sid.sid_model == SID_6581) {
  	resonance = pow(2.0, ((4.0 - m64_sid.sid_f_res) / 8));				// i.e. 1.41 to 0.39
  } else {
    sid_oneDivQ4 = 1 / (0.707 + ((m64_sid.sid_f_res * sid_resfactor) / 15));

//    resonance = ((m64_sid.sid_f_res > 0x5) ? 8.0 / (m64_sid.sid_f_res) : 1.41);
  }


  // from jsidplay:
  // XXX: resonance tuned by ear, based on a few observations:
//  sid_oneDivQ3 = 1 / (0.500 + ((m64_sid.sid_f_res * sid_resfactor) / 18));
//  sid_oneDivQ4 = 1 / (0.707 + ((m64_sid.sid_f_res * sid_resfactor) / 15));
}