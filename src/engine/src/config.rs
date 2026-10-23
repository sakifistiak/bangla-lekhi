//! Runtime configuration loader for Bangla Lekhi engine.
//!
//! Reads `~/.config/bangla-lekhi/settings.json` (or `$XDG_CONFIG_HOME/bangla-lekhi/settings.json`)
//! and exposes a `Config` struct consumed by the IBus engine to handle toggle key and preferences.

use serde::Deserialize;
use std::fs;
use std::path::PathBuf;

/// Parsed runtime configuration.
#[derive(Debug, Clone, Deserialize)]
pub struct Config {
    /// The key name used to toggle Bangla/English mode (default: "F12").
    #[serde(default = "default_toggle_key")]
    pub toggle_key: String,

    /// Whether autocorrect substitutions are applied (default: true).
    #[serde(default = "default_true")]
    pub autocorrect: bool,

    /// Whether candidate suggestions are shown (default: true).
    #[serde(default = "default_true")]
    pub show_candidates: bool,

    /// Maximum number of candidates to show (default: 5).
    #[serde(default = "default_candidate_count")]
    pub candidate_count: usize,
}

fn default_toggle_key() -> String {
    "F12".to_string()
}

fn default_true() -> bool {
    true
}

fn default_candidate_count() -> usize {
    5
}

impl Default for Config {
    fn default() -> Self {
        Self {
            toggle_key: default_toggle_key(),
            autocorrect: true,
            show_candidates: true,
            candidate_count: 5,
        }
    }
}

impl Config {
    /// Load configuration from disk, falling back to defaults on any error.
    pub fn load() -> Self {
        let path = Self::config_path();
        if let Ok(content) = fs::read_to_string(&path) {
            if let Ok(cfg) = serde_json::from_str::<Config>(&content) {
                return cfg;
            }
        }
        Config::default()
    }

    /// Resolve the settings.json path respecting XDG_CONFIG_HOME.
    pub fn config_path() -> PathBuf {
        if let Ok(xdg) = std::env::var("XDG_CONFIG_HOME") {
            PathBuf::from(xdg).join("bangla-lekhi").join("settings.json")
        } else if let Ok(home) = std::env::var("HOME") {
            PathBuf::from(home)
                .join(".config")
                .join("bangla-lekhi")
                .join("settings.json")
        } else {
            PathBuf::from("/tmp/bangla-lekhi-settings.json")
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_default_config() {
        let cfg = Config::default();
        assert_eq!(cfg.toggle_key, "F12");
        assert!(cfg.autocorrect);
        assert!(cfg.show_candidates);
        assert_eq!(cfg.candidate_count, 5);
    }

    #[test]
    fn test_config_deserialization() {
        let json = r#"{
            "toggle_key": "F10",
            "autocorrect": false,
            "show_candidates": true,
            "candidate_count": 3
        }"#;
        let cfg: Config = serde_json::from_str(json).unwrap();
        assert_eq!(cfg.toggle_key, "F10");
        assert!(!cfg.autocorrect);
        assert_eq!(cfg.candidate_count, 3);
    }

    #[test]
    fn test_config_partial_defaults() {
        // Only toggle_key specified — rest should fall back to defaults
        let json = r#"{"toggle_key": "F9"}"#;
        let cfg: Config = serde_json::from_str(json).unwrap();
        assert_eq!(cfg.toggle_key, "F9");
        assert!(cfg.autocorrect);
        assert_eq!(cfg.candidate_count, 5);
    }
}
