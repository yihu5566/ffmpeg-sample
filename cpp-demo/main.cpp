#include <iostream>
#include <vector>
#include <fstream>
#include <pthread.h>
#include <thread>
#include "safe_queue.h"

using namespace std;

class Test1 {
public:
    Test1() {}

    Test1 operator+(const Test1 &t1) {
        Test1 t;
        t.i = this->i + t1.i;
        return t;
    }

    int i;
};

void task(int i) {
    cout << "task:" << i << endl;
}

void *pthreadTask(void *args) {
    int *i = static_cast<int *>(args);
    cout << "posix线程：" << *i << endl;
    return nullptr;
}

SafeQueue<int> q;

void *get(void *args) {
    while (true) {
        int i;
        q.dequeue(i);
        cout << "消费：" << i << endl;
    }
}

void *put(void *args) {
    while (true) {
        int i;
        cin >> i;
        q.enqueue(i);
    }
}

int main() {
    //---只能指针
    //-------线程同步------------
//    pthread_t pid1, pid2;
//    pthread_create(&pid1, nullptr, get, &q);
//    pthread_create(&pid2, nullptr, put, &q);
//    pthread_join(pid2, nullptr);


//    pthread_attr_t attr;
//    初始化 attr中为操作系统实现支持的线程所有属性的默认值
//    pthread_attr_init(&attr);
//    pthread_t pid;
//    int pi = 100;
//    pthread_create(&pid, nullptr, pthreadTask, &pi);
//    //设置是否分离
//    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//    pthread_join(pid, nullptr);



//    pthread_self();
//    thread t1(task, 100);
//    t1.join();

//    char data[100];
//    ofstream outS;
//    outS.open("/Users/dongfang/CLionProjects/ffmpeg-sample/cpp-demo/text.txt");
//    cout << "输入文字:";
//    cin >> data;
//    outS << data << endl;
//    outS.close();

//    vector<int> v_1(6, 1);
//
//    vector<int> vec_3(v_1);
//    int i = vec_3[1];
//    auto iter = vec_3.begin();
//    for (; iter < vec_3.end(); iter++) {
//        cout << *iter << endl;
//    }
//
//
//    cout << "Hello, World:::" << i << std::endl;

//    Test1 t1;
//    Test1 t2;
//    t1.i = 10;
//    t2.i = 10;
//
//    Test1 t3 = t1 + t2;
//
//    std::cout << t3.i << std::endl;

    return 0;
}
