#include<sys/time.h>    //gettimeofday()를 위한 헤더

#define SECOND_TO_MICRO 1000000

struct timeval begin_t, end_t;

void ssu_runtime(struct timeval* begin_t, struct timeval* end_t)
{
    end_t -> tv_sec -= begin_t -> tv_sec;

    if(end_t -> tv_usec < begin_t -> tv_usec)
    {
        end_t -> tv_sec--;
        end_t -> tv_usec += SECOND_TO_MICRO;
    }

    end_t -> tv_usec -= begin_t -> tv_usec;
    printf("Runtime: %ld;%06ld(sec:usec)\n",end_t -> tv_sec, end_t -> tv_usec);
}
