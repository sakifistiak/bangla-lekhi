use bangla_lekhi_core::{convert_phonetic, BanglaLekhiEngine, SuggestionEngine};
use serde::Deserialize;
use std::fs;
use std::path::Path;

#[derive(Deserialize, Debug)]
struct TestCase {
    input: String,
    expected: String,
}

#[derive(Deserialize, Debug)]
struct CorpusCategory {
    category: String,
    cases: Vec<TestCase>,
}

#[test]
fn test_corpus_regression_suite() {
    let corpus_path = Path::new("../../tests/phonetic-corpus/corpus.json");
    assert!(corpus_path.exists(), "Corpus file must exist at {:?}", corpus_path);

    let content = fs::read_to_string(corpus_path).expect("Failed to read corpus file");
    let categories: Vec<CorpusCategory> = serde_json::from_str(&content).expect("Failed to parse corpus JSON");

    let suggestion_engine = SuggestionEngine::new();
    let mut total_cases = 0;
    let mut passed_cases = 0;

    for cat in &categories {
        println!("\n=== Testing Category: {} ({} cases) ===", cat.category, cat.cases.len());
        for tc in &cat.cases {
            total_cases += 1;

            let direct = convert_phonetic(&tc.input);
            let candidates = suggestion_engine.get_candidates(&tc.input);

            // A test passes if either direct transliteration matches expected,
            // or the primary candidate matches expected, or expected is within candidates.
            let matches_direct = direct == tc.expected;
            let matches_candidate = candidates.first().map(|s| s.as_str()) == Some(tc.expected.as_str())
                || candidates.contains(&tc.expected);

            if matches_direct || matches_candidate {
                passed_cases += 1;
            } else {
                eprintln!(
                    "FAILED: input '{}' | expected '{}' | direct was '{}' | candidates were {:?}",
                    tc.input, tc.expected, direct, candidates
                );
                panic!(
                    "Corpus case failed in [{}]: input '{}', expected '{}', got direct '{}', candidates {:?}",
                    cat.category, tc.input, tc.expected, direct, candidates
                );
            }
        }
    }

    println!("\nCorpus Regression Test: {}/{} passed (100%)", passed_cases, total_cases);
    assert_eq!(passed_cases, total_cases);
}

#[test]
fn test_unicode_normalization_and_boundaries() {
    let mut engine = BanglaLekhiEngine::new();

    // 1. Empty input
    assert_eq!(convert_phonetic(""), "");
    assert!(!engine.has_composition());

    // 2. Spaces and word boundaries
    engine.push_char('a');
    engine.push_char('m');
    engine.push_char('i');
    assert_eq!(engine.commit_candidate(0), "আমি");
    assert!(!engine.has_composition());

    engine.push_char('b');
    engine.push_char('a');
    engine.push_char('n');
    engine.push_char('g');
    engine.push_char('l');
    engine.push_char('a');
    assert!(engine.candidate_count() > 0);
    // "বাংলা" must be present somewhere in the candidate list
    // (position 0 may vary based on user-word entries on this machine)
    assert!(engine.candidates().contains(&"বাংলা".to_string()));
    // Commit whichever candidate contains "বাংলা" — find its index
    let bangla_idx = engine.candidates()
        .iter()
        .position(|c| c == "বাংলা")
        .expect("বাংলা must be in candidates");
    assert_eq!(engine.commit_candidate(bangla_idx), "বাংলা");

    // 3. Backspace boundary (pop on empty buffer)
    assert!(!engine.pop_char());
    assert!(!engine.has_composition());
}
