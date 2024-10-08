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
#include <stddef.h>
#include <stdint.h>



struct goertzel_state
{
  double prev1;
  double prev2;
  double power;
  size_t n;
  double k;
};


void goertzel_init(struct goertzel_state* restrict state, double freq, double rate);
double goertzel(struct goertzel_state* restrict state, const uint32_t* restrict samples, size_t n);

