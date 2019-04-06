/*************************************************************************
  > File Name: FreeListAllocate.h
  > Author: lihengfeng
  > Mail: 17704623602@163.com 
  > Created Time: Sun Dec  3 16:23:24 2017
 ************************************************************************/
#include"DirectlyAllocate.h"

#include<iostream>
#include<thread>
#include<mutex>
#include<mutex>
#include<vector>
#include<new>
#include<exception>

class FreeListAllocate{
	public:
		char* chunkAlloc(size_t size,size_t& blockCount){
			size_t needSize=size*blockCount;  //需要获取的bytes大小
			size_t poolSize=getMemoryPoolSize();//内存池剩余大小
			char* result=nullptr;

			if(poolSize>size){ //如果pool中多于blockCount区块，就只取blockCount，如果少于就取poolSize/size个
				size_t n=poolSize/size;
				if(n>blockCount)  //只需要count个size大小空间，所以如果多出就取count个
					n=blockCount;
				size_t totalSize=n*size;
				result=memoryPoolStart;
				memoryPoolStart+=totalSize;
				blockCount=n; //调整blockCount，便于refil函数操作
			}
			else{ //size大小空间也无法提供，只能重新申请

				if(poolSize>0){//将内存池中剩余零散空间插入对应的freeList
					size_t index=getFreeListIndex(poolSize);
					obj*volatile p=reinterpret_cast<obj*>(memoryPoolStart); 
					p->next=freeList[index];
					freeList[index]=p;
				}
				size_t needPoolSize=needSize*2+(allocedBytes/2);
				try{
					memoryPoolStart=static_cast<char*>(::operator new(needPoolSize));
				}catch(std::bad_alloc& ba){ 
					//申请出错，系统无足够空间，但我们可以检查freeList中是否有足够的未使用空间，如果有就取出那些空间还给内存池
					obj*volatile* myFreeList=nullptr;
					obj*p=nullptr;
					size_t index=getFreeListIndex(size);
					for(size_t i=index+1;i<=15;i++){  //从freeList[index+1]开始，
						myFreeList=freeList+i;
						p=*myFreeList;
						//如果不为空，就取出第一个区块是放到内存池中
						if(p!=nullptr){
							*myFreeList=p->next;
							//调整memoryPoolStart和memoryPoolEnd
							memoryPoolStart=reinterpret_cast<char*>(p);
							memoryPoolEnd=memoryPoolStart+ (i+1)*BORDERVALUE;
							//因为重新拓展了了内存池，所以需要重新chunkAlloc
							return chunkAlloc(size,blockCount);
						}
					}
					//来到这里说明freeList中也没有可用的内存了，只能将bad_alloc异常抛给下级
					throw;
				}catch(std::exception& e){
					std::cout<<e.what()<<std::endl;
					abort();
				}
				//来到这里说明内存池申请成功,需要让memoryPoolEnd指向正确位置。
				memoryPoolEnd=memoryPoolStart+needPoolSize;

				allocedBytes+=needPoolSize;
				return chunkAlloc(size,blockCount);
			}
			return result;
		}
		void * refill(size_t size){
			size_t blockCount=20;
			void* result;
			obj* currentObj;
			obj* nextObj;
			char* chunk;

			std::unique_lock<std::mutex> lock(mutex_);

			chunk=chunkAlloc(size,blockCount);
			
			//只需锁住chunkAlloc，当chunkAlloc结束
			lock.unlock();

			result=reinterpret_cast<void*>(chunk);

			if(blockCount>1){//大于1个区块，就留下一个区块返回给用户，剩余count-1个快插入freeList
				size_t index=getFreeListIndex(size);

				chunk=chunk+size; //现在chunk指向第二个区块，

				freeList[index]=currentObj=reinterpret_cast<obj*>(chunk); //currentObj和freeList都指向第二个区块

				for(size_t i=2;i<blockCount;i++){//从第2个区块开始串起来
					chunk=chunk+size;
					nextObj=reinterpret_cast<obj*>(chunk);
					currentObj->next=nextObj;
					currentObj=nextObj;
				}
				currentObj->next=nullptr;
			}
			return result; 
		}
		void* allocate(size_t size){
			//大于128字节，就调用第一级配置器
			if(size>128)
				return DirectlyAllocate::allocate(size);

			size_t index=getFreeListIndex(size);

			//当申请同样同样区块的时候才会互斥，不同的区块申请不会阻塞.
			std::lock_guard<std::mutex> lock(mutexs[index]);

			obj* p=freeList[index]; 

			if(p==nullptr){
				return refill(size);
			}
			freeList[index]=p->next;
			return p;
		}	
		static void deallocate(void *p, size_t size)
		{
			obj *q = reinterpret_cast<obj*>(p);
			if(q==nullptr){
				perror("deallocate 传递的指针为空");
				abort();
			}
			if(size>128){
				DirectlyAllocate::deallocate(p,size);
				return;
			}
			size_t index=getFreeListIndex(size);
			//这里设计到freeList[index]的修改，需要加锁
			std::lock_guard<std::mutex> lock(mutexs[index]);
			//头插法，插入对应大小的区块链表
			q->next=freeList[index];
			freeList[index]=q;
		}
	private:
		static size_t getMemoryPoolSize(){
			return memoryPoolEnd- memoryPoolStart;
		}
		static size_t getFreeListIndex(size_t size){//得到size大小匹配的freeList的下标
			size_t index=size%BORDERVALUE? size/BORDERVALUE : size/BORDERVALUE -1 ;
		}
		static size_t roundUp(size_t size){//上调到最近的8的倍数
			return (size+BORDERVALUE-1)&(~(BORDERVALUE -1)); //实际  (size+7)& 1111000，从而消去了后三位。
		}

		union obj{
			obj* next;  //指向下一个obj*
			char address[1];  //通过obj->address得到该obj对象的char*类型地址，不用转换.
		};//不要忘了加;

		static const size_t BORDERVALUE=8; //小型区块的上调边界
		static  const size_t MAX_BYTES=128; //最大的小型区块拥有的容量大小
		static const size_t FREE_LIST_COUNT=MAX_BYTES/BORDERVALUE; //freeList的数组容量

		static char* memoryPoolStart; //内存池开始位置
		static char* memoryPoolEnd; //内存池结束位置
		static size_t allocedBytes;  //已经申请了的字节数

		static obj* freeList[16];

		static std::vector<std::mutex> mutexs; //保护freeList[16]中的各个值.
		static std::mutex mutex_; //用于保护memoryPoolStart和memoryPoolEnd
};

size_t FreeListAllocate::allocedBytes=0;
std::vector<std::mutex> FreeListAllocate::mutexs(16); //保护freeList[16]中的各个值.
