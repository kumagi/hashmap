#ifndef MARKED_PTR_H
#define MARKED_PTR_H
#include <stdint.h>

template <typename T>
class marked_ptr{
public:
	marked_ptr():ptr(0){}
	explicit marked_ptr(T* const p):ptr(p){}
	void try_mark(){
		const uint32_t old = reinterpret_cast<uint32_t>(ptr);
		const uint32_t new_ptr = reinterpret_cast<uint32_t>(ptr) | 1;
		ptr.cas(ptr, reinterpret_cast<uint32_t>(old), new_ptr);
	}
	bool is_marked()const{
		return reinterpret_cast<T*>(
			(reinterpret_cast<uint32_t>(ptr) & 1)
		);
	}
	operator T*()const{
		return reinterpret_cast<T*>(
			reinterpret_cast<uint32_t>(ptr) 
			^ (reinterpret_cast<uint32_t>(ptr) & 1)
		);
	}
	bool cas(T* const expected, T* const new_ptr){
		return __sync_bool_compare_and_swap(&ptr, expected, new_ptr);
	}
	void rebind(T* const new_ptr){
		ptr = new_ptr;
	}
private:
	T* ptr;
};

#endif
