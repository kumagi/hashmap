CXX= g++
OPTS= -O4 -g
WARNS= -W -Wall -Wextra -Wformat=2 -Wstrict-aliasing=4 -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wfloat-equal -Wpointer-arith -Wswitch-enum
TEST_LD= -lpthread -lboost_thread
GTEST_INC= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_DIR=/opt/google/gtest-1.5.0

NOTIFY=&& notify-send Test success! -i ~/themes/ok_icon.png || notify-send Test failed... -i ~/themes/ng_icon.png

target:hashtest
target:insert_bench

hashtest:hashtest.o gtest_main.a
	$(CXX) $^ -o $@ $(OPTS) $(TEST_LD) $(WARNS)
	./$@ $(NOTIFY)
insert_bench:insert_bench.o
	$(CXX) $^ -o $@ -O4 -g -pg $(WARNS) -lpthread

hashtest.o:hashtest.cc hashmap.h
	$(CXX) -c $< -o $@ $(OPTS) $(WARNS)
insert_bench.o:insert_bench.cc
	$(CXX) -c $< -o $@ -O4 -g -pg $(WARNS) -fno-inline


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
