//! Phonetic parsing and transliteration module

use okkhor::parser::Parser;
use std::sync::Mutex;

pub struct PhoneticParser {
    parser: Mutex<Parser>,
}

impl PhoneticParser {
    pub fn new() -> Self {
        Self {
            parser: Mutex::new(Parser::new_phonetic()),
        }
    }

    /// Transliterate an Avro-style Latin string to Bengali Unicode.
    pub fn parse(&self, input: &str) -> String {
        if input.is_empty() {
            return String::new();
        }
        let parser = self.parser.lock().unwrap();
        parser.convert(input)
    }
}

impl Default for PhoneticParser {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_vowels() {
        let p = PhoneticParser::new();
        assert_eq!(p.parse("o"), "অ");
        assert_eq!(p.parse("a"), "আ");
        assert_eq!(p.parse("i"), "ই");
        assert_eq!(p.parse("I"), "ঈ");
        assert_eq!(p.parse("u"), "উ");
        assert_eq!(p.parse("U"), "ঊ");
        assert_eq!(p.parse("e"), "এ");
        assert_eq!(p.parse("OI"), "ঐ");
        assert_eq!(p.parse("O"), "ও");
        assert_eq!(p.parse("OU"), "ঔ");
    }

    #[test]
    fn test_consonants() {
        let p = PhoneticParser::new();
        assert_eq!(p.parse("k"), "ক");
        assert_eq!(p.parse("kh"), "খ");
        assert_eq!(p.parse("g"), "গ");
        assert_eq!(p.parse("gh"), "ঘ");
        assert_eq!(p.parse("Ng"), "ঙ");
        assert_eq!(p.parse("c"), "চ");
        assert_eq!(p.parse("ch"), "ছ");
        assert_eq!(p.parse("j"), "জ");
        assert_eq!(p.parse("jh"), "ঝ");
        assert_eq!(p.parse("t"), "ত");
        assert_eq!(p.parse("T"), "ট");
        assert_eq!(p.parse("th"), "থ");
        assert_eq!(p.parse("Th"), "ঠ");
        assert_eq!(p.parse("d"), "দ");
        assert_eq!(p.parse("D"), "ড");
        assert_eq!(p.parse("dh"), "ধ");
        assert_eq!(p.parse("Dh"), "ঢ");
        assert_eq!(p.parse("n"), "ন");
        assert_eq!(p.parse("N"), "ণ");
        assert_eq!(p.parse("p"), "প");
        assert_eq!(p.parse("ph"), "ফ");
        assert_eq!(p.parse("f"), "ফ");
        assert_eq!(p.parse("b"), "ব");
        assert_eq!(p.parse("bh"), "ভ");
        assert_eq!(p.parse("v"), "ভ");
        assert_eq!(p.parse("m"), "ম");
        assert_eq!(p.parse("r"), "র");
        assert_eq!(p.parse("R"), "ড়");
        assert_eq!(p.parse("Rh"), "ঢ়");
        assert_eq!(p.parse("l"), "ল");
        assert_eq!(p.parse("sh"), "শ");
        assert_eq!(p.parse("S"), "শ");
        assert_eq!(p.parse("Sh"), "ষ");
        assert_eq!(p.parse("s"), "স");
        assert_eq!(p.parse("h"), "হ");
    }

    #[test]
    fn test_conjuncts() {
        let p = PhoneticParser::new();
        assert_eq!(p.parse("kkh"), "ক্ষ");
        assert_eq!(p.parse("gg"), "জ্ঞ");
        assert_eq!(p.parse("Ngk"), "ঙ্ক");
        assert_eq!(p.parse("Ngkh"), "ঙ্খ");
        assert_eq!(p.parse("nd"), "ন্দ");
        assert_eq!(p.parse("nt"), "ন্ত");
        assert_eq!(p.parse("mp"), "ম্প");
        assert_eq!(p.parse("st"), "স্ত");
        assert_eq!(p.parse("sT"), "স্ট");
    }

    #[test]
    fn test_special_characters() {
        let p = PhoneticParser::new();
        // Khanda Ta: t`` -> ৎ
        assert_eq!(p.parse("t``"), "ৎ");
        // Anusvara: ng -> ং
        assert_eq!(p.parse("ng"), "ং");
        // Bisarga: : -> ঃ
        assert_eq!(p.parse(":"), "ঃ");
        // Chandrabindu: ^ -> ঁ
        assert_eq!(p.parse("^"), "ঁ");
        // Dari / Punctuation: . -> ।
        assert_eq!(p.parse("."), "।");
    }
}
