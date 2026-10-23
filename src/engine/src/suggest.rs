//! Word suggestion, autocorrect, and candidate generation module

use crate::phonetic::PhoneticParser;
use std::collections::HashMap;
use std::fs;
use std::path::PathBuf;
use std::sync::Mutex;
use upodesh::avro::Suggest;

pub struct SuggestionEngine {
    suggest: Mutex<Suggest>,
    phonetic: PhoneticParser,
    user_words: Mutex<Vec<String>>,
    autocorrect_map: Mutex<HashMap<String, String>>,
}

impl SuggestionEngine {
    pub fn new() -> Self {
        let engine = Self {
            suggest: Mutex::new(Suggest::new()),
            phonetic: PhoneticParser::new(),
            user_words: Mutex::new(Vec::new()),
            autocorrect_map: Mutex::new(HashMap::new()),
        };
        engine.load_user_data();
        engine
    }

    fn user_data_dir() -> PathBuf {
        if let Ok(xdg) = std::env::var("XDG_DATA_HOME") {
            PathBuf::from(xdg).join("bangla-lekhi")
        } else if let Ok(home) = std::env::var("HOME") {
            PathBuf::from(home).join(".local").join("share").join("bangla-lekhi")
        } else {
            PathBuf::from("/tmp/bangla-lekhi")
        }
    }

    /// Load user custom words and autocorrect entries from disk.
    pub fn load_user_data(&self) {
        let dir = Self::user_data_dir();
        let user_words_path = dir.join("user_words.json");
        let autocorrect_path = dir.join("autocorrect.json");

        if user_words_path.exists() {
            if let Ok(content) = fs::read_to_string(&user_words_path) {
                if let Ok(words) = serde_json::from_str::<Vec<String>>(&content) {
                    let mut lock = self.user_words.lock().unwrap();
                    *lock = words;
                }
            }
        }

        if autocorrect_path.exists() {
            if let Ok(content) = fs::read_to_string(&autocorrect_path) {
                if let Ok(map) = serde_json::from_str::<HashMap<String, String>>(&content) {
                    let mut lock = self.autocorrect_map.lock().unwrap();
                    *lock = map;
                }
            }
        }
    }

    /// Add a word to the user dictionary and save to disk.
    pub fn add_user_word(&self, word: &str) -> bool {
        if word.trim().is_empty() {
            return false;
        }
        let dir = Self::user_data_dir();
        if fs::create_dir_all(&dir).is_err() {
            return false;
        }

        let mut lock = self.user_words.lock().unwrap();
        if !lock.contains(&word.to_string()) {
            lock.push(word.to_string());
            let path = dir.join("user_words.json");
            if let Ok(json) = serde_json::to_string_pretty(&*lock) {
                let _ = fs::write(path, json);
            }
        }
        true
    }

    /// Generate candidate suggestions for a given phonetic Latin input string.
    pub fn get_candidates(&self, input: &str) -> Vec<String> {
        if input.is_empty() {
            return Vec::new();
        }

        let mut results = Vec::new();

        // 1. Direct phonetic transliteration
        let direct = self.phonetic.parse(input);

        // 2. Check autocorrect map
        {
            let ac_lock = self.autocorrect_map.lock().unwrap();
            if let Some(corrected) = ac_lock.get(&direct) {
                results.push(corrected.clone());
            } else if let Some(corrected) = ac_lock.get(input) {
                results.push(corrected.clone());
            }
        }

        // 3. Check user custom words
        {
            let uw_lock = self.user_words.lock().unwrap();
            for word in uw_lock.iter() {
                if word.starts_with(&direct) || (direct.len() > 1 && word.contains(&direct)) {
                    if !results.contains(word) {
                        results.push(word.clone());
                    }
                }
            }
        }

        // 4. Dictionary suggestions via upodesh FST
        let dict_suggestions = {
            let suggest = self.suggest.lock().unwrap();
            suggest.suggest(input)
        };

        if !dict_suggestions.is_empty() {
            // Prioritize direct match if present in dictionary suggestions
            if !direct.is_empty() && dict_suggestions.contains(&direct) && !results.contains(&direct) {
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
        } else if !direct.is_empty() && !results.contains(&direct) {
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
        // Use a temp data dir so this test is isolated from real user words on disk.
        let tmp = std::env::temp_dir().join("bangla-lekhi-test-suggest");
        let _ = std::fs::remove_dir_all(&tmp);
        std::env::set_var("XDG_DATA_HOME", &tmp);

        let engine = SuggestionEngine::new();

        // "ami" → "আমি" must be present (and first when no user words interfere)
        let c_ami = engine.get_candidates("ami");
        assert!(!c_ami.is_empty(), "ami should produce candidates");
        assert!(c_ami.contains(&"আমি".to_string()), "আমি must be in ami candidates");

        let c_bangla = engine.get_candidates("bangla");
        assert!(!c_bangla.is_empty(), "bangla should produce candidates");
        assert!(c_bangla.contains(&"বাংলা".to_string()), "বাংলা must be in bangla candidates");

        let c_bhalobashi = engine.get_candidates("bhalobashi");
        assert!(!c_bhalobashi.is_empty(), "bhalobashi should produce candidates");
        assert!(c_bhalobashi.contains(&"ভালোবাসি".to_string()), "ভালোবাসি must be in bhalobashi candidates");

        // Cleanup
        let _ = std::fs::remove_dir_all(&tmp);
    }

    #[test]
    fn test_user_word_addition() {
        let tmp = std::env::temp_dir().join("bangla-lekhi-test-userword");
        let _ = std::fs::remove_dir_all(&tmp);
        std::env::set_var("XDG_DATA_HOME", &tmp);

        let engine = SuggestionEngine::new();
        let custom_word = "বাংলালেখি";
        assert!(engine.add_user_word(custom_word), "add_user_word should return true");
        let candidates = engine.get_candidates("banglalekhi");
        assert!(
            candidates.contains(&custom_word.to_string()),
            "custom word 'বাংলালেখি' should appear in candidates for 'banglalekhi'"
        );

        let _ = std::fs::remove_dir_all(&tmp);
    }
}
