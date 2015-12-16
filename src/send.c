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
#include "common.h"

#include <stdio.h>
#include <math.h> /* -lm */
#include <stdint.h>
#include <alsa/asoundlib.h> /* dep: alsa-lib (pkg-config alsa) */
#include <signal.h>



#define UTYPE   uint32_t
#define SMIN    INT32_MIN
#define SMAX    INT32_MAX
#define FORMAT  SND_PCM_FORMAT_U32

#define SAMPLE_RATE  52000   /* Hz */
#define DURATION     100     /* ms */ /* 100 ms → 10 Bd → 5 B/s */
#define LATENCY      100000  /* µs */



/**
 * Name number of samples per tone.
 */
#define N  SAMPLE_RATE / 1000 * DURATION



/**
 * The process's name.
 */
static const char* argv0;

/**
 * Audio descriptor.
 */
static snd_pcm_t* sound_handle = NULL;

/**
 * Audio playback buffer for each nibble.
 */
static UTYPE buffers[16][N];



/**
 * Use to accept signals, without doing anything
 * about them, just be interrupted.
 * 
 * @param  signo  The received signal.
 */
static void signoop(int signo)
{
  (void) signo;
}


/**
 * Get the tones used to transmit a nibble.
 * 
 * @param  nibble  The nibble.
 * @param  low     Output parameter for the lower frequency.
 * @param  high    Output parameter for the higher frequency.
 */
static void get_freq(int nibble, int* low, int* high)
{
  static const int LOW[]  = {  697,  770,  852,  941 };
  static const int HIGH[] = { 1209, 1336, 1477, 1663 };
  
  *low  =  LOW[(nibble >> 0) & 0x03];
  *high = HIGH[(nibble >> 2) & 0x03];
}


/**
 * Initialise to before for each nibble.
 */
static void init_buffers(void)
{
#define GENERATE_TONE(tone)  sin(2 * M_PI * ((double)i / (SAMPLE_RATE / (tone))))
  
  size_t i;
  UTYPE* buffer;
  int j, low, high;
  
  for (j = 0; j < 16; j++)
    {
      buffer = buffers[j];
      get_freq(j, &low, &high);
      
      for (i = 0; i < N; i++)
	buffer[i] = (GENERATE_TONE(low * 1) + GENERATE_TONE(high * 1) + 
		     GENERATE_TONE(low * 4) + GENERATE_TONE(high * 4)) *
	  (SMAX / 4) - SMIN;
    }
}


/**
 * Transmit a nibble.
 * 
 * @param   n  The nibble.
 * @return     0 on success, -1 on error.
 */
static int send_nibble(int n)
{
  UTYPE* buffer = buffers[n];
  snd_pcm_sframes_t frames;
  int r;
  
  r = frames = snd_pcm_writei(sound_handle, buffer, N);
  if (frames < 0)
    r = frames = snd_pcm_recover(sound_handle, r, 0 /* do not print error reason? */);
  if (r < 0)
    {
      fprintf(stderr, "%s: snd_pcm_writei: %s\n", argv0, snd_strerror(r));
      return -1;
    }
  if ((r > 0) && ((size_t)r < N))
    printf("%s: short write: expected %zu, wrote %zu\n", argv0, N, (size_t)frames);
  
  return 0;
}


/**
 * Transmit a byte.
 * 
 * @param   c  The byte.
 * @return     0 on success, -1 on error.
 */
static int send_byte(int c)
{
  if (send_nibble((c >> 0) & 0x0F))  return -1;
  if (send_nibble((c >> 4) & 0x0F))  return -1;
  return 0;
}


/**
 * Transmit a byte with error correcting code.
 * 
 * This function uses a buffer so that it can
 * create an error correcting code. The bytes
 * are not transmitted until this buffer has
 * been filled.
 * 
 * @param   c  The byte, the negative of that byte (intended
 *             only for `CHAR_END` and `CHAR_CANCEL`) to fill
 *             the remained of the buffer with the byte.
 *             Note that if a negative value is used, it is
 *             no necessary that anything will happen.
 * @return     0 on success, -1 on error.
 */
static int send_byte_with_ecc(int c)
{
  static int data[4];
  static int ptr = 0;
  int code[8];
  int i, rc = 1;
  
  /* Fill buffer. */
  if (c < 0)
    {
      if (ptr > 0)
	while (ptr < 4)
	  data[ptr++] = -c;
    }
  else
    data[ptr++] = c;
  
  /* Is it full? */
  if (ptr < 4)
    return 0;
  ptr = 0;
  
  /* Hamming code. */
  code[0] = data[0] ^ data[1] ^           data[3];
  code[1] = data[0] ^           data[2] ^ data[3];
  code[2] = data[0];
  code[3] =           data[1] ^ data[2] ^ data[3];
  code[4] =           data[1];
  code[5] =                     data[2];
  code[6] =                               data[3];
  code[7] = data[0] ^ data[1] ^ data[2] ^ data[3];
  
  /* Transmit. */
  for (i = 0; i < 8; i++)
    if (send_byte(code[i]))
      return -1;
  
  return 0;
}


/**
 * Transmit a file over audio.
 * 
 * @param   argc  Not used for anything else than the process name.
 * @param   argv  Not used for anything else than the process name.
 * @return        0 on success, 1 on failure.
 */
int main(int argc, char* argv[])
{
  struct sigaction act;
  char buf[1024];
  int r, c, rc = 1;
  ssize_t n, i;
  
  argv0 = argc ? argv[0] : "";
  
  /* Set up signal handling. */
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  sigemptyset(&(act.sa_mask));
  act.sa_handler = signoop;
  act.sa_flags   = 0;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigprocmask(SIG_SETMASK, &(act.sa_mask), NULL);
  
  /* Generate the tones to play. */
  init_buffers();
  
  /* Set up audio. */
  r = snd_pcm_open(&sound_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (r < 0)
    return fprintf(stderr, "%s: snd_pcm_open: %s\n", *argv, snd_strerror(r)), 1;
  /* Configure audio. */
  r = snd_pcm_set_params(sound_handle, FORMAT, SND_PCM_ACCESS_RW_INTERLEAVED, 1 /* channels */,
			 SAMPLE_RATE, 1 /* allow resampling? */, LATENCY);
  if (r < 0)
    return fprintf(stderr, "%s: snd_pcm_set_params: %s\n", *argv, snd_strerror(r)), 1;
  
  /* TODO: if reading from the terminal, wait until all input is fetched. */
  /* Send message. */
  for (;;)
    {
      n = read(STDIN_FILENO, buf, sizeof(buf));
      if (n < 0)   goto fail;
      if (n == 0)  break;
      for (i = 0; i < n; i++)
	{
	  c = buf[i];
	  if ((c == CHAR_ESCAPE) || (c == CHAR_CANCEL) || (c == CHAR_END))
	    if (send_byte_with_ecc(CHAR_ESCAPE))
	      goto cleanup;
	  if (send_byte_with_ecc(c))
	    goto cleanup;
	}
    }
  /* Mark end of transmission. */
  if (send_byte_with_ecc(CHAR_END))   goto cleanup;
  if (send_byte_with_ecc(-CHAR_END))  goto cleanup;
  
  /* Done! */
  rc = 0;
  goto cleanup;
 fail:
  perror(argv0);
 cleanup:
  snd_pcm_close(sound_handle);
  /* Mark aborted transmission. */
  send_byte_with_ecc(CHAR_CANCEL);
  send_byte_with_ecc(-CHAR_CANCEL);
  return rc;
}

