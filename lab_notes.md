## 01 June ##
* Started on resolving bugs in new `Env`
  * fails on `default-easy`, `overloads-more-easy`, `parms-fewer-easy`, `poly-less2-easy`
    * `default-easy`:
      * merge fails at `resolver-bu.cc:196`
      * `iEnv` classes rooted at `0x607000046cd0`, `i->env` at `0x6070000179b0`
      * `i` at `0x60800001a820`
        * Env (same it keeps) copy of `arg->env`

## 24 May-01 June ##
* Started performance testing of GC branch.
* Started trimming `clone` calls
  * Taking out `clone` in `Alternative` copies broke stdlib, reverted
* Took out the `EqvClass` copies in `TypeEnvironment::lookup`
  * backported

**baseline - array**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1668342( 19%) |      1635605( 19%) |        55746( 37%) | 
               resolve |      3040425( 35%) |      3012828( 35%) |       113794( 76%) | 
               fixInit |      1082417( 12%) |      1047335( 12%) |       131872( 88%) | 
---------------------------------------------------------------------------------------
                   Sum |      8674133(100%) |      8559772(100%) |       148560(100%) | 
cfa array.c  2.21s user 0.02s system 98% cpu 2.256 total
```

**gc - array**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1948997( 19%) |      1912960( 19%) |        63614( 35%) | 
               resolve |      2812133( 28%) |      2792872( 29%) |       177296(100%) | 
               fixInit |      1274543( 13%) |      1236852( 12%) |       134716( 75%) | 
---------------------------------------------------------------------------------------
                   Sum |      9755274(100%) |      9605015(100%) |       177296(100%) | 
cfa array.c  2.65s user 0.03s system 98% cpu 2.712 total
```

**8e18b8e - array**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1948997( 20%) |      1912960( 20%) |        63614( 38%) | 
               resolve |      2602463( 27%) |      2583202( 27%) |       151713( 92%) | 
               fixInit |      1274193( 13%) |      1236502( 13%) |       134716( 82%) | 
---------------------------------------------------------------------------------------
                   Sum |      9545248(100%) |      9394989(100%) |       164009(100%) | 
cfa array.c  2.53s user 0.05s system 99% cpu 2.602 total
```

**baseline - attributes**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      7744158( 46%) |      7666294( 46%) |       105168( 50%) | 
               resolve |      3367053( 20%) |      3336438( 20%) |       161760( 77%) | 
               fixInit |      1353122(  8%) |      1314375(  8%) |       186414( 88%) | 
---------------------------------------------------------------------------------------
                   Sum |     16540865(100%) |     16379962(100%) |       209832(100%) |  
cfa attributes.c  3.17s user 0.04s system 99% cpu 3.233 total
```

**gc - attributes**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      7558786( 42%) |      7512802( 43%) |       109645( 56%) | 
               resolve |      3355864( 19%) |      3334926( 19%) |       189617( 98%) | 
               fixInit |      1579318(  8%) |      1537984(  8%) |       152269( 78%) | 
---------------------------------------------------------------------------------------
                   Sum |     17601239(100%) |     17426696(100%) |       192925(100%) | 
cfa attributes.c  3.78s user 0.06s system 98% cpu 3.900 total
```

**8e18b8e - attributes**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      7507691( 43%) |      7461707( 43%) |       104721( 54%) | 
               resolve |      3177570( 18%) |      3156632( 18%) |       173219( 89%) | 
               fixInit |      1578968(  9%) |      1537634(  8%) |       152269( 78%) | 
---------------------------------------------------------------------------------------
                   Sum |     17371491(100%) |     17196948(100%) |       192925(100%) | 
cfa attributes.c  3.70s user 0.03s system 99% cpu 3.755 total
```

**baseline - raii/dtor-early-exit**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |     48995173(  9%) |     48637501( 10%) |       403190(  5%) | 
               resolve |    355445653( 72%) |    353486464( 72%) |      2959112( 37%) | 
               fixInit |     68173504( 13%) |     62987655( 12%) |      7604708( 95%) | 
---------------------------------------------------------------------------------------
                   Sum |    492150706(100%) |    484912058(100%) |      7937757(100%) | 
cfa raii/dtor-early-exit.c  67.18s user 0.38s system 99% cpu 1:07.62 total
```

**gc - raii/dtor-early-exit**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |     44919556(  8%) |     44808978(  8%) |       254124(  1%) | 
               resolve |    379612890( 73%) |    379327063( 73%) |     15558395(100%) | 
               fixInit |     66460462( 12%) |     66300262( 12%) |      1392918(  8%) | 
---------------------------------------------------------------------------------------
                   Sum |    515907432(100%) |    514916261(100%) |     15558395(100%) | 
cfa raii/dtor-early-exit.c  82.18s user 0.81s system 99% cpu 1:23.13 total
```

**8e18b8e - raii/dtor-early-exit**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |     44183964( 10%) |     44073386( 10%) |       241507(  2%) | 
               resolve |    278778701( 67%) |    278492874( 67%) |     11530048(100%) | 
               fixInit |     65678011( 15%) |     65517811( 15%) |      1374877( 11%) | 
---------------------------------------------------------------------------------------
                   Sum |    413555200(100%) |    412564029(100%) |     11530048(100%) | 
cfa raii/dtor-early-exit.c  65.27s user 0.60s system 99% cpu 1:05.95 total
```

**baseline - expression**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1812682( 18%) |      1779083( 18%) |        60397( 39%) | 
               resolve |      3658858( 38%) |      3628841( 38%) |       115423( 75%) | 
               fixInit |      1112421( 11%) |      1077289( 11%) |       135998( 88%) | 
---------------------------------------------------------------------------------------
                   Sum |      9582892(100%) |      9465636(100%) |       152904(100%) | 
cfa expression.c  2.27s user 0.02s system 99% cpu 2.314 total
```

**gc - expression**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2082193( 19%) |      2045933( 19%) |        76147( 43%) | 
               resolve |      3358033( 31%) |      3337948( 31%) |       173239(100%) | 
               fixInit |      1308784( 12%) |      1271045( 12%) |       136050( 78%) | 
---------------------------------------------------------------------------------------
                   Sum |     10598879(100%) |     10447115(100%) |       173239(100%) | 
cfa expression.c  2.70s user 0.04s system 98% cpu 2.769 total
```

**8e18b8e - expression**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2080416( 19%) |      2044156( 19%) |        74605( 44%) | 
               resolve |      3298551( 31%) |      3278466( 31%) |       161360( 97%) | 
               fixInit |      1308434( 12%) |      1270695( 12%) |       136050( 82%) | 
---------------------------------------------------------------------------------------
                   Sum |     10537270(100%) |     10385506(100%) |       165846(100%) | 
cfa expression.c  2.62s user 0.05s system 99% cpu 2.693 total
```

**baseline - concurrent/monitor**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |    398639948( 66%) |    396464782( 66%) |      2244863( 51%) | 
               resolve |     77248468( 12%) |     76664731( 12%) |      3147496( 72%) | 
               fixInit |     94110988( 15%) |     92934426( 15%) |      4150213( 95%) | 
---------------------------------------------------------------------------------------
                   Sum |    600233343(100%) |    596433822(100%) |      4324127(100%) | 
cfa concurrent/monitor.c  80.94s user 0.90s system 99% cpu 1:21.90 total
```

**gc - concurrent/monitor**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |    352792674( 64%) |    352543820( 64%) |      1068361( 10%) | 
               resolve |     70444761( 12%) |     70355760( 12%) |     10145145(100%) | 
               fixInit |     87385288( 15%) |     87216527( 15%) |      1381466( 13%) | 
---------------------------------------------------------------------------------------
                   Sum |    546265216(100%) |    545373659(100%) |     10145145(100%) | 
cfa concurrent/monitor.c  86.83s user 0.51s system 99% cpu 1:27.41 total
```

**8e18b8e - concurrent/monitor**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |    341117538( 64%) |    340868684( 64%) |       771424( 13%) | 
               resolve |     64774959( 12%) |     64685958( 12%) |      5931446(100%) | 
               fixInit |     87249469( 16%) |     87080708( 16%) |      1363410( 22%) | 
---------------------------------------------------------------------------------------
                   Sum |    528784446(100%) |    527892889(100%) |      5931446(100%) | 
cfa concurrent/monitor.c  81.74s user 0.31s system 99% cpu 1:22.15 total
```

**baseline - operators**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1834397( 19%) |      1796835( 19%) |        64222( 36%) | 
               resolve |      3199571( 33%) |      3163002( 34%) |       118728( 67%) | 
               fixInit |      1435135( 15%) |      1385635( 14%) |       160205( 90%) | 
---------------------------------------------------------------------------------------
                   Sum |      9429797(100%) |      9287709(100%) |       177165(100%) | 
cfa operators.c  2.27s user 0.04s system 98% cpu 2.350 total
```

**gc - operators**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2103999( 19%) |      2067503( 19%) |        79372( 44%) | 
               resolve |      3117298( 29%) |      3097827( 29%) |       177873(100%) | 
               fixInit |      1608753( 15%) |      1570496( 14%) |       136614( 76%) | 
---------------------------------------------------------------------------------------
                   Sum |     10638816(100%) |     10486941(100%) |       177873(100%) | 
cfa operators.c  2.79s user 0.06s system 99% cpu 2.871 total
```

**8e18b8e - operators**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2098149( 20%) |      2061653( 19%) |        76259( 45%) | 
               resolve |      2980878( 28%) |      2961407( 28%) |       161484( 97%) | 
               fixInit |      1599895( 15%) |      1561638( 15%) |       136614( 82%) | 
---------------------------------------------------------------------------------------
                   Sum |     10487688(100%) |     10335813(100%) |       165926(100%) | 
cfa operators.c  2.74s user 0.02s system 99% cpu 2.781 total
```

**baseline - typeof**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1663441( 19%) |      1630762( 19%) |        55618( 37%) | 
               resolve |      3015135( 34%) |      2987788( 35%) |       113674( 76%) | 
               fixInit |      1078764( 12%) |      1043721( 12%) |       131459( 88%) | 
---------------------------------------------------------------------------------------
                   Sum |      8629507(100%) |      8515426(100%) |       148067(100%) | 
cfa typeof.c  2.10s user 0.03s system 98% cpu 2.162 total
```

**gc - typeof**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1943752( 19%) |      1907770( 19%) |        63470( 37%) | 
               resolve |      2815347( 28%) |      2796156( 29%) |       167196(100%) | 
               fixInit |      1270370( 13%) |      1232718( 12%) |       134446( 80%) | 
---------------------------------------------------------------------------------------
                   Sum |      9735000(100%) |      9585099(100%) |       167196(100%) | 
cfa typeof.c  2.58s user 0.02s system 99% cpu 2.622 total
```

**8e18b8e - typeof**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      1943752( 20%) |      1907770( 20%) |        63470( 38%) | 
               resolve |      2709169( 28%) |      2689978( 28%) |       160748( 98%) | 
               fixInit |      1270020( 13%) |      1232368( 13%) |       134446( 82%) | 
---------------------------------------------------------------------------------------
                   Sum |      9628463(100%) |      9478562(100%) |       163621(100%) | 
cfa typeof.c  2.57s user 0.03s system 99% cpu 2.621 total
```

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
