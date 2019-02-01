## Debugging Notes ##
* Setting up stdlib file for use:
  ```
  cd libcfa/src
  ../../driver/cfa -I. -I../.. -no-include-stdhdr -Istdhdr -XCFA -t -E rational.cfa > ~/Code/cfa-foo/rational-e.cfa
  cd ../..
  driver/cfa-cpp -t --prelude-dir=libcfa/x64-debug/prelude ~/Code/cfa-foo/rational-e.cfa
  ```
* Have ASAN print memory details: `call (void)__asan_describe_address(0x60e000162b80)`

## 28-30 Jan 2019 ##
* Get TD resolver building again to run tests
  * **TODO** Look into making "vars" list not flatmapped, you always end up using the whole thing
    * alternately, do something a bit more clever when you've got a target type
  * fixed assertion failure in `fnptr` to handle `FuncType` in `expand_conversion.h`
  * second assertion failure in `recursive`; "classes must be versions of same map" (`env.h:361`)
    * `classes = 0x729d40, oclasses = 0x72a860`
    * this environment doesn't seem to work with TD resolver caching, which precludes the environments being shared...
* Set up flags for choosing environment data structure
  * PER and ITI build and pass tests for BU-TEC

## 16-28 Jan 2019 ##
* Thesis writing
  * new `cfa-thesis` repo
  * Background work for resolver
  * Discussion of resolution algorithms
  * Analysis of environment algorithms

## 18-22 Jan 2019 ##
* Continue porting type-environment-cached resolution to CFA-CC
  * first compiling version
  * builds stdlib without error
  * passes all tests, ~1.5 speedup, pushed to master

## 15-17 Jan 2019 ##
* Worked on bug #118 for Thierry
  * minimal case:
    ```
    unsigned long long dev = 0;
    unsigned ret = (unsigned)(dev >> 32);
    ```
    * Incorrectly casts `dev` to `unsigned`
    * `castExpr == 0x331ef70`
  * long discussion with Peter, Thierry & Andrew:
    * proper semantics of cast is "choose the cheapest argument interpretation, break ties by cast conversion cost"
    * Issue here is changes you made to support auto-newline I/O in new cost model
      * going to try taking return type out of specialization cost
      * this builds and passes all tests
    * built with `AlternativeFinder.cc:488-490` commented out
      * works, cut the Richard approach to assertions out

## 11 Jan 2019 ##
* Continued porting type-environment-cached resolution to CFA-CC
* **TODO** Figure out how to only combine each defer item once
  * second question: do you *need* to always choose the same answer for cache-same?
  * well, the types *should* have been bound by formal-actual binding, so any free type variables should be set to the unique cheapest value, which should be uniform
* **TODO** check if you can make a `mangleAssn` variant that will do the `resType` key in `resolveAssertions`

## 7-11 Jan 2019 ##
* Started looking at OOM on `io` tests
  * no obvious threads from memory analysis
  * going to try a 1-thread test run overnight sometime
    * ran to completion, times all under ~4 min
  * Looking through the valgrind leakcheck output
    * some "definitely lost" at `ResolveAssertions.cc:393`
      * these all look like `VoidType` (?)
      * added `delete resType` at `398`
      * builds and passes test suite
    * "definitely lost" at `ResolveAssertions.cc:354` (`ObjectDecl` in `Alternative`)
      * similar case at line `401`
      * similar cases at `370/338` and `418/338`
      * deleted `need` from `Alternative` on destruction
      * builds and passes test suite
    * "definitely lost" at `ResolveAssertions.cc:254/AdjustExprType.cc:53`
      * also some "possibly lost" at `253`
      * looks like `adjType` not getting deleted
      * uncommenting `277` fixed bug (built and passed tests)
* looked at `ato` test and new newline code
  * ported modifications to `Cost` from other branch
  * re-ran memory tests, fixed minor output errors
  * pushed changes, for 1.5-2x speedup on longest-running tests
* Started porting manglename-key to CFA-CC for cached assertion resolution
  * not 100% sure about types which resolve to nested types containing others, but I think I can ignore that and just have slightly iffy cache efficiency
* looked into Mike's `zero_t` bug, filed bug report #115
  * think I remember the issue here was that a direct constructor for `int` makes expressions like `0 != 0` ambiguous

## 20-21 Dec 2018 ##
* Fix `searchsort` test
  * `?<?` overload was breaking loop control
* Attempt to merge deferred resolution branch
  * fails, doesn't handle Peter's new return-value-discriminated IO
* Raise `safe` cost above `var, spec`, based on results of return-value-discriminated IO
  * not enough, safe conversions are still the same there...
  * adding `thisCost` into first-round cost for `CastExpr` seems to make it work
  * need to regenerate `completeTypeError`
  * `io*` tests OOM the computer...
* Look at `cfa-foo/ionl.cfa` for trimmed-down case
  * Always selects `ostream&` over `void` return
  * Relevant expr is `(UntypedExpr *) 0x3327450`
  * Two candidates:
    ```
    [0] 
      cost = (0,0,0,0,0,0), cvtCost = (0,1,1,-4,0,1),
      expr = ApplicationExpr {
        function = `forall(dtype O | ostream(O)) O& ?|?( O& out, int x )`,
        args = [ `(struct stdout&)out`, `42` ]
      }
      thisCost (AlternativeFinder.cc:1229) = (0,0,0,0,2,0)
    
    [1]
      cost = (0,0,0,0,0,0), cvtCost = (0,1,1,-3,0,1),
      expr = ApplicationExpr {
        function = `forall(dtype O | ostream(O)) void ?|?( O& out, int x )`,
        args = [ `(struct stdout&)out`, `42` ]
      }
      thisCost (AlternativeFinder.cc:1229) = (0,0,0,0,0,0)
    ```
    * correct (1) survives `findMinCost` at `AlternativeFinder.cc:1114`
      * due to costs not being promoted out of `cvtCost` yet
    * also survives `pruneAlternatives` at `AlternativeFinder.cc:281
      * due to different return types
    * correct eliminated at `findMinCost` at `AlternativeFinder.cc:1241`
      * reference type on `O&` makes it a titch more specialized, and specialization ranks above return-type match
    

## 19-23 Nov, 03-19 Dec 2018 ##
* First draft of deferred assertion binding
  * Fails in `concurrency/thread.cfa`
    * fixed with missing `renameTyVars` call in `resolveAssertion`, wasn't renaming assertion type
  * Takes ~2-3x as long to build
    * This is just running in debug/ASAN mode
* Second draft fails some tests:
  * `io2`, `tupleAssign`
    * translator crashes with segfault
    * fixed with missing clone in `bindAssertion`
  * `completeTypeError`
    * changed error message format, rebuild expected
  * `function-operator`
    * failure is at line 93 `iter.out = new(out);`
      * trimmed down to `funop-build.cfa`
    * `resolveAssertions` is called on `new(out)` subexpr, due to need to prune
    * prunes down to version that constructs `void*` from `ofstream*`
    * solved by stripping pruning step from `resolveAssertions`
      * this breaks other things, notably `rational`
    * tried making pruning conditional on `mode.resolveAssns` flag
      * still broke `rational`
    * pruning unconditionally to cheapest result(s) for a given result type seems to work
    * fixed `datingService`, `quickSort`, `multi-monitor` too
  * `searchSort`
    * multiple "No reasonable alternatives" errors
    * line 46: failing assertions for `bsearch`
      * function: `forall(otype K, otype E | {int ?<?(K, K); K getKey(const E&);}) E* bsearch(K key, const E* vals, unsigned long dim);`
      * args: `size - i` as `unsigned`, `iarr` as `const int*`, `size` as `unsigned long`
      * env: `_1877_10_K => unsigned int, _1877_11_E => signed int (no widening)`
        * `int ?<?(unsigned int, unsigned int);` exists in prelude
        * missing the `getKey` function, because it actually wants the earlier `K == E` case
          * of course, the lower overload is cheaper (because more specific), but only if it resolves...
          * consider attempting to fix this with a generic `getKey` overload
          * handled properly in resolver-proto, maybe due to the reified `AmbiguousExpr`
            * but the only places you prune, you also resolve assertions, which should boot the `getKey` version
            * this might be about the usual issues with type variable binding, then
      * line 47 fails as well due to missing `v` from previous line
      * 51/52, 70/71, 75/76 look like the same error pair
      * trimmed to `searchsort-build.cfa`
        * failing expression is `0x33219c0`
          * only the one alternative survives
          * we get 2 possible resolutions for `bsearch`, 1 for `i`, 1 for `iarr`, 1 for `size`
          * correct function has `_41_66_E => int` in return type narrowing pass
            * `formalType => _41_66_E => int`, `actualType => unsigned`
              * does not unify
        * if I comment out the wrong declaration, doesn't resolve either, but with "No reasonable alternatives" error.
          * ditto on HEAD
          * oddly, HEAD does seem to still work for the full `searchsort` example
          * and, replacing `i` with `i-0u` does seem to change `searchsort-build` so that it works on HEAD
          * HEAD converts `i-0u` to `(int)i-(int)0u`
        * `searchsort-build` with `i-0u`: failing expression is `0x3321dd0`
          * 18 alternatives for `i-0u`; `alternatives[8]` is correct
          * 3 alternatives survive `makeFunctionAlternatives`; first has `int ?-?`, second has `short ?-?`, third has `unsigned short ?-?`
          * deferred branch loses correct resolution at `AlternativeFinder.cc:1229`, when it does a `findMinCost` on the candidates (which are no longer guaranteed resolvable)
            * stripping that line out breaks `bootloader.c`, and is a non-starter
            * `findMinCost` has a cost-moving side-effect (who writes this code?), but duplicating it still breaks `bootloader`


## 29 Oct 2018 ##
* Union-find Seminar
* Attempt to set up deferred assertion binding

## 22-26 Oct 2018 ##
* Correctness debugging on deferred assertion resolution
  * missing inferred parameter bug from `GenPoly/Box.cc:801`
    * failure is at `rational.cfa:47: r{ (RationalImpl){0}, (RationalImpl){1} };`
    * I am a little suspicious of "anything with an empty idChain was pulled in by the current assertion" in this context
      * okay, at top level call to `inferRecursive`, nothing has any `idChain` set
      * seems like top-level empty chains will be set to `[curDecl->uniqueId]`, which feels wrong
      * then we follow the chains to set in the `inferParams`
        * which is likely wrong, because it's either going for the wrong root, or an old clone?
          * consider adding a unique ID for unification passes, storing assertions in environment by `(uniqueId,unifyId)`, and withdrawing them from there in `Box`
            * `functionType` in `addInferredParams` only comes from the one caller
              * derived from `appExpr`
              * only used for list of assertions (could maybe come from `appExpr`)
                * safer just to give each `appExpr` an `inferId` that keys into the environment's data
        * also need to consider ordering factors, need to make sure inner assertions are bound first
          * this should be accomplished by the BFS
    * repeats in `simprat.cfa:16`
      * `appExpr=0x60e0001b61e0`; didn't get any `inferParams` somehow
        * created at `AlternativeFinder.cc:208` in `pruneAlternatives`
        * unique IDs for contained assertions are `9477`, `9481`, `9483`, `9486`, `9488`, `9489`
          * first time in `bindAssertion` for `9477`:
            * `info.idChain` is empty
            * target: 
              ```
              ConstructorExpr @0x60d0017e0670 {
                inferParams = []
                callExpr = ApplicationExpr @0x60e00002b460 {
                  args = [
                    VariableExpr @0x60d0017e45d0,
                    VariableExpr @0x60d0017e4430,
                    VariableExpr @0x60d0017e4360
                  ]
                }
              }
              ```
          * second time: 
            * `info.idChain` also empty
            * target: 
              ```
              CastExpr @0x60d0019aae80 { arg = ConstructorExpr @0x60d0019aac10 {
                inferParams = [ ... ],
                callExpr = ApplicationExpr @0x60e000013520 {
                  args = [
                    VariableExpr @0x60d0019a7810,
                    VariableExpr @0x60d0019a7260,
                    VariableExpr @0x60d0019a6ff0
                  ]
                }
              } }
              ```
              * `arg` has a variety of `inferParams` set
  * `findUnfinishedKindExpr` now does *not* clone moving `winners` into `candidates`
  * moved assertion chain computation into `resolveAssertion` from `bindAssertion`

## 17-18 Oct 2018 ##
* Correctness debugging on deferred assertion resolution
  * `rational.cfa:37` can't choose between two different `endl` implementations
    * Both polymorphic, one for `ostream` at `iostream.hfa:92`, one for `istream` at `iostream.hfa:154`
    * seems to have been fixed by doing a resolution pass on both `resolvAssns` and `prune`
  * `rational.cfa:[many]` can't choose between different `abs` implementations to satisfy traits

## 15-17 Oct 2018 ##
* Memory error debugging on deferred assertion resolution
  * more disciplined cloning story on `need` lists/sets (need to clone internal DWT)
  * lost the memory management code in the shift over from Richard's code via GC branch

## 11-12 Oct 2018 ##
* Continued port of deferred cached assertion resolution to CFA-CC
  * Modified `Alternative` to also carry `openVars` and `need`
  * **TODO** write pass looking for ambiguous expressions in `findUnfinishedKindExpression`

## 05 Oct 2018 ##
* Moved `ResolvMode` flag pack over from GC branch in prep for deferred assertion resolution
* Started work on deferred cached assertion resolution in CFA-CC
  * copied `ResolvMode.h` over from GC branch
  * rewrote `findUnfinishedKindExpression` to pattern from resolver prototype

## 02-03 Oct 2018 ##
* Started rewrite of cfa-cc to use deferred cached resolution
* Rewrote `Cost` to use extra fields
  * regenerated output for `castError`
  * **TODO** Fix `computeApplicationConversionCost` in `AlternativeFinder.cc`
* Ported environment merging over old RP code

## 01-02 Oct 2018 ##
* Ran RP tests on CFA stdlib.
* RPDump now prints var args variable types
* Fixed deferred cache lifetime issues
  * deferIds/deferred cleared at the end of the run, assn_cache not

## 26-28 Sep 2018 ##
* Looked at `rational` test case
  * 6230 is slow: `$bitor( $bitor( $bitor( $bitor( &sout &a ) &b ) &c ) &endl )`
    * 8 candidates, 0th candidate post-sort demonstrates hang
    * call to `?|?( Ostype, Rational(RationalImpl) )` doesn't force the different 
      `RationalImpl` instantiations to be bound in a class.
    * Solution to incorporate bound types in key
      * Important to distinguish bound types from monomorphic types in mangling
* RP now parses function pointer literals as arguments
  * **TODO** canonicalize function types in parser
* stripped reference types from codegen (can be re-added when RP supports proper conversions)
* re-wrote constructor expressions to generate a variable expression rather than a function call
* corrected for implmentation of `{wchar,char{16,32}}_t` as `TypeInstType`
* normalized compiler-generated function declarations to collide with user declarations in RPDump
* **TODO** some tuple generation seems to be missing its spaces
* More detailed look at test suites
  * most resolution errors are due to the `zero_t => T*` conversion not existing
    * ditto for no `void* => T*` conversion
  * Others due to missing var-args support in prototype (including `ttype`)
  * There's a conditional around `__assert_fail` that consistently doesn't work
  * Can't assign to `dtype&`, e.g. in returns from builtin `++?`, etc.
  * No support for function operator in prototype resolution impacts `function-operator`
  * No support for `WithStmt` or `AsmStmt`
* Ran performance tests vs. CFA-CC
  * average 500x speedup, though CFA-CC is a multi-pass program

## 21-24 Sep 2018 ##
* Got on type-environment cached deferred resolution working
  * Assertion cache is keyed by mangle name of assertion, with poly-types normalized to class root
  * Re-formatted deferIds for cached form
  * 1.2s for `bu-tec` on `io1`, vs. ~15min for `bu-def`
  * `rational` seems to still have some performance issues...
* Modified RP `Forall` to map to `Decl`, not `FuncDecl`

## 20 Sep 2018 ##
* Begin investigating CFA instances in RP
  * io1 the only slow suite (15 mins)
  * io1.in:2919 takes a long time (22 levels deep)
  * io1.in:2921 also (24 levels deep)
  * io1.in:2923 also (23 levels deep)
  * all 3 slow instances bind ~12 polymorphic variables into the same class, then resolve the same ~15 assertions for all of them
    * class-based-caching in deferred resolution would probably help

## 11-20 Sep 2018 ##
* CFA => prototype instance translator
  * Added `#$` and `$` private names to prototype
  * Translator fixes operator names to prototype textual format
  * Translator handles global decls and parameter decls in sub-scopes
  * Translator translates expressions, including initialization and return
  * Translator encodes field accesses as functions in scope of struct declaration
  * Minor tweaks to resolv-proto parser and RPDump generation to work together
  * Added function type to resolver prototype
    * **TODO** look into types with ftype parameters ... maybe just get rid of them?
  * Added Name/VarExpr to resolver prototype (old VarExpr now ValExpr)
  * Added VarDecl to resolver prototype
  * **TODO** Look at hard-coding implicit conversion table into resolver prototype
  * **TODO** Look at hard-coding cast conversion table into ProtoDump
  * **TODO** Look at encoding reference type in resolver prototype
  * **TODO** Look at var-args type in resolver prototype

## 4-7 Sep 2018 ##
* Added lexical scopes to resolver
  * Variable shadowing is an issue -- maybe I can just not put the shadowed variables in the new scope, using a ScopedMap for the manglenames in the cfa-cc end

## 22-30 Aug 2018 ##
* Go back to debugging breadth-first resolution order
  * trim `kernel-e.c` down to just problem case `kernel-trim.c`
  * built minimal `kernel-construct.c` test case from problem
    * 4-character code bug to update the old resolution state instead of the new one on found deferred assertion...

## 3-21 Aug 2018 ##
* Vacation

## 11 June-30 July 2018 ##
* Start work on deferred resolution in CFA-CC
  * Move find flags over to `ResolvMode` struct
  * Augmented cost struct with vars/spec costs
  * Need to write pass to do resolution differently
    * build is crashing in `rational.c`
      * stdlib polymorphic `abs` conflicting with implicit `abs` as cheapest assertion resolution
      * **TODO** make sure that type parameters don't get counted against assertion parameter cost
        * probably openType/closedType
      * problem is that current resolver just takes first unifying assertion, doesn't consider cost
    * started refactoring to use breadth-first assertion resolution order from prototype
      * `concurrency/kernel.c` fails with a missing assertion error
        * **TODO** try and trim down input file to just one function call...
        * `appExpr=0x3ef25c0` type `long double (15, sizeof(...))` // looks wrong...
          * clone of `ApplicationExpr @0x3e72580` at `AlternativeFinder.cc:838 ResnList resns{...}`
            * built from `VariableExpr @0x3f54aa0` at `AlternativeFinder.cc:1385 appExpr = new ...`
        * chosen function is `forall( dtype T | is_thread(T) ) coroutine_desc* get_coroutine(T&)`
        * function type seems to be the same, `T` is `_13136_71_T`
          * environment maps `_13136_71_T => struct processorCtx_t`
        * ...
      * building, unbounded(?) memory usage in stdlib file `rational.c`
        * not unbounded, just enormous ...
          * `rational.c` involves a defer-set with sizes `{ 15, 21, 66, 66, 67, 9, 66 }`
          * At combo `{ 0, 0, 0, 5, 47, 7, 49 }` ~10GB mem usage, ~200K combos in output
            * It seems like `T` isn't getting bound in the output matches, thus they *all* match, 
              rather than just the 100 or so that should match
        * Functions are `{ T -?(T); int ?<?(T, T); T ?=?(T&, T); void ?{}(T&); void ?{}(T&, T); void ?{}(T&, zero_t); void ^?{}(T&) }`
          * In initial env, `RationalImpl` same type var as `T`, which is unbound
          * At combo `{0, 0, 4, 1, 16, 0, 9}` there are two `DT` variables. 
            * Should they be the same?
              * no, they're independent, but `T` should be bound to `DT*` and isn't
                * fixed this, needed to track open vars
  * BF order requires environment merging
    * `TypeEnvironment::combine` never used, was broken, so deleted
    * Decided to port environment structure from prototype
      * **TL;DR** No appreciable speed change; very slight increase in memory usage
      * Some trouble with `std::string` keys in existing impl, wrote interned string lib
      * Problematic enough to actually realize `EqvClass` as separate entity that will cut and 
        refactor
        * replaced `EqvClass` with `ClassRef` where appropriate
      * Some fixes involving not mutating in parallel with an iteration
      * Fixed some memory bugs relating to not actually tracing things properly...
      * Stack-overflow in TypeInstType substitution exposed by concurrency examples
      * Fixed infinite loop in `TypeSubstitution.cc` exposed by `concurrency/thread.c`
  * The implementations of `bindVar` and `bindVarToVar` in `Unify.cc` are appalling; far too many 
    copies and full-environment searches for the necessary purpose.
    * Backported the fixes to this into cfa-head, 5% speedup
    * Ported fix into GC branch, performance testing ongoing

## 07-08 June 2018 ##
* Looked into bug #91
  * Turns out it really needs user-defined conversions
* Updated user-defined conversions proposal

## 01-06 June 2018 ##
* Finished fixing bugs in new `Env`
  * `Interpretation::merge_ambiguous()` generated a fresh environment for the ambiguous 
    expressions in a different tree; fixed by giving these an invalid environment, and 
    checking for this invalid environment before merging environments in.
    * Added new test, `amb_env` to do regression on this bug
  * fixed bug in new `Env::mergeAllClasses` where two separate trees can still be in the same class
    * Was actually a bug where a non-root key was passed to merge from the ADDTO merger
      * was non-root because of a previous (elided) REMFROM in the edit list
      * similar bugs where non-root merge-to key exists, or classes have already been merged
  * Fixed bug where some added classes weren't getting bindings
  * Fixed bug where forgot to trace `NamedType::params_`
  
## 24 May-01 June 2018 ##
* Started performance testing of GC branch.
* Started trimming `clone` calls
  * Taking out `clone` in `Alternative` copies broke stdlib, reverted
* Took out the `EqvClass` copies in `TypeEnvironment::lookup`
  * backported

## 02 Apr-23 May 2018 ##
* Continue work on persistency-based `Env`
  * finish `persistent_disjoint_set`
  * add `persistent_disjoint_set` for classes, `persistent_map` for representatives to `Env`
  * refactor to single base environment (TODO fix)
  * finish `mergeAllClasses`
  * code compiles, passes test suite
    * have a bug, suspect it's in ambiguity handler
    * fixed one bug, still another, initial results show slightly-higher time, roughly 60% memory usage
* Track down some GC bugs
  ```
  cd src/prelude/
  gdb ../driver/cfa-cpp
  break GC.cc:72
  run -l prelude.cf OR run -tpm bootloader.cf OR run -tn ~/Code/cfa-foo/small.c 
  ```
  * at -O0: SEGV at same spot, object is 0x7fffffffdb48, address 18341 in GC::old
    * watching the address doesn't seem to work, maybe watch the GC index?
    * check address in GC_Object constructor, throw trap if #18341
  * In bootloader, seems to be a use-after-free on 0x60f00000e410
    * use: `ResolvExpr::AlternativeFinder::find(Expression*, bool, bool, bool) ResolvExpr/AlternativeFinder.cc:236` (main.cc:287)
    * free: `main () at main.cc:247` (collection after validate)
    * alloc: `build_binary_val () at Parser/ExpressionNode.cc:475` (parse at main.cc:221)
  * Tracing `trace` on `run -tn ~/Code/cfa-foo/small.c`
    * Turns out that `PassVisitor` skips `UntypedExpr::function`; fixed this bug
  * Added young-gen collection for resolver
    * Fixed bug with wrongly-marked graduates to old generation
    * Looked at use-after-free in `bootloader.c`
      * Object is `Attribute` in `ObjectDecl::attributes` in `FunctionType::returnVals` in `PointerType::base` in `VariableExpr::result` in `ApplicationExpr::function`
        * created at `AlternativeFinder.cc:1312 -- alt.expr->clone()` ... seems to be wrapped in `CastExpr::arg`, result of `Resolver.cc:394 -- findIntegralExpression( ifStmt->condition )` stored in `IfStmt::condition` in `CompoundStmt::kids` in `FunctionDecl::statements`
      * Freed in `collect_young( alt )` at `Resolver.cc:202`
      * Accessed by a path that *only* touches common `PassVisitor` code...
      * Bug disappears if we take out the short-circuiting in `GcTracer`, but similar bugs remain...
  * Look at bugs from rest of stdlib:
    * Setting up stdlib file for use:
      ```
      cd src/libcfa
      ../driver/cfa -I. -I../.. -no-include-stdhdr -Istdhdr -XCFA -t -imacros libcfa-prelude.c -E limits.c > ~/Code/cfa-foo/limits-e.c
      ../driver/cfa-cpp -t ~/Code/cfa-foo/limits-e.c
      ```
    * `limits.c`: bug was `Type::attributes` not being traced
    * `stdlib.c`:
      * Error in `Resolver.cc:203 -- collect_young( alt )`
        * Perp is @7fffffffac30 -- stack-allocated in `AlternativeFinder::Finder::addAnonConversions`
        * `(gdb) break GC.cc:54 if obj == 0x7fffffffac30` (works w/out recompile, but slow)
          * (line 83 for young gen collect, 105 for old gen)
          * Added new `GC_TRAP` flag to GC.cc (see for usage details) to speed this up
      * Runs into assertion failure on recursive young-gen
        * this can plausibly be fixed, but it is also eating nearly all my RAM before getting there
          * tougher fix, I'm going to need to intersperse optimization+correctness fixes...
          * Put in arbitrarily-generational GC
    * `iterator.c`: `VariableExpr::var` not being traced
      * trace leads to stack overflow with loop -- re-enabled check for mark
    * `rational.c`: 
      * GcTracer didn't visit `UniqueExpr::var` (or `UniqueExpr::object`)
      * GcTracer didn't visit attributes in `Statement::labels`
    * `iostream.c`: Fix by tracing inferred parameters in `Expression`
    * `concurrency/preemption.c`:
      * seems to be a young collection that got missed somehow...
        * Updated `new_generation` to return a guard that ends the generation when it goes out of 
          scope
      * outstanding stack allocation in `Box.cc:addOtypeParams`
      * GcTracer didn't visit `AggregateDecl::attributes`
      * `Alternative` fix for `concurrency/coroutine.c` introduced infinite loop...
        * Dropped `Label::statement` from `GcTracer` to fix
    * `concurrency/coroutine.c`:
      * Slight change to `Alternative` copy constructor (didn't clone expression) seemed to be behind this error.
  * Failed test suite tests:
    * `boundedBufferEXT`
      * passes in updated trunk
    * `dtor-early-exit{,-ERR1,-ERR2}`
      * `BranchStmt` wasn't being traced, broke (computed) GOTOs, fixed now
    * `attributes`
      * fixed; problem was `base{Struct,Union,...}` not being traced in `{Struct,Union,...}InstType`
      ```
      concurrency/coroutine.c:96:1 error: No reasonable alternatives for expression Applying untyped: 
        Name: resume
      ...to: 
        Name: cor
      ```
      * trimmed bug down to `~/Code/cfa-foo/coroutine.c` -- can run with `-n` flag
        * Does manage to generate an `ApplicationExpr @27685f0` at `AlternativeFinder.cc:1117`
        * Fails at `computeApplicationConversionCost` at `AlternativeFinder.cc:1171`
          * `typesCompatibleIgnoreQualifiers` returns true on the actual/formal pair
          * Cost from `computeConversionCost` is `(0,1,0,0)`
            * zeroed out in `computeExpressionConversionCost` -- maybe?
          * Failure has got to be in assertion resolution in `AlternativeFinder.cc:447` (`453`)
            * Really in `conversionCost` at `AlternativeFinder.cc:368` (`374`)
            * `typesCompatibleIgnoreQualifiers` at `ConversionCost.cc:73` (`72`) fails
              * This passes in CFA-CC ... looks like our bug
              * both types include a type variable; in master, both end up named "T", in with_gc, one stays as "_9_1_T"
              **cfa-cc**
              ```
              env.env = [ EqvClass {
                vars = { "_9_0_T" },
                type = TypeInstType @26eacf0 {
                  name = "T",
                  baseType = TypeDecl @26d2b00 { name = "T" }
                }
              } ]

              TypeInstType @26e98b0, @26e9740 first to get "_9_1_T" name

              adjType at AlternativeFinder.cc:585 includes @26e9740

              inferParameters[14] = ParamEntry {
                decl = 9,
                actualType = PointerType @26ecff0 { base = FunctionType @26ed080 {
                  parameters = [ ObjectDecl @26edce0 { type = ReferenceType @26eddc0 {
                    base = TypeInstType @26ede40 {
                      name = "_9_1_T",
                      baseType = TypeDecl @26d2b00 ^^^ env.env[0].type.baseType
                    }
                  } } ]
                } }
                formalType = PointerType @26edf30 { base = FunctionType @26edfc0 {
                  parameters = [ ObjectDecl @26ee380 { type = ReferenceType @26ee460 {
                    base = TypeInstType @26ee4e0 {
                      name = "_9_0_T",
                      baseType = TypeDecl @26d2b00 ^^^ env.env[0].type.baseType
                    }
                  } } ]
                } }
                expr = @26ee5d0
                inferParams = @26ecfb0
              }

              first = PointerType @26e83e0 { base = FunctionType @26e7cb0 {
                parameters = [ ObjectDecl @26e8020 { type = ReferenceType @26e7ee0 { 
                  base = TypeInstType @26e8100 {
                    name = "T",
                    baseType = TypeDecl @26cab00 { name = "T" }
                  }
                } } ]
              } }

              second = PointerType @26e7e40 { base = FunctionType @26e7940 {
                parameters = [ ObjectDecl @26e7360 { type = ReferenceType @26e8500 {
                  base = TypeInstType @26e7440 {
                    name = "_9_0_T",
                    baseType = TypeDecl @26cab00 ^^^ first.base.parameters[0].type.base.baseType
                  }
                } } ]
              } }
              ```

              **with-gc**
              ```
              env.env = [ EqvClass {
                vars = { "_9_0_T" },
                type = TypeInstType @277d910 {
                  name = "T",
                  baseType = TypeDecl @2762d30 { name = "T" }
                }
              } ]

              TypeInstType @277e230, @277e6c0 first to get "_9_1_T" name

              TypeInstType @27859d0 is a clone of @27849b0 (a clone of @277e6c0) at AlternativeFinder.cc:578

              inferParameters[14] = ParamEntry {
                decl = 9,
                actualType = PointerType @2784ae0 { base = FunctionType @2784b80 {
                  parameters = [ ObjectDecl @2785860 { type = ReferenceType @2785940 {
                    base = TypeInstType @27859d0 {
                      name = "_9_1_T",
                      baseType = TypeDecl @2761b40 { name = "T" } 
                        !!! not same as env.env[0].type.baseType
                    }
                  } } ]
                } },
                formalType = PointerType @2785ac0 { base = FunctionType @2785b60 {
                  parameters = [ ObjectDecl @2785f50 { type = ReferenceType @2786030 {
                    base = TypeInstType @27860c0 {
                      name = "_9_0_T",
                      baseType = TypeDecl @2756790 { name = "T" }
                        !!! not same as env.env[0].type.baseType
                    }
                  } } ]
                } },
                expr = @27861b0,
                inferParams = @2771410
              }

              !!! this is the error:

              first = PointerType @2784ae0 ^^^ inferParemeters[14].actualType

              second = PointerType @278fac0 ^^^ inferParameter[14].formalType
              ```
              * Seems like satisfying assertion has an extra needed assertion in `with_gc` version
                * `unify` call at `AlternativeFinder.cc:539` second such
                  * only one call to `unify` at `AlternativeFinder.cc:545` in `no_gc` version...
                    * trace from `no_gc` identical to both traces from `with_gc`
                  * two candidates in both cases; chosen is taken from first candidate in `no_gc`, second in `with_gc`
                  * first candidates both unify, as do second candidates
                    * second candidates both add one `newerNeed` element
                      * I think it is the first candidate that I want, in both cases...
                      * Maybe related to missing clone in `Alternative` copy constructor...
                    **with_gc**
                    ```
                    newAlt.env.env = [ EqvClass {
                      vars = { "_9_0_T" },
                      type = TypeInstType @27688c0 {
                        name = "T",
                        baseType = TypeDecl @2762d30 { name = "T" }
                      },
                      allowWidening = false,
                      kind = Dtype,
                      isComplete = false
                    } ]

                    candidates = [
                      { id = ObjectDecl @2767a30 {
                        name = "gc",
                        type = PointerType @2767990 { base = FunctionType @2763d00 {
                          forall = []
                          returnVals = [ @27670e0 ]
                          parameters = [ @27673b0 ]
                        } }
                      } },
                      { id = FunctionDecl @2762080 {
                        name = "gc",
                        type = FunctionType @2761820 {
                          forall = [ @2761b40 ],
                          returnVals = [ @2761f80 ],
                          parameters = [ @2761a40 ]
                        }
                      } }
                    ]
                    ```
                    **no_gc**
                    ```
                    newAlt.env.env = [ EqvClass {
                      vars = { "_9_0_T" },
                      type = TypeInstType @26e58e0 {
                        name = "T",
                        baseType = TypeDecl @26d2b00 { name = "T" }
                      },
                      allowWidening = false,
                      kind = Dtype,
                      isComplete = false
                    } ]

                    candidates = [
                      { id = ObjectDecl @26d5d10 {
                        name = "gc",
                        type = PointerType @26d0360 { base = FunctionType @26d2370 {
                          forall = [],
                          returnVals = [ @26d46e0 ],
                          parameters = [ @26d5230 ]
                        } }
                      } }
                      { id = FunctionDecl @26d1ea0 {
                        name = "gc",
                        type = FunctionType @26d1680 {
                          forall = [ @26d1980 ],
                          returnVals = [ @26d1da0 ],
                          parameters = [ @26d1880 ]
                        }
                      } }
                    ]
                    ```
                  * assertion added by `findOpenVars` on `type2` at `Unify.cc:271`
              * Watch:
                * `first`, `second`, `env` at Unify.cc:107/3
                  * `src`, `dest`, `env` at ConversionCost.cc:72/3
                    * `appExpr->inferParameters[14].{actualType,formalType}` at AlternativeFinder.cc:453/47
                    * `alt.env` at AlternativeFinder.cc:453/47
              * Possible differences:
                * `resolveInVoidContext` has a different object for its `void` type...
                
  * merged in last 6 weeks of changes
    * still some outstanding build-bugs
    * verified bugs don't occur in non-GC build merged from...
      * try rebuild with ASAN, need to compile with `-j 2` to not exceed memory usage...
        * no dice, same error
      * try changing to `coroutine-trim.c` and cutting down
        * invoke command: `../driver/cfa -DHAVE_CONFIG_H -I. -I../.. -nodebug -O2 -quiet -no-include-stdhdr -Istdhdr -XCFA -t -B../driver -g -Wall -Wno-unused-function -imacros libcfa-prelude.c -c -o concurrency/libcfa_a-coroutine-trim.o concurrency/coroutine-trim.c`
  * looked at `Declaration` (not rest of Declaration.h), still need Expression/Statement/Type.h
    * looked at `NameExpr`

## 26-29 Mar 2018 ##
* Work on Persistent Union-Find data structure
  * TODO: look at not doing path compression (no path compression, but fewer updates to reverse)
  * TODO: look at persistent union-find as variant on persistent map
    * rename to `persistent_disjoint_set<Elm>`
    * no `Update` op
    * has `AddUnder { Elm root; Elm child; bool inc_rank }` op, and inverse `RemFrom`
      * probably lose path compression if you do this, but the equivalence classes shouldn't be *that* big
    * Should be a `compare` operation that takes two persistent maps, traces paths between them
      * `EQ` if same map
      * `SUB` if `a` to `b` by only `Add` or `AddUnder`
      * `SUP` if `a` to `b` by only `Rem` or `RemFrom`
      * `INC` otherwise
    * I think can make an `editPath` operation that gets a collapsed string of edits to move 
      from one state to another, then merge based on it
      * assuming editing the target, ignore `Add`, add `Rem`, `Update` is a failure
      * I think an insertion-ordered set is the right data structure for the path
        * update as traversed
        * maybe `std::list` with a `std::unordered_map` from key to list iterator
      * Don't go that complicated, just expose the internal list structure
    * Keep a map of roots to something else (used for class reps)
    * Should also be a `clone` operation that reroots a persistent data structure, then copies the map into an unrelated map
    * With this, modifications to `Env`:
      * remove `parent`
      * make `bindings` and `assns` pointers (initially copies of the parent pointers)
      * merge can be done *very* efficiently for `compare(a, b)` in `EQ`, `SUB`, `SUP`
        * even otherwise, don't need to worry about merging parent nodes recursively, because there 
          aren't any
      * `Env` might become `GC_Traceable` instead -- really just a wrapper for the two `GC_Object` pointers, can hold it by-value
        * can remove `localAssns`, only real use is in `getNonempty`, which is obviated by value type here
        * don't even bother with `GC_Traceable`, the extra `bool` isn't worth it, make it a value type with a `<<` operator on `GC`
* Start updating Env to use Persistent Union-Find
  * TODO: update mergeClasses
* Fixed bug with following code:
  ```
  Base&& base = take_as<Base>(); // should take by value, due to reset
  reset_as_base();               // may as well reset_as_base()
  ```

## 20-23 Mar 2018 ##
* Further work on CFA-CC GC
  * CFA-CC compiles now
  * Crashes come from by-value Constants as BaseSyntaxNode
    * Fixed

## 15-16 Mar 2018 ##
* Continue integrating `-def` into CFA-CC
  * look into segfaults on pair/maybe/result
* Pause integration to put GC in CFA-CC
  * possible deleting calls:
    * deleteAll: done
    * maybeMoveBuild: only for parse nodes
    * filter( container, pred, true ): done

## 14 Mar 2018 ##
* Continue integrating `-def` into CFA-CC
  * builds, horrifically broken
  * TODO: Wrap `need`/`have`/`openVars` in copy-on-write ref-counted smart pointer
  * TODO: Once done, make default-constructor for those three parameters to Alternative

## 13 Mar 2018 ##
* Continue integrating `-def` into CFA-CC
  * think I'm going to have to pull `need`/`have` up into `Alternative`
    * maybe transfer happens in `validateFunctionAlternative` (?)
* rewrite `waitfor` resolver to handle deleted expressions

## 12 Mar 2018 ##
* Start integrating `-def` resolution into CFA-CC
  * Find top-level entry point:
    * `findUnfinishedKindExpression`
      * findKindExpression forwards to ^
        * also calls finishExpr, which seems to just make sure all calls have a `TypeSubstitution`
        * findIntegralExpression forwards to ^
        * findSingleExpression forwards to ^
      * findVoidExpression forwards to resolveInVoidContext, which forwards to ^
      * Resolver::previsit(WaitForStmt*) seems to miss
        * does call AlternativeFinder::findWithAdjustment
      * calls AlternativeFinder::find
  * Re-write entry point to call `resolveAssertions`

## 26 Feb 2018 ##
* finish testing `td-imm`, `td-top`

## 23 Feb 2018 ##
* test `td-imm`, generate easy tests as needed
  * bad top-level call can make this go *really* badly; `k( 4 )` in `overloads-most` takes over half a minute (without eating all the RAM, it seems just to be really hammering cache)
  * done to `parms-fewer` (inclusive)

## 22 Feb 2018 ##
* fix segfault and test failures from yesterday

## 21 Feb 2018 ##
* met Peter van Beek about algorithms/AI approaches to problem
  * suggests looking into dynamic programming as cost/correctness model
    * problem might not be exponential
    * "non-linear dynamic programming" as phrase regarding cost
  * there exists literature on data structures for unification
* further testing of `td-*`: all tests work on `-def`
* debug `td-imm`
  * segfault in resolution (same as before: assertions recursively resolved at same level)
  * made caching depend on resolution mode, some test failures ensuing

## 20 Feb 2018 ##
* Switched asserts over to `unreachable`/`assume` in `debug.h`
* Update easy versions of inputs to be handled by `td-def` up to `nesting-more`
* Fix bug in occurs check (wasn't checking on all variable adds, got `T => #x<T>`)

## 14-16 Feb 2018 ##
* update testing scripts, generate tests
* fixed `td-imm` variant; same error as `bu-imm` variant from before
* fixed `td-def` variant; off-by-one error in `MatchIter::push_prefix`

## 13 Feb 2018 ##
* debug immediate assertion resolution order
  * Error was that `resolveWithType` shouldn't have assertion resolution, and I'd flipped the default too thoroughly
* change makefile to default to previous version of DIR, RES, error if invalid value provided

## 12 Feb 2018 ##
* Added immediate assertion resolution order

## 8 Feb 2018 ##
* Added deferred assertion resolution order
  * Initial timing results positive, completes all resolution tasks without exceeding RAM boundaries, even `tuple-return`

## 1 Feb 2018 ##
* Modify bench-gen to discourage duplicate parameter lists
* `tuple-return` has some memory usage issues
  * maybe switch semantics to disallow truncation, more like CFA
  * seems to be a memory leak somewhere in here; suspect there's something live through collect_young

## 31 Jan 2018 ##
* Fix memory error attempting to merge class without unseen vars

## 26 Jan 2018 ##
* Continue generating test cases
* Fix segfault in `less-poly2`

## 25 Jan 2018 ##
* Fix memory bug in `Env`
* Fix truncation bug in `expand_conversions`
* Fixed segfault in `resolve_assertions` on `tuple-return`

## 24 Jan 2018 ##
* Fix bug in type_unifier to work with new fake code
* Modify bench_gen to generate more assertions that will match

## 22-23 Jan 2018 ##
* Upgrade fake code generator to generate assertions

## 12-19 Jan 2018 ##
* Upgrade code analyzer and fake code generator to handle generic types

## 11 Jan 2018 ##
* Fixed second bug in TD resolver
* Started work on code analysis for generic types

## 10 Jan 2018 ##
* Built new assertion resolver, seems to work
* Fixed one bug in TD resolver

## 8 Jan 2018 ##
* Think about assertion resolution.
  * New model doesn't change costs of overall expression based on cost of assertion resolution
  * New model also says that any types left unbound until the assertions need to be bound, but won't specify the binding beyond what's constrained by the assertions -- ergo, what you bind these "auxiliary types" to doesn't really matter, as long as you can find a binding
  * I *think* if we BFS the space of bindings, stopping when we find a concrete one (which should be min-cost), or at a fixed depth (for no ultimate binding) that that might work

## 5 Jan 2018 ##
* Investigate exponential failure case:
  * for `f( make() )` case, does attempt assertion resolution on 
    `f-b{ Q.1 => #box<S.4> }( make() => Q )`
    * mutateAll on arguments seems to modify environment somewhere, as it should
      * `{ Q.1 => #box<S.4>, [T.5, S.4, T.6] => ???, dtor Q.1 => dtor-b Q.1, ctor Q.1 => ctor-b Q.1 }`
      * setting up the assertion satisfiers with variables of type `Q` seems sketchy
    * attempts to resolve `ctor S.4` to `Void` (ditto `dtor`)
      * and this is where the rabbit-trail breaks, line 135's `resolveWithType` does not attempt to 
        resolve assertions
    * working guess is it fails with unbound on `[T.5, S.4, T.6]` somewhere...

## 8 Dec 2017 ##
* Work on fixing TypeMap::PolyIter for generic types

## 7 Dec 2017 ##
* Implment occurs check and test case
* Implement exponential failure case
* Investigate why top-down algorithm fails on generic types

## 6 Dec 2017 ##
* Poke at cost tracking for bindVar in unification visitor

## 27-28 Nov 2017 ##
* Work on new unification visitor
  * Includes closest common type, recursive unification of NamedType

## 23 Nov 2017 ##
* Clean up ExplodedActual data structure (single env and cost)
  * no effect in benchmark:
```
               array: 0:01.52
          attributes: 0:02.14
               empty: 0:01.50
          expression: 0:01.57
                  io: 7:45.13
             monitor: 0:40.08
           operators: 0:01.59
              typeof: 0:01.49
```

## 22 Nov 2017 ##
* Pre-explode tuple arguments in CFA-CC
  * no effect in benchmark:
```
               array: 0:01.50
          attributes: 0:02.20
               empty: 0:01.54
          expression: 0:01.59
                  io: 8:01.16
             monitor: 0:39.83
           operators: 0:01.57
              typeof: 0:01.49
```

## 21 Nov 2017 ##
* Fix tuple errors on last CFA-CC refactor
* Refactor ResolvExpr::AltList to std::vector from std::list; 
  * need to fix castError test
  * no effect in benchmark:
```
               array: 0:01.51
          attributes: 0:02.19
               empty: 0:01.55
          expression: 0:01.57
                  io: 8:18.95
             monitor: 0:42.07
           operators: 0:01.62
              typeof: 0:01.51
```

## 20 Nov 2017 ##
* Fix tuple errors on last CFA-CC refactor

## 16-17 Nov 2017 ##
* Worked on refactoring CFA-CC resolver to put less pressure on the memory allocator: 8% speeedup

```
               array	0:01.48
          attributes	0:02.12
               empty	0:01.48
          expression	0:01.61
                  io	7:46.44
             monitor	0:29.56
           operators	0:01.54
              thread	0:41.33
              typeof	0:01.45
```

## 15 Nov 2017 ##
* Count specialization costs for nested polymorphic types

## 14 Nov 2017 ##
* Added generic parameters to unification, fixed visitor/mutator to properly traverse type parameters
* Added basic generic test; works for BU, CO, not TD
* Need to add pointer-type costing to function cost

## 13 Nov 2017 ##
Put in first draft of generic types; parses, in type_map, some tests work.

## 10 Nov 2017 ##
Ran Thierry's benchmark:

New:
```
               array	0:01.46
          attributes	0:02.13
               empty	0:01.46
          expression	0:01.52
                  io	8:23.63
             monitor	0:15.29
           operators	0:01.52
              thread	0:26.46
              typeof	0:01.44
```

Old:
```
               array	0:01.51
          attributes	0:02.10
               empty	0:01.51
          expression	0:01.56
                  io	8:16.29
             monitor	0:28.37
           operators	0:01.69
              thread	0:40.60
              typeof	0:01.50
```

## 8 Nov 2017 ##
Finished modifications to CFA-CC, timing results (note full rebuild of CFA-CC vs. partial rebuild in base case)
```
make -j 8 install            1380.64s user 22.90s system 488% cpu  4:47.20 total
make -C src/tests all-tests  3797.68s user 14.80s system 453% cpu 14:00.15 total
```
* Lack of observable performance difference is really disappointing, though Thierry points out that the test suite deliberately includes some long-running tests.
* Might be worth investigating if this is a memory-allocation issue (lots of clones in this algorithm, not so many in the prototype)

## 21 Sept 2017 ##
* Wrote new iterative version of BU
  * orders of magnitude faster than old BU, now renamed to CO
  * can sucessfully complete full test suite in original forms

Investigating same modification to CFA-CC, timing results (note that build timings include a variable number of modules rebuilt in CFA-CC)
baseline:
```
make -j 8 install            1308.45s user 17.83s system 502% cpu  4:24.18 total
make -C src/tests all-tests  3637.23s user 11.10s system 448% cpu 13:34.15 total
```

## 20 Sept 2017 ##
* Modified TD resolver to cache independently of environment, ran experimental tests
* Needed to make `fewer-basic` and `deeper-nesting` even easier (same pattern as before for hard TD instances)
* New TD otherwise unambiguously better

## 13-15 Sept 2017 ##
* Results from today's benchmarks; BU generally wins
```
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q default-easy.in    
1.16,0.20,1.37,433044
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q default-easy.in  
39.60,3.04,42.75,9565192
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q default2-easy.in
27.70,3.13,30.97,7871240
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q default2-easy.in
14.31,1.72,16.06,4993704
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q tuple-return.in
24.28,2.67,26.98,7009500
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q tuple-return.in
12.72,2.27,15.03,5396704
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q less-poly-easy.in   
0.38,0.04,0.42,116992
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q less-poly-easy.in
53.65,3.87,57.67,5661532
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q less-poly2-easy.in
0.97,0.09,1.06,256932
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q less-poly2-easy.in
10.98,1.25,12.24,3182772
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more-poly-easy.in
27.23,1.67,28.93,4309532
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more-poly-easy.in
6.58,0.73,7.32,1864968
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q fewer-basic-easy.in
37.56,3.30,40.90,8659444
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q fewer-basic-easy.in
8.05,0.48,8.54,1126148
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q fewer-overloads-easy.in
0.06,0.00,0.07,21444
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q fewer-overloads-easy.in
82.60,6.88,89.62,10314168
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q fewer-overloads2.in
0.08,0.00,0.09,28200
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q fewer-overloads2.in
80.99,5.89,86.94,10914496
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more-overloads-easy.in
7.82,0.99,8.82,2339880
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more-overloads-easy.in
34.26,2.90,37.33,5757888
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q most-overloads-easy.in
10.08,1.11,11.23,2656920
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q most-overloads-easy.in
26.64,2.77,29.43,6437004
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q long-tail-overloads-easy.in
28.68,4.54,33.26,11563980
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q long-tail-overloads-easy.in
6.42,0.58,7.01,1157656
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q fewer-parms.in         
20.08,4.32,24.52,12342852
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q fewer-parms.in
1.14,0.04,1.18,160996
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more-parms-easier.in
0.94,0.12,1.08,236152
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more-parms-easier.in
436.50,13.62,450.38,7593036
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q shallower-nesting.in       
1.61,0.21,1.82,508252
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q shallower-nesting.in
8.52,1.26,9.78,2935392
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q deeper-nesting-easy.in
33.10,4.27,37.40,9970700
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q deeper-nesting-easy.in
54.56,2.62,57.21,5480724
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q fewer-decls.in        
23.42,2.49,25.95,6835568
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q fewer-decls.in
17.94,2.18,20.17,5288480
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more-decls-easy.in
1.77,0.26,2.03,640544
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more-decls-easy.in
30.29,3.58,33.90,9613316
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more-decls2-easy.in
1.78,0.29,2.09,673632
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more-decls2-easy.in
39.34,2.28,41.79,5019144
```
* `default-easy` seems like an outlier; `default2-easy` shows a more typical performance profile based on the same random generation parameters
* Neither algorithm performed particularly well on `more-poly`, needed to trim about a half-dozen cases before both algorithms would run without OOM
  * `more-poly-easy` does seem somewhat BU-biased, though, and `less-poly-easy` has a fairly strong TD bias, verified by `less-poly2-easy`
* Note that `fewer-basic` doesn't so much have fewer basic type-parameters as a restricted range of basic types and more struct types
* `fewer-overloads` is a clear win for TD, which holds through `fewer-overloads2`
  * needed to go to the fairly extreme `long-tail-overloads` (>50% of functions have >40 overloads) to get a win for BU here; this seems like a potentially key observation
  * note that the `*-overloads` series tends to overweight small overload sets relative to `default`
* `fewer-parms` is a particularly strong win for BU, avoiding its worst failure case
  * by contrast, `more-parms` loses dozens of instances to become something BU can handle; worst instances seem to have large numbers of convertable parameters (basic type instead of struct type)
* The `shallower-nesting` instances are particularly easy for both algorithms, while the `deeper-nesting` instances are harder
  * However, aside from the fact that TD OOMs on significantly more instances than BU, it seems to behave better on both...
* `more-decls` seems to benefit TD, I have no idea why...

## 12 Sept 2017 ##
* TD Caching strategy seems generally effective:
  * pre-bif         -- 45119 :    42550 (51%)
  * bif (truncated) --  5146 : 12402771 ( 0.04%)
* Eliminating cache did not solve TD memory usage issues on bif.in, and resulted in significant time degredation:
  * pre-bif (td)          --  0.26s user, 0.06s system
  * pre-bif (bu)          --  0.38s user, 0.02s system
  * pre-bif (td;no-cache) --  0.89s user, 0.16s system
* TD uses less time and memory than BU on all non-bif cases:
```
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q default-easy.in
1.34,0.24,1.59,634524
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q default-easy.in
38.24,3.32,41.60,9565580
```
* `Env::from()` accounts for ~30% of memory usage for TD on both default-easy and bif cases.
* Trimming some `Env::from()`:
```
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q default-easy.in             
1.18,0.17,1.36,434396
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q default-easy.in
39.69,3.37,43.09,9564556
```
* `Env::from()` now down to ~20% of memory usage for TD
