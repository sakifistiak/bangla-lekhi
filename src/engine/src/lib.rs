//! Bangla Lekhi Core Engine
//!
//! Provides Avro-compatible phonetic transliteration, candidate suggestion,
//! and C-ABI export for the IBus input method engine.

pub mod c_abi;
pub mod config;
pub mod phonetic;
pub mod state;
pub mod suggest;

pub use c_abi::*;
pub use phonetic::PhoneticParser;
pub use state::{BanglaLekhiEngine, BanglaLekhiMode};
pub use suggest::SuggestionEngine;

use std::sync::Mutex;
use std::sync::OnceLock;

static GLOBAL_PARSER: OnceLock<Mutex<PhoneticParser>> = OnceLock::new();

fn get_global_parser() -> &'static Mutex<PhoneticParser> {
    GLOBAL_PARSER.get_or_init(|| Mutex::new(PhoneticParser::new()))
}

/// Convert an Avro phonetic Latin string to Bengali Unicode.
pub fn convert_phonetic(input: &str) -> String {
    let parser = get_global_parser().lock().unwrap();
    parser.parse(input)
}
