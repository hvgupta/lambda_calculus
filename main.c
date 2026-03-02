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
    const char *arg;
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
    Expr *var_expr = (Expr *)malloc(sizeof(Expr));
    var_expr->expr_type = VAR;
    var_expr->content.var = var;

    return var_expr;
}  

Expr *create_func(const char *arg, Expr *body){
    Expr *func_expr = (Expr *)malloc(sizeof(Expr));

    func_expr->expr_type = FUN;
    
    func_expr->content.fun.arg = arg;
    func_expr->content.fun.body = body;

    return func_expr;
}

Expr *create_grp(Expr *lhs, Expr *rhs){
    Expr *grp_expr = (Expr *)malloc(sizeof(Expr));

    grp_expr->expr_type = GRP;

    grp_expr->content.group.lhs = lhs;
    grp_expr->content.group.rhs = rhs;

    return grp_expr;
}

/*
print_for_var -> string
print_for_func -> f",\{arg}." + if arg-> print_arg, if func print_
*/



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
    strcat(buff, func->arg);
    strcat(buff, ".");
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
    Expr *func_y = create_func("y", func_y_body_grp);

    Expr *func_x = create_func("x", func_y);

    Expr *rhs_grp_func_body_var1 = create_var("x");
    Expr *rhs_grp_func_body_var2 = create_var("x");

    Expr *rhs_grp_func_body = create_grp(rhs_grp_func_body_var1, rhs_grp_func_body_var2);
    Expr *rhs_grp_func = create_func("x", rhs_grp_func_body);

    Expr *cmplx_grp = create_grp(func_x, rhs_grp_func);

    char buff[50] = "";
    print_expr(cmplx_grp, buff);

    printf("%s\n",buff);

    free(cmplx_grp);
    free(rhs_grp_func);
    free(rhs_grp_func_body);
    free(rhs_grp_func_body_var2);
    free(rhs_grp_func_body_var1);
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
    Expr *func = create_func("test", var);
    char func_buff[50] = "";
    print_expr(func, func_buff);
    printf("%s\n", func_buff);

    // Nested Func test
    Expr *nested_func = create_func("test", func);
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
