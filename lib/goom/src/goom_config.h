#ifndef _GOOM_CONFIG_H
#define _GOOM_CONFIG_H

#define NUM_AUDIO_SAMPLES 2
#define AUDIO_SAMPLE_LEN 512

#ifdef WORDS_BIGENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

#ifndef WORDS_BIGENDIAN
// position des composantes
#define ALPHA 3
#define BLEU 2
#define VERT 1
#define ROUGE 0
#else
#define BLEU 3
#define VERT 2
#define ROUGE 1
#define ALPHA 0
#endif

#endif
