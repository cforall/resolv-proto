## 25 Jan 2018 ##
* Fix memory bug in Env

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
