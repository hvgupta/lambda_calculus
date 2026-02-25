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


typedef enum {
    VAR,
    FUN,
    GRP
} ExprType;

struct Expr;

typedef struct
{
    const char *arg;
    struct Expr *body;
} Func;

typedef struct {
    struct Expr *lhf;
    struct Expr *rhs;
} Group;

typedef struct {
    ExprType expr_type;
    union
    {
        const char *var;
        Func fun; 
        Group group;
    } content;
} Expr;

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

// void get_func_repr(const Func *func, char *buff, int brack_count){
//
// }


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
    printf("hello world\n");
    printf("this is number %d\n", 10);
};
