# Post-tag expectations attached to existing tagged sources

These two `.expected` additions did not exist at v0.0.83. They accompanied post-tag edits to their `.sn` files, which this correction restores. Leaving the additions in the normal C fixture directory changes the tagged test oracle even after restoring the 39 modified files.

The original main (`7af0d192`) source/expectation snapshots are preserved here, unchanged. The current-main corpus evidence ran them in their original paths. They are explicit post-tag regression fixtures, not substituted tagged expectations. For reproducing the full current corpus, use the exact main fixture checkout documented in the evidence; do not combine these expectations with the restored tag sources.
