CXX= g++
OPTS= -O4 -g
PROF=
#-pg -fno-inline
WARNS=-W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
TEST_LD= -lpthread -lboost_thread
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_DIR=/opt/google/gtest-1.6.0

NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png

target:hashtest
target:contains_bench
target:insert_bench
target:remove_bench

target:checker

hashtest:hashtest.o gtest_main.a
	$(CXX) $^ -o $@ $(OPTS) $(TEST_LD) $(WARNS)
	./$@ $(NOTIFY)
contains_bench:contains_bench.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread $(PROF)
insert_bench:insert_bench.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread $(PROF)
remove_bench:remove_bench.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread  $(PROF)
checker:checker.o
	$(CXX) $^ -o $@ -O4 -g $(WARNS) -lpthread -lboost_thread  $(PROF)

hashtest.o:hashtest.cc hashmap.h locks.h
	$(CXX) -c $< -o $@ $(OPTS) $(WARNS) $(PROF)
insert_bench.o:insert_bench.cc hashmap.h locks.h Makefile
	$(CXX) -c $< -o $@ -O4 -g $(WARNS) $(PROF)
contains_bench.o:contains_bench.cc hashmap.h locks.h Makefile
	$(CXX) -c $< -o $@ -O4 -g $(WARNS) $(PROF)
remove_bench.o:remove_bench.cc hashmap.h locks.h Makefile
	$(CXX) -c $< -o $@ -O4 -g $(WARNS) $(PROF)

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
