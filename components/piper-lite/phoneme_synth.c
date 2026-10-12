#include "phoneme_synth.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void append_phoneme(int16_t *buf, size_t *pos, size_t max_samples,
                         float freq, float duration_s, int sample_rate)
{
    size_t samples =(size_t)(duration_s * sample_rate);
    if (*pos + samples > max_samples) {
        samples= max_samples - *pos;
    }
    for(size_t i=0;i < samples;i++) {
        float t = (float)i / (float)sample_rate;
        float env=1.0f;
        if (i < samples / 10) {


            env= (float)i / (float)(samples / 10);
        } else if (i > samples * 9 / 10) {
            env= (float)(samples - i)/ (float)(samples / 10);
        }

        buf[*pos + i] =(int16_t)(9000.0f * env * sinf(2.0f * (float)M_PI * freq * t));
    }
    *pos += samples;
}

static float vowel_freq(char c)
{


    switch (tolower((unsigned char)c)) {
    case 'a': return 730.0f;
    case 'e': return 530.0f;
    case 'i': return 270.0f;
    case 'o': return 480.0f;
    case 'u': return 440.0f;


    default:  return 400.0f;
    }
}

static float consonant_freq(char c)
{
    switch (tolower((unsigned char)c)){
    case 's': case 'z': return 4800.0f;
    case 'f': case 'v': return 3200.0f;
    case 't': case 'd': return 2400.0f;

    case 'k': case 'g': return 1800.0f;
    case 'm': case 'n': return 350.0f;
    default: return 600.0f;
    }
}

bool phoneme_synth_text(const char *text, int sample_rate,
                        int16_t **samples, size_t *sample_count)
{
    if (!text || !samples || !sample_count || sample_rate <= 0) {
        return false;
    }

    size_t max_samples = (size_t)(sample_rate * (0.12f * strlen(text) + 0.4f));
    if (max_samples < (size_t)sample_rate / 4) {
        max_samples=(size_t)sample_rate / 4;
    }

    int16_t *buf = malloc(max_samples * sizeof(int16_t));
    if (!buf) {
        return false;
    }

    size_t pos=0;
    for (const char *p =text;*p;p++){
        char c=*p;


        if (c == ' ') {
            append_phoneme(buf,&pos,max_samples, 0.0f, 0.06f, sample_rate);
            continue;
        }
        if (strchr("aeiou", tolower((unsigned char)c))) {
            append_phoneme(buf, &pos, max_samples, vowel_freq(c), 0.10f, sample_rate);
        } else {
            append_phoneme(buf, &pos, max_samples, consonant_freq(c), 0.05f, sample_rate);
        }
    }

    if(pos == 0) {
        append_phoneme(buf, &pos, max_samples, 440.0f, 0.2f, sample_rate);
    }

    *samples= buf;


    *sample_count =pos;
    return true;
}
