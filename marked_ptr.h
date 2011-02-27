#ifndef MARKED_PTR_H
#define MARKED_PTR_H
#include <stdint.h>
#include <utility>
#include <assert.h>

template <typename T>
class marked_ptr{
	// warning: this is not a smart pointer
	typedef marked_ptr<T> marked_ptr_t;
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
	void swap(marked_ptr_t& other){
		std::swap(ptr, other.ptr);

	}
private:
	marked_ptr();
	marked_ptr(const marked_ptr<T>&);
	marked_ptr& operator=(const marked_ptr<T>&);
	T* ptr;
};

template <typename T>
class marked_vector{
	// warning: this is a smart pointer
	typedef marked_vector<T> marked_vector_t;
public:
	explicit marked_vector(const int& num):size_(num),ptr_(new T[num]){}
	bool try_mark(){
		T* const old = ptr_;
		if(reinterpret_cast<T*>(
				 (reinterpret_cast<uint32_t>(ptr_) & 1)
			 )){return false;}
		T* const new_ptr = reinterpret_cast<T*>(
			reinterpret_cast<uint32_t>(ptr_) | 1
		);
		return cas(old, new_ptr);
	}
	bool is_marked()const{
		return reinterpret_cast<T*>(
			(reinterpret_cast<uint32_t>(ptr_) & 1)
		);
	}
	T* get()const{
		return reinterpret_cast<T*>(
			reinterpret_cast<uint32_t>(ptr_)	& (static_cast<uint32_t>(-1) - 1)
		);
	}
	bool cas(T* const expected, T* const new_ptr){
		return __sync_bool_compare_and_swap(&ptr_, expected, new_ptr);
	}
	void rebind(T* const new_ptr){
		ptr_ = new_ptr;
	}
	void swap(marked_vector_t& other){
		std::swap(ptr_, other.ptr_);
		std::swap(size_, other.size_);
	}
	T& operator[](const int index){
		assert(0 <= index);
		assert(index < size_);
		return get()[index];
	}
	int size()const{return  size_;}
	const T& operator[](const int index)const{
		return get()[index];
	}
	~marked_vector(){
		delete [] get();
	}
private:
	marked_vector();
	marked_vector(const marked_vector<T>&);
	marked_vector& operator=(const marked_vector<T>&);
	int size_;
	T* ptr_;
};

#endif
