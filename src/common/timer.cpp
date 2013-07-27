
// timer is not a service

#include <common/timer.h>

#ifdef _WINDOWS

#include <windows.h>

struct TimerImpl
{
	// useful discussion on Timers 
	// http://stackoverflow.com/questions/88/is-gettimeofday-guaranteed-to-be-of-microsecond-resolution
	// gives examples for linux, 
	// this works pretty well in wine if set sleep( 1234) we get 1235ms

	__int64 freq, start_;//, end;
/*
	Timer() 
	{
		// determine freq
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	}
*/

};

Timer::Timer()
	: self( * new TimerImpl )
{ 
	QueryPerformanceFrequency((LARGE_INTEGER*)&self.freq);
	restart();
} 

Timer::~Timer()
{
	delete & self;
}

void Timer::restart()
{
	// start
	QueryPerformanceCounter((LARGE_INTEGER*)&self.start_);
}

unsigned int Timer::elapsed()
{
	// code to measure
//	Sleep( 1234);

	__int64 diff, end; 

	// end
	QueryPerformanceCounter((LARGE_INTEGER*)& end);
	diff = ((end - self.start_) * 1000) / self.freq;

	unsigned int milliseconds = (unsigned int)(diff & 0xffffffff);
	return milliseconds; 
//	std::cout << "It took "  << milliseconds << "%u ms\n";
}

#else	// linux



#include <sys/time.h>
#include <cstddef> // NULL	


struct TimerImpl
{
	/* if have serveral are there potential link issues here ? */

	struct timeval start_;
};

Timer::Timer()
	: self( new TimerImpl )
{ 
	restart();
} 

Timer::~Timer()
{ }

void Timer::restart()
{
	gettimeofday( &self->start_, NULL);
}

// int - is not going to be precise enough 

unsigned int Timer::elapsed()
{
	struct timeval end;
	gettimeofday (&end, NULL);
	long seconds  = end.tv_sec  - self->start_.tv_sec;
	long useconds = end.tv_usec - self->start_.tv_usec;
	long mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	return mtime;
}


#endif


