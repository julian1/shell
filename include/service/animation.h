
#pragma once

#include <set> 

#include <platform/async.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH




struct IAnimationJob 
{
	virtual void tick() = 0; 

};


struct IAnimation 
{
	virtual void add( IAnimationJob & job ) = 0;  
	virtual void remove( IAnimationJob & job ) = 0; 
};

/*
	having a general purpose async abstraction that integrates with the app message pump 
	will be very nice.
	for both this timer tick, and managing rendering etc.
*/

struct Animation : IAnimation 
{
	// this needs to synchronize with drawing queue stalls, and not just queue timing events 
	// i think this will synchronize with drawing queue stalls okif it just calls it's own function
	// change name to something like animation manager

	typedef Animation this_type; 

	typedef std::set< IAnimationJob *>	jobs_t;
	jobs_t								jobs;

	int									tick_interval;

	Async								async;

	Animation (  )  
		: tick_interval( 60 )
	{  
	//	Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), tick_interval );

		async.run( boost::bind( & this_type::update, this), tick_interval );
	}	

	void update( )
	{
		//		std::cout << "timer update" << std::endl;

		// It's not possible to put the timer here, because
		// other things force rendering and not just timer ticks

		foreach( IAnimationJob *job, jobs )
		{
			job->tick() ;
		}


		async.run( boost::bind( & this_type::update, this), tick_interval );

	//	Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), tick_interval );
	}

	void add( IAnimationJob & job ) 
	{
		jobs.insert( & job);
	} 

	void remove( IAnimationJob & job ) 
	{
		jobs.erase( & job);
	}

};


