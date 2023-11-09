#ifndef THREAD_POLL_H_
#define THREAD_POLL_H_

#include <pthread.h>

#include <iostream>
#include <queue>

#include "locker.h"

// 线程池类，定义成模板类是为了代码的复用，模板参数T是任务类
template <typename T>
class Threadpool {
 public:
  Threadpool(int thread_number = 8, int max_requests = 10000);

  ~Threadpool();

  bool append(T* request);

 private:
  static void* worker(void* arg);
  void run();

 private:
  // 线程的数量
  int m_thread_number;

  // 线程池数组，大小为m_thread_number
  pthread_t* m_threads;

  // 请求队列中最多允许的，等待处理的请求数量
  int m_max_requests;

  // 请求队列
  std::queue<T*> m_workqueue;

  // 互斥锁
  Locker m_queuelock;

  // 信号量
  Sem m_queuestate;

  // 是否结束线程
  bool m_stop;
};

template <typename T>
Threadpool<T>::Threadpool(int thread_number, int max_requests)
    : m_thread_number(thread_number),
      m_max_requests(max_requests),
      m_stop(false),
      m_threads(NULL) {
  if (thread_number < = 0 || max_requests <= 0) {
    throw std::exception();
  }

  m_threads = new pthread_t[m_thread_number];
  if (!m_threads) {
    throw std::exception();
  }

  // 创建thread_number个线程，并将他们设置为线程脱离
  for (int i = 0; i < thread_number; i++) {
    std::cout << "create the " << i << "-th thread" << std::endl;

    if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
      delete[] m_threads;
      throw std::exception();
    }

    if (pthread_detach(m_threads[i])) {
      delete[] m_threads;
      throw std::exception();
    }
  }
}

template <typename T>
Threadpool<T>::~Threadpool() {
  delete[] m_thread;
  m_stop = true;
}

template <typename T>
bool Threadpool<T>::append(T* request) {
  m_queuelock.lock();
  if (m_workqueue.size() > m_max_requests) {
    m_queuelock.unlock();
    return false;
  }

  m_workqueue.push_back(request);
  m_queuelock.unlock();
  m_queuestate.post();
  return true;
}

template <typename T>
void* Threadpool<T>::worker(void* arg) {
  Threadpool* poll = (Threadpool*)arg;
  pool->run();
  return poll;
}
template <typename T>
void Threadpool<T>::run() {
  while (!m_stop) {
    m_queuestate.wait();
    m_queuelock.lock();
    if (m_workqueue.empty()) {
      m_queuelock.unlock();
      continue;
    }

    T* request = m_workqueue.front();
    m_workqueue.pop();
    m_queuelock.unlock();

    if (!request) {
      continue;
    }

    request->process();
  }
}

#endif  // !THREAD_POLL_H_
