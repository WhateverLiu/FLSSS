1. finished FLSSS_variable_len.

2. was thinking about employ infra. There are 
   too many functions that repeatedly malloc
   small vectors for processing. for example
   tighten_bounds_for_len() in the loop in 
   flsss_variable_len.hpp. also, it might
   be worth storing sorted columns for 
   tighten_bounds_for_len() in flsss_variable_len
   path.

3. Once those are optimized, the last part is
   just parallelization.

so, a good cross-platform compile flag would be
-O3 -ffast-math -march=native -mtune=native
you've run many tests. do not revisit this.

it would not be worth it to compute the logarithm
of the erf function, since it just takes log of it
just use erf is enough. (done)

do not underestimate the speedup brought by using templated 
container size. for small sizes like 10 ~ 20, it can 
be close to 2x. for great sizes like 100, the speedup can still
reach 1.1x to 1.2x!
