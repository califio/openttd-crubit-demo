use icu_segmenter::{WordSegmenter, WordSegmenterBorrowed};
use std::sync::LazyLock;

static SEGMENTER: LazyLock<WordSegmenterBorrowed<'static>> =
    LazyLock::new(|| WordSegmenter::new_dictionary(Default::default()));

const DONE: i32 = -1;

/// ICU4C-style cursor navigation over ICU4X word boundaries.
///
/// Positions count UTF-16 code units. The input is borrowed only during
/// `set_text`; the binding owns the boundary vector and cursor state.
/// This compatibility API is independent of any application's editing policy.
#[derive(Default)]
pub struct WordIterator {
    boundaries: Vec<usize>,
    current: usize,
}

impl WordIterator {
    /// Replace the text and reset the cursor, reusing boundary capacity.
    /// Reject lengths that cannot be represented by the signed position API.
    /// On failure the previous state is unchanged.
    pub fn set_text(&mut self, text: &[u16]) -> bool {
        if text.len() > i32::MAX as usize {
            return false;
        }
        self.boundaries.clear();
        self.boundaries.extend(SEGMENTER.segment_utf16(text));
        self.current = 0;
        true
    }

    pub fn first(&mut self) -> i32 {
        self.current = 0;
        0
    }

    /// Find the first boundary strictly after `pos`.
    pub fn following(&mut self, pos: i32) -> i32 {
        if pos < 0 {
            return self.first();
        }
        let index = self
            .boundaries
            .partition_point(|&boundary| boundary <= pos as usize);
        if index == self.boundaries.len() {
            // ICU4C keeps its cursor at the last boundary after returning DONE.
            self.current = self.boundaries.len().saturating_sub(1);
            return DONE;
        }
        self.current = index;
        self.position()
    }

    /// Find the last boundary strictly before `pos`.
    pub fn preceding(&mut self, pos: i32) -> i32 {
        if pos <= 0 {
            self.current = 0;
            return DONE;
        }
        let index = self
            .boundaries
            .partition_point(|&boundary| boundary < pos as usize);
        self.current = index.saturating_sub(1);
        if index == 0 { DONE } else { self.position() }
    }

    pub fn next(&mut self) -> i32 {
        if self.current + 1 >= self.boundaries.len() {
            return DONE;
        }
        self.current += 1;
        self.position()
    }

    pub fn previous(&mut self) -> i32 {
        if self.current == 0 {
            return DONE;
        }
        self.current -= 1;
        self.position()
    }

    fn position(&self) -> i32 {
        self.boundaries
            .get(self.current)
            .map_or(DONE, |&pos| pos as i32)
    }
}
