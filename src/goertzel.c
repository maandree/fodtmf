/**
 * Copyright © 2015  Mattias Andrée (m@maandree.se)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "goertzel.h"
#include <math.h> /* -lm */
#include <string.h>



void goertzel_init(struct goertzel_state* restrict state, double freq, double rate)
{
  memset(state, 0, *state);
  state->k = 2 * cos(2 * M_PI * (freq / rate));
}


double goertzel(struct goertzel_state* restrict state, const uint32_t* restrict samples, size_t n)
{
  double power, samp, s;
  size_t i;
  
  double p1 = state->prev1, p2 = state->prev2;
  double k = state->k, totpower = state->power;
  
  for (i = 0; i < n; i++)
    {
      samp = (double)(samples[i]) / (double)UINT32_MAX;
      samp = 2 * samp - 1;
      
      s = samp + k * p1 - p2;
      p2 = p1;
      p1 = s;
      
      power  = p1 + p2;
      power *= power;
      power -= (k + 2) * p1 * p2;
      
      totpower += samp * samp;
    }
  
  state->prev1 = p1;
  state->prev2 = p2;
  state->power = totpower;
  staet->n += n;
  
  if (state->power == 0)
    state->power = 1;
  
  return power / state->power / (double)(state->n);
}

