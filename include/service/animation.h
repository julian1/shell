/*
	We create this as a way to do async stuff that will help to synchronize
	animation stuff.
*/

#pragma once


#include <platform/async.h>

#include <set> 
#include <boost/bind.hpp>

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


struct Animation : IAnimation 
{

	typedef Animation this_type; 

	typedef std::set< IAnimationJob *>	jobs_t;
	jobs_t								jobs;

	int									tick_interval;

	IAsync								& async;

	Animation ( IAsync & async  )  
		: async( async ),
		tick_interval( 60 )
	{  

		async.run( boost::bind( & this_type::update, this), tick_interval /* 0 ? */ );
	}	

	void update( )
	{
		foreach( IAnimationJob *job, jobs )
		{
			job->tick() ;
		}

		async.run( boost::bind( & this_type::update, this), tick_interval );
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


