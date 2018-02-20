# TODO #

## Next few days ##
* generate `overloads-more2`
* Run TD experiments on DEF/IMM res
* Look at using this (or something like):
```
#if defined NASSERT
#define unreachable(msg) __builtin_unreachable();
#else
#define unreachable(msg) assert(!msg);
#endif
```

## Next few weeks ##
* Modify semantics of top-level resolution to resolve to `void` (see `void` test case)
* Possibly distinguish in AST between Type and Multitype (just an alias for List<Type> -- empty list is VoidType, list > 1 is TupleType)
  * Look at tuple truncations as conversions in expand_conversions
  * Use Rob's semantics for tuples (just flatten to lists, maybe truncate at top-level)
    * Check if `tuple-return.in` has any better performance afterward
* Rewrite uses of `merge()` in `resolve_assertions.h` to use `flattenOut()`
  * possibly just take advantage of new Env as GC_Object
  * audit uses of `Env::from` and trim cases where they're not actually modified

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, caching resolver queries, etc.)
* investigate user-defined conversions
  * It's possible that polymorphic conversions will basically push this on us anyway...
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)

## Indeterminate Future ##
* Look at Martelli (Bilson03p.39 cite. 25) for linear-space type representation, (ibid. cite. 4) for set operations on types
* Refine cost model -- look at cases in `convert_amb`
* Possibly change mapped assertion value to an Interpretation from a TypedExpr
* Investigate better fundamental data structures - LLVM's ShortVec, ShortMap, etc. might be useful (check licence compatibility, but you may be able to just take them.)
  * small-set optimization on TypeMap may also be a good idea
* Ideas for improvements to TD resolver:
  * `polys` loop in `resolveTo` should environmentally bind the target type to the poly type _before_ resolution (in lieu of after-the-fact unification)
  * Investigate TD cache duplication over environments, may be the suspect for deep-nesting problems
    * Maybe cache against null environment unless argument is basic type
    * Maybe just don't cache against environment -- can you make lower environment independent?
  * TD variant that builds a "filter set" of types to search for, avoids caching
* refactoring (?):
  * Rewrite the mergeClasses() algorithm to have a public entry point, count cost
  * classBinds in `expand_conversions.h` should be merged in to TypeUnifier or something
  * TypeUnifier should maybe take `env` by reference


