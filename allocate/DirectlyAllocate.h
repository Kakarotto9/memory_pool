/*************************************************************************
	> File Name: DirectlyAllocate.h
	> Author: lihengfeng
	> Mail: 17704623602@163.com 
	> Created Time: Sun Dec  3 16:22:59 2017
 ************************************************************************/
#include"HandlerSupport.h"

#include<iostream>
#include<new>

class DirectlyAllocate{
public:
 	DirectlyAllocate(const DirectlyAllocate& other)=delete;
 	DirectlyAllocate& operator=(const DirectlyAllocate& other)=delete;
 	DirectlyAllocate& operator=(DirectlyAllocate&& other)=delete;
	
	static void setHandler(std::new_handler f){
		handler=f;
	}

	static void* allocate(size_t size){
		HandlerGuard hg(handler);
		return ::operator new(size);
	}

	static void* deallocate(void*p,size_t size){
		::operator delete(p);
	}
private:
	static std::new_handler handler; 
};


