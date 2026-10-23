/*
 * Bangla Lekhi IBus Engine Implementation
 *
 * Copyright (C) 2026 Bangla Lekhi Contributors
 * Licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later)
 */

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

G_DEFINE_TYPE(BanglaLekhiIBusEngine, bangla_lekhi_ibus_engine, IBUS_TYPE_ENGINE)

/* Forward declarations */
static gboolean engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint state);
static void engine_focus_in(IBusEngine *engine);
static void engine_focus_out(IBusEngine *engine);
static void engine_reset(IBusEngine *engine);
static void engine_enable(IBusEngine *engine);
static void engine_disable(IBusEngine *engine);
static void engine_candidate_clicked(IBusEngine *engine, guint index, guint button, guint state);

static void update_preedit_and_lookup_table(BanglaLekhiIBusEngine *self);
static void commit_current_composition(BanglaLekhiIBusEngine *self, guint candidate_idx);
static void reset_engine_state(BanglaLekhiIBusEngine *self);

/* ── Config helpers (minimal JSON reader — no external deps) ─────────────── */

/**
 * Read the "toggle_key" field from ~/.config/bangla-lekhi/settings.json.
 * Supported values: "F1"–"F12", "F9", "F10", "F11", "F12" (default).
 * Returns the IBus keyval, defaulting to IBUS_KEY_F12.
 */
static guint load_toggle_keyval(void) {
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char path[512] = {0};

    if (xdg_config) {
        snprintf(path, sizeof(path), "%s/bangla-lekhi/settings.json", xdg_config);
    } else if (home) {
        snprintf(path, sizeof(path), "%s/.config/bangla-lekhi/settings.json", home);
    } else {
        return IBUS_KEY_F12;
    }

    FILE *f = fopen(path, "r");
    if (!f) return IBUS_KEY_F12;

    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    if (n == 0) return IBUS_KEY_F12;

    /* Minimal "toggle_key" extraction — look for "toggle_key": "FN" */
    char *pos = strstr(buf, "\"toggle_key\"");
    if (!pos) return IBUS_KEY_F12;
    pos = strchr(pos, ':');
    if (!pos) return IBUS_KEY_F12;
    pos++;
    while (*pos == ' ' || *pos == '\t') pos++;
    if (*pos != '"') return IBUS_KEY_F12;
    pos++; /* skip opening quote */

    char key_name[16] = {0};
    int i = 0;
    while (*pos && *pos != '"' && i < 15) {
        key_name[i++] = *pos++;
    }

    /* Map key name to IBus keyval */
    struct { const char *name; guint val; } map[] = {
        {"F1",  IBUS_KEY_F1},  {"F2",  IBUS_KEY_F2},  {"F3",  IBUS_KEY_F3},
        {"F4",  IBUS_KEY_F4},  {"F5",  IBUS_KEY_F5},  {"F6",  IBUS_KEY_F6},
        {"F7",  IBUS_KEY_F7},  {"F8",  IBUS_KEY_F8},  {"F9",  IBUS_KEY_F9},
        {"F10", IBUS_KEY_F10}, {"F11", IBUS_KEY_F11}, {"F12", IBUS_KEY_F12},
    };
    for (size_t j = 0; j < sizeof(map) / sizeof(map[0]); j++) {
        if (strcmp(key_name, map[j].name) == 0) return map[j].val;
    }
    return IBUS_KEY_F12;
}

static void bangla_lekhi_ibus_engine_init(BanglaLekhiIBusEngine *self) {
    self->core = bangla_lekhi_engine_new();
    self->table = ibus_lookup_table_new(9, 0, TRUE, TRUE);
    g_object_ref_sink(self->table);
    ibus_lookup_table_set_orientation(self->table, IBUS_ORIENTATION_VERTICAL);
    self->is_composing = FALSE;
    self->alt_gr_active = FALSE;
    self->toggle_keyval = load_toggle_keyval();
    g_message("Bangla Lekhi: toggle key loaded as keyval 0x%04X", self->toggle_keyval);
}

static void bangla_lekhi_ibus_engine_finalize(GObject *object) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(object);

    if (self->core) {
        bangla_lekhi_engine_free(self->core);
        self->core = NULL;
    }

    if (self->table) {
        g_object_unref(self->table);
        self->table = NULL;
    }

    G_OBJECT_CLASS(bangla_lekhi_ibus_engine_parent_class)->finalize(object);
}

static void bangla_lekhi_ibus_engine_class_init(BanglaLekhiIBusEngineClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);

    object_class->finalize = bangla_lekhi_ibus_engine_finalize;

    engine_class->process_key_event = engine_process_key_event;
    engine_class->focus_in = engine_focus_in;
    engine_class->focus_out = engine_focus_out;
    engine_class->reset = engine_reset;
    engine_class->enable = engine_enable;
    engine_class->disable = engine_disable;
    engine_class->candidate_clicked = engine_candidate_clicked;
}

void bangla_lekhi_ibus_engine_register_type(IBusFactory *factory) {
    ibus_factory_add_engine(factory, "bangla-lekhi", BANGLA_LEKHI_TYPE_ENGINE);
}

/* Helper: update pre-edit string and lookup table candidates */
static void update_preedit_and_lookup_table(BanglaLekhiIBusEngine *self) {
    IBusEngine *engine = IBUS_ENGINE(self);

    if (!bangla_lekhi_engine_has_composition(self->core)) {
        reset_engine_state(self);
        return;
    }

    self->is_composing = TRUE;

    // 1. Update candidate table
    ibus_lookup_table_clear(self->table);
    size_t count = bangla_lekhi_engine_get_candidate_count(self->core);

    for (size_t i = 0; i < count; i++) {
        const char *cand = bangla_lekhi_engine_get_candidate_at(self->core, i);
        if (cand && *cand) {
            IBusText *cand_text = ibus_text_new_from_string(cand);
            ibus_lookup_table_append_candidate(self->table, cand_text);
        }
    }

    guint cursor_pos = ibus_lookup_table_get_cursor_pos(self->table);

    // 2. Determine pre-edit display text
    const char *preedit_str = NULL;
    if (count > 0 && cursor_pos < count) {
        preedit_str = bangla_lekhi_engine_get_candidate_at(self->core, cursor_pos);
    }
    if (!preedit_str) {
        preedit_str = bangla_lekhi_engine_get_preedit(self->core);
    }

    if (preedit_str && *preedit_str) {
        IBusText *preedit_text = ibus_text_new_from_string(preedit_str);
        // Show with standard underline attribute
        ibus_text_append_attribute(preedit_text, IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, -1);
        ibus_engine_update_preedit_text(engine, preedit_text, ibus_text_get_length(preedit_text), TRUE);
    }

    // 3. Display lookup table if there are multiple candidates
    if (count > 1) {
        ibus_engine_update_lookup_table_fast(engine, self->table, TRUE);
    } else {
        ibus_engine_hide_lookup_table(engine);
    }
}

/* Helper: commit current composition into client application */
static void commit_current_composition(BanglaLekhiIBusEngine *self, guint candidate_idx) {
    if (!bangla_lekhi_engine_has_composition(self->core)) {
        return;
    }

    const char *committed = bangla_lekhi_engine_commit_candidate(self->core, candidate_idx);
    if (committed && *committed) {
        IBusText *commit_text = ibus_text_new_from_string(committed);
        ibus_engine_commit_text(IBUS_ENGINE(self), commit_text);
    }

    reset_engine_state(self);
}

/* Helper: clear engine pre-edit and candidate table */
static void reset_engine_state(BanglaLekhiIBusEngine *self) {
    IBusEngine *engine = IBUS_ENGINE(self);
    bangla_lekhi_engine_clear(self->core);
    self->is_composing = FALSE;
    ibus_lookup_table_clear(self->table);
    ibus_engine_hide_preedit_text(engine);
    ibus_engine_hide_lookup_table(engine);
}

/* Key Event Handler */
static gboolean engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint state) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);

    // Track AltGr state
    if (keyval == IBUS_KEY_Alt_R || keyval == IBUS_KEY_ISO_Level3_Shift) {
        self->alt_gr_active = !(state & IBUS_RELEASE_MASK);
    }

    // Ignore key release events
    if (state & IBUS_RELEASE_MASK) {
        return FALSE;
    }

    // Global Mode Toggle (configurable, default: F12)
    if (keyval == self->toggle_keyval) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            commit_current_composition(self, 0);
        }
        BanglaLekhiMode new_mode = bangla_lekhi_engine_toggle_mode(self->core);
        g_message("Bangla Lekhi: Toggled mode to %s", new_mode == BANGLA_LEKHI_MODE_BANGLA ? "Bangla" : "English");
        return TRUE;
    }

    // If English mode is active, pass all keys through directly
    if (bangla_lekhi_engine_get_mode(self->core) == BANGLA_LEKHI_MODE_ENGLISH) {
        return FALSE;
    }

    // When modifiers like Ctrl or Alt are pressed (e.g. Ctrl+C, Ctrl+V, Alt+Tab)
    gboolean has_ctrl = (state & IBUS_CONTROL_MASK) != 0;
    gboolean has_alt = (state & IBUS_MOD1_MASK) != 0;
    if ((has_ctrl && !has_alt) || (!has_ctrl && has_alt && !self->alt_gr_active)) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            commit_current_composition(self, 0);
        }
        return FALSE;
    }

    // Handle BackSpace
    if (keyval == IBUS_KEY_BackSpace) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            gboolean remaining = bangla_lekhi_engine_pop_char(self->core);
            if (remaining) {
                update_preedit_and_lookup_table(self);
            } else {
                reset_engine_state(self);
            }
            return TRUE;
        }
        return FALSE;
    }

    // Handle Escape
    if (keyval == IBUS_KEY_Escape) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            reset_engine_state(self);
            return TRUE;
        }
        return FALSE;
    }

    // Handle Space
    if (keyval == IBUS_KEY_space) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            guint cursor_pos = ibus_lookup_table_get_cursor_pos(self->table);
            commit_current_composition(self, cursor_pos);
            // Return FALSE to let standard space follow the committed word
            return FALSE;
        }
        return FALSE;
    }

    // Handle Return / Enter
    if (keyval == IBUS_KEY_Return || keyval == IBUS_KEY_KP_Enter) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            guint cursor_pos = ibus_lookup_table_get_cursor_pos(self->table);
            commit_current_composition(self, cursor_pos);
            return TRUE;
        }
        return FALSE;
    }

    // Handle Candidate Navigation (Arrow Up / Down / Tab)
    if (bangla_lekhi_engine_has_composition(self->core) &&
        bangla_lekhi_engine_get_candidate_count(self->core) > 1) {
        if (keyval == IBUS_KEY_Down || keyval == IBUS_KEY_Tab) {
            ibus_lookup_table_cursor_down(self->table);
            update_preedit_and_lookup_table(self);
            return TRUE;
        }
        if (keyval == IBUS_KEY_Up) {
            ibus_lookup_table_cursor_up(self->table);
            update_preedit_and_lookup_table(self);
            return TRUE;
        }
    }

    // Handle Number selection (1..9) when candidate table is displayed
    if (bangla_lekhi_engine_has_composition(self->core) &&
        bangla_lekhi_engine_get_candidate_count(self->core) > 1 &&
        keyval >= IBUS_KEY_1 && keyval <= IBUS_KEY_9) {
        guint selected_idx = keyval - IBUS_KEY_1;
        if (selected_idx < bangla_lekhi_engine_get_candidate_count(self->core)) {
            commit_current_composition(self, selected_idx);
            return TRUE;
        }
    }

    // Handle Printable ASCII Characters (32 < keyval < 127)
    if (keyval > 32 && keyval < 127) {
        char ch = (char)keyval;
        bangla_lekhi_engine_push_char(self->core, ch);
        update_preedit_and_lookup_table(self);
        return TRUE;
    }

    return FALSE;
}

static void engine_focus_in(IBusEngine *engine) {
    // Focus gained; do not reset silently if user was typing
}

static void engine_focus_out(IBusEngine *engine) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    if (bangla_lekhi_engine_has_composition(self->core)) {
        // Automatically commit pending composition on focus out so text isn't lost
        commit_current_composition(self, 0);
    }
    reset_engine_state(self);
}

static void engine_reset(IBusEngine *engine) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    reset_engine_state(self);
}

static void engine_enable(IBusEngine *engine) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    reset_engine_state(self);
}

static void engine_disable(IBusEngine *engine) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    if (bangla_lekhi_engine_has_composition(self->core)) {
        commit_current_composition(self, 0);
    }
    reset_engine_state(self);
}

static void engine_candidate_clicked(IBusEngine *engine, guint index, guint button, guint state) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    if (button == 1) { // Left click
        commit_current_composition(self, index);
    }
}
