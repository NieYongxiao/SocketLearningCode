#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<string>
#include <unistd.h>
#include<iostream>
#include<thread>

//测试代码，并没有使用c++面向对象的思想
using std::string,std::cout,std::endl,std::thread;

class CoutTest{
public:
    CoutTest(){
        std::cout<<"CoutTest 无参构造函数"<<std::endl;
    }
    CoutTest(CoutTest &test){
        *this = test;
        std::cout<<"CoutTest 复制构造函数"<<std::endl;
    }
    ~CoutTest(){
        std::cout<<"CoutTest 析构函数"<<std::endl;
    }
public:
    void cout_test_a(){
        std::cout << "Test A" << std::endl;
    }
    void cout_test_b(){
        std::cout << "Test B" << std::endl;
    }
};

void class_test(){
    CoutTest ct;
    std::thread t1(&CoutTest::cout_test_a,ct); 
    t1.join();
    std::thread t2(&CoutTest::cout_test_b,&ct); 
    t2.join();
}

void default_test(int num,const string& str){  //若使用thread t(default_test,5,"default"); 则此函数参数必须为const string&
    for(int i=0;i<num;++i){
        cout<<std::this_thread::get_id()<<" cout : "<<str<<endl;
    }
}
int main(){
    thread t1(default_test,10000,"default_t1");
    thread t2(default_test,10000,"default_t2");
    t1.join();
    t2.join();
    return 0;
}

