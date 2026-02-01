# Code Line Count - Top 50 Files

For each file in this list, I want you to create a planning task to track it, then break the file down into smaller files of no more than 300 lines. In between each file I want you to run 'make build' and 'make test' to be sure everything is still working before moving on to the next file. Continue until all the files in this list are done.  

## Summary

- **Total project lines:** ~99,126 (src + tests)
- **Source code:** ~57,338 lines
- **Tests:** ~41,788 lines

## Top 50 Files by Line Count

| Rank | Lines | File |
|------|-------|------|
| 2 | 1,712 | `src/code_gen/code_gen_expr.c` |
| 3 | 1,686 | `src/gcc_backend.c` |
| 4 | 1,540 | `src/parser/parser_expr.c` |
| 5 | 1,514 | `src/type_checker/type_checker_util.c` |
| 6 | 1,483 | `src/code_gen/code_gen_expr_thread.c` |
| 7 | 1,431 | `src/code_gen.c` |
| 8 | 1,367 | `tests/unit/parser/parser_tests_basic.c` |
| 9 | 1,360 | `src/parser/parser_stmt_decl.c` |
| 10 | 1,350 | `tests/unit/type_checker/type_checker_tests_memory.c` |
| 11 | 1,339 | `tests/unit/code_gen/code_gen_tests_optimization.c` |
| 12 | 1,321 | `tests/unit/type_checker/type_checker_tests_native_pointer.c` |
| 13 | 1,200 | `src/package.c` |
| 14 | 1,194 | `src/code_gen/code_gen_expr_lambda.c` |
| 15 | 1,189 | `tests/unit/standalone/symbol_table_tests_edge_cases.c` |
| 16 | 1,180 | `tests/unit/standalone/symbol_table_tests_core.c` |
| 17 | 1,178 | `tests/unit/lexer/lexer_tests_edge_cases.c` |
| 18 | 1,175 | `src/lexer/lexer_scan.c` |
| 19 | 1,134 | `src/runtime/runtime_string.c` |
| 20 | 1,121 | `src/runtime/arena/managed_arena.c` |
| 21 | 1,107 | `src/runtime/runtime_thread.c` |
| 22 | 1,100 | `tests/integration/test_struct_heap_alloc.sn` |
| 23 | 1,092 | `src/package/package_lfs.c` |
| 24 | 1,071 | `tests/unit/type_checker/type_checker_tests_edge_cases.c` |
| 25 | 1,043 | `tests/unit/parser/parser_tests_expressions.c` |
| 26 | 1,034 | `src/runtime/runtime_malloc_redirect.c` |
| 27 | 1,021 | `tests/unit/type_checker/type_checker_tests_array.c` |
| 28 | 1,008 | `tests/unit/ast/ast_tests_edge_cases.c` |
| 29 | 1,004 | `src/parser.c` |
| 30 | 973 | `tests/unit/parser/parser_tests_struct.c` |
| 31 | 960 | `tests/unit/parser/parser_tests_edge_cases.c` |
| 32 | 952 | `src/ast/ast_expr.c` |
| 33 | 950 | `src/parser/parser_stmt.c` |
| 34 | 947 | `tests/unit/parser/parser_tests_control.c` |
| 35 | 939 | `src/runtime/hooks/minhook/hook.c` |
| 36 | 927 | `src/runtime/hooks/plthook_elf.c` |
| 37 | 896 | `src/optimizer/optimizer_util.c` |
| 38 | 895 | `tests/unit/type_checker/type_checker_tests_native_slice.c` |
| 39 | 858 | `tests/unit/runtime/runtime_array_tests.c` |
| 40 | 849 | `src/symbol_table/symbol_table_namespace.c` |
| 41 | 847 | `tests/unit/optimizer/optimizer_tests.c` |
| 42 | 813 | `tests/unit/code_gen/code_gen_tests_stmt.c` |
| 43 | 808 | `tests/unit/code_gen/code_gen_tests_helpers.c` |
| 44 | 795 | `src/ast.h` |
| 45 | 794 | `src/code_gen/code_gen_expr_call_intercept.c` |
| 46 | 787 | `tests/unit/type_checker/type_checker_tests_promotion.c` |
| 47 | 784 | `src/code_gen/code_gen_expr_call_array.c` |
| 48 | 780 | `src/type_checker/type_checker_expr.c` |
| 49 | 776 | `src/code_gen/code_gen_stmt_core.c` |
| 50 | 770 | `src/symbol_table.c` |

## Observations

- **Largest source files:** Code generation (`code_gen_expr.c`) and GCC backend
- **Parser complexity:** Expression parsing is the largest parser module
- **Test coverage:** Extensive unit tests for parser, type checker, and code gen
