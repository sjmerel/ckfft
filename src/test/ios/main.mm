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


void* doTest(void*)
{
    /*
    Stats stats;
    int outliers = 0;
    for (int i = 0; i < 1000; ++i)
    {
        uint64_t start = mach_absolute_time();
        for (int j = 0; j < 100000; ++j);
        uint64_t stop = mach_absolute_time();
        int t = (int) (stop-start);
        stats.sample(t);
        if (stats.getCount() > 2 && t > stats.getMean() * 2.0f)
        {
            ++outliers;
        }
        printf("%llu\n", stop-start);
    }
    printf("%d outliers; mean %f, max %f\n", outliers, stats.getMean(), stats.getMax());
    */
    test();

    return NULL;
}

#define PTHREAD_VERIFY(x) { int ret = x; assert(ret == 0); }

void startTest()
{
    pthread_attr_t attr;
    PTHREAD_VERIFY( pthread_attr_init(&attr) );

    PTHREAD_VERIFY( pthread_attr_setschedpolicy(&attr, SCHED_FIFO) );

    sched_param param;
    PTHREAD_VERIFY( pthread_attr_getschedparam(&attr, &param) );
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    PTHREAD_VERIFY( pthread_attr_setschedparam(&attr, &param) );

    pthread_t threadId;
    PTHREAD_VERIFY( pthread_create(&threadId, &attr, &doTest, NULL) );

    void* ret;
    pthread_join(threadId, &ret);
}

int main(int argc, char *argv[]) 
{
    @autoreleasepool
    {
       startTest();
    }

    return 0;
}


