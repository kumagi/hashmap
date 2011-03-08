#define NDEBUG
#include "hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>


hashmap<int,int> hmp;
int j = 0;
void worker(const int work, const int offset){
	for(int i = 0; i<work; ++i){
		hmp.insert(std::make_pair(i + offset, i*2));
		j++;
	}
}
int main(int argc, char**argv){
	if(argc != 3){
		printf("input the try number\n");
		exit(1);
	}
	const int trysize = atoi(argv[1]);
	const int threads = atoi(argv[2]);
	boost::thread_group tg;
	for(int i=0; i<threads; ++i){
		tg.create_thread(boost::bind(worker,trysize/threads,i*(trysize/threads)));
	}
	tg.join_all();
	std::cout << j << std::endl;
}

