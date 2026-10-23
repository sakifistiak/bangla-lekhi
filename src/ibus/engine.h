/*
 * Bangla Lekhi IBus Engine Header
 *
 * Copyright (C) 2026 Bangla Lekhi Contributors
 * Licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later)
 */

#ifndef BANGLA_LEKHI_IBUS_ENGINE_H
#define BANGLA_LEKHI_IBUS_ENGINE_H

#include <ibus.h>
#include "../engine/include/bangla_lekhi_core.h"

G_BEGIN_DECLS

#ifndef glib_autoptr_clear_IBusEngine
G_DEFINE_AUTOPTR_CLEANUP_FUNC(IBusEngine, g_object_unref)
#endif

#define BANGLA_LEKHI_TYPE_ENGINE (bangla_lekhi_ibus_engine_get_type())

G_DECLARE_FINAL_TYPE(BanglaLekhiIBusEngine, bangla_lekhi_ibus_engine, BANGLA_LEKHI, ENGINE, IBusEngine)

struct _BanglaLekhiIBusEngine {
    IBusEngine parent;

    /* Core Phonetic Transliteration Engine */
    BanglaLekhiEngine *core;

    /* Candidate Lookup Table */
    IBusLookupTable *table;

    /* Auxiliary status and state */
    gboolean is_composing;
    gboolean alt_gr_active;

    /* Runtime config: toggle key (IBus keyval, default IBUS_KEY_F12) */
    guint toggle_keyval;
};

void bangla_lekhi_ibus_engine_register_type(IBusFactory *factory);

G_END_DECLS

#endif /* BANGLA_LEKHI_IBUS_ENGINE_H */
