# C∀ Resolver Prototype #

This is a prototype system for development of the [C∀](https://cforall.uwaterloo.ca) ("Cforall") compiler. 
It is used to explore algorithmic and architectural modifications to the primary [C∀ compiler](https://github.com/cforall/cforall). 
The prototype system does not implement a full C∀ compiler, but instead does expression resolution on an input language capturing the major features of C∀ (see [`parser.h`](./src/driver/parser.h) for a description of the input language, and [`tests/*.in`](./tests/) for examples).

## Build Instructions ##

`make rp` will build the resolver prototype; available build flags:
* `DBG` (default absent)
  * absent: release build
  * `0`: development build
  * `1`: development build with extra safety checks
  * `2`: development build with extra safety checks and debug traces
* `DIR` (default `bu` or last used)
  * `bu`: bottom-up resolution
  * `co`: bottom-up-combined expression resolution
  * `td`: top-down resolution
* `ASN` (default `dca` or last used)
  * `dca`: deferred-cached assertion satisfaction
  * `def`: deferred assertion satisfaction
  * `imm`: immediate assertion satisfaction
  * `top`: top-level assertion satisfaction
* `ENV` (default `per` or last used)
  * `bas`: basic type environment
  * `inc`: incremental inheritance type environment
  * `per`: persistent union-find type environment
Changing any build flag will cause a clean rebuild.

## Test Instructions ##

Regression tests are in [`tests`](./tests/) and can be run with `run.sh`. Various benchmark suites are located in [`bench`](./bench/), a[`cfabench`](./cfabench/), [`cfastdlib`](./cfastdlib/), and [`cfaother`](./cfaother/), and can also be run with the appropriate `run.sh`.