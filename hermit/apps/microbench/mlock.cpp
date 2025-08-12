/*
 * The benchmark to test how many pages the system can handle
 */
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <malloc.h>
#include <algorithm>
#include "numa-config.h"
#include <sys/mman.h>

constexpr static uint64_t kTotalSize = 6ULL * 1024 * 1024 * 1024; /* 6GB total size */
constexpr static uint64_t kMlockSize = 2ULL * 1024 * 1024 * 1024; /* 2GB mlock size */
constexpr static uint64_t kSampleGap = 16ULL; /* Sample every 16 access */
constexpr static double kFrequency = 2.6; /* 2.6 GHz */

#define THREAD_PINNING

class Barrier {
public:
    Barrier(int numThreads) : initialCount(numThreads), count(numThreads), generation(0) {}

    void Wait() {
        std::unique_lock<std::mutex> lock(mutex);
        int gen = generation;

        if (--count == 0) {
            generation++;
            count = static_cast<int>(initialCount);
            cv.notify_all();
        } else {
            cv.wait(lock, [this, gen]() { return gen != generation; });
        }
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    int count;
    int generation;
    const size_t initialCount;
};

static cpu_set_t cpu_set[500]; // Up to 500 threads;

int getCPUid(bool reset)
{
  static int cur_socket = 0;
  static int cur_physical_cpu = 0;
  static int cur_smt = 0;

  if(reset){
          cur_socket = 0;
          cur_physical_cpu = 0;
          cur_smt = 0;
          return 1;
  }

  int ret_val = OS_CPU_ID[cur_socket][cur_physical_cpu][cur_smt];
  cur_physical_cpu++;

  if(cur_physical_cpu == NUM_PHYSICAL_CPU_PER_SOCKET){
          cur_physical_cpu = 0;
          cur_socket++;
          if(cur_socket == NUM_SOCKET){
                  cur_socket = 0;
                  cur_smt++;
                  if(cur_smt == SMT_LEVEL)
                          cur_smt = 0;
          }
  }

  return ret_val;
                        
}

inline void timer_start(unsigned *cycles_high_start, unsigned *cycles_low_start) {
  asm volatile("xorl %%eax, %%eax\n\t"
               "CPUID\n\t"
               "RDTSC\n\t"
               "mov %%edx, %0\n\t"
               "mov %%eax, %1\n\t"
               : "=r"(*cycles_high_start), "=r"(*cycles_low_start)::"%rax",
                 "%rbx", "%rcx", "%rdx");
}

inline void timer_end(unsigned *cycles_high_end, unsigned *cycles_low_end) {
  asm volatile("RDTSCP\n\t"
               "mov %%edx, %0\n\t"
               "mov %%eax, %1\n\t"
               "xorl %%eax, %%eax\n\t"
               "CPUID\n\t"
               : "=r"(*cycles_high_end), "=r"(*cycles_low_end)::"%rax", "%rbx",
                 "%rcx", "%rdx");
}

inline uint64_t get_elapsed_cycles(unsigned cycles_high_start, unsigned cycles_low_start, unsigned cycles_high_end, unsigned cycles_low_end) {
  uint64_t start, end;
  start = ((static_cast<uint64_t>(cycles_high_start) << 32) | cycles_low_start);
  end = ((static_cast<uint64_t>(cycles_high_end) << 32) | cycles_low_end);
  return end - start;
}

/*
 * MemUsage() - Reads memory usage from /proc file system
 */
size_t MemUsage() {
  FILE *fp = fopen("/proc/self/statm", "r");
  if(fp == nullptr) {
    fprintf(stderr, "Could not open /proc/self/statm to read memory usage\n");
    exit(1);
  }

  unsigned long unused;
  unsigned long rss;
  if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld", &unused, &rss, &unused, &unused, &unused, &unused, &unused) != 7) {
    perror("");
    exit(1);
  }

  (void)unused;
  fclose(fp);

  return rss * (4096 / 1024); // in KiB (not kB)
}

int test(uint32_t n_threads, uint32_t stride, bool latency_test){
    long kPageSize = sysconf(_SC_PAGE_SIZE);
    char* test_array = reinterpret_cast<char*>(memalign(kPageSize, kTotalSize * sizeof(char)));
    uint64_t *total_count = reinterpret_cast<uint64_t*>(malloc(n_threads * sizeof(uint64_t)));
    uint64_t per_thread_size = kTotalSize / n_threads / kPageSize * kPageSize;
    assert(per_thread_size % kPageSize == 0);
    assert((uint64_t) test_array % kPageSize == 0);
    struct rusage start, end;
    Barrier bar(n_threads + 1);
    std::vector<std::thread> threads;
    uint64_t *latency_sample;
    if (latency_test){
        latency_sample = reinterpret_cast<uint64_t*>(memalign(64, (per_thread_size / kPageSize * n_threads + kSampleGap) / kSampleGap * sizeof(uint64_t)));
    }

    /* Populate the how area with some value */
    for (uint32_t tid = 0; tid < n_threads; tid ++){
        threads.emplace_back([&, tid](){
            uint64_t thread_start = tid * per_thread_size;
            std::memset(test_array + thread_start, 1, per_thread_size);
        });
    }

    for (auto& thread: threads){
        thread.join();
    }


    /* Test */
    threads.clear();
    assert(kMlockSize <= kTotalSize);
    #ifndef THREAD_PINNING
    uint32_t j = 0;
    CPU_ZERO(&cpu_set[0]);
    for(j = 0; j < n_threads; j++){
      int cpuid = getCPUid(0);
      CPU_SET(cpuid, &cpu_set[0]);
    }
    getCPUid(1);
    #endif
    for (uint32_t tid = 0; tid < n_threads; tid++){
        #ifdef THREAD_PINNING
        int cpuid = getCPUid(0);
        CPU_ZERO(&cpu_set[tid]);
        CPU_SET(cpuid, &cpu_set[tid]);
        #endif
        threads.emplace_back([&, tid](){
            #ifdef THREAD_PINNING
            sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[tid]);
            #else
            sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set[0]);
            #endif
            /* [thread_start, thread_end) */
            uint64_t thread_start = tid * per_thread_size;
            uint64_t thread_end = tid * per_thread_size + kMlockSize / n_threads / kPageSize * kPageSize; 
            assert(thread_start % kPageSize == 0);
            assert(thread_end % kPageSize == 0);
            uint64_t cnt = 0;
            unsigned cycles_high_start, cycles_low_start, cycles_high_end, cycles_low_end;
            bar.Wait();
            for (uint64_t i = thread_start; i + kPageSize < thread_end; i += ((stride) * kPageSize)){
                if (latency_test && (cnt % kSampleGap == 0)){
                    timer_start(&cycles_high_start, &cycles_low_start);

                }
                mlock(test_array + i, stride * kPageSize); 
                if (latency_test && (cnt % kSampleGap == 0)){
                    timer_end(&cycles_high_end, &cycles_low_end);
                    uint64_t elapsed_cycles = get_elapsed_cycles(cycles_high_start, cycles_low_start, cycles_high_end, cycles_low_end);
                    latency_sample[per_thread_size / kPageSize * tid / kSampleGap + cnt / kSampleGap] = elapsed_cycles;
                }
                cnt ++;
            }
            total_count[tid] = cnt;
        }); 
    }

    /* Read the statistics before test */
    int ret = getrusage(RUSAGE_SELF, &start);
    if (ret){
        std::cerr<<"getrusage error before test: "<<ret<<std::endl;
        exit(-1);
    }
    std::cout<<"Before test: mem = "<<MemUsage()<<std::endl;

    auto start_time = std::chrono::system_clock::now();
    bar.Wait();
    for (auto& thread: threads){
        thread.join();
    }
    auto end_time = std::chrono::system_clock::now();

    std::chrono::duration<double, std::micro> elapsed_time = end_time - start_time;

    uint64_t count = 0;
    for (uint32_t i = 0; i < n_threads; i++){
        count += total_count[i];
    }
    double tput = count / elapsed_time.count(); 

    /* Read the statistics after test */ 
    ret = getrusage(RUSAGE_SELF, &end);
    if (ret){
        std::cerr<<"getrusage error after test: "<<ret<<std::endl;
        exit(-1);
    }
    std::cout<<"After test: mem = "<<MemUsage()<<std::endl;
    std::cout<<"Duration: "<<elapsed_time.count()<<" us"<<std::endl;
    std::cout<<"tput: "<<tput<<" mops"<<std::endl;
    std::cout<<"count: "<<count<<std::endl;
    std::cout<<"major page faults: "<<end.ru_majflt - start.ru_majflt<<std::endl;
    std::cout<<"minor page faults: "<<end.ru_minflt - start.ru_minflt<<std::endl;
    if (latency_test){
        uint64_t p50_position = count / kSampleGap / 2;
        uint64_t p99_position = count / kSampleGap / 100;
        std::sort(latency_sample, latency_sample + (per_thread_size / kPageSize * n_threads + kSampleGap) / kSampleGap, std::greater<uint64_t>());
        double p50_latency = latency_sample[p50_position] * 1.0 / kFrequency;
        double p99_latency = latency_sample[p99_position] * 1.0 / kFrequency;
        std::cout<<"p50-latency: "<<p50_latency<<" ns"<<std::endl;
        std::cout<<"p99-latency: "<<p99_latency<<" ns"<<std::endl;
    }
    return 0;

}

int main(int argc, char** argv){
    uint32_t n_threads = 1;
    uint32_t stride = 1; /* The number of pages between two accesses */
    bool latency_test = false;

    int c;
    while((c = getopt(argc, argv, "dt:s:"))!= -1){
        switch (c)
        {
        case 't':
            n_threads = strtoul(optarg, NULL, 10);
            break;
        case 's':
            stride = strtoul(optarg, NULL, 10);
            break;
        case 'd':
            latency_test = true;
            break;
        case '?':
            std::cerr<<"Unknown option"<<std::endl;
            exit(-1);
        default:
            std::cerr<<"Unknown option "<<c<<std::endl;
            exit(-1);
        }
    }

    test(n_threads, stride, latency_test);
    return 0;
}
