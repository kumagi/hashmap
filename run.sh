./insert_bench 10000000 2 8 2
./remove_bench 10000000 2 8 2
./contains_bench 10000000 2 8 2
gprof insert_bench >> prof_insert
gprof contains_bench >> prof_contains
gprof remove_bench >> prof_remove
