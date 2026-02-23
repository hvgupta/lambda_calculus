#include <stdio.h>
#include <stdlib.h>


/*
(,\a. a) (,\ p. p p)
def a (b) -> b

a((,\ p. p p)) -> (,\ p. p p)

(,\y.,\x.y)x

app(func(arg, body), expr)

Tsoding

Var
App
Fun
*/


typedef enum {
    VAR,
    FUN,
    APP
} ExprType;

typedef struct Expr;

typedef struct
{
    const char * arg;
    Expr body;
} Func;


typedef struct {
    ExprType expr_type;
    union
    {
        const char * var;
        Func fun; 
    };
} Expr;


int main(){
    printf("hello world\n");
    printf("this is number %d\n", 10);
};
