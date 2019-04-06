/*************************************************************************
  > File Name: MemoryPool.h
  > Author: lihengfeng
  > Mail: 17704623602@163.com 
  > Created Time: Sat Dec  9 16:23:16 2017
 ************************************************************************/
#include"HandlerSupport.h"

#include<iostream>
#include<mutex>
#include<new>
#include<exception>
void inline defaultHandler(){
	perror("bad alloc");
	abort();
}

template<typename T>
class MemoryPool{
	public:
		MemoryPool() noexcept{
			//	handler=std::bind(&MemoryPool<T>::defaultHandler,this);
			handler=defaultHandler;
		}
		explicit MemoryPool(size_t size) noexcept:chunkSize(size){
			//	handler=std::bind(&MemoryPool::defaultHandler,this);
			handler=defaultHandler;
		}

		//允许使用移动构造
		MemoryPool(MemoryPool&& rhs) noexcept:
			currentBlock(rhs.currentBlock),firstFloatBlock(rhs.firstFloatBlock),lastFloatBlock(rhs.lastFloatBlock)
			{
				currentBlock=firstFloatBlock=lastFloatBlock=nullptr;
			}

		MemoryPool(const MemoryPool& other)=delete; 
		MemoryPool& operator=(const MemoryPool& other)=delete; 
		MemoryPool& operator=(MemoryPool&& other)=delete; 


		//向系统申请一块大的内存
		void*  allocChunk(){
			//HandlerGuard以对象管理handler资源，构造时调用set_new_handler(handler),析构时还原到最初的new_handler
			HandlerGuard hg(handler);
			char*p =static_cast<char*>(::operator new(chunkSize));

			currentBlock=reinterpret_cast<obj*>(p);
			poolEnd=p+chunkSize;
			//进入连续申请模式，所以isLink=false
			isLink=false;

			return p;
		}
		//向内存池申请一个区块
		void* allocate(){
			void* result=reinterpret_cast<void*>(currentBlock);

			//result为空或者等于poolEnd，说明游离区块耗光，或者chunk到了底部,所以判断是有还有游离区块
			if(result==nullptr||result==poolEnd){
				//如果firstFloatBlock==null.需要重新申请内存
				if(firstFloatBlock==nullptr){
					result=allocChunk();
				}
				else{
					isLink=true;//使用游离的区块，所以设置isLink
					result=firstFloatBlock;
				}
			}
			//将currentBlock指向下一个可用区块
			if(isLink){
				firstFloatBlock=firstFloatBlock->next;
				currentBlock=firstFloatBlock; //标记(1)
			}
			else 
				++currentBlock;

			return result;
		}
		//将占用内存还给内存池,通过lastFloatBlock插入到链表的最后
		void deallocate(void* p){

			if(firstFloatBlock==nullptr)
				firstFloatBlock=reinterpret_cast<obj*>(p);

			obj* temp=lastFloatBlock;
			lastFloatBlock=reinterpret_cast<obj*>(p);
			lastFloatBlock->next=nullptr;
			if(temp!=nullptr)
				temp->next=lastFloatBlock;
		}

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

		obj* currentBlock=nullptr;  //指向一个可用区块
		obj* firstFloatBlock=nullptr;  //指向第一个可用的游离区块
		obj* lastFloatBlock=nullptr;    //指向最后一个游离快

		char* poolEnd=nullptr; //pool的最后一个chunk的最后一个block的后面位置

		const size_t chunkSize=2048;
		bool isLink=false;  //isLink=true表示使用的是链表上的区块，否则是使用顺序表的区块

		std::new_handler handler;//c++ set_new_handler机制 
};
