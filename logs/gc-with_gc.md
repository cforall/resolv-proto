This document represents the test of the straight GC port on the GC branch. 
The git commit hash is 28f3a19.

**array**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2021702( 20%) |      1985867( 20%) |        63460( 38%) | 
               resolve |      2749942( 28%) |      2730729( 28%) |       160404( 98%) | 
               fixInit |      1273512( 13%) |      1235944( 12%) |       134305( 82%) | 
         convertLvalue |       639503(  6%) |       639303(  6%) |       148611( 90%) | 
                   box |       766772(  7%) |       765276(  8%) |       163572(100%) | 
---------------------------------------------------------------------------------------
                   Sum |      9688465(100%) |      9538640(100%) |       163572(100%) | 
cfa array.c  2.54s user 0.04s system 98% cpu 2.610 total
```

**attributes**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      7562605( 43%) |      7516811( 43%) |       104001( 54%) | 
               resolve |      3125920( 18%) |      3105024( 18%) |       167754( 87%) | 
               fixInit |      1578284(  9%) |      1537055(  8%) |       151898( 78%) | 
         convertLvalue |       931857(  5%) |       928815(  5%) |       178729( 92%) | 
                   box |      1119995(  6%) |      1116411(  6%) |       192534(100%) | 
---------------------------------------------------------------------------------------
                   Sum |     17269755(100%) |     17095600(100%) |       192534(100%) |
cfa attributes.c  3.61s user 0.05s system 98% cpu 3.703 total
```

**raii/dtor-early-exit**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |     46900391( 11%) |     46790123( 11%) |       242985(  2%) | 
               resolve |    273996223( 66%) |    273710510( 66%) |     11552202(100%) | 
               fixInit |     67516460( 16%) |     67357311( 16%) |      1378018( 11%) | 
         convertLvalue |      4537413(  1%) |      4538785(  1%) |       890355(  7%) | 
                   box |      7763917(  1%) |      7588049(  1%) |      1089167(  9%) | 
---------------------------------------------------------------------------------------
                   Sum |    413059846(100%) |    412070452(100%) |     11552202(100%) | 
cfa raii/dtor-early-exit.c  63.38s user 0.58s system 99% cpu 1:04.04 total
```

**expression**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2152406( 20%) |      2116329( 20%) |        74210( 44%) | 
               resolve |      3254375( 31%) |      3234329( 31%) |       156640( 94%) | 
               fixInit |      1307753( 12%) |      1270110( 12%) |       135698( 82%) | 
         convertLvalue |       670287(  6%) |       670064(  6%) |       150822( 91%) | 
                   box |       800598(  7%) |       799060(  7%) |       165477(100%) | 
---------------------------------------------------------------------------------------
                   Sum |     10487047(100%) |     10335649(100%) |       165477(100%) | 
cfa expression.c  2.63s user 0.01s system 98% cpu 2.685 total
```

**concurrent/monitor**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |    406506702( 64%) |    406249384( 64%) |       772599( 10%) | 
               resolve |     75394923( 11%) |     75303304( 11%) |      7386294(100%) | 
               fixInit |    112420314( 17%) |    112246399( 17%) |      1469382( 19%) | 
         convertLvalue |      7587280(  1%) |      7554675(  1%) |      1052574( 14%) | 
                   box |      9397032(  1%) |      9294438(  1%) |      1019111( 13%) | 
---------------------------------------------------------------------------------------
                   Sum |    630848832(100%) |    629924676(100%) |      7386294(100%) | 
cfa concurrent/monitor.c  97.55s user 0.34s system 99% cpu 1:37.99 total
```

**operators**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2169844( 20%) |      2133542( 20%) |        75829( 45%) | 
               resolve |      2932701( 28%) |      2913274( 28%) |       160989( 97%) | 
               fixInit |      1598927( 15%) |      1560812( 15%) |       136196( 82%) | 
         convertLvalue |       660602(  6%) |       660351(  6%) |       150842( 91%) | 
                   box |       788572(  7%) |       787036(  7%) |       165486(100%) | 
---------------------------------------------------------------------------------------
                   Sum |     10432911(100%) |     10281473(100%) |       165486(100%) | 
cfa operators.c  2.72s user 0.04s system 98% cpu 2.808 total
```

**typeof**
```
                  Pass |       Malloc Count |         Free Count |        Peak Allocs |
---------------------------------------------------------------------------------------
              validate |      2016265( 21%) |      1980469( 21%) |        63332( 38%) | 
               resolve |      2640978( 27%) |      2621827( 27%) |       151064( 92%) | 
               fixInit |      1269339( 13%) |      1231786( 13%) |       134083( 82%) | 
         convertLvalue |       636997(  6%) |       636794(  6%) |       148351( 90%) | 
                   box |       763024(  7%) |       761608(  8%) |       163240(100%) | 
---------------------------------------------------------------------------------------
                   Sum |      9556016(100%) |      9406493(100%) |       163240(100%) | 
cfa typeof.c  2.51s user 0.02s system 99% cpu 2.550 total
```