# TODO #

## Next few days ##
* Investigate whether "bif" memory usage in TD resolver is bug or algorithmic failure
  * Likely algorithmic failure, investigate caching argument values between function options
  * Initial caching experiments positive, look into including top-level cache in resolver to allow shared grandchild interpretations
  * Look into loosening "Environment" restrictions on cache ... you want them for sibling interpretations, but but not for child, where you can just replace the target type according to the environment
    * Cache hit data suggests that the hit ratio is fine, top-down just uses too much memory for this instance.
  * Caching results (miss : hit (miss%))
    * pre-bif         -- 45119 :    42550 (51%)
    * bif (truncated) --  5146 : 12402771 ( 0.04%)
  * Timing results
    * pre-bif (td)          --  0.26s user, 0.06s system
    * pre-bif (bu)          --  0.38s user, 0.02s system
    * pre-bif (td;no-cache) --  0.89s user, 0.16s system
* Benchmark top-down and bottom-up resolvers against each other

## Next few weeks ##
* Modify semantics of top-level resolution to resolve to `void`
* Possibly distinguish in AST between Type and Multitype (just an alias for List<Type> -- empty list is VoidType, list > 1 is TupleType)
  * Look at tuple truncations as conversions in expand_conversions
* Rewrite uses of `merge()` in `resolve_assertions.h` to use `flattenOut()`
  * possibly just take advantage of new Env as GC_Object
  * audit uses of `Env::from` and trim cases where they're not actually modified
* Generic types in resolver prototype (right now I can't model pointers, which are a sort of generic type, and the exponential failure cases in the existing resolver need either pointers or generics to trigger)

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, caching resolver queries, etc.)
* investigate user-defined conversions
  * It's possible that polymorphic conversions will basically push this on us anyway...
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)

## Indeterminate Future ##
* Refine cost model -- look at cases in `convert_amb`
* Possibly change mapped assertion value to an Interpretation from a TypedExpr
* Investigate better fundamental data structures - LLVM's ShortVec, ShortMap, etc. might be useful (check licence compatibility, but you may be able to just take them.)
  * small-set optimization on TypeMap may also be a good idea
