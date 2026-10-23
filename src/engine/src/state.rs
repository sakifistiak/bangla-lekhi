//! Input method state machine

use crate::suggest::SuggestionEngine;
use std::ffi::CString;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BanglaLekhiMode {
    Bangla = 0,
    English = 1,
}

pub struct BanglaLekhiEngine {
    mode: BanglaLekhiMode,
    buffer: String,
    candidates: Vec<String>,
    suggestion_engine: SuggestionEngine,

    // C-compatible cached strings for zero-copy FFI inspection
    c_buffer: CString,
    c_preedit: CString,
    c_candidates: Vec<CString>,
    c_last_committed: CString,
}

impl BanglaLekhiEngine {
    pub fn new() -> Self {
        Self {
            mode: BanglaLekhiMode::Bangla,
            buffer: String::with_capacity(64),
            candidates: Vec::new(),
            suggestion_engine: SuggestionEngine::new(),
            c_buffer: CString::default(),
            c_preedit: CString::default(),
            c_candidates: Vec::new(),
            c_last_committed: CString::default(),
        }
    }

    pub fn mode(&self) -> BanglaLekhiMode {
        self.mode
    }

    pub fn set_mode(&mut self, mode: BanglaLekhiMode) {
        if self.mode != mode {
            self.mode = mode;
            self.clear();
        }
    }

    pub fn toggle_mode(&mut self) -> BanglaLekhiMode {
        let new_mode = match self.mode {
            BanglaLekhiMode::Bangla => BanglaLekhiMode::English,
            BanglaLekhiMode::English => BanglaLekhiMode::Bangla,
        };
        self.set_mode(new_mode);
        new_mode
    }

    pub fn has_composition(&self) -> bool {
        !self.buffer.is_empty()
    }

    pub fn raw_buffer(&self) -> &str {
        &self.buffer
    }

    pub fn push_char(&mut self, c: char) {
        if self.mode == BanglaLekhiMode::English {
            return;
        }
        self.buffer.push(c);
        self.recompute();
    }

    pub fn pop_char(&mut self) -> bool {
        if self.buffer.is_empty() {
            return false;
        }
        self.buffer.pop();
        if self.buffer.is_empty() {
            self.clear();
            false
        } else {
            self.recompute();
            true
        }
    }

    pub fn clear(&mut self) {
        self.buffer.clear();
        self.candidates.clear();
        self.c_buffer = CString::default();
        self.c_preedit = CString::default();
        self.c_candidates.clear();
    }

    fn recompute(&mut self) {
        if self.buffer.is_empty() {
            self.clear();
            return;
        }

        self.candidates = self.suggestion_engine.get_candidates(&self.buffer);

        // Pre-edit is the best candidate (candidate[0]) or fallback
        let preedit_str = if let Some(first) = self.candidates.first() {
            first.clone()
        } else {
            String::new()
        };

        self.c_buffer = CString::new(self.buffer.as_str()).unwrap_or_default();
        self.c_preedit = CString::new(preedit_str.as_str()).unwrap_or_default();
        self.c_candidates = self
            .candidates
            .iter()
            .map(|s| CString::new(s.as_str()).unwrap_or_default())
            .collect();
    }

    pub fn preedit(&self) -> &str {
        if let Some(first) = self.candidates.first() {
            first.as_str()
        } else {
            ""
        }
    }

    pub fn candidates(&self) -> &[String] {
        &self.candidates
    }

    pub fn candidate_count(&self) -> usize {
        self.candidates.len()
    }

    pub fn candidate_at(&self, index: usize) -> Option<&str> {
        self.candidates.get(index).map(|s| s.as_str())
    }

    pub fn commit_candidate(&mut self, index: usize) -> String {
        let committed = if index < self.candidates.len() {
            self.candidates[index].clone()
        } else if let Some(first) = self.candidates.first() {
            first.clone()
        } else {
            self.buffer.clone()
        };

        self.c_last_committed = CString::new(committed.as_str()).unwrap_or_default();
        self.clear();
        committed
    }

    // FFI Helper pointers (valid until next mutating method call)
    pub fn c_raw_buffer_ptr(&self) -> *const std::os::raw::c_char {
        self.c_buffer.as_ptr()
    }

    pub fn c_preedit_ptr(&self) -> *const std::os::raw::c_char {
        self.c_preedit.as_ptr()
    }

    pub fn c_candidate_ptr_at(&self, index: usize) -> *const std::os::raw::c_char {
        if let Some(c) = self.c_candidates.get(index) {
            c.as_ptr()
        } else {
            std::ptr::null()
        }
    }

    pub fn c_last_committed_ptr(&self) -> *const std::os::raw::c_char {
        self.c_last_committed.as_ptr()
    }
}

impl Default for BanglaLekhiEngine {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_engine_state_flow() {
        let mut engine = BanglaLekhiEngine::new();
        assert!(!engine.has_composition());

        engine.push_char('a');
        engine.push_char('m');
        engine.push_char('i');
        assert!(engine.has_composition());
        assert_eq!(engine.raw_buffer(), "ami");
        assert_eq!(engine.preedit(), "আমি");
        assert!(engine.candidate_count() >= 1);

        let committed = engine.commit_candidate(0);
        assert_eq!(committed, "আমি");
        assert!(!engine.has_composition());
    }

    #[test]
    fn test_engine_backspace() {
        let mut engine = BanglaLekhiEngine::new();
        engine.push_char('a');
        engine.push_char('m');
        assert_eq!(engine.raw_buffer(), "am");

        assert!(engine.pop_char());
        assert_eq!(engine.raw_buffer(), "a");

        assert!(!engine.pop_char());
        assert!(!engine.has_composition());
    }

    #[test]
    fn test_mode_switch() {
        let mut engine = BanglaLekhiEngine::new();
        assert_eq!(engine.mode(), BanglaLekhiMode::Bangla);

        assert_eq!(engine.toggle_mode(), BanglaLekhiMode::English);
        assert_eq!(engine.mode(), BanglaLekhiMode::English);

        // When in English, push_char does not compose
        engine.push_char('a');
        assert!(!engine.has_composition());

        assert_eq!(engine.toggle_mode(), BanglaLekhiMode::Bangla);
        engine.push_char('a');
        assert!(engine.has_composition());
    }
}
