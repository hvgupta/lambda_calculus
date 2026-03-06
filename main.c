#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
(,\a. a) (,\ p. p p)
def a (b) -> b

a((,\ p. p p)) -> (,\ p. p p)

(l y. lx. y) a b
(lx. a) b

a


(l y. lx. y) x b
(lx. x) b

b


f() x
(l a: (l b: b (l c: a c))) x y z



,\ a: a s d f

Groups -> ((a s) d) f

,\a: (a ,\b: b)

(l b: b (l c: a c)) y z

app(func(arg, body), expr)

Tsoding

Var
App
Fun
*/



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
    } content;
};

Expr const *const create_var(const char *var){
    if (var == NULL){
        return NULL;
    }

    Expr *var_expr = (Expr *)malloc(sizeof(Expr));
    var_expr->expr_type = VAR;
    var_expr->content.var = var;

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
    
    func_expr->content.fun.arg = arg;
    func_expr->content.fun.body = body;

    insert_into_gc(func_expr);

    return func_expr;
}

Expr const *const create_grp(Expr const *lhs, Expr const *rhs){
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->content.group.lhs = lhs;
    grp_expr->content.group.rhs = rhs;

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
        return create_func(body->content.fun.arg, replace_vars(body->content.fun.body, arg_expr, rplc_expr));

    } else if (body->expr_type == GRP){
        return eval_expr(create_grp(
                    replace_vars(body->content.group.lhs, arg_expr, rplc_expr), 
                    replace_vars(body->content.group.rhs, arg_expr, rplc_expr)
                ));
    }
    
    return NULL;
}


Expr const *eval_grp_expr(Expr const *grp_expr){
    if (grp_expr == NULL){
        return NULL;
    }
    Expr const *lhs = grp_expr->content.group.lhs;
    Expr const *rhs = grp_expr->content.group.rhs;

    if (lhs->expr_type == VAR){
        if (rhs->expr_type != GRP){
            return grp_expr; // just to stop from too much mem being allocated
        }
        return create_grp(lhs, eval_expr(rhs));
    } else if (lhs->expr_type == FUN){
        Expr const *func_body = lhs->content.fun.body; 
        Expr const *func_arg = lhs->content.fun.arg; 

        return replace_vars(func_body, func_arg, eval_expr(rhs));
    } else if (lhs->expr_type == GRP){
        Expr const *new_lhs = eval_grp_expr(lhs);
        Expr const *new_rhs = eval_expr(rhs);
        
        if (new_lhs->expr_type == GRP && new_rhs->expr_type == GRP){
            return create_grp(new_lhs->content.group.lhs, 
                    eval_expr(create_grp(new_lhs->content.group.rhs, new_rhs)
                ));
        }

        return create_grp(eval_grp_expr(lhs), eval_expr(rhs));
    }
    
    return NULL;
}

Expr const *eval_func(Expr const *func){
    return create_func(func->content.fun.arg, eval_expr(func->content.fun.body));
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


void _print_expr(const Expr* expr, char *buff, int brack_count);
    
 
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
        get_var_repr(expr->content.var, buff, brack_count);
        break;
    case FUN:
        get_func_repr(&expr->content.fun, buff, brack_count);
        break;
    case GRP:
        get_grp_repr(&expr->content.group, buff, brack_count);
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
     *
     * 
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

int main(){
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

    more_complex_grp_repr();
    /* test_inf(); */
    func_grp_weird_thing();

    //group
    //
    free_all();
};
