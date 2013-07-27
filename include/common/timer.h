
#ifndef MY_TIMER_H
#define MY_TIMER_H

// change name to service ? It doesn't hold persistent component state like other services
// it's similar to ILogger

#include <boost/scoped_ptr.hpp>

struct ITimer
{
	// in milliseconds
	virtual void restart() = 0;
	virtual unsigned elapsed() = 0; 
};



struct Timer : ITimer
{
	Timer();
	~Timer();
	void restart();
	unsigned int elapsed();
private:
	boost::scoped_ptr< struct TimerImpl>	self; 
/*
	struct TimerImpl &self;
	Timer( const Timer &) ; 
	Timer & operator = ( const Timer &) ; 
*/
};

#endif

