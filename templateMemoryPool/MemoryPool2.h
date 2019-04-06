/*************************************************************************
	> File Name: MemoryPool2.h
	> Author: lihengfeng
	> Mail: 17704623602@163.com 
	> Created Time: Sat Dec  9 20:22:02 2017
 ************************************************************************/

#include"HandlerSupport.h"

#include<iostream>
#include<mutex>
#include<new>
#include<exception>

	void defaultHandler(){
		perror("bad alloc");
		abort();
	}
template<typename T>
class MemoryPool{
public:
	MemoryPool() noexcept{
		//将handler设置为默认的
	//	handler=std::bind(&MemoryPool<T>::defaultHandler,this);
		handler=defaultHandler;
	}
	explicit MemoryPool(size_t size) noexcept:chunkSize(size){
		//handler=std::bind(&MemoryPool<T>::defaultHandler,this);
		handler=defaultHandler;
	}

	MemoryPool(const MemoryPool& other)=delete; 
	MemoryPool& operator=(const MemoryPool& other)=delete; 


	~MemoryPool(){
		for(int i=0;i<chunkSize;i++)
		{
			::operator delete(chunkBeginList[i]);
		}

		/*释放游离的区块。
		obj* p=firstFloatBlock;
		obj* q=p;
		while(q!=nullptr){
			q=p->next;
			::operator delete(reinterpret_cast<void*>(p));
			p=q;
		}
		//isLink=false，说明了allocate获取的是最新申请的chunk部分，也就是chunk还没有全部分配出去，所以要把未分配的delete掉.
		if(!isLink){
			::operator delete(reinterpret_cast<void*>(currentBlock));
		}
		*/
	}

	//向系统申请一块大的内存
	void*  allocChunk(){
	if(chunkAmount==MAX_CHUNK_AMOUNT)
	throw std::bad_alloc();

		//HandlerGuard以对象管理handler资源，构造时调用set_new_handler(handler)
		HandlerGuard hg(handler);
		char*p =static_cast<char*>(::operator new(chunkSize));

	chunkBeginList[chunkAmount]=p;	
		currentBlock=reinterpret_cast<obj*>(p);
		poolEnd=p+chunkSize;  

	++chunkAmount;
		isLink=false;

		return p;
	}
	//向内存池申请内存
	void* allocate(){
		void* result=reinterpret_cast<void*>(currentBlock);

		if(result==nullptr||result==poolEnd){
			if(firstFloatBlock==nullptr){
				result=allocChunk();
			}
			else{
				isLink=true;
				result=firstFloatBlock;
			}
		}
		if(isLink){
			firstFloatBlock=firstFloatBlock->next;
			currentBlock=firstFloatBlock; //标记(1)
		}
		else 
			++currentBlock;
		return result;
	}
	//将占用内存还给内存池
	void deallocate(void* p){
		//这里会和标记1 造成冲突，
		//obj* q=firstFloatBlock;
		//firstFloatBlock=reinterpret_cast<obj*>(p);
		//firstFloatBlock->next=q;
		
		if(firstFloatBlock==nullptr)
			firstFloatBlock=reinterpret_cast<obj*>(p);

		obj* temp=lastFloatBlock;
		lastFloatBlock=reinterpret_cast<obj*>(p);
		lastFloatBlock->next=nullptr;
		if(temp!=nullptr)
			temp->next=lastFloatBlock;
	}

	//模板类的模板成员函数，所以要重新定义typeName
	template<typename U,typename... Args>
	void construct(U* adrress,Args&&... args){
		new(adrress) U (std::forward<Args>(args)...);
	}
	template<typename U>
	void destroy(U* p){
		p->~U();
	}

	//设置内存不足时的异常处理方案
	void setHandler(std::new_handler f){
		handler=f;
	}

private:

	union obj{
		T t;
		obj* next;
	};
	obj* currentBlock=nullptr;
	obj* firstFloatBlock=nullptr;
	obj* lastFloatBlock=nullptr;
	char* poolEnd=nullptr;  //指向内存新申请的大块内存的最后一个区块后面的位置
	char* poolBegin=nullptr;  //指向内存新申请的大块内存的最后一个区块后面的位置

	
	int  chunkAmount=0;
	const size_t chunkSize=2048;

	const static  size_t MAX_CHUNK_AMOUNT=128;
	char* chunkBeginList[MAX_CHUNK_AMOUNT];

	bool isLink=false;

	std::mutex mutex_;

	std::new_handler handler;//c++ set_new_handler机制 
};
