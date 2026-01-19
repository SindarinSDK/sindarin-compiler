// ast.h
#ifndef AST_H
#define AST_H

#include "arena.h"
#include "token.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;
typedef struct Parameter Parameter;
typedef struct StructField StructField;
typedef struct FieldInitializer FieldInitializer;
typedef struct StructMethod StructMethod;

/* Struct field definition */
struct StructField
{
    const char *name;       /* Field name */
    Type *type;             /* Field type */
    size_t offset;          /* Byte offset within struct (computed during type checking) */
    Expr *default_value;    /* Optional default value (NULL if none) */
    const char *c_alias;    /* C name alias (from #pragma alias), NULL if none */
};

typedef enum
{
    TYPE_INT,
    TYPE_INT32,
    TYPE_UINT,
    TYPE_UINT32,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_BYTE,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_NIL,
    TYPE_ANY,
    TYPE_POINTER,
    TYPE_OPAQUE,
    TYPE_STRUCT
} TypeKind;

/* Memory qualifier for variables and parameters */
typedef enum
{
    MEM_DEFAULT,    /* Default behavior (reference for arrays, value for primitives) */
    MEM_AS_VAL,     /* as val - explicit copy semantics */
    MEM_AS_REF      /* as ref - heap allocation for primitives */
} MemoryQualifier;

/* Sync modifier for thread-safe atomic variables */
typedef enum
{
    SYNC_NONE,      /* No synchronization (default) */
    SYNC_ATOMIC     /* sync keyword - uses atomic operations */
} SyncModifier;

/* Block modifier for memory management */
typedef enum
{
    BLOCK_DEFAULT,  /* Normal block with own arena */
    BLOCK_SHARED,   /* shared block - uses parent's arena */
    BLOCK_PRIVATE   /* private block - isolated arena, only primitives escape */
} BlockModifier;

/* Function modifier for memory management */
typedef enum
{
    FUNC_DEFAULT,   /* Normal function with own arena */
    FUNC_SHARED,    /* shared function - uses caller's arena */
    FUNC_PRIVATE    /* private function - isolated arena, only primitives return */
} FunctionModifier;

/* Struct method definition */
struct StructMethod
{
    const char *name;           /* Method name */
    Parameter *params;          /* Method parameters (does NOT include implicit 'self') */
    int param_count;            /* Number of parameters */
    Type *return_type;          /* Return type */
    Stmt **body;                /* Method body statements (NULL for native declarations) */
    int body_count;             /* Number of body statements */
    FunctionModifier modifier;  /* shared or private modifier */
    bool is_static;             /* True if declared with 'static' keyword */
    bool is_native;             /* True if declared with 'native' keyword */
    Token name_token;           /* Token for error reporting */
    const char *c_alias;        /* C function name alias (from #pragma alias), NULL if none */
};

struct Type
{
    TypeKind kind;

    union
    {
        struct
        {
            Type *element_type;
        } array;

        struct
        {
            Type *return_type;
            Type **param_types;
            MemoryQualifier *param_mem_quals; /* Memory qualifiers for each parameter (NULL if all default) */
            int param_count;
            bool is_variadic;                 /* true if function accepts variadic arguments */
            bool is_native;                   /* true if this is a native callback type (C-compatible function pointer) */
            bool has_body;                    /* true if function has a Sindarin body (vs true extern) */
            const char *typedef_name;         /* Name of the typedef for native callback types (NULL if anonymous) */
        } function;

        struct
        {
            Type *base_type;  /* The type being pointed to (e.g., int for *int, *int for **int) */
        } pointer;

        struct
        {
            const char *name;  /* Name of the opaque type (e.g., "FILE") */
        } opaque;

        struct
        {
            const char *name;       /* Struct name */
            StructField *fields;    /* Array of struct fields */
            int field_count;        /* Number of fields */
            StructMethod *methods;  /* Array of struct methods */
            int method_count;       /* Number of methods */
            size_t size;            /* Total size in bytes (computed during type checking) */
            size_t alignment;       /* Alignment requirement (computed during type checking) */
            bool is_native;         /* True if declared with 'native struct' (allows pointer fields) */
            bool is_packed;         /* True if preceded by #pragma pack(1) */
            bool pass_self_by_ref;  /* True if 'as ref' - native methods receive self by pointer */
            const char *c_alias;    /* C type name alias (from #pragma alias), NULL if none */
        } struct_type;
    } as;
};

/* Escape analysis flags for expressions
 * Used by the type checker to track which expressions escape their scope
 * and require heap allocation vs stack allocation */
typedef struct
{
    bool escapes_scope;          /* Expression result escapes its declaring scope */
    bool needs_heap_allocation;  /* Expression needs heap allocation (large size or escapes) */
} EscapeInfo;

typedef enum
{
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_ASSIGN,
    EXPR_INDEX_ASSIGN,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_ARRAY_ACCESS,
    EXPR_INCREMENT,
    EXPR_DECREMENT,
    EXPR_INTERPOLATED,
    EXPR_MEMBER,
    EXPR_ARRAY_SLICE,
    EXPR_RANGE,
    EXPR_SPREAD,
    EXPR_LAMBDA,
    EXPR_STATIC_CALL,
    EXPR_SIZED_ARRAY_ALLOC,
    EXPR_THREAD_SPAWN,
    EXPR_THREAD_SYNC,
    EXPR_SYNC_LIST,
    EXPR_AS_VAL,
    EXPR_AS_REF,
    EXPR_TYPEOF,
    EXPR_IS,
    EXPR_AS_TYPE,
    EXPR_STRUCT_LITERAL,
    EXPR_MEMBER_ACCESS,
    EXPR_MEMBER_ASSIGN,
    EXPR_SIZEOF,
    EXPR_COMPOUND_ASSIGN,
    EXPR_METHOD_CALL
} ExprType;

typedef struct
{
    Expr *left;
    Expr *right;
    SnTokenType operator;
} BinaryExpr;

typedef struct
{
    Expr *operand;
    SnTokenType operator;
} UnaryExpr;

typedef struct
{
    LiteralValue value;
    Type *type;
    bool is_interpolated;
} LiteralExpr;

typedef struct
{
    Token name;
} VariableExpr;

typedef struct
{
    Token name;
    Expr *value;
} AssignExpr;

typedef struct
{
    Expr *array;
    Expr *index;
    Expr *value;
} IndexAssignExpr;

/* Member assignment expression: point.x = value */
typedef struct
{
    Expr *object;        /* The struct expression */
    Token field_name;    /* Name of the field being assigned */
    Expr *value;         /* Value to assign */
} MemberAssignExpr;

/* Compound assignment expression: x += value, x -= value, etc. */
typedef struct
{
    Expr *target;        /* The left-hand side (variable, array index, or member) */
    SnTokenType operator; /* The operation: TOKEN_PLUS, TOKEN_MINUS, etc. */
    Expr *value;         /* The right-hand side value */
} CompoundAssignExpr;

typedef struct
{
    Expr *callee;
    Expr **arguments;
    int arg_count;
    bool is_tail_call;  /* Marked by optimizer for tail call optimization */
} CallExpr;

typedef struct
{
    Expr **elements;
    int element_count;
} ArrayExpr;

typedef struct
{
    Expr *array;
    Expr *index;
} ArrayAccessExpr;

typedef struct
{
    Expr *array;
    Expr *start;  // NULL means from beginning
    Expr *end;    // NULL means to end
    Expr *step;   // NULL means step of 1
    bool is_from_pointer;  // True if slicing a pointer type (set by type checker)
} ArraySliceExpr;

typedef struct
{
    Expr *start;  // Start of range (required)
    Expr *end;    // End of range (required)
} RangeExpr;

typedef struct
{
    Expr *array;  // The array being spread
} SpreadExpr;

typedef struct
{
    Expr **parts;
    char **format_specs;  // Format specifier for each part (NULL if none)
    int part_count;
} InterpolExpr;

typedef struct
{
    Expr *object;
    Token member_name;
    StructMethod *resolved_method;    /* Resolved method (set during type checking if this is a method call) */
    Type *resolved_struct_type;       /* Struct type containing the method (set during type checking) */
} MemberExpr;

typedef struct
{
    Token type_name;     // The type name (e.g., "TextFile", "Bytes", "Path")
    Token method_name;   // The method name (e.g., "open", "fromHex")
    Expr **arguments;    // Arguments to the method
    int arg_count;
    StructMethod *resolved_method;    // Resolved method for user-defined struct static methods
    Type *resolved_struct_type;       // Struct type containing the method
} StaticCallExpr;

typedef struct
{
    Type *element_type;  // Type of array elements (e.g., int, str, bool)
    Expr *size_expr;     // Expression for array size (must evaluate to int)
    Expr *default_value; // Optional default value for all elements (can be NULL)
} SizedArrayAllocExpr;

typedef struct
{
    Expr *call;              // The function call expression to spawn as thread
    FunctionModifier modifier; // Function modifier: shared/private/default
} ThreadSpawnExpr;

typedef struct
{
    Expr *handle;        // Thread handle or sync list of handles to sync
    bool is_array;       // True if syncing a list of thread handles: [r1, r2]!
} ThreadSyncExpr;

typedef struct
{
    Expr **elements;     // Variables to sync: [r1, r2, r3]
    int element_count;
} SyncListExpr;

typedef struct
{
    Expr *operand;          // The expression to copy/pass by value
    bool is_cstr_to_str;    // True if this is *char => str (null-terminated string conversion)
    bool is_noop;           // True if operand is already array type (ptr[0..len] produces array)
    bool is_struct_deep_copy; // True if this is struct deep copy (copies array fields independently)
} AsValExpr;

/* as ref - get pointer to value (counterpart to as val) */
typedef struct
{
    Expr *operand;          // The expression to get a pointer to
} AsRefExpr;

/* typeof operator - returns the runtime type of an any value or a type literal */
typedef struct
{
    Expr *operand;       /* Expression to get type of (can be value or type name) */
    Type *type_literal;  /* If typeof(int), the type itself; NULL if typeof(value) */
} TypeofExpr;

/* is operator - checks if an any value is of a specific type */
typedef struct
{
    Expr *operand;       /* The any value to check */
    Type *check_type;    /* The type to check against */
} IsExpr;

/* as operator - casts an any value to a concrete type */
typedef struct
{
    Expr *operand;       /* The any value to cast */
    Type *target_type;   /* The type to cast to */
} AsTypeExpr;

/* Struct literal expression: Point { x: 1.0, y: 2.0 } */
typedef struct
{
    Token struct_name;           /* Name of the struct type */
    FieldInitializer *fields;    /* Array of field initializers */
    int field_count;             /* Number of field initializers */
    Type *struct_type;           /* Resolved struct type (set during type checking) */
    bool *fields_initialized;    /* Boolean array tracking which fields were explicitly initialized
                                  * (indexed by struct field index, allocated during type checking,
                                  * size matches struct_type->as.struct_type.field_count) */
    int total_field_count;       /* Total number of fields in the struct type (set during type checking) */
} StructLiteralExpr;

/* Member access expression for struct fields: point.x */
typedef struct
{
    Expr *object;                /* The struct expression */
    Token field_name;            /* Name of the field being accessed */
    int field_index;             /* Index of the field (set during type checking) */
    bool escaped;                /* True if this field access escapes its declaring scope */
    int scope_depth;             /* Scope depth where this access occurs (set during type checking) */
} MemberAccessExpr;

/* sizeof operator - returns the size of a type or expression */
typedef struct
{
    Type *type_operand;          /* Type operand (e.g., sizeof(Point)) - NULL if expression */
    Expr *expr_operand;          /* Expression operand (e.g., sizeof point) - NULL if type */
} SizeofExpr;

typedef struct
{
    Parameter *params;
    int param_count;
    Type *return_type;
    Expr *body;              /* Expression body for single-line lambdas (NULL if has_stmt_body) */
    struct Stmt **body_stmts; /* Statement body for multi-line lambdas (NULL if !has_stmt_body) */
    int body_stmt_count;      /* Number of statements in body_stmts */
    int has_stmt_body;        /* True if lambda has statement body instead of expression body */
    FunctionModifier modifier;  /* shared, private, or default */
    bool is_native;           /* True if this is a native callback lambda (no closures, C-compatible) */
    /* Capture info (filled during type checking) */
    Token *captured_vars;
    Type **captured_types;
    int capture_count;
    int lambda_id;  /* Unique ID for code gen */
} LambdaExpr;

/* Method call expression: point.magnitude() or Point.create() */
typedef struct
{
    Expr *object;              /* The struct instance (NULL for static calls) */
    Token struct_name;         /* For static calls: the struct type name */
    Token method_name;         /* Name of the method being called */
    Expr **args;               /* Method arguments */
    int arg_count;             /* Number of arguments */
    StructMethod *method;      /* Resolved method (set during type checking) */
    Type *struct_type;         /* Resolved struct type (set during type checking) */
    bool is_static;            /* True if this is a static method call (Type.method()) */
} MethodCallExpr;

struct Expr
{
    ExprType type;
    Token *token;

    union
    {
        BinaryExpr binary;
        UnaryExpr unary;
        LiteralExpr literal;
        VariableExpr variable;
        AssignExpr assign;
        IndexAssignExpr index_assign;
        CallExpr call;
        ArrayExpr array;
        ArrayAccessExpr array_access;
        ArraySliceExpr array_slice;
        RangeExpr range;
        SpreadExpr spread;
        Expr *operand;
        MemberExpr member;
        InterpolExpr interpol;
        LambdaExpr lambda;
        StaticCallExpr static_call;
        SizedArrayAllocExpr sized_array_alloc;
        ThreadSpawnExpr thread_spawn;
        ThreadSyncExpr thread_sync;
        SyncListExpr sync_list;
        AsValExpr as_val;
        AsRefExpr as_ref;
        TypeofExpr typeof_expr;
        IsExpr is_expr;
        AsTypeExpr as_type;
        StructLiteralExpr struct_literal;
        MemberAccessExpr member_access;
        MemberAssignExpr member_assign;
        SizeofExpr sizeof_expr;
        CompoundAssignExpr compound_assign;
        MethodCallExpr method_call;
    } as;

    Type *expr_type;
    EscapeInfo escape_info;  /* Escape analysis metadata (set during type checking) */
};

typedef enum
{
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FOR_EACH,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_IMPORT,
    STMT_PRAGMA,
    STMT_TYPE_DECL,
    STMT_STRUCT_DECL,
    STMT_LOCK
} StmtType;

/* Pragma directive types */
typedef enum
{
    PRAGMA_INCLUDE,
    PRAGMA_LINK,
    PRAGMA_SOURCE,   /* #pragma source "file.c" */
    PRAGMA_PACK,     /* #pragma pack(1) or #pragma pack() */
    PRAGMA_ALIAS     /* #pragma alias "c_name" - applies to next native struct/field/method */
} PragmaType;

typedef struct
{
    Expr *expression;
} ExprStmt;

typedef struct
{
    Token name;
    Type *type;
    Expr *initializer;
    MemoryQualifier mem_qualifier;  /* as val or as ref modifier */
    SyncModifier sync_modifier;     /* sync for atomic operations */
} VarDeclStmt;

struct Parameter
{
    Token name;
    Type *type;
    MemoryQualifier mem_qualifier;  /* as val modifier for copy semantics */
    SyncModifier sync_modifier;     /* sync for atomic operations */
};

typedef struct
{
    Token name;
    Parameter *params;
    int param_count;
    Type *return_type;
    Stmt **body;
    int body_count;
    FunctionModifier modifier;  /* shared or private modifier */
    bool is_native;             /* true if declared with 'native' keyword */
    bool is_variadic;           /* true if function has variadic parameters (...) */
    const char *c_alias;        /* C function name alias (from #pragma alias), NULL if none */
} FunctionStmt;

typedef struct
{
    Token keyword;
    Expr *value;
} ReturnStmt;

typedef struct
{
    Stmt **statements;
    int count;
    BlockModifier modifier;  /* shared or private block modifier */
} BlockStmt;

typedef struct
{
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} IfStmt;

typedef struct
{
    Expr *condition;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} WhileStmt;

typedef struct
{
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} ForStmt;

typedef struct
{
    Token var_name;
    Expr *iterable;
    Stmt *body;
    bool is_shared;  /* shared loop - no per-iteration arena */
} ForEachStmt;

typedef struct
{
    Token module_name;
    Token *namespace;          /* Optional namespace identifier (NULL if not namespaced) */
    struct Stmt **imported_stmts;  /* For namespaced imports: statements from imported module */
    int imported_count;        /* Number of imported statements */
    bool also_imported_directly; /* True if this module was also imported without namespace */
} ImportStmt;

typedef struct
{
    PragmaType pragma_type;    /* PRAGMA_INCLUDE or PRAGMA_LINK */
    const char *value;         /* The value (e.g., "<math.h>" or "m") */
} PragmaStmt;

typedef struct
{
    Token name;                /* The type alias name (e.g., "FILE") */
    Type *type;                /* The underlying type (for opaque: TYPE_OPAQUE with name) */
} TypeDeclStmt;

/* Field initializer for struct literals */
struct FieldInitializer
{
    Token name;                /* Field name */
    Expr *value;               /* Field value expression */
};

typedef struct
{
    Token name;                /* Struct name */
    StructField *fields;       /* Array of field definitions */
    int field_count;           /* Number of fields */
    StructMethod *methods;     /* Array of method definitions */
    int method_count;          /* Number of methods */
    bool is_native;            /* True if declared with 'native struct' (allows pointer fields) */
    bool is_packed;            /* True if preceded by #pragma pack(1) */
    bool pass_self_by_ref;     /* True if 'as ref' - native methods receive self by pointer */
    const char *c_alias;       /* C type name alias (from #pragma alias), NULL if none */
} StructDeclStmt;

/* Lock statement for synchronized blocks: lock(expr) => body */
typedef struct
{
    Expr *lock_expr;           /* The sync variable to lock on */
    Stmt *body;                /* The lock block body */
} LockStmt;

struct Stmt
{
    StmtType type;
    Token *token;

    union
    {
        ExprStmt expression;
        VarDeclStmt var_decl;
        FunctionStmt function;
        ReturnStmt return_stmt;
        BlockStmt block;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        ForEachStmt for_each_stmt;
        ImportStmt import;
        PragmaStmt pragma;
        TypeDeclStmt type_decl;
        StructDeclStmt struct_decl;
        LockStmt lock_stmt;
    } as;
};

typedef struct
{
    Stmt **statements;
    int count;
    int capacity;
    const char *filename;
} Module;

void ast_print_stmt(Arena *arena, Stmt *stmt, int indent_level);
void ast_print_expr(Arena *arena, Expr *expr, int indent_level);

/* Escape analysis helpers */
void ast_expr_mark_escapes(Expr *expr);
void ast_expr_mark_needs_heap(Expr *expr);
void ast_expr_clear_escape_info(Expr *expr);
bool ast_expr_escapes_scope(Expr *expr);
bool ast_expr_needs_heap_allocation(Expr *expr);

Token *ast_clone_token(Arena *arena, const Token *src);

Type *ast_clone_type(Arena *arena, Type *type);
Type *ast_create_primitive_type(Arena *arena, TypeKind kind);
Type *ast_create_array_type(Arena *arena, Type *element_type);
Type *ast_create_pointer_type(Arena *arena, Type *base_type);
Type *ast_create_opaque_type(Arena *arena, const char *name);
Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count);
Type *ast_create_struct_type(Arena *arena, const char *name, StructField *fields, int field_count,
                             StructMethod *methods, int method_count, bool is_native, bool is_packed,
                             bool pass_self_by_ref, const char *c_alias);
StructMethod *ast_struct_get_method(Type *struct_type, const char *method_name);
int ast_type_equals(Type *a, Type *b);
int ast_type_is_pointer(Type *type);
int ast_type_is_opaque(Type *type);
int ast_type_is_struct(Type *type);
const char *ast_type_to_string(Arena *arena, Type *type);
StructField *ast_struct_get_field(Type *struct_type, const char *field_name);
int ast_struct_get_field_index(Type *struct_type, const char *field_name);

/* Check if a specific field was explicitly initialized in a struct literal.
 * Returns true if the field at field_index was explicitly initialized,
 * false if it was not initialized (will use default or zero value).
 * field_index must be valid (0 <= field_index < total_field_count).
 * Returns false if fields_initialized is NULL (not yet type-checked). */
bool ast_struct_literal_field_initialized(Expr *struct_literal_expr, int field_index);

Expr *ast_create_binary_expr(Arena *arena, Expr *left, SnTokenType operator, Expr *right, const Token *loc_token);
Expr *ast_create_unary_expr(Arena *arena, SnTokenType operator, Expr *operand, const Token *loc_token);
Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token);
Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token);
Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token);
Expr *ast_create_index_assign_expr(Arena *arena, Expr *array, Expr *index, Expr *value, const Token *loc_token);
Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token);
Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token);
Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token);
Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, char **format_specs, int part_count, const Token *loc_token);
Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token);
Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, SnTokenType comparison_type, const Token *loc_token);
Expr *ast_create_array_slice_expr(Arena *arena, Expr *array, Expr *start, Expr *end, Expr *step, const Token *loc_token);
Expr *ast_create_range_expr(Arena *arena, Expr *start, Expr *end, const Token *loc_token);
Expr *ast_create_spread_expr(Arena *arena, Expr *array, const Token *loc_token);
Expr *ast_create_lambda_expr(Arena *arena, Parameter *params, int param_count,
                             Type *return_type, Expr *body, FunctionModifier modifier,
                             bool is_native, const Token *loc_token);
Expr *ast_create_lambda_stmt_expr(Arena *arena, Parameter *params, int param_count,
                                  Type *return_type, struct Stmt **body_stmts, int body_stmt_count,
                                  FunctionModifier modifier, bool is_native, const Token *loc_token);
Expr *ast_create_thread_spawn_expr(Arena *arena, Expr *call, FunctionModifier modifier, const Token *loc_token);
Expr *ast_create_thread_sync_expr(Arena *arena, Expr *handle, bool is_array, const Token *loc_token);
Expr *ast_create_sync_list_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token);
Expr *ast_create_as_val_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_as_ref_expr(Arena *arena, Expr *operand, const Token *loc_token);
Expr *ast_create_struct_literal_expr(Arena *arena, Token struct_name, FieldInitializer *fields,
                                      int field_count, const Token *loc_token);
Expr *ast_create_member_access_expr(Arena *arena, Expr *object, Token field_name, const Token *loc_token);
Expr *ast_create_member_assign_expr(Arena *arena, Expr *object, Token field_name, Expr *value, const Token *loc_token);
Expr *ast_create_sizeof_type_expr(Arena *arena, Type *type_operand, const Token *loc_token);
Expr *ast_create_sizeof_expr_expr(Arena *arena, Expr *expr_operand, const Token *loc_token);

Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression, const Token *loc_token);
Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer, const Token *loc_token);
Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count, const Token *loc_token);
Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value, const Token *loc_token);
Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count, const Token *loc_token);
Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch, const Token *loc_token);
Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body, const Token *loc_token);
Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body, const Token *loc_token);
Stmt *ast_create_for_each_stmt(Arena *arena, Token var_name, Expr *iterable, Stmt *body, const Token *loc_token);
Stmt *ast_create_import_stmt(Arena *arena, Token module_name, Token *namespace, const Token *loc_token);
Stmt *ast_create_pragma_stmt(Arena *arena, PragmaType pragma_type, const char *value, const Token *loc_token);
Stmt *ast_create_type_decl_stmt(Arena *arena, Token name, Type *type, const Token *loc_token);
Stmt *ast_create_struct_decl_stmt(Arena *arena, Token name, StructField *fields, int field_count,
                                   StructMethod *methods, int method_count,
                                   bool is_native, bool is_packed, bool pass_self_by_ref,
                                   const char *c_alias, const Token *loc_token);
Stmt *ast_create_lock_stmt(Arena *arena, Expr *lock_expr, Stmt *body, const Token *loc_token);

void ast_init_module(Arena *arena, Module *module, const char *filename);
void ast_module_add_statement(Arena *arena, Module *module, Stmt *stmt);

#endif