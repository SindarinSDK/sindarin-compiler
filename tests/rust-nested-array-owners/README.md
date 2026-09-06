These sources exercise ordinary nested sized-array handle storage. The clean
shared-mutation control explicitly pops its stored handles before scope exit;
this is a distinct lifetime case, not a rewritten oracle for the original.

`observations/original_abnormal.sn.raw` preserves Terra's exact f10f8f38 source.
Fresh tagged default/checked/unchecked O0/O1/O2 runs printed `9,9,9` and then
terminated with signal 11. It is not included among clean parity comparisons.
Raw evidence: /tmp/sn-nested-f10-fresh-4ga44x0w.

Current implementation is a private projection for selected local initializer
owners and private specializations of ordinary direct functions. Existing
non-owner callers retain their original function signatures and borrowing. Global storage, method/closure
transport, arbitrary produced initializer places, outer-container forwarding,
and composition with thread-specific owner transport need separate evidence.
No rejection is considered a resolution of those required boundaries.
