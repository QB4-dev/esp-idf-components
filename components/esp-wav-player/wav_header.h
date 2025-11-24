#ifndef _WAV_HEADER_H_
#define _WAV_HEADER_H_

#include <inttypes.h>

// WAV header spec information:
//https://web.archive.org/web/20140327141505/https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
//http://www.topherlee.com/software/pcm-tut-wavformat.html

typedef struct wav_header {
    /* RIFF Header */
    char     riff_header[4]; /*!< Contains the ASCII tag "RIFF". */
    uint32_t wav_size;       /*!< Size of the WAV portion of the file (file size - 8). */
    char     wave_header[4]; /*!< Contains the ASCII tag "WAVE". */

    /* Format Header */
    char     fmt_header[4];    /*!< Contains the ASCII tag "fmt " (includes trailing space). */
    uint32_t fmt_chunk_size;   /*!< Size of the fmt chunk (typically 16 for PCM). */
    uint16_t audio_format;     /*!< Audio format (1 = PCM, 3 = IEEE float). */
    uint16_t num_channels;     /*!< Number of audio channels. */
    uint32_t sample_rate;      /*!< Sampling rate in Hz. */
    uint32_t byte_rate;        /*!< Bytes per second (sample_rate * num_channels * bytes_per_sample). */
    uint16_t sample_alignment; /*!< Block alignment (num_channels * bytes_per_sample). */
    uint16_t bit_depth;        /*!< Bits per sample (e.g. 16). */

    /* Data */
    char     data_header[4]; /*!< Contains the ASCII tag "data". */
    uint32_t data_bytes;     /*!< Number of bytes in the data chunk (samples * frame_size). */
} wav_header_t;

#endif /* _WAV_HEADER_H_ */
