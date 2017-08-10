# TODO #

## Next few days ##
* Look at failing tests for top-down resolver
  * higher poly-cost: `multi_type_var`, etc.
    * Looks like it's double-counting the cost of the #b assignment somehow
  * resolution failure (no conversion on type variable): `assertion`, `convert_amb`
  * resolution failure (tuple): `tuple`
* Rewrite uses of `merge()` in `resolve_assertions.h` to use `flattenOut()`
  * possibly just take advantage of new Env as GC_Object
* Write concise-output version of Interpretation printer (elides environment for concision and more comparable tests)
* Re-write Makefile to make suffixed versions of the rp executables, consistent rp symlink
* Handle multi-parameter tuple arguments in multi-arg top-down resolver

## Next few weeks ##
* Look at caching argument calls between function resolutions in top-level resolver
* Modify semantics of top-level resolution to resolve to `void`
* Possibly distinguish in AST between Type and Multitype (just an alias for List<Type> -- empty list is VoidType, list > 1 is TupleType)
  * Look at tuple truncations as conversions in expand_conversions
* Generic types in resolver prototype (right now I can't model pointers, which are a sort of generic type, and the exponential failure cases in the existing resolver need either pointers or generics to trigger)

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, caching resolver queries, etc.)
* investigate user-defined conversions
  * It's possible that polymorphic conversions will basically push this on us anyway...
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)

## Indeterminate Future ##
* Refine cost model for assertions
  * Cases in `convert_amb` would be a good start.
  * Consider not including assertion costs in cost of resolving parent expression 
    * i.e. if there is a unique consistent min-cost resolution for all the assertions, take it, but only represent it in the cost tuple as -(#assns).
    * May need to split -(#generic type specializations) into separate counter if you do this
      * Nope, the count of nested assertions to satisfy assertion parameters should be ignored, just count 1 for each assertion parameter.
* Investigate better fundamental data structures - LLVM's ShortVec, ShortMap, etc. might be useful (check licence compatibility, but you may be able to just take them.)
