These sources exercise ordinary nested sized-array handle storage. The clean
shared-mutation control explicitly pops its stored handles before scope exit;
this is a distinct lifetime case, not a rewritten oracle for the original.

`observations/original_abnormal.sn.raw` preserves Terra's exact f10f8f38 source.
Fresh tagged default/checked/unchecked O0/O1/O2 runs printed `9,9,9` and then
terminated with signal 11. It is not included among clean parity comparisons.
Raw evidence: /tmp/sn-nested-f10-fresh-4ga44x0w.

Current coverage includes retained local and selected global initializer
owners, ordinary outer-container forwarding, selected aggregate fields,
receiver field writes, fresh produced arrays, value-capture outer snapshots,
recursive handle types, and bounded thread transport. Private owner-mask
specializations retain ordinary function callers and their original borrowing.

General mutator-call transport, method/closure array parameter interfaces,
returned outer-container ownership, arbitrary pointer-backed aggregate paths,
and broader concurrent alias/lifetime/error forms remain required work.
No rejection is considered a resolution of those boundaries. The original
abnormal probe remains separate from all clean-control parity counts.
