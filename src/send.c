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
#include <stdio.h>
#include <math.h> /* -lm */
#include <stdint.h>
#include <alsa/asoundlib.h> /* dep: alsa-lib (pkg-config alsa) */


#define UTYPE   uint32_t
#define SMIN    INT32_MIN
#define SMAX    INT32_MAX
#define FORMAT  SND_PCM_FORMAT_U32

#define SAMPLE_RATE  52000   /* Hz */
#define DURATION     100     /* ms */ /* 100 ms → 10 Bd → 5 B/s */
#define LATENCY      100000  /* µs */


#define N(buf)  (sizeof(buf) / sizeof(*buf))


static const char* argv0;
static snd_pcm_t* sound_handle = NULL;


void get_freq(int nibble, int* low, int* high)
{
  static const int LOW[] = { 697, 770, 852, 941 };
  static const int HIGH[] = { 1209, 1336, 1477, 1663 };
  
  *low  =  LOW[(nibble >> 0) & 0x03];
  *high = HIGH[(nibble >> 2) & 0x03];
}

int send_tones(int low, int high)
{
#define GENERATE_TONE(tone)  \
  sin(2 * M_PI * ((double)i / (SAMPLE_RATE / (tone))))
  
  static UTYPE buffer[SAMPLE_RATE / 1000 * DURATION];
  snd_pcm_sframes_t frames;
  int r;
  size_t i;
  
  for (i = 0; i < N(buffer); i++)
    buffer[i] = (GENERATE_TONE(low * 1) + GENERATE_TONE(high * 1) + 
		 GENERATE_TONE(low * 4) + GENERATE_TONE(high * 4)) *
                (SMAX / 4) - SMIN;
  
  r = frames = snd_pcm_writei(sound_handle, buffer, N(buffer));
  if (frames < 0)
    r = frames = snd_pcm_recover(sound_handle, r, 0 /* do not print error reason? */);
  if (r < 0)
    {
      fprintf(stderr, "%s: snd_pcm_writei: %s\n", argv0, snd_strerror(r));
      return -1;
	}
  if ((r > 0) && ((size_t)r < N(buffer)))
    printf("%s: short write: expected %zu, wrote %zu\n", argv0, N(buffer), (size_t)frames);
  
  return 0;
}

int send_byte(int c)
{
  int low1, high1, low2, high2;
  get_freq((c >> 0) & 0x0F, &low1, &high1);
  get_freq((c >> 4) & 0x0F, &low2, &high2);
  if (send_tones(low1, high1))
    return -1;
  if (send_tones(low2, high2))
    return -1;
  return 0;
}


int main(int argc, char* argv[])
{
  int r;
  char buf[1024];
  ssize_t n, i;
  
  (void) argc;
  argv0 = argv[0];
  
  r = snd_pcm_open(&sound_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (r < 0)
    return
      fprintf(stderr, "%s: snd_pcm_open: %s\n", *argv, snd_strerror(r)),
      1;
  
  r = snd_pcm_set_params(sound_handle,
			 FORMAT,
			 SND_PCM_ACCESS_RW_INTERLEAVED,
			 1,            /* channels */
			 SAMPLE_RATE,
			 1,            /* allow resampling? */
			 LATENCY);
  if (r < 0)
    {
      fprintf(stderr, "%s: snd_pcm_set_params: %s\n", *argv, snd_strerror(r));
      goto snd_fail;
    }
  
  for (;;)
    {
      n = read(STDIN_FILENO, buf, sizeof(buf));
      if (n < 0)
	goto fail;
      if (n == 0)
	break;
      for (i = 0; i < n; i++)
	if (send_byte(buf[i]))
	  goto snd_fail;
    }
  
  snd_pcm_close(sound_handle);
  return 0;
 fail:
  perror(argv0);
  return 1;
 snd_fail:
  snd_pcm_close(sound_handle);
  return 1;
}

