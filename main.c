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
 
void get_var_repr(const char *var, char *buff, int brack_count){
    // actually these function would be an internal function, the main entry point of this function will be from print_expr
    if (var == NULL || buff == NULL){
        return;
    }
    strcat(buff, var);

    if (brack_count > 0){
        strcat(buff, ")");
    }
}

void get_func_repr(const Func *func, char *buff, int brack_count){
    if (func == NULL || buff == NULL) {
        return; 
    }
    strcat(buff, ",\\");
    strcat(buff, func->arg);
    strcat(buff, ".");

    switch (func->body->expr_type){
    case VAR:
        get_var_repr(func->body->content.var, buff, brack_count);
        break;
    case FUN:
        get_func_repr(&func->body->content.fun, buff, brack_count);
        break;
    case GRP:
        break;
    }
}


void print_expr(const Expr* expr){
    switch (expr->expr_type)
    {
    case VAR:
        printf("%s\n", expr->content.var);
        break;
    case FUN:
        /*
            while loop, 
                - where I track the '(',
                (expr1 expr2)
        */
        const char* cur_string = "";

        break;
    default:
        printf("this is currently not possible");
        break;
    }
}


int main(){
    // Var test
    Expr *var = create_var("test");
    char buff[50] = "";
    get_var_repr(var->content.var, buff, 0);
    printf("%s\n",buff);
    
    // Func test
    Expr *func = create_func("test", var);
    char func_buff[50] = "";
    get_func_repr(&func->content.fun, func_buff, 0);
    printf("%s\n", func_buff);
};
