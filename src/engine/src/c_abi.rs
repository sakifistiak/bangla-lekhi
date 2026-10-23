//! C-ABI Foreign Function Interface

use crate::convert_phonetic;
use crate::state::{BanglaLekhiEngine, BanglaLekhiMode};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn bangla_lekhi_engine_new() -> *mut BanglaLekhiEngine {
    Box::into_raw(Box::new(BanglaLekhiEngine::new()))
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_free(engine: *mut BanglaLekhiEngine) {
    if !engine.is_null() {
        drop(Box::from_raw(engine));
    }
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_get_mode(
    engine: *const BanglaLekhiEngine,
) -> BanglaLekhiMode {
    if engine.is_null() {
        return BanglaLekhiMode::Bangla;
    }
    (*engine).mode()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_set_mode(
    engine: *mut BanglaLekhiEngine,
    mode: BanglaLekhiMode,
) {
    if !engine.is_null() {
        (*engine).set_mode(mode);
    }
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_toggle_mode(
    engine: *mut BanglaLekhiEngine,
) -> BanglaLekhiMode {
    if engine.is_null() {
        return BanglaLekhiMode::Bangla;
    }
    (*engine).toggle_mode()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_push_char(engine: *mut BanglaLekhiEngine, c: c_char) {
    if !engine.is_null() && c > 0 {
        (*engine).push_char(c as u8 as char);
    }
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_pop_char(engine: *mut BanglaLekhiEngine) -> bool {
    if engine.is_null() {
        return false;
    }
    (*engine).pop_char()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_clear(engine: *mut BanglaLekhiEngine) {
    if !engine.is_null() {
        (*engine).clear();
    }
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_has_composition(
    engine: *const BanglaLekhiEngine,
) -> bool {
    if engine.is_null() {
        return false;
    }
    (*engine).has_composition()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_get_raw_buffer(
    engine: *const BanglaLekhiEngine,
) -> *const c_char {
    if engine.is_null() {
        return c"".as_ptr();
    }
    (*engine).c_raw_buffer_ptr()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_get_preedit(
    engine: *const BanglaLekhiEngine,
) -> *const c_char {
    if engine.is_null() {
        return c"".as_ptr();
    }
    (*engine).c_preedit_ptr()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_get_candidate_count(
    engine: *const BanglaLekhiEngine,
) -> usize {
    if engine.is_null() {
        return 0;
    }
    (*engine).candidate_count()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_get_candidate_at(
    engine: *const BanglaLekhiEngine,
    index: usize,
) -> *const c_char {
    if engine.is_null() {
        return std::ptr::null();
    }
    (*engine).c_candidate_ptr_at(index)
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_engine_commit_candidate(
    engine: *mut BanglaLekhiEngine,
    index: usize,
) -> *const c_char {
    if engine.is_null() {
        return c"".as_ptr();
    }
    (*engine).commit_candidate(index);
    (*engine).c_last_committed_ptr()
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_transliterate(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return std::ptr::null_mut();
    }
    let c_str = match CStr::from_ptr(input).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let converted = convert_phonetic(c_str);
    match CString::new(converted) {
        Ok(c_res) => c_res.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn bangla_lekhi_string_free(ptr: *mut c_char) {
    if !ptr.is_null() {
        drop(CString::from_raw(ptr));
    }
}
