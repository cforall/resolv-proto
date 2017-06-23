# TODO #

## Next few days ##
* Modify FuncTable to also index by type
  * This may need a map adapter that can pass a query down to its sub-map to handle poly types, or possibly a similar adapter for type_map
  * Might just need a flatten operator/iterator (do you already have one?)

## Next few weeks ##
* Top-down resolver algorithm (to compare to my existing bottom-up algorithm - I'll finally have some real data when I'm done this)
* Generic types in resolver prototype (right now I can't model pointers, which are a sort of generic type, and the exponential failure cases in the existing resolver need either pointers or generics to trigger)

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, etc.)
* investigate user-defined conversions
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)

## Indeterminate Future ##
* Nail down exact semantics of type bindings, type assertions, cost model; cases in convert_amb.in would be a good start.