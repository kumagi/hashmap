#ifndef ATOMIC_INTGER_H
#define ATOMIC_INTGER_H

template <typename T>
class atomic_integer
{
public:
	atomic_integer():value_(0){}
	atomic_integer(const T& v):value_(v){}
	atomic_integer(const atomic_integer& v):value_(v.get()){}
	const T& get()const{return value_;}
	T faa(const T& v){
		return __sync_fetch_and_add(&value_, v);
	}
	void cas(const T& expect, const T& new_value){
		__sync_bool_compare_and_swap(&value_, expect, new_value);
	}
private:
	T value_;
}; 

#endif
