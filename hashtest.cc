#include <gtest/gtest.h>
#include "hashmap.h"
#include <string>
#include <list>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

TEST(construct,buckets){
	hashmap<int,std::string> hm;
	hashmap<std::string, int> hmp;
	hashmap<int, std::list<int> > h;
}

TEST(insert, int_char)
{
	hashmap<int, char> hmp;
	hmp.insert(std::make_pair<int,char>(2,4));
}
TEST(contains, one_key)
{
	hashmap<int, char> hmp;
	hmp.insert(std::make_pair<int,char>(2,4));
	EXPECT_TRUE(hmp.contains(2));
}
TEST(contains, two_key)
{
	hashmap<int, char> hmp;
	hmp.insert(std::make_pair<int,char>(2,4));
	hmp.insert(std::make_pair<int,char>(3,4));
	EXPECT_TRUE(hmp.contains(2));
	EXPECT_TRUE(hmp.contains(2));
}
TEST(contains, many_key)
{
	hashmap<int, char> hmp;
	for(int i=0; i < 256; ++i){
		hmp.insert(std::make_pair<int,char>(i,i));
	}
	for(int i=0; i < 256; ++i){
		EXPECT_TRUE(hmp.contains(i));
	}
}
TEST(get, one_key){
	hashmap<int, char> hmp;
	hmp.insert(std::make_pair(1,2));
	EXPECT_EQ(hmp.get(1),2);
}
TEST(get, many_key){
	hashmap<int, int> hmp;
	for(int i=0; i < 256; ++i){
		hmp.insert(std::make_pair<int,int>(i,i));
	}
	for(int i=0; i < 256; ++i){
		EXPECT_EQ(hmp.get(i), i);
	}
}

TEST(remove, one_key){
	hashmap<int, char> hmp;
	hmp.insert(std::make_pair(1,2));
	EXPECT_TRUE(hmp.contains(1));
	hmp.remove(1);
	EXPECT_FALSE(hmp.contains(1));
}

TEST(remove, many_key){
	hashmap<int, int> hmp;
	for(int i=0; i < 1024; ++i){
		hmp.insert(std::make_pair<int,int>(i,i));
	}
	for(int i=0; i < 1024; i+=2){
		hmp.remove(i);
	}
	for(int i=0; i < 1024; ++i){
		if(i&1){
			EXPECT_TRUE(hmp.contains(i));
		}else{
			EXPECT_FALSE(hmp.contains(i));
		}
	}
}

template<typename key, typename value>
void insert_worker(hashmap<key,value>* target
									 ,boost::barrier* b
									 ,const std::vector<key> keys
									 , const std::vector<value> values)
{
	assert(keys.size() == values.size());
	b->wait();
	for(int i=0 ; i < keys.size(); ++i){
		target->insert(std::make_pair(keys[i],values[i]));
	}
}
template<typename key, typename value>
void remove_worker(hashmap<key,value>* target
									 ,boost::barrier* b
									 ,const std::vector<key> keys)
{
	b->wait();
	for(int i=0 ; i < keys.size(); ++i){
		target->remove(keys[i]);
	}
}

TEST(concurrent, insert){
	const int testnum = 512;
	std::vector<int> keys_1, keys_2, values_1, values_2;
	for(int i=0;i<testnum;i++){
		keys_1.push_back(i);
		keys_2.push_back(i+testnum);
		values_1.push_back(i*i);
		values_2.push_back((i+testnum) * (i+testnum));
	}
	hashmap<int, int> hmp;
	boost::barrier bar(2);
	boost::thread a(boost::bind(insert_worker<int,int>
															, &hmp, &bar, keys_1, values_1))
		, b(boost::bind(insert_worker<int,int>
										, &hmp, &bar, keys_2, values_2));
	a.join();
	b.join();
}

TEST(concurrent, insert_and_check){
	std::vector<int> keys_1, keys_2, values_1, values_2;
	const int test = 4096;
	for(int i=0;i<test;i++){
		keys_1.push_back(i);
		keys_2.push_back(i+test);
		values_1.push_back(i*i);
		values_2.push_back((i+test) * (i+test));
	}
	hashmap<int, int> hmp;
	boost::barrier bar(2);
	boost::thread a(boost::bind(insert_worker<int,int>
															, &hmp, &bar, keys_1, values_1))
		, b(boost::bind(insert_worker<int,int>
										, &hmp, &bar,  keys_2, values_2));
	a.join();
	b.join();
	for(int i=0;i<test;i++){
		EXPECT_TRUE(hmp.contains(i));
		EXPECT_TRUE(hmp.contains(i + test));
	}
}



TEST(concurrent, remove){
	std::vector<int> keys_1, keys_2, values_1, values_2;
	const int testsize = 4096;
	hashmap<int, int> hmp;
	for(int i=0;i<testsize;i++){
		hmp.insert(std::make_pair(i,i));
		hmp.insert(std::make_pair(i+testsize, (i+testsize)*(i+testsize)));
		keys_1.push_back(i);
		keys_2.push_back(i+testsize);
	}

	boost::barrier bar(2);
	boost::thread a(boost::bind(remove_worker<int,int>
															, &hmp, &bar, keys_1))
		, b(boost::bind(remove_worker<int,int>
										, &hmp, &bar,  keys_2));
	a.join();	b.join();
	for(int i=0;i<testsize;i++){
		EXPECT_FALSE(hmp.contains(i));
		EXPECT_FALSE(hmp.contains(i + testsize));
	}
}


TEST(concurrent, insert_and_remove){
	std::vector<int> keys_1, keys_2, values_1, values_2;
	const int test = 4096;
	hashmap<int, int> hmp;
	for(int i=0;i<test;i++){
		keys_1.push_back(i*2); // to remove
		hmp.insert(std::make_pair(i*2, i));
		keys_2.push_back(i*2+1); // to insert
		values_2.push_back((i+test) * (i+test)); // to insert
	}

	boost::barrier bar(2);
	boost::thread
		a(boost::bind(remove_worker<int,int>, &hmp, &bar, keys_1)),
		b(boost::bind(insert_worker<int,int>, &hmp, &bar,  keys_2, values_2));
	a.join();
	b.join();
	for(int i=0;i<test;i++){
		EXPECT_FALSE(hmp.contains(i*2));
		EXPECT_TRUE(hmp.contains(i*2 + 1));
	}
}
TEST(concurrent, insert_and_remove_more){
	boost::array<std::vector<int>, 4> keys, values;
	hashmap<int, int> hmp;
	const int testsize = 4096;
	for(int i = 0; i<4; ++i){
		keys[i].resize(testsize);
		values[i].resize(testsize);
	}
	for(int i=0;i<4096;i++){
		keys[0][i] = i*4; // to remove
		keys[1][i] = i*4 + 1; // to insert
		keys[2][i] = i*4 + 2; // to remove
		keys[3][i] = i*4 + 3; // to insert
		values[1][i] = i;
		values[3][i] = i;
	}
	boost::barrier bar(4);
	boost::thread
		a(boost::bind(remove_worker<int,int>, &hmp, &bar, keys[0])),
		b(boost::bind(insert_worker<int,int>, &hmp, &bar, keys[1], values[1])),
		c(boost::bind(remove_worker<int,int>, &hmp, &bar, keys[2])),
		d(boost::bind(insert_worker<int,int>, &hmp, &bar, keys[3], values[3]));
	a.join();
	b.join();
	c.join();
	d.join();
	for(int i=0;i<testsize;i++){
		EXPECT_FALSE(hmp.contains(i*4));
		EXPECT_TRUE(hmp.contains(i*4 + 1));
		EXPECT_FALSE(hmp.contains(i*4 + 2));
		EXPECT_TRUE(hmp.contains(i*4 + 3));
	}
}
