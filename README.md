# Lambda Calculus Interpreter in C

A lambda calculus interpreter written in C featuring beta reduction, Church encodings, recursion through the Y combinator, and a symbolic representation system for common lambda expressions.

The project was built to explore the implementation of the untyped lambda calculus from first principles while keeping the core evaluator simple and efficient.

---

## Features

- Expression tree representation of lambda calculus
  - Variables
  - Lambda abstractions
  - Function application
- Beta reduction evaluator
- Capture-free substitution by design (no alpha conversion required)
- Church booleans (`TRUE`, `FALSE`)
- Church numerals
- Arithmetic operations
  - Addition
  - Multiplication
  - Predecessor
- Zero testing
- Recursive functions using the Y combinator
- Trie-based symbolic expression lookup
- Pretty-printing of lambda expressions
- Lightweight automatic memory management

---

## Design

Expressions are represented as immutable trees.

```text
Expr
├── Variable
├── Function
│   ├── Argument
│   └── Body
└── Application
    ├── Left
    └── Right
```

Every expression is dynamically allocated and never modified after creation, allowing existing expressions to be safely shared throughout the evaluator.

---

## Variable Capture

Most lambda calculus interpreters compare variables by their names, meaning substitution must rename bound variables whenever a capture could occur.

This interpreter instead represents every variable as its own unique object.

For example,

```text
λx. λx. x
```

contains **two distinct variable objects**, even though both are printed as `x`.

During substitution, variables are compared using **pointer identity** rather than their textual names.

As a result:

- Variable capture cannot occur.
- Alpha conversion is unnecessary.
- Substitution remains simple and efficient.
- Variable names exist purely for readability when printing expressions.

This behavior is built directly into the representation of expressions rather than being handled as a special case during evaluation.

---

## Symbolic Expression Representation

Complex lambda expressions can be registered with human-readable names.

For example, instead of printing

```text
λn.λm.λf.λx.(m f)((n f)x)
```

the interpreter can display

```text
+
```

This is implemented using a trie that maps expression structures to symbolic representations.

The same mechanism is used for expressions such as:

- `TRUE`
- `FALSE`
- `+`
- `*`
- `-1`
- `is_zero`
- `U`

making printed expressions significantly easier to read.

---

## Built-in Church Encodings

### Booleans

- `TRUE`
- `FALSE`

### Numerals

Church numerals can be generated for any non-negative integer.

```c
Expr const *three = get_church_numerals(3);
Expr const *five  = get_church_numerals(5);
```

---

## Built-in Operations

The interpreter currently includes implementations of:

- Addition
- Multiplication
- Predecessor (`n - 1`)
- Zero test (`is_zero`)
- Y combinator
- Recursive factorial

All of these are implemented entirely in lambda calculus using Church encodings.

---

## Evaluation

Expressions are evaluated using repeated beta reduction until a normal form is reached.

```c
Expr const *result = full_eval_expr(expr);
```

Evaluation performs:

- Function application
- Recursive reduction
- Capture-free substitution
- Normal-form evaluation

---

## Memory Management

Every created expression is automatically registered with a lightweight garbage collector.

Instead of manually freeing every expression, the interpreter releases every allocated node when execution finishes.

The expression representation trie is also cleaned up before program termination.

---

## Building

Compile using GCC:

```bash
gcc -std=c11 -O2 main.c -o lambda
```

Run:

```bash
./lambda
```

---

## Example

Creating and evaluating the identity function:

```c
Expr const *x = create_var("x");

Expr const *identity = create_func(x, x);

Expr const *expr = create_grp(identity, create_var("y"));

Expr const *result = full_eval_expr(expr);

print_expr(result);
```

Output:

```text
y
```

---

## Current Demonstrations

The project currently demonstrates:

- Basic lambda expression construction
- Beta reduction
- Church numeral generation
- Addition
- Multiplication
- Predecessor
- Zero testing
- Recursive factorial using the Y combinator
- Symbolic expression lookup
- Pretty-printing