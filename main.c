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

typedef struct Expr;

typedef struct
{
    const char * arg;
    Expr body;
} Func;

typedef struct {
    Expr lhf;
    Expr rhs;
} Group;

typedef struct {
    ExprType expr_type;
    union
    {
        const char * var;
        Func fun; 
        Group group;
    } content;
} Expr;

/*
print_for_var -> string
print_for_func -> f",\{arg}." + if arg-> print_arg, if func print_
*/

void print_expr(const Expr* expr){
    switch (expr->expr_type)
    {
    case VAR:
        printf("%s\n", expr->content.var);
        break;
    case FUN:
        /*
            while loop, 
                - wherre I track the '(',
                (expr1 expr2)
        */
        const char* cur_string = "";



        printf(",\\%s. %s");
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
