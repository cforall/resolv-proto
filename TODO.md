# TODO #

## Next few days ##
* Write top-down resolver algorithm
* Modify semantics of top-level resolution to resolve to `void`
* Rewrite uses of `merge()` in `resolve_assertions.h` to use `flattenOut()`
  * possibly make `Env` a `GC_Object`, change `unique_ptr<Env>&` to `Env*` as needed

## Next few weeks ##
* Generic types in resolver prototype (right now I can't model pointers, which are a sort of generic type, and the exponential failure cases in the existing resolver need either pointers or generics to trigger)

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, caching resolver queries, etc.)
* investigate user-defined conversions
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)

## Indeterminate Future ##
* Nail down exact semantics of type bindings, type assertions, cost model; cases in convert_amb.in would be a good start.
* Investigate better fundamental data structures - LLVM's ShortVec, ShortMap, etc. might be useful (check licence compatibility, but you may be able to just take them.)