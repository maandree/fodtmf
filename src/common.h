/**
 * Copyright © 2015  Mattias Andrée (maandree@member.fsf.org)
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


/**
 * Byte used to escape bytes with special interpretation.
 * 
 * This is the 'Data link escape' character.
 */
#define CHAR_ESCAPE  0x10

/**
 * Byte used to mark a transmission as aborted (abnormal termination.)
 * 
 * This is the 'Cancel' character.
 */
#define CHAR_CANCEL  0x18

/**
 * Byte used to mark the end of a transmission (normal termination.)
 * 
 * This is the 'End of transmission' character.
 */
#define CHAR_END  0x04

