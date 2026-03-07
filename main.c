#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Expr;
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
        gc->list_of_expr = (Expr const **)malloc(sizeof(Expr const *)*8);
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
    VAR,
    FUN,
    GRP
} ExprType;

typedef struct
{
    Expr const *arg; // do we want to allow for non-var expr? 
    // this can be an easy way to reduce the chances of capture cases, we can just check memory addr
    Expr const *body;
} Func;

typedef struct {
    Expr const *lhs;
    Expr const *rhs;
} Group;

struct Expr {
    ExprType expr_type;
    union
    {
        const char *var;
        Func fun; 
        Group group;
    };
};

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

    insert_into_gc(func_expr);

    return func_expr;
}

Expr const *const create_grp(Expr const *lhs, Expr const *rhs){
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->group.lhs = lhs;
    grp_expr->group.rhs = rhs;

    insert_into_gc(grp_expr);

    return grp_expr;
}
void print_expr(const Expr *expr);
/*
print_for_var -> string
print_for_func -> f",\{arg}." + if arg-> print_arg, if func print_
*/
Expr const *eval_expr(Expr const *exp);

Expr const *replace_vars(Expr const *body, const Expr *arg_expr , Expr const *rplc_expr){
    
    if (body->expr_type == VAR){
        return (body == arg_expr) ? rplc_expr : body;

    } else if (body->expr_type == FUN){
        return create_func(body->fun.arg, replace_vars(body->fun.body, arg_expr, rplc_expr));

    } else if (body->expr_type == GRP){
        return eval_expr(create_grp(
                    replace_vars(body->group.lhs, arg_expr, rplc_expr), 
                    replace_vars(body->group.rhs, arg_expr, rplc_expr)
                ));
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
        return create_grp(lhs, eval_expr(rhs));
    } else if (lhs->expr_type == FUN){
        Expr const *func_body = lhs->fun.body; 
        Expr const *func_arg = lhs->fun.arg; 

        return replace_vars(func_body, func_arg, eval_expr(rhs));
    } else if (lhs->expr_type == GRP){
        Expr const *new_lhs = eval_grp_expr(lhs);
        Expr const *new_rhs = eval_expr(rhs);
        
        if (new_lhs->expr_type == GRP && new_rhs->expr_type == GRP){
            return create_grp(new_lhs->group.lhs, 
                    eval_expr(create_grp(new_lhs->group.rhs, new_rhs)
                ));
        }

        return create_grp(new_lhs, new_rhs);
    }
    
    return NULL;
}

Expr const *eval_func(Expr const *func){
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
    }
}

Expr const *restructed_expr(Expr const *lhs, Expr const *rhs, const char dir){
    if (dir == 'l' && rhs->group.rhs->expr_type == FUN && rhs->group.lhs->expr_type != FUN){
        // ((* *) (x f)) --> (((* *) x) f)
        return create_grp(create_grp(lhs, rhs->group.lhs), rhs->group.rhs);
    } else if (dir == 'r' && lhs->group.lhs->expr_type != FUN){
        // ((x *) (* *)) --> (x (* (* *)))
        return create_grp(lhs->group.lhs, create_grp(lhs->group.rhs, rhs));
    }
    // in the cases where we want to return the current group structure
    return NULL;
}

Expr const *simplify_expr(Expr const *expr, char cur_dir){ // cur_dir can only be l or r
    // the idea of this function is to have function/group be in the lhs and the variables be in the rhs
    if (expr == NULL){
        return NULL;
    }
    switch (expr->expr_type) {
        case GRP:
            break;
        default:
            return expr;
    } 
    Expr const *lhs = simplify_expr(expr->group.lhs, 'l');
    Expr const *rhs = simplify_expr(expr->group.rhs, 'r');
    printf("---------------\n");
    print_expr(lhs);
    print_expr(rhs);

    if (lhs->expr_type == VAR){
        // lhs is var then will be checked in the lower call stack 
        return expr;

    } else if (lhs->expr_type == FUN){
        if (rhs->expr_type != GRP){
            // (f x)
            return expr;
        } else if (rhs->group.lhs->expr_type == FUN){
            // since the func is in the left side of the group, so we dont need to change the group
            // f (f *)
            return expr;
        }
        // (f (*  *)) --> ((f *) *)
        return create_grp(create_grp(lhs, rhs->group.lhs), rhs->group.rhs);

    } else if (lhs->expr_type == GRP){
        if (rhs->expr_type != GRP){
            if (rhs->expr_type == FUN && cur_dir == 'l'){
                return expr;
            }else if (lhs->group.rhs->expr_type != FUN && cur_dir == 'l'){
                return expr;
            } 
            // ((* f) *) --> (* (f *))
            return create_grp(lhs->group.lhs, create_grp(lhs->group.rhs, rhs));
        } 

        if (lhs->group.lhs->expr_type == FUN && rhs->group.lhs->expr_type == FUN){
            // ((f *) (f *))
            return expr;
        } else if (lhs->group.rhs->expr_type != FUN){
            if (cur_dir == 'l' && rhs->group.rhs->expr_type == FUN && rhs->group.lhs->expr_type != FUN){
            // ((* *) (x f)) --> (((* *) x) f)
                return create_grp(create_grp(lhs, rhs->group.lhs), rhs->group.rhs);
            } else if (cur_dir == 'r' && lhs->group.lhs->expr_type != FUN){
                // ((x *) (* *)) --> (x (* (* *)))
                return create_grp(lhs->group.lhs, create_grp(lhs->group.rhs, rhs));
            }
            return expr;
        }

        if (cur_dir == 'r' && lhs->group.lhs->expr_type != FUN){
            return create_grp(
                    lhs->group.lhs, 
                    create_grp(create_grp(lhs->group.rhs, rhs->group.lhs), rhs->group.rhs)
                );
        } else if (lhs->group.lhs->expr_type == FUN){
            return create_grp(create_grp(lhs, rhs->group.lhs), rhs->group.rhs);
        }

        return create_grp(
                create_grp(lhs->group.lhs, create_grp(lhs->group.rhs, rhs->group.lhs)),
                rhs->group.rhs
            );
        
    }
    
    return NULL;
}

void _print_expr(Expr const *expr, char *buff, int brack_count);
    
 
void get_var_repr(const char *var, char *buff, int brack_count){
    // actually these function would be an internal function, the main entry point of this function will be from print_expr
    if (var == NULL || buff == NULL){
        return;
    }
    strcat(buff, var);
    
    while (brack_count > 0){
        strcat(buff, ")");
        --brack_count;
    }
        
}

void get_func_repr(const Func *func, char *buff, int brack_count){
    if (func == NULL || buff == NULL) {
        return; 
    }
    strcat(buff, "λ");
    _print_expr(func->arg, buff, 0);
    strcat(buff, ".");
    if (func->body->expr_type != GRP){
        strcat(buff, "(");
        ++brack_count;
    }
    _print_expr(func->body, buff, brack_count);
}

void get_grp_repr(const Group *grp, char *buff, int brack_count){
    if (grp == NULL || buff == NULL) {
        return; 
    }
    strcat(buff, "(");
    _print_expr(grp->lhs, buff, 0);
    strcat(buff, " ");
    _print_expr(grp->rhs, buff, brack_count+1);
}


void _print_expr(const Expr *expr, char *buff, int brack_count){
    // This is supposed to be the route of the repr functions
    switch (expr->expr_type)
    {
    case VAR:
        get_var_repr(expr->var, buff, brack_count);
        break;
    case FUN:
        get_func_repr(&expr->fun, buff, brack_count);
        break;
    case GRP:
        get_grp_repr(&expr->group, buff, brack_count);
        break;
    }
}

void print_expr(const Expr *expr){
    char local_buff[50] = "";
    _print_expr(expr, local_buff, 0);
    printf("%s\n", local_buff);
}


void more_complex_grp_repr(){
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
    

    
    Expr const *evalled_expr = eval_expr(cmplx_grp);
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

void func_grp_weird_thing(){
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

void test_restructuring(){
    Expr const *var_x = create_var("x");
    Expr const *func_x = create_func(var_x, var_x);

    Expr const *var_y = create_var("y");
    Expr const *func_y = create_func(var_y, var_y);

    Expr const *var_a = create_var("a");
    Expr const *var_b = create_var("b");
    Expr const *var_c = create_var("c");
    Expr const *var_d = create_var("d");

    Expr const *unstructured = create_grp(
            create_grp(var_a, func_x), create_grp(create_grp(var_b, func_y),  var_d)
            );
    printf("==== structuring test ====\n");
    printf("Unstructured ");
    print_expr(unstructured);

    Expr const *structured = simplify_expr(unstructured, 'r');
    printf("structured ");
    print_expr(structured);
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

    //Group
    Expr const *const group_expr = create_grp(nested_func, var);
    print_expr(group_expr);

}

int main(){
    basic_test();
    more_complex_grp_repr();
    /* test_inf(); */
    func_grp_weird_thing();
    test_restructuring();

    free_all();
};
