#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/engine/include/bangla_lekhi_core.h"

int main() {
    printf("Testing C-ABI bindings...\n");

    // 1. Standalone transliteration
    char *trans = bangla_lekhi_transliterate("bangladesh");
    assert(trans != NULL);
    assert(strcmp(trans, "বাংলাদেশ") == 0);
    printf("  [PASS] Transliterate 'bangladesh' -> '%s'\n", trans);
    bangla_lekhi_string_free(trans);

    // 2. Engine lifecycle
    BanglaLekhiEngine *engine = bangla_lekhi_engine_new();
    assert(engine != NULL);
    assert(bangla_lekhi_engine_get_mode(engine) == BANGLA_LEKHI_MODE_BANGLA);
    assert(!bangla_lekhi_engine_has_composition(engine));

    // 3. Typing keystrokes: 'a', 'm', 'i'
    bangla_lekhi_engine_push_char(engine, 'a');
    bangla_lekhi_engine_push_char(engine, 'm');
    bangla_lekhi_engine_push_char(engine, 'i');
    assert(bangla_lekhi_engine_has_composition(engine));
    assert(strcmp(bangla_lekhi_engine_get_raw_buffer(engine), "ami") == 0);

    const char *preedit = bangla_lekhi_engine_get_preedit(engine);
    assert(strcmp(preedit, "আমি") == 0);
    printf("  [PASS] Preedit for 'ami' -> '%s'\n", preedit);

    size_t count = bangla_lekhi_engine_get_candidate_count(engine);
    assert(count >= 1);
    const char *c0 = bangla_lekhi_engine_get_candidate_at(engine, 0);
    assert(strcmp(c0, "আমি") == 0);
    printf("  [PASS] Candidate 0 -> '%s' (total candidates: %zu)\n", c0, count);

    // 4. Commit candidate
    const char *committed = bangla_lekhi_engine_commit_candidate(engine, 0);
    assert(strcmp(committed, "আমি") == 0);
    assert(!bangla_lekhi_engine_has_composition(engine));
    printf("  [PASS] Committed text -> '%s'\n", committed);

    // 5. Backspace handling
    bangla_lekhi_engine_push_char(engine, 'b');
    bangla_lekhi_engine_push_char(engine, 'a');
    assert(strcmp(bangla_lekhi_engine_get_raw_buffer(engine), "ba") == 0);
    bool has_more = bangla_lekhi_engine_pop_char(engine);
    assert(has_more == true);
    assert(strcmp(bangla_lekhi_engine_get_raw_buffer(engine), "b") == 0);
    has_more = bangla_lekhi_engine_pop_char(engine);
    assert(has_more == false);
    assert(!bangla_lekhi_engine_has_composition(engine));
    printf("  [PASS] Backspace handling verified\n");

    // 6. Mode toggle
    BanglaLekhiMode mode = bangla_lekhi_engine_toggle_mode(engine);
    assert(mode == BANGLA_LEKHI_MODE_ENGLISH);
    bangla_lekhi_engine_push_char(engine, 'x');
    assert(!bangla_lekhi_engine_has_composition(engine)); // No composition in English mode
    mode = bangla_lekhi_engine_toggle_mode(engine);
    assert(mode == BANGLA_LEKHI_MODE_BANGLA);
    printf("  [PASS] Mode toggling verified\n");

    bangla_lekhi_engine_free(engine);
    printf("All C-ABI unit tests PASSED successfully!\n");
    return 0;
}
