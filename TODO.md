# TODO #

* Modify semantics of top-level resolution to resolve to `void` (see `void.in` test case)
* Possibly distinguish in AST between Type and Multitype (just an alias for List<Type> -- empty list is VoidType, list > 1 is TupleType)
  * Look at tuple truncations as conversions in expand_conversions
  * Use Rob's semantics for tuples (just flatten to lists, maybe truncate at top-level)
    * Check if `tuple-return.in` has any better performance afterward
* more resolver algorithms (hybrid traversal orders, lazy evaluation, caching resolver queries, etc.)
  * Try a variant of `bu`/`td` where it tries to find the combos **before** the next argument (if there's only one, the base environment can be updated)
* investigate user-defined conversions
  * It's possible that polymorphic conversions will basically push this on us anyway...
* look up unification data structures literature
  * Martelli & Montanari '82 "An efficient unification algorithm" describes types as sharing common structure (DAG subgraphs), reduces unification time if you're careful (I might already be doing this; it's also Bilson03 cite. 25)
  * Knight '89 "Unification: A multidisciplinary survey" seems like another promising lit review root
* investigate problem as dynamic programming
* Possibly change mapped assertion value to an `Interpretation` from a `TypedExpr`
* Investigate better fundamental data structures - LLVM's ShortVec, ShortMap, etc. might be useful (check licence compatibility, but you may be able to just take them.)
  * small-set optimization on TypeMap may also be a good idea
* Ideas for improvements to TD resolver:
  * `polys` loop in `resolveTo` should environmentally bind the target type to the poly type _before_ resolution (in lieu of after-the-fact unification)
  * Investigate TD cache duplication over environments, may be the suspect for deep-nesting problems
    * Maybe cache against null environment unless argument is basic type
    * Maybe just don't cache against environment -- can you make lower environment independent?
  * TD variant that builds a "filter set" of types to search for, avoids caching
