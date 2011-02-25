#ifndef MARKED_PTR_H
#define MARKED_PTR_H
#include <stdint.h>

template <typename T>
class marked_ptr{
	// warning: this is not a smart pointer
public:
	explicit marked_ptr(T* const p):ptr(p){}
	bool try_mark(){
		T* const old = ptr;
		T* const new_ptr = reinterpret_cast<T*>(
			reinterpret_cast<uint32_t>(ptr) | 1
		);
		return cas(old, new_ptr);
	}
	bool is_marked()const{
		return reinterpret_cast<T*>(
			(reinterpret_cast<uint32_t>(ptr) & 1)
		);
	}
	
	operator T*()const {
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
	marked_ptr();
	marked_ptr(const marked_ptr<T>&);
	marked_ptr& operator=(const marked_ptr<T>&);
	T* ptr;
};

#endif
