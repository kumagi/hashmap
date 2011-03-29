#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <sys/time.h>
namespace{
double gettime() __attribute__((always_inline));
double gettime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec*1e-6;
}
}

namespace detail{
class timer{
public:
	timer():now(gettime()){}
	void restart(){now = gettime();}
	double elapsed()const{return gettime() - now;}
private:
	double now;
};
}

#endif
