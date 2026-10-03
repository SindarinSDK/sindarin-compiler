Confirmed follow-up on main ee3879f5: floating array equality

Report .sn/floating-array-equality-before.json, source .sn/floating-array-equality-probe.sn. All nine C/Rust executions compile and exit zero; every pair differs. C outputs false/true/true/false/true for float signed-zero arrays, copied float NaN arrays, self NaN array, double signed-zero arrays, copied double NaN arrays. Rust outputs true/false/false/true/false. No parity credit.

C sn_array_equals in src/runtime/sn_array.h checks pointer identity, nil, lengths, then memcmp of a.len*a.elem_size bytes. Rust expr/binary.hbs currently falls through to Vec numeric PartialEq for floating arrays. Bytewise comparison is required, including inequality and first-array width. No C/shared change is needed.

Check mixed float[]/double[] source admission too: shared float/double type compatibility can extend through arrays. C equality compares a contiguous byte prefix using the left runtime element width, not a per-item prefix. Example little-endian float[]{0f,1.875f} and double[]{1.0,1000.0} have matching first eight bytes despite very different numeric values. Reverse-width comparisons may read beyond valid initialized object storage; do not count them as defined parity. Empty mixed-width arrays can be defined even when nonempty narrower storage is not. Do not silently numerically cast or zip corresponding items for a different-width raw prefix.

Keep this separate from the 95 corpus compilation denominator: it is a new behavior gap in a new C-valid source, not an admitted old corpus failure. Source, oracle, evaluation order and alias/lifetime checks still need permanent coverage and all-platform integration.
