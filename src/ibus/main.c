/*
 * Bangla Lekhi IBus Daemon Entry Point
 *
 * Copyright (C) 2026 Bangla Lekhi Contributors
 * Licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ibus.h>
#include "engine.h"

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;
static gboolean ibus_mode = FALSE;

static const GOptionEntry entries[] = {
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus_mode, "Run as an IBus component process", NULL },
    { NULL }
};

static void sigterm_handler(int sig) {
    g_message("Bangla Lekhi: Received signal %d, exiting cleanly.", sig);
    ibus_quit();
}

int main(int argc, char **argv) {
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("- Native Bengali Phonetic IBus Engine");
    g_option_context_add_main_entries(context, entries, "bangla-lekhi");
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("Failed to parse command-line arguments: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return EXIT_FAILURE;
    }
    g_option_context_free(context);

    // Setup signal handling
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    // Initialize IBus
    ibus_init();

    bus = ibus_bus_new();
    if (!bus) {
        g_printerr("Bangla Lekhi: Failed to connect to IBus daemon.\n");
        return EXIT_FAILURE;
    }

    if (!ibus_bus_is_connected(bus)) {
        g_printerr("Bangla Lekhi: IBus bus is not connected.\n");
        g_object_unref(bus);
        return EXIT_FAILURE;
    }

    g_signal_connect(bus, "disconnected", G_CALLBACK(ibus_quit), NULL);

    // Create factory and register Bangla Lekhi engine type
    factory = ibus_factory_new(ibus_bus_get_connection(bus));
    bangla_lekhi_ibus_engine_register_type(factory);

    // Request component name if running as standalone component
    if (ibus_mode) {
        ibus_bus_request_name(bus, "org.freedesktop.IBus.BanglaLekhi", 0);
    } else {
        // Standalone debug execution
        IBusComponent *component = ibus_component_new(
            "org.freedesktop.IBus.BanglaLekhi",
            "Bangla Lekhi Input Method",
            "1.0.0",
            "GPL-3.0-or-later",
            "Bangla Lekhi Contributors",
            "https://github.com/bangla-lekhi",
            "",
            "bangla-lekhi"
        );
        ibus_component_add_engine(
            component,
            ibus_engine_desc_new(
                "bangla-lekhi",
                "Bangla Lekhi",
                "Native Bengali Phonetic Input Method (Avro scheme)",
                "bn",
                "GPL-3.0-or-later",
                "Bangla Lekhi Contributors",
                "/usr/share/icons/hicolor/scalable/apps/bangla-lekhi.svg",
                "default"
            )
        );
        ibus_bus_register_component(bus, component);
        g_object_unref(component);
    }

    g_message("Bangla Lekhi IBus Engine started successfully.");

    // Run GLib main loop
    ibus_main();

    // Clean up
    if (factory) {
        g_object_unref(factory);
    }
    if (bus) {
        g_object_unref(bus);
    }

    return EXIT_SUCCESS;
}
