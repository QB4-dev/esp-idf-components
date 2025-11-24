#ifndef _WAV_OBJECT_H_
#define _WAV_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @file wav_object.h
 * @brief Lightweight description of WAV sources supported by the player.
 *
 * This header defines a small `wav_obj_t` descriptor used to refer to WAV
 * data located in different storage backends (embedded in flash, SPIFFS,
 * or MMC/SD). Helper macros are provided to create static descriptors.
 */

/**
 * @brief Type of WAV data source.
 */
typedef enum {
    WAV_SRC_EMBED,  /*!< WAV file embedded in program memory (pointer to data). */
    WAV_SRC_SPIFFS, /*!< WAV file stored in SPIFFS filesystem (path string). */
    WAV_SRC_MMC,    /*!< WAV file stored on MMC/SD card (path string). */
} wav_source_type_t;

/**
 * @brief Descriptor for a WAV resource.
 *
 * The `wav_obj_t` contains a `type` field indicating where the WAV data
 * is located and a union with storage-specific metadata. For embedded
 * data, provide a pointer to the raw WAV bytes. For SPIFFS/MMC, provide
 * the path to the file.
 */
typedef struct {
    wav_source_type_t type; /*!< Source type selecting the active union member. */
    union {
        struct {
            const uint8_t *addr; /*!< Pointer to embedded WAV data in flash/ROM. */
        } embed;
        struct {
            const char *path; /*!< Path to WAV file inside SPIFFS. */
        } spiffs;
        struct {
            const char *path; /*!< Path to WAV file on MMC/SD card. */
        } mmc;
    };
} wav_obj_t;

/**
 * @brief Macro to declare an embedded WAV descriptor.
 *
 * Example:
 * @code
 *   extern const uint8_t my_wav_data[];
 *   WAV_DECLARE_EMBED(my_wav, my_wav_data);
 * @endcode
 *
 * @param name Identifier to create (static `wav_obj_t`).
 * @param addr Pointer to embedded WAV data.
 */
#define WAV_DECLARE_EMBED(name, addr) static const wav_obj_t name = { .type = WAV_SRC_EMBED, .embed = { addr } }

/**
 * @brief Macro to declare a SPIFFS WAV descriptor.
 *
 * @param name Identifier to create (static `wav_obj_t`).
 * @param path Null-terminated path in SPIFFS (e.g. "/audio/darude.wav").
 */
#define WAV_DECLARE_SPIFFS(name, path) static const wav_obj_t name = { .type = WAV_SRC_SPIFFS, .spiffs = { path } }

/**
 * @brief Macro to declare an MMC/SD WAV descriptor.
 *
 * @param name Identifier to create (static `wav_obj_t`).
 * @param path Null-terminated path on MMC/SD (e.g. "/sdcard/song.wav").
 */
#define WAV_DECLARE_MMC(name, path) static const wav_obj_t name = { .type = WAV_SRC_MMC, .mmc = { path } }

#ifdef __cplusplus
}
#endif

#endif // _WAV_OBJECT_H_
