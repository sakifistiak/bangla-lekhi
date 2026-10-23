//! Word suggestion and candidate generation module

use crate::phonetic::PhoneticParser;
use std::sync::Mutex;
use upodesh::avro::Suggest;

pub struct SuggestionEngine {
    suggest: Mutex<Suggest>,
    phonetic: PhoneticParser,
}

impl SuggestionEngine {
    pub fn new() -> Self {
        Self {
            suggest: Mutex::new(Suggest::new()),
            phonetic: PhoneticParser::new(),
        }
    }

    /// Generate candidate suggestions for a given phonetic Latin input string.
    pub fn get_candidates(&self, input: &str) -> Vec<String> {
        if input.is_empty() {
            return Vec::new();
        }

        let mut results = Vec::new();

        // 1. Direct phonetic transliteration
        let direct = self.phonetic.parse(input);

        // 2. Dictionary suggestions via upodesh
        let dict_suggestions = {
            let suggest = self.suggest.lock().unwrap();
            suggest.suggest(input)
        };

        if !dict_suggestions.is_empty() {
            // Prioritize direct match if present in dictionary suggestions
            if !direct.is_empty() && dict_suggestions.contains(&direct) {
                results.push(direct.clone());
            }

            for item in &dict_suggestions {
                if !results.contains(item) {
                    results.push(item.clone());
                }
            }

            // Ensure direct phonetic conversion is available as an option
            if !direct.is_empty() && !results.contains(&direct) {
                results.push(direct);
            }
        } else if !direct.is_empty() {
            results.push(direct);
        }

        results
    }
}

impl Default for SuggestionEngine {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_candidate_generation() {
        let engine = SuggestionEngine::new();

        let c_ami = engine.get_candidates("ami");
        assert!(!c_ami.is_empty());
        assert_eq!(c_ami[0], "আমি");

        let c_bangla = engine.get_candidates("bangla");
        assert!(!c_bangla.is_empty());
        assert!(c_bangla.contains(&"বাংলা".to_string()));

        let c_bhalobashi = engine.get_candidates("bhalobashi");
        assert!(!c_bhalobashi.is_empty());
        assert!(c_bhalobashi.contains(&"ভালোবাসি".to_string()));
    }
}
