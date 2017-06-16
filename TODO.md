# TODO #

## Next few days ##
* Test failing cases for type variables, to make sure that the _expected_ errors occur.
* Add TypedExprTypeMutator (mutates the types in a TypedExpr according to some TypeMutator), use for interpretation printing.

## Next few weeks ##
* Top-down resolver algorithm (to compare to my existing bottom-up algorithm - I'll finally have some real data when I'm done this)
* Generic types in resolver prototype (right now I can't model pointers, which are a sort of generic type, and the exponential failure cases in the existing resolver need either pointers or generics to trigger)

## Next few months ##
* more resolver algorithms (hybrid traversal orders, lazy evaluation, etc.)
* investigate user-defined conversions
* maybe model traits in the prototype (I have some hazy ideas about using them as a caching mechanism, possibly to cut off the exponential failure case)