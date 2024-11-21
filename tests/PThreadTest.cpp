#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <mutex>
#include <vector>

using namespace std;

#define PRODUCE_NUM_MAX 10000
#define PRODUCE_BUFF_SIZE 40
static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static pthread_key_t s_thread_key;

struct MsgVec {
  // 一个mutex可以对应多个cond，但是一个cond只能对应一个mutex.
  pthread_mutex_t m_mutex;
  pthread_cond_t *m_wcond;  // 写条件变量，动态申请的必须要调用destroy.
  pthread_cond_t m_rcond;  // 读条件变量.
  int m_maxid;
  vector<int> m_vec;

  MsgVec() {
    m_maxid = 0;
    m_mutex = PTHREAD_MUTEX_INITIALIZER;

    m_wcond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(m_wcond, NULL);

    m_rcond = PTHREAD_COND_INITIALIZER;
  }

  ~MsgVec() {
    if (m_wcond) {
      pthread_cond_destroy(m_wcond);
      free(m_wcond);
      m_wcond = NULL;
    }
    cout << "~Msg()" << endl;
  }
};

struct ConsumeInfo {
  uint64_t m_cnt{0};
  uint64_t m_sum{0};
};

struct ThreadInfo {
  pthread_t m_tid{nullptr};
  uint64_t m_start{0};  // millisecond
  uint64_t m_end{0};
  uint64_t m_cnt{0};
  uint64_t m_sum{0};
  uint32_t m_fakewakes{0};  // 假唤醒次数。唤醒后条件不满足。
};

std::atomic<uint64_t> s_sum;

// 使用s_printmutex对输出进行保护，使本文中的各输出信息不会出现串行
static pthread_mutex_t s_printmutex = PTHREAD_MUTEX_INITIALIZER;
void FreeKeyValue(void *data) {
  ThreadInfo *pInfo = (ThreadInfo *)data;

  pthread_mutex_lock(&s_printmutex);
  //   cout << "mutex: count=" << s_printmutex.__data.__count
  //        << ", lock=" << s_printmutex.__data.__lock
  //        << ", users=" << s_printmutex.__data.__nusers
  //        << ", owner=" << s_printmutex.__data.__owner
  //        << ", thread=" << pthread_self()
  //        << endl;

  cout << "producer:" << pInfo->m_tid << " cnt:" << pInfo->m_cnt
       << " sum:" << pInfo->m_sum << " fake wake cnt:" << pInfo->m_fakewakes
       << " use time:" << pInfo->m_end - pInfo->m_start << endl;
  pthread_mutex_unlock(&s_printmutex);

  delete pInfo;
}

// 创建线程私有数据key和对应的资源清理函数
void CreateThreadKey() { pthread_key_create(&s_thread_key, FreeKeyValue); }

void *Produce(void *pData) {
  // 使自身线程分离，这样主线程不需要join回收资源.
  pthread_detach(pthread_self());

  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  pthread_once(&s_once, CreateThreadKey);  // 所有线程只会调用一次.

  // 各线程第一次pthread_getspecific反回NULL，然后设置自己的私有数据.
  ThreadInfo *threadInfo = (ThreadInfo *)pthread_getspecific(s_thread_key);
  if (threadInfo == NULL) {
    threadInfo = new ThreadInfo;
    pthread_setspecific(s_thread_key, threadInfo);  // 设置线程私有数据
    threadInfo->m_tid = pthread_self();
    threadInfo->m_start = tp.tv_sec * 1000000LL + tp.tv_nsec / 1000ll;
  }

  MsgVec *pVec = (MsgVec *)pData;
  for (;;) {
    pthread_mutex_lock(&pVec->m_mutex);
    while (pVec->m_vec.size() >= PRODUCE_BUFF_SIZE &&
           pVec->m_maxid < PRODUCE_NUM_MAX) {
      pthread_cond_wait(pVec->m_wcond, &pVec->m_mutex);
      if (pVec->m_vec.size() >= PRODUCE_BUFF_SIZE) {
        threadInfo->m_fakewakes++;
      }
    }
    if (pVec->m_maxid >= PRODUCE_NUM_MAX) {
      pthread_mutex_unlock(&pVec->m_mutex);
      break;
    }
    for (int i = 0; i < 10; ++i) {
      int r = rand() % 1000;
      pVec->m_vec.push_back(r);
      threadInfo->m_sum += r;

      s_sum += r;
    }
    pVec->m_maxid += 10;
    threadInfo->m_cnt += 10;
    pthread_mutex_unlock(&pVec->m_mutex);

    // pthread_cond_signal可以放在mutex锁内也可以放在mutex锁外。假设在线程A的mutex锁内调用
    // pthread_cond_signal，唤醒了等待线程B。在线程A退出mutex锁前，操作系统调度到线程B执行，
    // 线程B此时在pthread_cond_wait内被唤醒，退出pthread_cond_wait时需要竞争再次锁上mutex，
    // 因为线程A还没释放锁，会导致线程B再被等待，从而会浪费一次线程调度切换。
    pthread_cond_signal(&pVec->m_rcond);
  }
  clock_gettime(CLOCK_REALTIME, &tp);
  threadInfo->m_end = tp.tv_sec * 1000000LL + tp.tv_nsec / 1000ll;
  pthread_exit(NULL);
}

void *Consume(void *pData) {
  MsgVec *pVec = (MsgVec *)pData;
  ConsumeInfo *result = new ConsumeInfo;
  bool hasMore = true;
  for (; hasMore;) {
    vector<int> batch;

    pthread_mutex_lock(&pVec->m_mutex);
    while (pVec->m_vec.empty()) {
      pthread_cond_wait(&pVec->m_rcond, &pVec->m_mutex);
    }

    // 将数据处理放到锁外
    batch.swap(pVec->m_vec);

    if (pVec->m_maxid == PRODUCE_NUM_MAX) {
      hasMore = false;
    }
    pthread_mutex_unlock(&pVec->m_mutex);

    // 如果不是broadcast模式，只会有一个producer退出，为确保所有producert唤醒，
    // 这里需要唤醒所有producert.
    pthread_cond_broadcast(pVec->m_wcond);

    result->m_cnt += batch.size();
    for (const auto &item : batch) {
      result->m_sum += item;
    }
  }

  // 1.如果有多个信息要返回，可以把这些信息放在一个结构中，返回结构的地址。
  // 2.不要返回栈上变量的地址，因为线程退出后栈销毁，再访问的话就会出错。
  pthread_exit(result);
}

#define arraysize(array) (sizeof(array) / sizeof(array[0]))

// 编译命令：g++ -std=c++17 PThreadTest.cpp -lpthread
int main() {
  MsgVec vec;
  pthread_t producers[10], consumer;

  cout << "producer count: " << arraysize(producers) << endl;
  for (int i = 0; i < arraysize(producers); ++i) {
    pthread_create(&producers[i], NULL, Produce, &vec);
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 4 * 1024 * 1024);
  pthread_create(&consumer, &attr, Consume, &vec);
  pthread_setconcurrency(arraysize(producers) + 1);

  // 用ret接收pt_consumer线程返回的信息的地址。
  ConsumeInfo *result = nullptr;
  pthread_join(consumer, (void **)&result);

  if (result != nullptr) {
    pthread_mutex_lock(&s_printmutex);
    cout << "consume cnt: " << result->m_cnt << ", sum: " << result->m_sum
         << endl;
    pthread_mutex_unlock(&s_printmutex);
  }

  cout << "sum: " << s_sum << endl;

  // 多线程编程时，主线程退出最好用pthread_exit。注意return和pthread_exit的区别。
  pthread_exit(0);
}