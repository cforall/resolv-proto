## Input/Output Format ##
There is a full description of the grammar in a code comment in [`parser.h`](../src/driver/parser.h), but here is a summary of the input format:

* A resolver-prototype input file consists of a list of newline-separated declarations followed by a `%%` separator line, followed by a list of newline-separated expressions (this simulates a CFA source file)
  * The expressions are block-scoped, so you can put in a curly-brace-surrounded block with declarations-`%%`-expressions anywhere an expression is valid (including inside blocks). Declarations are lexically-scoped (only used for resolution of expressions inside the block they’re contained in)
  * C-style `//` single-line comments are available
* Declarations can be either function declarations or variable declarations
  * A function has zero-or-more return types, a name (with optional tag that’s not part of the name, but lets you figure out which overload was selected), and zero-or-more parameter types, all space-separated (except the tag, which is separated from the name by a `-`)
    * e.g. `foo f(bar, baz);` would be `#foo f-tag #bar #baz`
  * A variable declaration has a single type, an `&`-prefixed name, and an optional tag
    * e.g. `foo x;` would be: `#foo &x-foo_overload`
  * Function declarations can have type assertions, the syntax for which is basically identical to the existing declarations (except no tags)
    * e.g. `forall(dtype T, dtype S | {T max; T conv(S);}) T blah(T, S);` would be `T blah T S | T &max | T conv S`
    * Note that any alphanumeric string starting with an uppercase character is a type variable, introduced implicitly
* Expressions have three formats: literal value of type, variable reference, and function call
  * Any valid type represents a literal value of that type (the actual value is a runtime detail that doesn’t matter to the resolver, so why write `NULL` when you could just write `#void_ptr`)
  * You can name a variable with `&name` – e.g. `&x` says “resolve variable `x`” (there may be multiple declarations with multiple types)
  * A function call looks similar to a C function call, but without commas.
    * e.g. `f(g(x), y)` would be `f( g( &x ) &y )`
    * The arguments to the function call can be any valid expression
* The resolver prototype supports four kinds of types: concrete (think integral), named (think struct), polymorphic type variables, and function types
  * Concrete types are represented by a single integer
    * there is an implicit conversion from _x_ to _y_ with cost |_x_-_y_| (safe if _x_ < _y_, unsafe if _x_ > _y_)
    * These are generally used for the numeric types, something like `int` => `1`, `long` => `2`, `double` => `3` (depending on your needs for a given test case)
  * Named types have alphanumeric names starting with a `#`
    * These are generally used for struct types, e.g. `struct foo` => `#foo`
    * Named types may have generic type parameters in a space-separated `<>` list
      * e.g. `map(string, int)` might be `#map<#string 1>`
    * If you want to encode pointers or arrays, `#ptr<#string>` or `#arr<#string>` is about as well as you’ll do in the current prototype
  * Polymorphic type variables are any uppercase string, implicitly declared (per declaration) at first use, as mentioned above
  * Function types are mostly only needed in certain type assertions, but are formatted as follows:
    * `foo (*)(bar, baz)` would be `[ #foo : #bar #baz ]`

The output format can be modified by some command-line arguments, but the default looks like this (this example from running `rp` on [`assertion.in`](../tests/assertion.in)):

```
[1 / (0,1,1,-1,0)]{T.1 => 1 | T.1 f T.1 => f( 1 )} g{T.1 | T.1 f T.1}( 1 )

[1 / (1,1,1,-1,0)]{T.1 => 1 | T.1 f T.1 => f( 1 )} g{T.1 | T.1 f T.1}( 2 => 1 )

[Void / (0,1,1,-1,0)]{T.2 => 1 | T.2 x => &x} h{T.2 | T.2 x}( 1 )
```

* The first bit (before the `/`) is the return type of the expression
* The next bit is the cost tuple (not identical to CFA-CC, there’s a couple fields there not represented by the resolver prototype)
* After that in curly braces is the set of type and assertion bindings
  * The first line says that type variable `T.1` is bound to type `1`
    * It’s possible to have distinct instances type variables with the same name, so the resolver prototype adds a numeric disambiguator – CFA-CC just changes the variable name.
  * It also says that assertion `T.1 f T.1` is bound to `f(1)`, the expression that satisfies it
* After the curly braces comes the typed expression that the untyped expression resolves to
  * Looking at the second line, this says we’re using function `g` (the tag would show up here if present), includes information about `g`’s assertions in curly braces so you can match them to the bindings (it can be tricky to figure out what `T.1` vs. `T.2` is in a more-deeply-nested example), and then says that the argument is a literal of type `2`, implicitly (unsafely) cast to type `1`.
