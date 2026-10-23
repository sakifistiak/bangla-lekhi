/*
 * Bangla Lekhi Core Engine C-ABI Header
 *
 * Copyright (C) 2026 Bangla Lekhi Contributors
 * Licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later)
 */

#ifndef BANGLA_LEKHI_CORE_H
#define BANGLA_LEKHI_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BANGLA_LEKHI_MODE_BANGLA = 0,
    BANGLA_LEKHI_MODE_ENGLISH = 1
} BanglaLekhiMode;

typedef struct BanglaLekhiEngine BanglaLekhiEngine;

/* Lifecycle */
BanglaLekhiEngine *bangla_lekhi_engine_new(void);
void bangla_lekhi_engine_free(BanglaLekhiEngine *engine);

/* Mode */
BanglaLekhiMode bangla_lekhi_engine_get_mode(const BanglaLekhiEngine *engine);
void bangla_lekhi_engine_set_mode(BanglaLekhiEngine *engine, BanglaLekhiMode mode);
BanglaLekhiMode bangla_lekhi_engine_toggle_mode(BanglaLekhiEngine *engine);

/* Keystroke & Composition Buffer */
void bangla_lekhi_engine_push_char(BanglaLekhiEngine *engine, char c);
bool bangla_lekhi_engine_pop_char(BanglaLekhiEngine *engine);
void bangla_lekhi_engine_clear(BanglaLekhiEngine *engine);
bool bangla_lekhi_engine_has_composition(const BanglaLekhiEngine *engine);
const char *bangla_lekhi_engine_get_raw_buffer(const BanglaLekhiEngine *engine);

/* Pre-edit and Candidate Suggestions */
const char *bangla_lekhi_engine_get_preedit(const BanglaLekhiEngine *engine);
size_t bangla_lekhi_engine_get_candidate_count(const BanglaLekhiEngine *engine);
const char *bangla_lekhi_engine_get_candidate_at(const BanglaLekhiEngine *engine, size_t index);
const char *bangla_lekhi_engine_commit_candidate(BanglaLekhiEngine *engine, size_t index);

/* Standalone Transliteration Utilities */
char *bangla_lekhi_transliterate(const char *input);
void bangla_lekhi_string_free(char *ptr);

#ifdef __cplusplus
}
#endif

#endif /* BANGLA_LEKHI_CORE_H */
