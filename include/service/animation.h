
#pragma once

#include <common/timer.h>

#include <set> 
#include <gtkmm.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH



struct IAnimationJob 
{
	virtual void animate( int dt ) = 0; 

};


struct IAnimation 
{
	virtual void add( IAnimationJob & job ) = 0;  
	virtual void remove( IAnimationJob & job ) = 0; 
};

struct Animation : IAnimation 
{
	// this needs to synchronize with drawing queue stalls, and not just queue timing events 
	// i think it's alright if it just calls it's own function
	// change name to something like animation manager

	typedef Animation this_type; 

	typedef std::set< IAnimationJob *>	jobs_t;
	jobs_t								jobs;

	Animation (  )  
		: timer()
	{  
		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), 60 );
	}	

	Timer				timer;		// Must remove used for animation, not performance, change name animation_timer

	void timer_update( )
	{
//		std::cout << "timer update" << std::endl;
//		render_control.update();

		unsigned elapsed = timer.elapsed();
		timer.restart();

		foreach( IAnimationJob *job, jobs )
		{
			job->animate( elapsed ) ;
		}

		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), 60 );
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


