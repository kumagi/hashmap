#define NDEBUG
#include "hashmap.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/timer.hpp>
#include <vector>
#include <assert.h>
#include <stdio.h>
#include <iostream>

template<typename key, typename value>
void remove_worker(hashmap<key,value>* target
									 ,boost::barrier* b
									 ,const std::vector<key> keys)
{
	b->wait();
	for(size_t i=0 ; i < keys.size(); ++i){
		target->remove(keys[i]);
	}
}

int main(int argc, char** argv){
	if(argc != 5){
		std::cout << "usage: ./checker [trysize] [threads] [locks] [max chain]\n";
		return 1;
	}
	const int testsize = atoi(argv[1]);
	const int threads = atoi(argv[2]);
	const int locks = atoi(argv[3]);
	const int max_chain = atoi(argv[4]);
	if(locks < 1){
		std::cout << "invalid lock number " << locks << std::endl;
		return 1;
	}
	if(max_chain < 1){
		std::cout << "invalid max chain " << max_chain << std::endl;
	}
	hashmap<int, int> hmp(locks, 11, max_chain);
	
	std::vector<std::vector<int> > keys;
	for(int i = 0; i<threads; ++i){
		keys.reserve(threads);
	}
	for(int i=0;i<testsize;i++){
		const int target = i % threads;
		keys[target].push_back(i);
		hmp.insert(std::make_pair(i, i * i));
	}
	boost::timer time;
	boost::barrier bar(threads);
	boost::thread_group tg;
	for(int i=0;i<threads;++i){
		tg.create_thread(bind(remove_worker<int,int>, &hmp, &bar, keys[i]));
	}
	tg.join_all();
	double elapsed = time.elapsed();
	printf("remove:%d items by %d threads %d locks %f q/s\n",
				 testsize, threads, locks, (double)testsize/elapsed);
	//hmp.dump();
}
