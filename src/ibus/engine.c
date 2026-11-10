/*
 * Bangla Lekhi IBus Engine Implementation
 *
 * Copyright (C) 2026 Bangla Lekhi Contributors
 * Licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later)
 */

#include "engine.h"
#include <gio/gio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIPBOARD_HISTORY_LIMIT 9
#define CLIPBOARD_ENTRY_MAX_BYTES 4096

typedef struct _GdkDisplay GdkDisplay;
typedef struct _GdkClipboard GdkClipboard;
typedef gboolean (*GtkInitCheckFunc)(void);
typedef GdkDisplay *(*GdkDisplayGetDefaultFunc)(void);
typedef GdkClipboard *(*GdkDisplayGetClipboardFunc)(GdkDisplay *display);
typedef void (*GdkClipboardReadTextAsyncFunc)(
    GdkClipboard *clipboard,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data
);
typedef gchar *(*GdkClipboardReadTextFinishFunc)(
    GdkClipboard *clipboard,
    GAsyncResult *result,
    GError **error
);

G_DEFINE_TYPE(BanglaLekhiIBusEngine, bangla_lekhi_ibus_engine, IBUS_TYPE_ENGINE)

static GPtrArray *clipboard_history;
static GdkDisplay *clipboard_display;
static gboolean clipboard_monitor_started;
static gboolean clipboard_sensitive_context;
static gboolean clipboard_read_pending;
static void *gtk4_library;
static GtkInitCheckFunc gtk_init_check_func;
static GdkDisplayGetDefaultFunc gdk_display_get_default_func;
static GdkDisplayGetClipboardFunc gdk_display_get_clipboard_func;
static GdkClipboardReadTextAsyncFunc gdk_clipboard_read_text_async_func;
static GdkClipboardReadTextFinishFunc gdk_clipboard_read_text_finish_func;

/* Forward declarations */
static gboolean engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint state);
static void engine_focus_in(IBusEngine *engine);
static void engine_focus_out(IBusEngine *engine);
static void engine_reset(IBusEngine *engine);
static void engine_enable(IBusEngine *engine);
static void engine_disable(IBusEngine *engine);
static void engine_candidate_clicked(IBusEngine *engine, guint index, guint button, guint state);
static void engine_set_surrounding_text(IBusEngine *engine, IBusText *text, guint cursor_pos, guint anchor_pos);
static void engine_set_content_type(IBusEngine *engine, guint purpose, guint hints);

static void update_preedit_and_lookup_table(BanglaLekhiIBusEngine *self);
static void commit_current_composition(BanglaLekhiIBusEngine *self, guint candidate_idx);
static void reset_engine_state(BanglaLekhiIBusEngine *self);
static void show_clipboard_history(BanglaLekhiIBusEngine *self);
static void commit_clipboard_history(BanglaLekhiIBusEngine *self, guint index);
static void close_clipboard_history(BanglaLekhiIBusEngine *self);
static void capture_selected_text(BanglaLekhiIBusEngine *self);
static void add_clipboard_history(const gchar *text);

static void clipboard_text_ready(GObject *source_object, GAsyncResult *result, gpointer user_data) {
    BanglaLekhiIBusEngine *self = user_data;
    GError *error = NULL;
    gchar *text = gdk_clipboard_read_text_finish_func(
        (GdkClipboard *)source_object, result, &error);
    if (error) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Bangla Lekhi: Could not read clipboard text: %s", error->message);
        }
        g_error_free(error);
        if (self) {
            clipboard_read_pending = FALSE;
            if (self->showing_clipboard_history) {
                IBusText *message = ibus_text_new_from_static_string("Clipboard unavailable; copy text first");
                ibus_engine_update_auxiliary_text(IBUS_ENGINE(self), message, TRUE);
            }
            g_object_unref(self);
        }
        return;
    }

    if (!clipboard_sensitive_context && (!self || !self->sensitive_context) &&
        text && *text &&
        strlen(text) <= CLIPBOARD_ENTRY_MAX_BYTES && g_utf8_validate(text, -1, NULL)) {
        add_clipboard_history(text);
    }
    g_free(text);
    if (self) {
        clipboard_read_pending = FALSE;
        if (self->showing_clipboard_history) {
            if (clipboard_history->len > 0) {
                show_clipboard_history(self);
            } else {
                IBusText *message = ibus_text_new_from_static_string("No clipboard text; copy text first");
                ibus_engine_update_auxiliary_text(IBUS_ENGINE(self), message, TRUE);
            }
        }
        g_object_unref(self);
    }
}

static void clipboard_changed(GdkClipboard *clipboard, gpointer user_data) {
    (void)user_data;
    if (!clipboard_sensitive_context) {
        gdk_clipboard_read_text_async_func(clipboard, NULL, clipboard_text_ready, NULL);
    }
}

static void start_clipboard_monitor(void) {
    if (clipboard_monitor_started) {
        return;
    }

    gtk4_library = dlopen("libgtk-4.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!gtk4_library) {
        g_warning("Bangla Lekhi: Could not load GTK4 clipboard support: %s", dlerror());
        return;
    }

    gtk_init_check_func = (GtkInitCheckFunc)dlsym(gtk4_library, "gtk_init_check");
    gdk_display_get_default_func =
        (GdkDisplayGetDefaultFunc)dlsym(gtk4_library, "gdk_display_get_default");
    gdk_display_get_clipboard_func =
        (GdkDisplayGetClipboardFunc)dlsym(gtk4_library, "gdk_display_get_clipboard");
    gdk_clipboard_read_text_async_func =
        (GdkClipboardReadTextAsyncFunc)dlsym(gtk4_library, "gdk_clipboard_read_text_async");
    gdk_clipboard_read_text_finish_func =
        (GdkClipboardReadTextFinishFunc)dlsym(gtk4_library, "gdk_clipboard_read_text_finish");
    if (!gtk_init_check_func || !gdk_display_get_default_func || !gdk_display_get_clipboard_func ||
        !gdk_clipboard_read_text_async_func || !gdk_clipboard_read_text_finish_func) {
        g_warning("Bangla Lekhi: GTK4 is missing required clipboard functions");
        dlclose(gtk4_library);
        gtk4_library = NULL;
        return;
    }

    if (!gtk_init_check_func()) {
        g_warning("Bangla Lekhi: GTK4 could not initialize the desktop display");
        return;
    }
    clipboard_display = gdk_display_get_default_func();
    if (!clipboard_display) {
        g_warning("Bangla Lekhi: Could not connect to the desktop clipboard display");
        return;
    }

    clipboard_monitor_started = TRUE;
    GdkClipboard *clipboard = gdk_display_get_clipboard_func(clipboard_display);
    g_signal_connect(clipboard, "changed", G_CALLBACK(clipboard_changed), NULL);
    g_message("Bangla Lekhi: desktop clipboard history monitor is ready");
}

static void add_clipboard_history(const gchar *text) {
    if (!clipboard_history) {
        clipboard_history = g_ptr_array_new_with_free_func(g_free);
    }

    for (guint i = 0; i < clipboard_history->len; i++) {
        if (g_strcmp0(g_ptr_array_index(clipboard_history, i), text) == 0) {
            g_ptr_array_remove_index(clipboard_history, i);
            break;
        }
    }

    g_ptr_array_insert(clipboard_history, 0, g_strdup(text));
    if (clipboard_history->len > CLIPBOARD_HISTORY_LIMIT) {
        g_ptr_array_remove_index(clipboard_history, clipboard_history->len - 1);
    }
}

/* ── Config helpers (minimal JSON reader — no external deps) ─────────────── */

/**
 * Read a function-key toggle or Ctrl+Space from settings.json.
 */
static void load_toggle_key(guint *keyval, gboolean *ctrl) {
    *keyval = IBUS_KEY_F12;
    *ctrl = FALSE;

    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char path[512] = {0};

    if (xdg_config) {
        snprintf(path, sizeof(path), "%s/bangla-lekhi/settings.json", xdg_config);
    } else if (home) {
        snprintf(path, sizeof(path), "%s/.config/bangla-lekhi/settings.json", home);
    } else {
        return;
    }

    FILE *f = fopen(path, "r");
    if (!f) return;

    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    if (n == 0) return;

    /* Minimal "toggle_key" extraction — look for "toggle_key": "FN" */
    char *pos = strstr(buf, "\"toggle_key\"");
    if (!pos) return;
    pos = strchr(pos, ':');
    if (!pos) return;
    pos++;
    while (*pos == ' ' || *pos == '\t') pos++;
    if (*pos != '"') return;
    pos++; /* skip opening quote */

    char key_name[32] = {0};
    int i = 0;
    while (*pos && *pos != '"' && i < (int)sizeof(key_name) - 1) {
        key_name[i++] = *pos++;
    }

    if (g_ascii_strcasecmp(key_name, "Ctrl+Space") == 0 ||
        g_ascii_strcasecmp(key_name, "Control+Space") == 0) {
        *keyval = IBUS_KEY_space;
        *ctrl = TRUE;
        return;
    }

    /* Map key name to IBus keyval */
    struct { const char *name; guint val; } map[] = {
        {"F1",  IBUS_KEY_F1},  {"F2",  IBUS_KEY_F2},  {"F3",  IBUS_KEY_F3},
        {"F4",  IBUS_KEY_F4},  {"F5",  IBUS_KEY_F5},  {"F6",  IBUS_KEY_F6},
        {"F7",  IBUS_KEY_F7},  {"F8",  IBUS_KEY_F8},  {"F9",  IBUS_KEY_F9},
        {"F10", IBUS_KEY_F10}, {"F11", IBUS_KEY_F11}, {"F12", IBUS_KEY_F12},
    };
    for (size_t j = 0; j < sizeof(map) / sizeof(map[0]); j++) {
        if (strcmp(key_name, map[j].name) == 0) {
            *keyval = map[j].val;
            return;
        }
    }
}

static void bangla_lekhi_ibus_engine_init(BanglaLekhiIBusEngine *self) {
    self->core = bangla_lekhi_engine_new();
    self->table = ibus_lookup_table_new(9, 0, TRUE, TRUE);
    g_object_ref_sink(self->table);
    ibus_lookup_table_set_orientation(self->table, IBUS_ORIENTATION_VERTICAL);
    self->is_composing = FALSE;
    self->alt_gr_active = FALSE;
    self->showing_clipboard_history = FALSE;
    self->sensitive_context = FALSE;
    self->toggle_ctrl = FALSE;
    self->selected_text = NULL;
    if (!clipboard_history) {
        clipboard_history = g_ptr_array_new_with_free_func(g_free);
    }
    load_toggle_key(&self->toggle_keyval, &self->toggle_ctrl);
    g_message("Bangla Lekhi: toggle key loaded as keyval 0x%04X", self->toggle_keyval);
    start_clipboard_monitor();
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

    g_clear_pointer(&self->selected_text, g_free);

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
    engine_class->set_surrounding_text = engine_set_surrounding_text;
    engine_class->set_content_type = engine_set_content_type;
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
    guint cursor_pos = ibus_lookup_table_get_cursor_pos(self->table);
    ibus_lookup_table_clear(self->table);
    size_t count = bangla_lekhi_engine_get_candidate_count(self->core);

    for (size_t i = 0; i < count; i++) {
        const char *cand = bangla_lekhi_engine_get_candidate_at(self->core, i);
        if (cand && *cand) {
            IBusText *cand_text = ibus_text_new_from_string(cand);
            ibus_lookup_table_append_candidate(self->table, cand_text);
        }
    }

    cursor_pos = count > 0 ? MIN(cursor_pos, (guint)count - 1) : 0;
    ibus_lookup_table_set_cursor_pos(self->table, cursor_pos);

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
    g_clear_pointer(&self->selected_text, g_free);
    self->is_composing = FALSE;
    self->showing_clipboard_history = FALSE;
    ibus_lookup_table_clear(self->table);
    ibus_lookup_table_set_cursor_pos(self->table, 0);
    ibus_engine_hide_preedit_text(engine);
    ibus_engine_hide_lookup_table(engine);
    ibus_engine_hide_auxiliary_text(engine);
}

static void show_clipboard_history(BanglaLekhiIBusEngine *self) {
    IBusEngine *engine = IBUS_ENGINE(self);
    ibus_lookup_table_clear(self->table);

    if (self->sensitive_context) {
        IBusText *message = ibus_text_new_from_static_string("Clipboard history disabled in private fields");
        ibus_engine_update_auxiliary_text(engine, message, TRUE);
        self->showing_clipboard_history = FALSE;
        return;
    }

    if (!clipboard_history || clipboard_history->len == 0) {
        IBusText *message = ibus_text_new_from_static_string("Checking clipboard; copy text to add history");
        ibus_engine_update_auxiliary_text(engine, message, TRUE);
        self->showing_clipboard_history = TRUE;
        if (clipboard_monitor_started && !clipboard_read_pending) {
            clipboard_read_pending = TRUE;
            gdk_clipboard_read_text_async_func(
                gdk_display_get_clipboard_func(clipboard_display),
                NULL,
                clipboard_text_ready,
                g_object_ref(self)
            );
        }
        return;
    }

    self->showing_clipboard_history = TRUE;
    ibus_engine_hide_auxiliary_text(engine);
    ibus_engine_hide_preedit_text(engine);
    for (guint i = 0; i < clipboard_history->len; i++) {
        const gchar *entry = g_ptr_array_index(clipboard_history, i);
        IBusText *text = ibus_text_new_from_string(entry);
        ibus_lookup_table_append_candidate(self->table, text);
    }
    ibus_lookup_table_set_cursor_pos(self->table, 0);
    ibus_engine_update_lookup_table_fast(engine, self->table, TRUE);
}

static void close_clipboard_history(BanglaLekhiIBusEngine *self) {
    if (!self->showing_clipboard_history) {
        return;
    }

    self->showing_clipboard_history = FALSE;
    ibus_engine_hide_lookup_table(IBUS_ENGINE(self));
    ibus_engine_hide_auxiliary_text(IBUS_ENGINE(self));
}

static void commit_clipboard_history(BanglaLekhiIBusEngine *self, guint index) {
    if (!clipboard_history || index >= clipboard_history->len) {
        close_clipboard_history(self);
        return;
    }

    const gchar *entry = g_ptr_array_index(clipboard_history, index);
    IBusText *text = ibus_text_new_from_string(entry);
    ibus_engine_commit_text(IBUS_ENGINE(self), text);
    close_clipboard_history(self);
}

static void update_selected_text(BanglaLekhiIBusEngine *self, IBusText *text, guint cursor_pos, guint anchor_pos) {
    g_clear_pointer(&self->selected_text, g_free);

    if (self->sensitive_context || !text) {
        return;
    }

    const gchar *surrounding = ibus_text_get_text(text);
    if (!surrounding || cursor_pos == anchor_pos) {
        return;
    }

    glong char_count = g_utf8_strlen(surrounding, -1);
    guint start_pos = MIN(cursor_pos, anchor_pos);
    guint end_pos = MAX(cursor_pos, anchor_pos);
    if (end_pos > (guint)char_count) {
        return;
    }

    const gchar *start = g_utf8_offset_to_pointer(surrounding, start_pos);
    const gchar *end = g_utf8_offset_to_pointer(surrounding, end_pos);
    gsize byte_count = (gsize)(end - start);
    if (byte_count == 0 || byte_count > CLIPBOARD_ENTRY_MAX_BYTES) {
        return;
    }

    gchar *selection = g_strndup(start, byte_count);
    gchar *trimmed = g_strdup(selection);
    g_strstrip(trimmed);
    if (*trimmed) {
        self->selected_text = selection;
    } else {
        g_free(selection);
    }
    g_free(trimmed);
}

static void capture_selected_text(BanglaLekhiIBusEngine *self) {
    IBusText *text = NULL;
    guint cursor_pos = 0;
    guint anchor_pos = 0;
    ibus_engine_get_surrounding_text(IBUS_ENGINE(self), &text, &cursor_pos, &anchor_pos);
    update_selected_text(self, text, cursor_pos, anchor_pos);
}

static void engine_set_surrounding_text(IBusEngine *engine, IBusText *text, guint cursor_pos, guint anchor_pos) {
    update_selected_text(BANGLA_LEKHI_ENGINE(engine), text, cursor_pos, anchor_pos);
}

static void engine_set_content_type(IBusEngine *engine, guint purpose, guint hints) {
    BanglaLekhiIBusEngine *self = BANGLA_LEKHI_ENGINE(engine);
    self->sensitive_context =
        purpose == IBUS_INPUT_PURPOSE_PASSWORD ||
        purpose == IBUS_INPUT_PURPOSE_PIN ||
        (hints & (IBUS_INPUT_HINT_PRIVATE | IBUS_INPUT_HINT_HIDDEN_TEXT)) != 0;
    clipboard_sensitive_context = self->sensitive_context;
    if (self->sensitive_context) {
        g_clear_pointer(&self->selected_text, g_free);
    }
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

    gboolean has_ctrl = (state & IBUS_CONTROL_MASK) != 0;
    gboolean has_shift = (state & IBUS_SHIFT_MASK) != 0;
    gboolean has_alt = (state & IBUS_MOD1_MASK) != 0;

    // Global mode toggle (F-key or Ctrl+Space from Settings).
    gboolean toggle_pressed = keyval == self->toggle_keyval &&
        (self->toggle_ctrl
            ? has_ctrl && !has_shift && !has_alt
            : !has_ctrl && !has_alt);
    if (toggle_pressed) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            commit_current_composition(self, 0);
        }
        BanglaLekhiMode new_mode = bangla_lekhi_engine_toggle_mode(self->core);
        g_message("Bangla Lekhi: Toggled mode to %s", new_mode == BANGLA_LEKHI_MODE_BANGLA ? "Bangla" : "English");
        return TRUE;
    }

    if (has_ctrl && !has_alt && (keyval == IBUS_KEY_c || keyval == IBUS_KEY_C)) {
        capture_selected_text(self);
        if (!self->sensitive_context && self->selected_text) {
            add_clipboard_history(self->selected_text);
        }
        g_clear_pointer(&self->selected_text, g_free);
    }

    if (has_ctrl && has_shift && !has_alt &&
        (keyval == IBUS_KEY_v || keyval == IBUS_KEY_V)) {
        if (bangla_lekhi_engine_has_composition(self->core)) {
            commit_current_composition(self, 0);
        }
        show_clipboard_history(self);
        return TRUE;
    }

    if (self->showing_clipboard_history) {
        if (keyval == IBUS_KEY_Escape) {
            close_clipboard_history(self);
            return TRUE;
        }
        if (keyval == IBUS_KEY_Down || keyval == IBUS_KEY_Tab) {
            ibus_lookup_table_cursor_down(self->table);
            ibus_engine_update_lookup_table_fast(engine, self->table, TRUE);
            return TRUE;
        }
        if (keyval == IBUS_KEY_Up) {
            ibus_lookup_table_cursor_up(self->table);
            ibus_engine_update_lookup_table_fast(engine, self->table, TRUE);
            return TRUE;
        }
        if (keyval == IBUS_KEY_Return || keyval == IBUS_KEY_KP_Enter ||
            keyval == IBUS_KEY_space) {
            commit_clipboard_history(self, ibus_lookup_table_get_cursor_pos(self->table));
            return TRUE;
        }
        if (keyval >= IBUS_KEY_1 && keyval <= IBUS_KEY_9) {
            guint selected_idx = keyval - IBUS_KEY_1;
            if (clipboard_history && selected_idx < clipboard_history->len) {
                commit_clipboard_history(self, selected_idx);
                return TRUE;
            }
        }
        close_clipboard_history(self);
    }

    // If English mode is active, pass all keys through directly
    if (bangla_lekhi_engine_get_mode(self->core) == BANGLA_LEKHI_MODE_ENGLISH) {
        return FALSE;
    }

    // When modifiers like Ctrl or Alt are pressed (e.g. Ctrl+C, Ctrl+V, Alt+Tab)
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
                ibus_lookup_table_set_cursor_pos(self->table, 0);
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

    // Handle Printable ASCII Characters (32 < keyval < 127)
    if (keyval > 32 && keyval < 127) {
        char ch = (char)keyval;
        bangla_lekhi_engine_push_char(self->core, ch);
        ibus_lookup_table_set_cursor_pos(self->table, 0);
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
    ibus_engine_get_surrounding_text(engine, NULL, NULL, NULL);
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
        if (self->showing_clipboard_history) {
            commit_clipboard_history(self, index);
        } else {
            commit_current_composition(self, index);
        }
    }
}
