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
	hashmap()
		:bucket_size(32),load_factor(200),loads(0),bucket(new bucket_t[bucket_size])
	{}
	hashmap(const uint8_t lf)
		:bucket_size(32),load_factor(lf),loads(0),bucket(new bucket_t[bucket_size])
	{}
	bool insert(const std::pair<const key, value>& kvp){
		std::pair<const key, value>* new_kvp = new std::pair<const key, value>(kvp);
		insert_mov(new_kvp);
	}
	bool insert_mov(std::pair<const key,value>* kvp){
		const std::size_t hashed = hash_value(kvp->first);
		bucket_t *target = &bucket[hashed % bucket_size]
			,*old_target;
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed%locks]);

		while(target != NULL){
			if(target->kvp == NULL){
				target->kvp = kvp;
				if(loads.faa(1) * load_factor > load_factor * bucket_size){
					buckets_extend();
				}
				return true;
			}else if(target->kvp->first == kvp->first){
				return false;
			}
			old_target = target;
			target = target->next;
		}
		old_target->next = new bucket_t();
		old_target->next->kvp = kvp;
		return true;
	}
	bool contains(const key& k)const{
		const std::size_t hashed = hash_value(k);
		const bucket_t* target = &bucket[hashed % bucket_size];
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed%locks]);
		while(target != NULL && target->kvp != NULL){
			if(target->kvp->first == k){ return true; }
			target = target->next;
		}
		return false;
	}
	value* get_unsafe(const key& k)const{
		const std::size_t hashed = hash_value(k);
		bucket_t* target = &bucket[hashed % bucket_size];
		while(target != NULL && target->kvp != NULL){
			if(target->kvp->first == k){ return &target->kvp->second; }
			target = target->next;
		}
		return NULL;
	}
	value get(const key& k)const throw(not_found){
		const std::size_t hashed = hash_value(k);
		const bucket_t* target = &bucket[hashed % bucket_size];
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed%locks]);
		while(target != NULL && target->kvp != NULL){
			if(target->kvp->first == k){ return value(target->kvp->second); }
			target = target->next;
		};
		throw not_found();
	}
	bool remove(const key& k){
		const std::size_t hashed = hash_value(k);
		bucket_t* target = &bucket[hashed % bucket_size]
			,*old_target = NULL;
		
		detail::scoped_lock<detail::spin_lock> lk(lock[hashed%locks]);
		while(target != NULL && target->kvp != NULL){
			if(target->kvp->first == k){
				if(old_target != NULL){
					old_target->next = target->next;
					target->unlink();
					delete target;
					return true;
				}else{
					target->kvp = NULL;
				}
			}
			old_target = target;
			target = target->next;
		};
	}
	~hashmap(){
		if(bucket){
			delete[] bucket;
			bucket.rebind(NULL);
		}
	}
	void dump()const{
		for(int i=0; i < locks; ++i){	lock[i].lock();	}
		for(int i=0; i < bucket_size; ++i){
			const bucket_t* ptr = &bucket[i];
			std::cout << "[" << i << "]->";
			while(ptr != NULL){
				if(ptr->kvp){
					std::cout << "[" << ptr->kvp->first << "=" << ptr->kvp->second << "]->";
				}else{
					std::cout << "[ ]->";
				}
				ptr=ptr->next;
			}
			std::cout << std::endl;
		}
		for(int i=0; i < locks; ++i){	lock[i].unlock();	}
	}
private:
	void buckets_extend(){
		std::cout << "resize\n";
		if(!bucket.try_mark()){
			return; // other one already extending bucket
		}
		const std::size_t newsize = bucket_size * 2;
		marked_ptr<bucket_t> new_bucket(new bucket_t[newsize]);
	}
	struct bucket_t{
		std::pair<const key,value>* kvp;
		bucket_t* next;
		bucket_t():kvp(NULL),next(NULL){}
		void unlink(){
			next = NULL;
		}
		~bucket_t(){
			if(next != NULL){
				delete next;
			}
			if(kvp != NULL){
				delete kvp;
			}
		}
	};

	mutable detail::spin_lock lock[locks];
	std::size_t bucket_size;
	const uint8_t load_factor; // (load_factor/255) is load_factor percentage
	atomic_integer<uint32_t> loads;
	marked_ptr<bucket_t> bucket;
};

#endif
