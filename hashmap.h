#ifndef HASHMAP_H
#define HASHMAP_H

#include <linux/futex.h>
#include <pthread.h>
#include <map>
#include <cstddef>
#include "locks.h"
#include "hash.h"
#include "marked_vector.h"

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
	hashmap(const uint32_t size = 11, const uint32_t mc = 3)
		:max_chain(mc), bucket_vector(size)
	{
		for(size_t i=0;i < bucket_vector.size();++i){
			bucket_vector[i] = NULL;
		}
	}
	bool insert(const std::pair<const key,value>& kvp){
		const std::size_t hashed = hash_value(kvp.first);
	retry:
		const std::size_t got_size = bucket_vector.size();
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed % got_size % locks]);
		if(got_size != bucket_vector.size()) {
			lk.unlock();
			goto retry;
		}
				
		bucket_t *curr = bucket_vector[hashed % got_size];
		bucket_t **prev = &bucket_vector[hashed % got_size];
		
		uint32_t chain_counter = 0;
		while(curr != NULL){
			if(curr->kvp.first == kvp.first){
				return false;
			}
			prev = &curr->next;
			curr = curr->next;
			++chain_counter;
		}

		// allocate and combine
		*prev = new bucket_t(kvp);
		
		if(max_chain < chain_counter){
			lk.unlock();
			buckets_extend();
		}
		return true;
	}
	bool contains(const key& k)const{
		const std::size_t hashed = hash_value(k);
		if(locks > 0){
			detail::scoped_lock<detail::spin_lock>
				lk(lock[hashed % bucket_vector.size() % locks]);
		}
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
		if(locks > 0){
			detail::scoped_lock<detail::spin_lock>
				lk(lock[hashed % bucket_vector.size() % locks]);
		}
		while(target != NULL){
			if(target->kvp.first == k){ return value(target->kvp.second); }
			target = target->next;
		};
		throw not_found();
	}
	bool remove(const key& k){
		const std::size_t hashed = hash_value(k);
		if(locks > 0){
			detail::scoped_lock<detail::spin_lock>
				lk(lock[hashed % bucket_vector.size() % locks]);
		}
		bucket_t **pred = &bucket_vector[hashed % bucket_vector.size()];
		bucket_t *curr = *pred;
		while(curr != NULL){
			if(curr->kvp.first == k){
				*pred = curr->next;
				delete curr;
				return true;
			}
			pred = &curr->next;
			curr = curr->next;
		};
		return false;
	}
	~hashmap(){
		for(size_t i=0; i < bucket_vector.size(); i++){
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
		const std::size_t oldsize = bucket_vector.size();
		const std::size_t newsize = oldsize * 2 + 1;
		std::cout << "extend to " << newsize << std::endl;
		marked_vector<bucket_t*> new_buckets(newsize);
		bool locked[locks];
		
		for(size_t i=0; i < locks; ++i){locked[i] = false;}
		for(size_t i=0; i < newsize; ++i){new_buckets[i] = NULL;}
		for(size_t i=0; i < oldsize; ++i){
			const int target_lock = i % locks;
			if(!locked[target_lock]){ // get lock
				lock[target_lock].lock();
				locked[target_lock] = true;
			}
			bucket_t *curr = bucket_vector[i];
			while(curr != NULL){
				const std::size_t new_place = hash_value(curr->kvp.first) % newsize;
				bucket_t* const old_next = curr->next;
				curr->next = new_buckets[new_place];
				new_buckets[new_place] = curr;
				curr = old_next;
			}
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
	const uint32_t max_chain;
	marked_vector<bucket_t*> bucket_vector;
};

template <typename key, typename value>
class hashmap<key,value,0> /* without lock! */
{
public:
	bool insert(const std::pair<key,value>& kvp){
		
	}
};
#endif
