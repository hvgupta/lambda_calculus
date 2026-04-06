#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Expr Expr;
typedef struct{
    Expr const **list_of_expr;
    int cur_size;
    int size;
} GC;

GC *gc = NULL;

void insert_into_gc(Expr const *expr){
    if (gc == NULL){
        gc = (GC *)malloc(sizeof(GC));
        gc->list_of_expr = (Expr const **)malloc(sizeof(Expr const *) * 8);
        gc->cur_size = 0;
        gc->size = 8;
    } else if (gc->cur_size == gc->size){
        gc->list_of_expr = (Expr const **)realloc((void *)gc->list_of_expr, gc->size*2*sizeof(Expr const *));
        gc->size *= 2;
    }
    gc->list_of_expr[gc->cur_size++] = expr;
}

void free_all(){
    if (gc == NULL){
        return;
    }
    for (int i = 0; i < gc->cur_size; i++){
        free((void *)gc->list_of_expr[i]);
        gc->list_of_expr[i] = NULL;
    }
    free(gc->list_of_expr);
    gc->list_of_expr = NULL;

    free(gc);
    gc = NULL;
}


typedef enum {
    ROOT,
    VAR,
    FUN,
    GRP,
    VAL
} Expr_Type;

/* =========== Start of Expression Definition ============ */

typedef struct {
    Expr const *arg; // do we want to allow for non-var expr?
    // this can be an easy way to reduce the chances of capture cases, we can just check memory addr
    Expr const *body;
    const char *alt_repr;
} Func;

typedef struct {
    Expr const *lhs;
    Expr const *rhs;
    const char *alt_repr;
} Group;

struct Expr {
    Expr_Type expr_type;
    union
    {
        const char *var;
        Func fun;
        Group group;
    };
};

/* ============= Start of the Trie Definition ================ */

typedef struct Repr_Node Repr_Node;

struct Repr_Node{
    // When valid
    const char *equiv_repr;
    
    // Identifying
    Expr_Type node_type;
    
    Repr_Node **children;
    int next_child_idx;
    int num_children;
};
/* ----------- Start of Trie Implementation ------------------- */

Repr_Node *trie_root = NULL;
void _set_root_repr_trie(){
    if (trie_root != NULL){
        return;
    }

    trie_root = (Repr_Node *)malloc(sizeof(Repr_Node));
    trie_root->equiv_repr = NULL;

    trie_root->node_type = ROOT;

    trie_root->children = (Repr_Node **)malloc(sizeof(Repr_Node *) * 2);
    trie_root->next_child_idx = 0; 
    trie_root->num_children = 2;
}

Repr_Node *create_empty_node(){
    Repr_Node *node = (Repr_Node *)malloc(sizeof(Repr_Node));
    
    node->equiv_repr = NULL;
    node->node_type = VAR;
    node->children = NULL;
    node->next_child_idx = 0;
    node->num_children = 0;

    return node;
}

void _free_expr_trie(Repr_Node *node){
    if (node->children != NULL){
        for (int child_idx = 0; child_idx < node->next_child_idx; ++child_idx){
            _free_expr_trie(node->children[child_idx]);
        }
        free(node->children);
    }
    
    free(node);
    node = NULL;
}

void add_child_to_parent(Repr_Node *parent_node, Repr_Node *cur_node){
    if (parent_node->num_children == 0){
        parent_node->children = (Repr_Node **)malloc(sizeof(Repr_Node *));
        trie_root->num_children = 1;
    } else if (parent_node->next_child_idx == parent_node->num_children){
        parent_node->children = \
            (Repr_Node **)realloc((void *)parent_node->children, parent_node->num_children*2*sizeof(Repr_Node *));
        parent_node->num_children *= 2;
    }
    parent_node->children[parent_node->next_child_idx++] = cur_node;
}

Repr_Node *_create_expr_node(Expr const *expr, char const *expr_name);

Repr_Node *_create_var_node(Expr const *var_expr, char const *expr_name){
    Repr_Node *var_node = create_empty_node(); 
    var_node->node_type = VAR;
    var_node->equiv_repr = var_expr->var;
    
    if (expr_name != NULL){
        Repr_Node *repr_node = create_empty_node();
        var_node->node_type = VAL;
        var_node->equiv_repr = expr_name;
        add_child_to_parent(var_node, repr_node);
    }
    
    return var_node;
}

Repr_Node *_create_func_node(Expr const *fun_expr, char const *expr_name){
    Repr_Node *fun_node = create_empty_node();
    fun_node->node_type = FUN;
    fun_node->equiv_repr = fun_expr->fun.arg->var;

    Repr_Node *func_body = _create_expr_node(fun_expr->fun.body, expr_name);
    add_child_to_parent(fun_node, func_body);

    return fun_node;
}

Repr_Node *get_deepest_node(Repr_Node *node){
    if (node->num_children > 0){
        return get_deepest_node(node->children[0]);
    }
    return node;
}

Repr_Node *_create_grp_node(Expr const *grp_expr, char const *expr_name){
    Repr_Node *grp_node = create_empty_node();
    grp_node->node_type = GRP;
    
    Repr_Node *left_tree = _create_expr_node(grp_expr->group.lhs, NULL);
    Repr_Node *right_tree = _create_expr_node(grp_expr->group.rhs, expr_name);
    Repr_Node *left_tree_lowest = get_deepest_node(left_tree);
    add_child_to_parent(left_tree_lowest, right_tree);
    
    add_child_to_parent(grp_node, left_tree);

    return grp_node;
}

Repr_Node *_create_expr_node(Expr const *expr, char const *expr_name){
    switch (expr->expr_type) {
        case VAR:
            return _create_var_node(expr, expr_name);
        case FUN:
            return _create_func_node(expr, expr_name);
        case GRP:
            return _create_grp_node(expr, expr_name);
        default:
            return NULL;
    } 
}

Repr_Node *_register_expr(Repr_Node *parent_node, Expr const *expr, char const *expr_name){
    int num_children = (parent_node == NULL) ? 0 : parent_node->next_child_idx; 
    for (int i = 0; i < num_children; i++){
        if (parent_node->children[i]->node_type != expr->expr_type){
            continue;
        }
        Repr_Node *cur_node = parent_node->children[i]; 
        if (cur_node->node_type == GRP){
            Repr_Node *left_side = _register_expr(cur_node, expr->group.lhs, NULL);
            return _register_expr(left_side, expr->group.rhs, expr_name);
        } else if (cur_node->node_type == FUN){
            if (cur_node->equiv_repr != expr->fun.arg->var){
                continue;
            }
            return _register_expr(cur_node, expr->fun.body, expr_name);
        } else if (cur_node->equiv_repr == expr->var){
            return cur_node;
        }
    }
    Repr_Node *created_node = _create_expr_node(expr, expr_name); 
    add_child_to_parent(parent_node, created_node);
    return get_deepest_node(created_node);
}

void register_expr(Expr const *expr, char const *expr_name){
    _register_expr(trie_root, expr, expr_name);
}

Repr_Node *get_latest_lowest_node_expr(Repr_Node *node, Expr const *expr){
    if (node == NULL){
        return NULL;
    }
    int num_children = node->next_child_idx;
    if (num_children == 0){
        return NULL;
    }   
    for (int i = 0; i < num_children; i++){
        Repr_Node *cur_node = node->children[i];
        if (cur_node->node_type == VAR){
            if (cur_node->equiv_repr != expr->var){
                continue;
            }
            return cur_node;
        } else if (cur_node->node_type == FUN){
            if (cur_node->equiv_repr != expr->fun.arg->var){
                continue;
            }
            return get_latest_lowest_node_expr(cur_node, expr->fun.body);
        } else if (cur_node->node_type == GRP){
            Repr_Node *left_bottom = get_latest_lowest_node_expr(cur_node, expr->group.lhs);
            return get_latest_lowest_node_expr(left_bottom, expr->group.rhs);
        }
    }

    return NULL;
}

char check_for_expr(Repr_Node *node, Expr const *expr){
    int num_children = node->next_child_idx;
    if (num_children == 0){
        return 'F';
    }
    for (int i = 0; i < num_children; i++){
        Repr_Node *cur_node = node->children[i];
        if (cur_node->node_type == VAL){
            if (expr != NULL){
                continue;
            }
            return 'T';
        } else if (cur_node->node_type == VAR){
            if (cur_node->equiv_repr != expr->var){
                continue;
            }
            return check_for_expr(cur_node, NULL);
        } else if (cur_node->node_type == FUN){
            if (cur_node->equiv_repr != expr->fun.arg->var){
                continue;
            }
            return check_for_expr(cur_node, expr->fun.body);
        } else {
            char left_output = check_for_expr(cur_node, expr->group.lhs);
            if (left_output == 'F'){
                return 'F';
            }
            return check_for_expr(get_latest_lowest_node_expr(cur_node, expr->group.lhs), expr->group.rhs);
        }
    }
    return 'F';
}

/* ---------- End of Trie Implementation --------------- */
/* ---------- Start of Expression Implementation ------- */

Expr const *const create_var(const char *var){
    if (var == NULL){
        return NULL;
    }

    Expr *var_expr = (Expr *)malloc(sizeof(Expr));
    var_expr->expr_type = VAR;
    var_expr->var = var;

    insert_into_gc(var_expr);

    return var_expr;
}

Expr const *const create_func_with_repr(Expr const *arg, Expr const *body, const char *repr){
    if (arg == NULL || body == NULL){
        return NULL;
    }

    if (arg->expr_type != VAR){
        return NULL;
    }

    Expr *func_expr = (Expr *)malloc(sizeof(Expr));

    func_expr->expr_type = FUN;

    func_expr->fun.arg = arg;
    func_expr->fun.body = body;
    func_expr->fun.alt_repr = repr;

    insert_into_gc(func_expr);

    return func_expr;
}

Expr const *const create_func(Expr const *arg, Expr const *body){
    if (arg == NULL || body == NULL){
        return NULL;
    }

    if (arg->expr_type != VAR){
        return NULL;
    }

    Expr *func_expr = (Expr *)malloc(sizeof(Expr));

    func_expr->expr_type = FUN;

    func_expr->fun.arg = arg;
    func_expr->fun.body = body;
    func_expr->fun.alt_repr = NULL;

    insert_into_gc(func_expr);

    return func_expr;
}

Expr const *const create_grp_with_repr(Expr const *lhs, Expr const *rhs, const char *repr){
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->group.lhs = lhs;
    grp_expr->group.rhs = rhs;
    grp_expr->group.alt_repr = repr;

    insert_into_gc(grp_expr);

    return grp_expr;
}

Expr const *const create_grp(Expr const *lhs, Expr const *rhs){
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->group.lhs = lhs;
    grp_expr->group.rhs = rhs;
    grp_expr->group.alt_repr = NULL;

    insert_into_gc(grp_expr);

    return grp_expr;
}

/* ---- End of the Expression Implementation ---- */

Expr const *TRUE = NULL;
void _set_TRUE(){
    Expr const *bool_x = create_var("bool_x");
    Expr const *bool_y = create_var("bool_y");

    TRUE = create_func(bool_x, create_func(bool_y, bool_x));
}

Expr const *FALSE = NULL;
void _set_FALSE(){
    Expr const *bool_x = create_var("bool_x");
    Expr const *bool_y = create_var("bool_y");
    
    FALSE = create_func(bool_x, create_func(bool_y, bool_y));
}

Expr const *get_church_numerals(int num){
    Expr const *var_x = create_var("x");
    Expr const *func = create_var("f");
    Expr const *body = var_x;

    while (num-- != 0){
        body = create_grp(func, body);
    }
    return create_func(func, create_func(var_x, body));
}

Expr const *get_addition_function(){
    Expr const *var_n = create_var("n");
    Expr const *var_m = create_var("m");
    Expr const *func = create_var("f");
    Expr const *var_x = create_var("x");
    Expr const *addition = create_func(var_n,
        create_func(var_m,
            create_func(func, 
                create_func(var_x,
                    create_grp(
                        create_grp(var_m, func), 
                        create_grp(create_grp(var_n, func), var_x)
                    )
                )
            )
        )
    );
    return addition;
}

Expr const *get_multiplication_function(){
    Expr const *var_n = create_var("n");
    Expr const *var_m = create_var("m");
    Expr const *func = create_var("f");
    Expr const *var_x = create_var("x");

    Expr const *mult = create_func(var_n, 
        create_func(var_m, 
            create_func(func, 
                create_func(var_x,
                    create_grp(
                        create_grp(var_n, create_grp(var_m, func)), 
                        var_x
                    )
                )
            )
        )
    );

    return mult;
}

Expr const *get_minus_one(){
    Expr const *var_n = create_var("n");
    Expr const *var_f = create_var("f");
    Expr const *var_x = create_var("x");

    Expr const *var_g = create_var("g");
    Expr const *var_h = create_var("h");
    Expr const *gh_function = create_func(var_g, create_func(var_h, create_grp(var_h, create_grp(var_g, var_f))));

    Expr const *var_u = create_var("u");
    Expr const *func_ux = create_func(var_u, var_x);

    Expr const *another_u = create_var("u");
    Expr const *func_uu = create_func(another_u, another_u);

    Expr const *inner_func = create_grp(create_grp(create_grp(var_n, gh_function), func_ux), func_uu);

    return create_func(var_n, create_func(var_f, create_func(var_x, inner_func)));
}

Expr const *y_combinator(){
    Expr const *yy = create_var("yy");
    Expr const *yx = create_var("yx");

    Expr const *U = create_func(yx,
        create_func(yy,
            create_grp(yy,
                create_grp(
                    create_grp(yx, yx),
                    yy
                )
            )
        )
    );

    return create_grp(U, U);
}

void print_expr(const Expr *expr);

Expr const *eval_expr(Expr const *exp);

Expr const *replace_vars(Expr const *body, const Expr *arg_expr, Expr const *rplc_expr){

    if (body->expr_type == VAR){
        return (body == arg_expr) ? rplc_expr : body;

    } else if (body->expr_type == FUN){
        if (body->fun.arg == arg_expr){
            return body;
        }
        Expr const *replaced_body = replace_vars(body->fun.body, arg_expr, rplc_expr);
        if (replaced_body == body->fun.body){
            return body;
        }
        return create_func(body->fun.arg, replaced_body);

    } else if (body->expr_type == GRP){
        Expr const *replaced_lhs = replace_vars(body->group.lhs, arg_expr, rplc_expr);
        Expr const *replaced_rhs = replace_vars(body->group.rhs, arg_expr, rplc_expr);
        if (replaced_lhs == body->group.lhs && replaced_rhs == body->group.rhs){
            return body;
        }
        return create_grp(replaced_lhs, replaced_rhs);
    }

    return NULL;
}

Expr const *eval_grp_expr(Expr const *grp_expr){
    if (grp_expr == NULL){
        return NULL;
    }
    Expr const *lhs = grp_expr->group.lhs;
    Expr const *rhs = grp_expr->group.rhs;

    if (lhs->expr_type == VAR){
        if (rhs->expr_type != GRP){
            return grp_expr; // just to stop from too much mem being allocated
        }

        Expr const *evalled_rhs = eval_expr(rhs);
        if (evalled_rhs == rhs){
            return grp_expr;
        }
        return create_grp(lhs, evalled_rhs);
    } else if (lhs->expr_type == FUN){
        Expr const *func_body = eval_expr(lhs->fun.body); 
        Expr const *func_arg = lhs->fun.arg; 

        return replace_vars(func_body, func_arg, eval_expr(rhs));
    } else if (lhs->expr_type == GRP){    
        Expr const *new_lhs = eval_grp_expr(lhs);
        // Expr const *new_rhs = eval_expr(rhs);

        if (lhs == new_lhs){
            return grp_expr;
        }

        return create_grp(new_lhs, rhs);
    }
    
    return NULL;
}

Expr const *eval_func(Expr const *func){
    Expr const *evalled_body = eval_expr(func->fun.body);
    if (evalled_body == func->fun.body){
        return func;
    }
    return create_func(func->fun.arg, eval_expr(func->fun.body));
}

Expr const *eval_expr(Expr const *exp){
    if (exp == NULL){
        return NULL;
    }
    switch (exp->expr_type) {
    case VAR:
        return exp;
    case FUN:
        return eval_func(exp);
    case GRP:
        return eval_grp_expr(exp);
    default:
        return NULL; // this is never executed
    }
}

Expr const *full_eval_expr(Expr const * const expr){
    Expr const *prev = NULL;
    Expr const *now = expr;
    while (1) {
        now = eval_expr(now);
        if (prev == now){
            break;
        }
        prev = now;
    }

    return now;
}

void _print_expr(Expr const *expr, int brack_count);

Expr const *is_zero_op(){
    Expr const *is_zero_n = create_var("izn");
    Expr const *is_zero = create_func(is_zero_n, create_grp(create_grp(is_zero_n, create_func(create_var(""), FALSE)), TRUE));

    return is_zero;
}

Expr const *get_factorial_op(){
    Expr const *factorial_f = create_var("ff");
    Expr const *factorial_n = create_var("fn");

    Expr const *factorial_body = \
        create_grp(
            create_grp(
                create_grp(is_zero_op(), factorial_n),
                get_church_numerals(1)),
            create_grp(
                create_grp(get_multiplication_function(), factorial_n),
                create_grp(factorial_f, create_grp(get_minus_one(), factorial_n))
            )
        );

    Expr const *F = create_func(factorial_f, create_func(factorial_n, factorial_body));
    return create_grp(F, create_grp(y_combinator(), F));
}

void get_var_repr(const char *var, int brack_count){
    // actually these function would be an internal function, the main entry point of this function will be from print_expr
    if (var == NULL){
        return;
    }
    printf("%s", var);
    
    while (brack_count > 0){
        printf(")");
        --brack_count;
    }
}

void get_func_repr(const Func *func, int brack_count){
    if (func == NULL) {
        return; 
    }
    printf("λ");
    _print_expr(func->arg, 0);
    printf(".");
    if (func->body->expr_type != GRP){
        printf("(");
        ++brack_count;
    }
    _print_expr(func->body, brack_count);
}

void get_grp_repr(const Group *grp, int brack_count){
    if (grp == NULL) {
        return; 
    }
    printf("(");
    _print_expr(grp->lhs, 0);
    printf(" ");
    _print_expr(grp->rhs, brack_count + 1);
}


void _print_expr(const Expr *expr, int brack_count){
    // This is supposed to be the route of the repr functions
    switch (expr->expr_type)
    {
    case VAR:
        get_var_repr(expr->var, brack_count);
        break;
    case FUN:
        get_func_repr(&expr->fun, brack_count);
        break;
    case GRP:
        get_grp_repr(&expr->group, brack_count);
        break;
    default:
        break;
    }
}

void print_expr(const Expr *expr){
    _print_expr(expr,  0);
    printf("\n");
}

void factorial_test(){

    printf("checking is_zero\n");
    printf("validating that zero is returned as True for 0: ");
    Expr const *zero = get_church_numerals(0);
    Expr const *zero_is_zero = full_eval_expr(create_grp(is_zero_op(), zero));
    print_expr(zero_is_zero);

    printf("validating that non-zero is returned as False for non-zero: ");
    Expr const *four = get_church_numerals(4);
    Expr const *four_is_zero = full_eval_expr(create_grp(is_zero_op(), four));
    print_expr(four_is_zero);

    printf("the factorial op is ");
    Expr const *factorial_op = get_factorial_op();
    print_expr(factorial_op);

    printf("factorial of n is ");
    Expr const *m_minus_one_factorial = full_eval_expr(create_grp(factorial_op, four));
    print_expr(m_minus_one_factorial);
}

void test_nums_and_addition(Expr const *n, Expr const *m){
    // being able to understand this as it is going to be very difficult
    printf("this is the start of the mathematical operations section where n and m will be presented");
    print_expr(n);
    print_expr(m);
    printf("=====Addition=====\n");
    Expr const *n_plus_m_expr = create_grp(create_grp(get_addition_function(), n), m);
    printf("the addition before eval is ");
    print_expr(n_plus_m_expr);

    Expr const *evalled_sum = full_eval_expr(n_plus_m_expr);
    printf("the addition after eval is ");
    print_expr(evalled_sum);

    printf("==== end of addition ======\n");

    printf("====Multiplication===========\n");
    Expr const *n_mult_m_expr = create_grp(create_grp(get_multiplication_function(), n), m);
    printf("multiplication before eval ");
    print_expr(n_mult_m_expr);

    Expr const *evalled_mult = full_eval_expr(n_mult_m_expr);
    printf("multiplication after eval ");
    print_expr(evalled_mult);
    printf("==== end of Multiplication =====\n");

    printf("printing out minus one\n");
    Expr const *minus_one = get_minus_one();
    print_expr(minus_one);

    printf("Minus one of n ");
    Expr const *n_minus_one = full_eval_expr(create_grp(minus_one, n));
    print_expr(n_minus_one);

    printf("Minus one of m ");
    Expr const *m_minus_one = full_eval_expr(create_grp(minus_one, m));
    print_expr(m_minus_one);
}

void test_complex_grp_repr(){
    /* this function only tests repr, not eval
     *  (λx.(λy.(x y) g))(λx. x x)
     */
    Expr const *const var_x = create_var("x");
    Expr const *const var_y = create_var("y");

    Expr const *const func_y_body_lhs_grp = create_grp(var_x, var_y);
    Expr const *const var_g = create_var("g");

    Expr const *const func_y_body_grp = create_grp(func_y_body_lhs_grp, var_g);
    Expr const *const func_y = create_func(var_y, func_y_body_grp);

    Expr const *const func_x = create_func(var_x, func_y);

    Expr const *const rhs_grp_x = create_var("x");

    Expr const *const rhs_grp_func_body = create_grp(rhs_grp_x, rhs_grp_x);
    Expr const *const rhs_grp_func = create_func(rhs_grp_x, rhs_grp_func_body);

    Expr const *const cmplx_grp = create_grp(func_x, rhs_grp_func);

    printf("before eval ");
    print_expr(cmplx_grp);

    Expr const *evalled_expr = full_eval_expr(cmplx_grp);
    printf("after eval ");
    print_expr(evalled_expr);
}

void test_inf(){
    Expr const *const x = create_var("x");
    Expr const *const grp_x = create_grp(x, x);
    Expr const *const func_grp_x = create_func(x, grp_x);

    Expr const *const grp_func_grp_x = create_grp(func_grp_x, func_grp_x);
    Expr const *const output = eval_expr(grp_func_grp_x);

    print_expr(output);
}

void test_grp_func_eval(){
    Expr const *x = create_var("x");
    Expr const *func_x = create_func(x, x);
    Expr const *grp_x_funcx = create_grp(x, func_x);

    Expr const *y = create_var("y");
    Expr const *grp_y_y = create_grp(y, y);

    Expr const *total = create_grp(grp_x_funcx, grp_y_y);

    printf("before ");
    print_expr(total);

    Expr const *evalled_total = eval_expr(total);
    printf("after ");
    print_expr(evalled_total);
}

void basic_test(){
    // Var test
    Expr const *const var = create_var("test");
    print_expr(var);

    // Func test
    Expr const *const func = create_func(var, var);
    print_expr(func);

    // Nested Func test
    Expr const *const nested_func = create_func(var, func);
    print_expr(nested_func);

    // Group
    Expr const *const group_expr = create_grp(nested_func, var);
    print_expr(group_expr);
}

void code_main(){
    basic_test();
    test_complex_grp_repr();
    /* test_inf(); */
    test_grp_func_eval();
    test_nums_and_addition(get_church_numerals(3), get_church_numerals(5));
    factorial_test();
}

int main()
{
    _set_TRUE();
    _set_FALSE();
    
    _set_root_repr_trie();

    code_main();

    _free_expr_trie(trie_root); 
    free_all();
};
