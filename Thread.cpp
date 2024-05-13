#include <iostream>
#include <pthread.h>
#include "thread.h"

using namespace std;

Thread::Thread(): _thread(0), _runnable(0) {
}

Thread::Thread(Runnable* pRunnable): _thread(0), _runnable(pRunnable) {
}

void Thread::start() {
  cout << "Thread::Start() called" << endl;
  //remove warning
  //int nr = pthread_create(&_thread, NULL, &Thread::Main, this);
  pthread_create(&_thread, NULL, &Thread::Main, this);
}

void Thread::wait() {
  cout << "Thread::Wait() called" << endl;
  void* pData;
  //remove warning
  //int nr = pthread_join(_thread, &pData);
  pthread_join(_thread, &pData);
}

void Thread::run() {
  if (_runnable != 0) {
    cout << "Thread::Run(): calling runnable" << endl;
    _runnable->run();
  }
}

void* Thread::Main(void* pInst) {
  cout << "Thread::Main() called" << endl;
  Thread* pt = nullptr;

  try{
      pt = static_cast<Thread*>(pInst);
      pt->run();
  }catch(std::runtime_error& exception) {
      cout << "Thread::Main() Failed to create thread" << endl;
  }

  return pt;
}

