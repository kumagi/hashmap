CXX= g++
OPTS= -O4 -g
WARNS= -W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
TEST_LD= -lpthread -lboost_thread
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_DIR=/opt/google/gtest-1.5.0

NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png

target:hashtest
#target:insert_bench
#target:insert_bench_cov
target:checker

hashtest:hashtest.o gtest_main.a
	$(CXX) $^ -o $@ $(OPTS) $(TEST_LD) $(WARNS)
	./$@ $(NOTIFY)
insert_bench:insert_bench.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread
insert_bench_cov:insert_bench_cov.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -coverage -lpthread -lgcov -lboost_thread
checker:checker.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread

hashtest.o:hashtest.cc hashmap.h locks.h
	$(CXX) -c $< -o $@ $(OPTS) $(WARNS)
insert_bench.o:insert_bench.cc hashmap.h locks.h
	$(CXX) -c $< -o $@ -O4 -g $(WARNS) -fno-inline -pg 
insert_bench_cov.o:insert_bench.cc hashmap.h locks.h
	$(CXX) -c $< -o $@ -O4 -g $(WARNS) -fno-inline -coverage
checker.o:checker.cc hashmap.h locks.h
	$(CXX) -c $< -o $@ -O4 -g $(WARNS)

#	$(CXX) -c -o $@ sl.cc  $(OPTS) $(WARNS) -I$(GTEST_DIR)/include -I$(GTEST_DIR)
# gtest
gtest_main.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest_main.cc -o $@
gtest-all.o:
	$(CXX) $(GTEST_INC) -c $(OPTS) $(GTEST_DIR)/src/gtest-all.cc -o $@
gtest_main.a:gtest-all.o gtest_main.o
	ar -r $@ $^

clean:
	rm -f *.o
	rm -f *~
