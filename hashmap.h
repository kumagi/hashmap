#ifndef HASHMAP_H
#define HASHMAP_H

#include <linux/futex.h>
#include <pthread.h>
#include <map>
#include <cstddef>
#include "locks.h"
#include "hash.h"
#include "marked_ptr.h"
#include "atomic_integer.h"

#include <iostream>
#include <stdexcept>
#include <string>
class not_found : public std::logic_error {
public:
	not_found():std::logic_error("not found"){}
};

template <typename key, typename value, int locks = 8>
class hashmap{
public:
	hashmap(const uint32_t size = 32, const uint16_t lf = 256)
		:load_factor(lf),loads(0),bucket_vector(size)
	{
		for(int i=0;i < bucket_vector.size();++i){
			bucket_vector[i] = NULL;
		}
	}
	bool insert(const std::pair<const key,value>& kvp){
		const std::size_t hashed = hash_value(kvp.first);

		detail::scoped_lock<detail::spin_lock> 
			lk(lock[hashed % bucket_vector.size() % locks]);
		bucket_t *curr = bucket_vector[hashed % bucket_vector.size()];
		bucket_t **prev = &bucket_vector[hashed % bucket_vector.size()];
		while(curr != NULL){
			if(curr->kvp.first == kvp.first){
				return false;
			}
			prev = &curr->next;
			curr = curr->next;
		}
		
		*prev = new bucket_t(kvp);
		
		const uint32_t load = loads.faa(1);
		if(load * 255 > static_cast<uint64_t>(load_factor) * bucket_vector.size()){
			uint32_t old_size = bucket_vector.size();
			lk.unlock();
			if(buckets_extend()){
				std::cout << "load:" << load << " * 255 > " <<
					load_factor << " * " << old_size << std::endl;
				loads.faa(-load);
			}
		}
		return true;
	}
	bool contains(const key& k)const{
		const std::size_t hashed = hash_value(k);
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed % bucket_vector.size() % locks]);
		const bucket_t* target = bucket_vector[hashed % bucket_vector.size()];
		while(target != NULL){
			if(target->kvp.first == k){ return true; }
			target = target->next;
		}
		return false;
	}
	value* get_unsafe(const key& k)const{
		const std::size_t hashed = hash_value(k);
		bucket_t* target = &bucket_vector[hashed % bucket_vector.size()];
		while(target != NULL && target->kvp != NULL){
			if(target->kvp->first == k){ return &target->kvp->second; }
			target = target->next;
		}
		return NULL;
	}
	value get(const key& k)const throw(not_found){
		const std::size_t hashed = hash_value(k);
		const bucket_t* target = bucket_vector[hashed % bucket_vector.size()];
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed % bucket_vector.size() % locks]);
		while(target != NULL){
			if(target->kvp.first == k){ return value(target->kvp.second); }
			target = target->next;
		};
		throw not_found();
	}
	bool remove(const key& k){
		const std::size_t hashed = hash_value(k);
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed % bucket_vector.size() % locks]);
		bucket_t **pred = &bucket_vector[hashed % bucket_vector.size()];
		bucket_t *curr = *pred;
		while(curr != NULL){
			if(curr->kvp.first == k){
				*pred = curr->next;
				delete curr;
				loads.faa(-1);
				return true;
			}
			pred = &curr->next;
			curr = curr->next;
		};
		return false;
	}
	~hashmap(){
		for(int i=0; i < bucket_vector.size(); i++){
			bucket_t*  ptr = bucket_vector[i], *old_next;
			while(ptr != NULL){
				old_next = ptr->next;
				delete ptr;
				ptr = old_next;
			}
		}
	}
	void dump()const{
		for(int i=0; i < locks; ++i){	lock[i].lock();	}
		bucket_t* const * const array = bucket_vector.get();
		for(int i=0; i < bucket_vector.size(); ++i){
			const bucket_t* ptr = array[i];
			std::cout << "[" << i << "]->";
			while(ptr != NULL){
				std::cout << "[" << ptr->kvp.first << "=" << ptr->kvp.second << "]->";
				ptr = ptr->next;
			}
			std::cout << std::endl;
		}
		for(int i=0; i < locks; ++i){	lock[i].unlock();	}
	}
private:
	bool buckets_extend(){
		if(!bucket_vector.try_mark()){
			return false; // other one already extending bucket
		}
		const std::size_t newsize = bucket_vector.size() * 2;
		marked_vector<bucket_t*> new_buckets(newsize);
		bool locked[locks];

		for(int i=0; i < locks; ++i){locked[i] = false;}
		for(int i=0; i < bucket_vector.size(); ++i){
			const int target_lock = i % locks;
			if(!locked[target_lock]){ // get lock
				lock[target_lock].lock();
				locked[target_lock] = true;
			}
			bucket_t *curr = bucket_vector[i];
			bucket_t **tail_smaller = &new_buckets[i];
			bucket_t **tail_bigger = &new_buckets[i+bucket_vector.size()];
			while(curr != NULL){
				const std::size_t hashed = hash_value(curr->kvp.first);
				if(hashed % newsize == i){
					*tail_smaller = curr;
					tail_smaller = &curr->next;
				}else{
					*tail_bigger = curr;
					tail_bigger = &curr->next;
				}
				curr = curr->next;
			}
			*tail_smaller = *tail_bigger = NULL;
		}
		// assert all lock gained
		bucket_vector.swap(new_buckets);
		for(int i=0; i < locks; ++i){
			lock[i].unlock();
		}
		return true;
	}
	struct bucket_t{
		typedef	std::pair<const key,value> pair_t;
		pair_t kvp;
		bucket_t* next;
		bucket_t(const pair_t& k):kvp(k),next(NULL){}
	};

	mutable detail::spin_lock lock[locks];
	const uint32_t load_factor; // (load_factor/255) is load_factor percentage
	atomic_integer<uint32_t> loads;
	marked_vector<bucket_t*> bucket_vector;
};

#endif
