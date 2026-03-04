#include <complex.h>
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

typedef enum {
    VAR,
    FUN,
    GRP
} ExprType;

typedef struct
{
    struct Expr *arg; // do we want to allow for non-var expr? 
    // this can be an easy way to reduce the chances of capture cases, we can just check memory addr
    struct Expr *body;
} Func;

typedef struct {
    struct Expr *lhs;
    struct Expr *rhs;
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

Expr *create_var(const char *var){
    if (var == NULL){
        return NULL;
    }

    Expr *var_expr = (Expr *)malloc(sizeof(Expr));
    var_expr->expr_type = VAR;
    var_expr->content.var = var;

    return var_expr;
}  

Expr *create_func(Expr* arg, Expr *body){
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

    return func_expr;
}

Expr *create_grp(Expr *lhs, Expr *rhs){
    if (lhs == NULL || rhs == NULL){
        return NULL;
    }

    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->content.group.lhs = lhs;
    grp_expr->content.group.rhs = rhs;

    return grp_expr;
}
void print_expr(const Expr *expr, char *buff);
/*
print_for_var -> string
print_for_func -> f",\{arg}." + if arg-> print_arg, if func print_
*/
Expr *eval_expr(Expr *exp);

Expr *replace_vars(Expr *body, const Expr *arg_expr , Expr *rplc_expr){
    switch (body->expr_type) {
        case VAR:
            return (body == arg_expr) ? rplc_expr : body;
        case FUN:
            break;
        case GRP:
            return eval_expr(create_grp(
                        replace_vars(body->content.group.lhs, arg_expr, rplc_expr),
                        replace_vars(body->content.group.rhs, arg_expr, rplc_expr)
                ));
        }
    body->content.fun.body = replace_vars(body->content.fun.body, arg_expr, rplc_expr);
    return body;
}

// Expr *eval_func(Expr *func, Expr *rhs){
//     /* Var -> if var addr matches, then replace, otherwise just return Var 
//      * 
//     */
//     Expr *func_body = func->content.fun.body;
//
//     const Expr *func_arg = func->content.fun.arg;
//
//     return replace_vars(func_body, func_arg, rhs);
//
//     switch (rhs->expr_type) {
//         case VAR:
//         case FUN:
//             return replace_vars(func_body, func_arg, rhs);
//         case GRP:
//             break;
//     }
//     Expr *grp_lhs = rhs->content.group.lhs;
//     Expr *grp_rhs = rhs->content.group.rhs;
//     return create_grp(replace_vars(func_body, func_arg, grp_lhs), grp_rhs);
// }

Expr *eval_grp_expr(Expr *grp_expr){
    if (grp_expr == NULL){
        return NULL;
    }
    Expr *lhs = grp_expr->content.group.lhs;
    Expr *rhs = grp_expr->content.group.rhs;

    switch (lhs->expr_type) {
        case VAR:
            if (rhs->expr_type == VAR){
                return grp_expr; // just to stop from too much mem being allocated
            }
            return create_grp(lhs, eval_expr(rhs));
        case FUN:
            return replace_vars(lhs->content.fun.body, lhs->content.fun.arg, rhs);
        case GRP:
            return create_grp(eval_grp_expr(lhs), eval_expr(rhs));
    }
}

Expr *eval_expr(Expr *exp){
    if (exp == NULL){
        return NULL;
    }
    switch (exp->expr_type) {
    case VAR:
    case FUN:
        return exp; // we are not able to evaluate a variable or function by itself
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


void _print_expr(const Expr* expr, char *buff, int brack_count){
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

void print_expr(const Expr *expr, char *buff){
    _print_expr(expr, buff, 0);
}


void more_complex_grp_repr(){
    /* this function only tests repr, not eval
     *  (λx.(λy.(x y) g))(λx. x x)
     *
     * 
    */
    Expr *var_x = create_var("x");
    Expr *var_y = create_var("y");
    
    Expr *func_y_body_lhs_grp = create_grp(var_x, var_y);
    Expr *var_g = create_var("g");

    Expr *func_y_body_grp = create_grp(func_y_body_lhs_grp, var_g);
    Expr *func_y = create_func(var_y, func_y_body_grp);

    Expr *func_x = create_func(var_x, func_y);

    Expr *rhs_grp_x = create_var("x");

    Expr *rhs_grp_func_body = create_grp(rhs_grp_x, rhs_grp_x);
    Expr *rhs_grp_func = create_func(rhs_grp_x, rhs_grp_func_body);

    Expr *cmplx_grp = create_grp(func_x, rhs_grp_func);

    char buff[50] = "";
    print_expr(cmplx_grp, buff);
    printf("before eval %s\n",buff);

    char new_buff[50] = "";
    Expr *evalled_expr = eval_expr(cmplx_grp);
    print_expr(evalled_expr, new_buff);
    printf("after eval %s\n", new_buff);

    free(cmplx_grp);
    free(rhs_grp_func);
    free(rhs_grp_func_body);
    free(rhs_grp_x);
    free(func_x);
    free(func_y);
    free(func_y_body_grp);
    free(var_g);
    free(func_y_body_lhs_grp);
    free(var_y);
    free(var_x);
}

int main(){
    // Var test
    Expr *var = create_var("test");
    char buff[50] = "";
    print_expr(var, buff);
    printf("%s\n",buff);
    
    // Func test
    Expr *func = create_func(var, var);
    char func_buff[50] = "";
    print_expr(func, func_buff);
    printf("%s\n", func_buff);

    // Nested Func test
    Expr *nested_func = create_func(var, func);
    char nested_func_buff[50] = "";
    print_expr(nested_func, nested_func_buff);
    printf("%s\n", nested_func_buff);

    //Group
    Expr *group_expr = create_grp(nested_func, var);
    char group_buff[50] = "";
    print_expr(group_expr, group_buff);
    printf("%s\n", group_buff);

    more_complex_grp_repr();

    free(var);
    free(func);
    free(nested_func);
    free(group_expr);
    //group
};
