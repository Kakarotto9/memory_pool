/*************************************************************************
	> File Name: HandlerSupport.h
	> Author: lihengfeng
	> Mail: 18554239115@163.com 
	> Created Time: Fri Dec  1 19:03:59 2018
 ************************************************************************/

#include<iostream>
class HandlerGuard{
public:
	explicit HandlerGuard(std::new_handler h){
		oldHandler=std::set_new_handler(h);	
	}
	HandlerGuard(const HandlerGuard& other)=delete;
	HandlerGuard& operator=(const HandlerGuard& other)=delete;
	
	~HandlerGuard(){ //还原到之前的handler
		std::set_new_handler(oldHandler);	
	}
private:
	std::new_handler oldHandler;
};
//必须声明为template类，因为成员变量handler是static的，如果不声明为template，会导致HandlerSupport的所有派生类使用共同的handler,这明显不对
template<typename T>
class HandlerSupport{
public:
	static void setHandler(std::new_handler f){
		handler=f;
	}
	static void* operator new(size_t size) throw(std::bad_alloc){
		HandlerGuard hg(handler);
		return ::operator new(size);
	}
private:
	//必须是静态，因为setHandler和operator是静态函数
	static std::new_handler handler; 
};
//在类内只进行了声明，这里才是定义，一个变量如果被使用必须有定义
template<typename T>
std::new_handler HandlerSupport<T>::handler=nullptr;

