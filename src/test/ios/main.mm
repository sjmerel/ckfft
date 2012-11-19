#include "../test.h"
#include <pthread.h>

// When I just called test() directly from main(), most of the timing results were consistent, 
// but occasionally there would be outliers that would take as much as 100x longer.  This 
// happened whether I was running CkFft, a 3rd-party FFT, or even a simple for loop; I first
// noticed it after upgrading to iOS 6.
//
// Running in a high-priority FIFO thread seems to mostly fix the problem, at least so the
// timing results are repeatable; I'm guessing the main thread was being interrupted by the OS
// periodically.


void* testThreadProc(void*)
{
    int val = test() ? 0 : 1;
    return (void*) val;
}

#define PTHREAD_VERIFY(x) { int ret = x; assert(ret == 0); }

int startTest()
{
    pthread_attr_t attr;
    PTHREAD_VERIFY( pthread_attr_init(&attr) );

    PTHREAD_VERIFY( pthread_attr_setschedpolicy(&attr, SCHED_FIFO) );

    sched_param param;
    PTHREAD_VERIFY( pthread_attr_getschedparam(&attr, &param) );
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    PTHREAD_VERIFY( pthread_attr_setschedparam(&attr, &param) );

    pthread_t threadId;
    PTHREAD_VERIFY( pthread_create(&threadId, &attr, &testThreadProc, NULL) );

    void* ret;
    pthread_join(threadId, &ret);
    return (int) ret;
}

int main(int argc, char *argv[]) 
{
    @autoreleasepool
    {
       return startTest();
    }
}


