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
static gboolean xml_mode  = FALSE;

static const GOptionEntry entries[] = {
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus_mode,
      "Run as an IBus component process", NULL },
    { "xml",  0,  0, G_OPTION_ARG_NONE, &xml_mode,
      "Print component XML and exit (used by ibus write-cache)", NULL },
    { NULL }
};

static void sigterm_handler(int sig) {
    g_message("Bangla Lekhi: Received signal %d, exiting cleanly.", sig);
    ibus_quit();
}

/**
 * Print the component XML to stdout so IBus write-cache can index our engine.
 * IBus calls: bangla-lekhi-engine --xml
 */
static int print_xml_and_exit(void) {
    const char *home = getenv("HOME");
    char local_path[512] = {0};
    if (home) {
        snprintf(local_path, sizeof(local_path),
                 "%s/.local/share/ibus/component/bangla-lekhi.xml", home);
        FILE *f = fopen(local_path, "r");
        if (f) {
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                fwrite(buf, 1, n, stdout);
            fclose(f);
            return EXIT_SUCCESS;
        }
    }

    const char *sys_path = "/usr/share/ibus/component/bangla-lekhi.xml";
    FILE *f = fopen(sys_path, "r");
    if (f) {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
            fwrite(buf, 1, n, stdout);
        fclose(f);
        return EXIT_SUCCESS;
    }

    const char *exec_path = home
        ? g_strdup_printf("%s/.local/bin/bangla-lekhi-engine", home)
        : "/usr/lib/bangla-lekhi/bangla-lekhi-engine";

    printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    printf("<component>\n");
    printf("    <name>org.freedesktop.IBus.BanglaLekhi</name>\n");
    printf("    <description>Bangla Lekhi Input Method</description>\n");
    printf("    <exec>%s --ibus</exec>\n", exec_path);
    printf("    <version>1.0.0</version>\n");
    printf("    <author>Bangla Lekhi Contributors</author>\n");
    printf("    <license>GPL-3.0-or-later</license>\n");
    printf("    <homepage>https://github.com/bangla-lekhi</homepage>\n");
    printf("    <textdomain>bangla-lekhi</textdomain>\n");
    printf("    <engines>\n");
    printf("        <engine>\n");
    printf("            <name>bangla-lekhi</name>\n");
    printf("            <language>bn</language>\n");
    printf("            <license>GPL-3.0-or-later</license>\n");
    printf("            <author>Bangla Lekhi Contributors</author>\n");
    printf("            <icon>bangla-lekhi</icon>\n");
    printf("            <layout>default</layout>\n");
    printf("            <longname>Bangla Lekhi (Avro Phonetic)</longname>\n");
    printf("            <description>Native Bengali Phonetic Input Method for Linux</description>\n");
    printf("            <rank>99</rank>\n");
    printf("        </engine>\n");
    printf("    </engines>\n");
    printf("</component>\n");
    return EXIT_SUCCESS;
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

    if (xml_mode) {
        return print_xml_and_exit();
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

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

    factory = ibus_factory_new(ibus_bus_get_connection(bus));
    bangla_lekhi_ibus_engine_register_type(factory);

    if (ibus_mode) {
        ibus_bus_request_name(bus, "org.freedesktop.IBus.BanglaLekhi", 0);
    } else {
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
                "bangla-lekhi",
                "default"
            )
        );
        ibus_bus_register_component(bus, component);
        g_object_unref(component);
    }

    g_message("Bangla Lekhi IBus Engine started successfully.");

    ibus_main();

    if (factory) {
        g_object_unref(factory);
    }
    if (bus) {
        g_object_unref(bus);
    }

    return EXIT_SUCCESS;
}
