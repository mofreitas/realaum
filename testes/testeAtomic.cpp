#if 1
#include <atomic>
std::atomic<bool> stop_(false);
#else
volatile bool stop_ = false;
#endif

int main(int argc, char **argv)
{
    long n = 1000000000;
    while (!stop_) {
        if (--n < 0)
            stop_ = true;
    }

    return 0;
}
