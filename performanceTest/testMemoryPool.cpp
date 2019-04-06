/*************************************************************************
	> File Name: testMemoryPool.cpp
	> Author: lihengfeng
	> Mail: 17704623602@163.com 
	> Created Time: Mon Dec 11 19:22:22 2017
 ************************************************************************/
#include <stdlib.h>

#include <cassert>
#include <ctime>
#include <vector>
#include<iostream>
#include <sys/time.h>  

#include"../templateMemoryPool/MemoryPool.h"
using namespace std;

int main(){
	double mempooltime1 = 0.0;  
	double mempooltime2 = 0.0;  
	double mempooltime3 = 0.0;  

	double systime1 = 0.0;  
	double systime2 = 0.0;  
	double systime3 = 0.0;  

	double vectortime1 = 0.0;  
	double vectortime2 = 0.0;  
	double vectortime3 = 0.0;  
	double timeuse = 0.0;  


	struct timeval start;  
	struct timeval end;  

	MemoryPool<int> pool;
	int* memAddrs[123456]; 

	//测试 mempool 的acllcoate 与 construct
	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		int* p=static_cast<int*>(pool.allocate());
		pool.construct(p,6);
		memAddrs[i]=p;
	}
	gettimeofday( &end, NULL );  
	mempooltime1= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	//测试 mempool 的destroy与deallocate 
	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		pool.destroy(memAddrs[i]);
		pool.deallocate(memAddrs[i]);
	}
	gettimeofday( &end, NULL );  
	mempooltime2= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	//继续测试 mempool 的acllcoate 与 construct
	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		int* p=static_cast<int*>(pool.allocate());
		pool.construct(p,6);
		memAddrs[i]=p;
	}
	gettimeofday( &end, NULL );  
	mempooltime3= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  


	//测试普通new/delete所需时间
	int* addrs[123456]; 
	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		int* p=new int(6);
		addrs[i]=p;
	}
	gettimeofday( &end, NULL );  
	systime1= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		delete addrs[i];
	}
	gettimeofday( &end, NULL );  
	systime2= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		int* p=new int(6);
		addrs[i]=p;
	}
	gettimeofday( &end, NULL );  
	systime3= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	//测试vector 100000 所需时间
	vector<int> v;
	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		v.push_back(6);
	}
	gettimeofday(&end,NULL);  
	vectortime1= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	gettimeofday(&start, NULL );  
	v.clear();
	gettimeofday( &end, NULL );  
	vectortime2= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	gettimeofday(&start, NULL );  
	for(int i=0;i<100000;i++){
		v.push_back(6);
	}
	gettimeofday( &end, NULL );  
	vectortime3= 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;  

	cout<<" 	  memorypool(us)	new/delete(us)	vector(us)"<<endl;
	cout<<"alloc:		"<<mempooltime1<<"		"<<systime1<<"		"<<vectortime1<<endl;
	cout<<"deallocate:	"<<mempooltime2<<"		"<<systime2<<"		"<<vectortime2<<endl;
	cout<<"again alloc:	"<<mempooltime3<<"		"<<systime3<<"		"<<vectortime3<<endl;
	cout<<"all time:	"<<mempooltime1+mempooltime2+mempooltime3<<"		"<<systime1+systime2+systime3<<"		"<<vectortime1+vectortime2+vectortime3<<endl;
}
