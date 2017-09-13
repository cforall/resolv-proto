## 13 Sept 2017 ##
* Results from new cases -- `tuple_return` slightly in favour of BU, `more_poly_easy` greatly so:
```
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q tuple_return.in
24.28,2.67,26.98,7009500
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q tuple_return.in
12.72,2.27,15.03,5396704
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q more_poly_easy.in
27.23,1.67,28.93,4309532
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q more_poly_easy.in
6.58,0.73,7.32,1864968
```

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
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q no-bif.in
1.34,0.24,1.59,634524
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q no-bif.in
38.24,3.32,41.60,9565580
```
* `Env::from()` accounts for ~30% of memory usage for TD on both no-bif and bif cases.
* Trimming some `Env::from()`:
```
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-td -q no-bif.in             
1.18,0.17,1.36,434396
% /usr/bin/time -f "%U,%S,%e,%M" ../rp-bu -q no-bif.in
39.69,3.37,43.09,9564556
```
* `Env::from()` now down to ~20% of memory usage for TD