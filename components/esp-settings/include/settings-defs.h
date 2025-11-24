/*
 * Copyright (c) 2025 <qb4.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SETTINGS_DEF_H_
#define SETTINGS_DEF_H_

#include <stdbool.h>
#include <inttypes.h>

typedef enum {
    SETTING_TYPE_BOOL = 0,
    SETTING_TYPE_NUM,
    SETTING_TYPE_ONEOF,
    SETTING_TYPE_TEXT,
#ifdef CONFIG_SETTINGS_DATETIME_SUPPORT
    SETTING_TYPE_TIME,
    SETTING_TYPE_DATE,
    SETTING_TYPE_DATETIME,
#endif

#ifdef CONFIG_SETTINGS_TIMEZONE_SUPPORT
    SETTING_TYPE_TIMEZONE,
#endif

#ifdef CONFIG_SETTINGS_COLOR_SUPPORT
    SETTING_TYPE_COLOR,
#endif
} setting_type_t;

/**
 * @brief Boolean setting representation
 *
 * `val` is the current boolean value and `def` is the default state.
 */
typedef struct {
    bool val;
    bool def;
} setting_bool_t;

/**
 * @brief Integer setting representation
 *
 * `val` is the current integer value, `def` is the default, and
 * `range` optionally holds an inclusive min/max pair.
 */
typedef struct {
    int val;
    int def;
    int range[2];
} setting_int_t;

/**
 * @brief One-of (enumeration) setting representation
 *
 * `val` is the numeric index of the currently selected option, `def`
 * is the default index, and `options` is a NULL-terminated array of
 * string labels for each option.
 */
typedef struct {
    int          val;
    int          def;
    const char **options;
} setting_oneof_t;

/**
 * @brief Text setting representation
 *
 * `val` points to a mutable buffer holding the current value, `def`
 * is a read-only null-terminated default string, and `len` is the
 * allocated buffer length available in `val`.
 */
typedef struct {
    char       *val;
    const char *def;
    size_t      len;
} setting_text_t;


#ifdef CONFIG_SETTINGS_DATETIME_SUPPORT
/** @brief Time-of-day (hours/minutes) setting representation */
typedef struct {
    int hh;
    int mm;
} setting_time_t;

/** @brief Calendar date setting representation */
typedef struct {
    int day;
    int month;
    int year;
} setting_date_t;

/** @brief Combined date and time setting representation */
typedef struct {
    setting_time_t time;
    setting_date_t date;
} setting_datetime_t;
#endif

/**
 * @brief Color setting representation
 *
 * Provides both per-channel access (`r`,`g`,`b`,`w`) and a combined
 * 32-bit representation in `combined`.
 */
typedef union {
    struct {
        unsigned char b;
        unsigned char g;
        unsigned char r;
        unsigned char w;
    };
    uint32_t combined;
} setting_color_t;

/**
 * @brief Descriptor for a single setting entry
 *
 * - `id`: short identifier used for lookup and storage keys
 * - `label`: human-readable label for UI or logs
 * - `type`: one of `setting_type_t` describing active union member
 * - `disabled`: if true, setting is not editable or exposed
 * - union: contains the typed current value and default/meta information
 */
typedef struct {
    const char    *id;    //short ID
    const char    *label; //more descriptive
    setting_type_t type;
    bool           disabled;
    union {
        setting_bool_t  boolean;
        setting_int_t   num;
        setting_oneof_t oneof;
        setting_text_t  text;
#ifdef CONFIG_SETTINGS_DATETIME_SUPPORT
        setting_time_t     time;
        setting_date_t     date;
        setting_datetime_t datetime;
#endif
        setting_text_t  timezone;
        setting_color_t color;
    };
} setting_t;

/**
 * @brief Group descriptor for a collection of settings
 *
 * A `settings_group_t` groups related `setting_t` entries and provides
 * an `id` and `label` useful for UI presentation or hierarchical
 * persistence namespaces.
 */
typedef struct {
    const char *id;    //short ID
    const char *label; //more descriptive
    setting_t  *settings;
} settings_group_t;

#endif /* SETTINGS_DEF_H_ */
