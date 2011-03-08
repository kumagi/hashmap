#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <vector>
#include <assert.h>
#include "hashmap.h"
template<typename key, typename value>
void insert_worker(hashmap<key,value>* target
									 ,boost::barrier* b
									 ,const std::vector<key> keys
									 , const std::vector<value> values)
{
	assert(keys.size() == values.size());
	b->wait();
	for(size_t i=0 ; i < keys.size(); ++i){
		target->insert(std::make_pair(keys[i],values[i]));

	}
}
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
template<typename key, typename value>
void get_worker(hashmap<key,value>* target
									 ,boost::barrier* b
									 ,const std::vector<key> keys)
{
	b->wait();
	for(size_t i=0 ; i < keys.size(); ++i){
		try{
		target->get(keys[i]);
		}catch(not_found e){};
	}
}

int main(int argc, char** argv){

	hashmap<int, int> hmp;
	if(argc != 3){
		std::cout << "usage: ./checker trysize threads\n";
		return 1;
	}
	const int testsize = atoi(argv[1]);
	const int threads = atoi(argv[2]) * 3;
	
	std::vector<std::vector<int> > keys, values;
	keys.resize(threads);
	values.resize(threads);
	for(int i = 0; i<threads; ++i){
		keys[i].reserve((testsize+threads-1) / threads);
		values[i].reserve((testsize+threads-1) / threads);
	}
	for(int i=0;i<testsize;i++){
		const int target = i % threads;
		keys[target].push_back(i);
		values[target].push_back(i*i + 1);
		if((target % 3) != 0){
			hmp.insert(std::make_pair(i, i * i));
		}
	}
	boost::barrier bar(threads);
	boost::thread_group tg;
	for(int i=0;i<threads/3;++i){
		tg.create_thread(bind(insert_worker<int,int>,
													&hmp, &bar, keys[i*3], values[i*3]));
		tg.create_thread(bind(get_worker<int,int>, &hmp, &bar, keys[i*3+1]));
		tg.create_thread(bind(remove_worker<int,int>, &hmp, &bar, keys[i*3+2]));
	}
	tg.join_all();
	//hmp.dump();
	for(int i=0;i<testsize;++i){
		if((i % threads % 3) == 2){
			assert(!hmp.contains(i)); // must be removed
		}else{
			assert(hmp.contains(i)); // insert or get
		}
	}
}
