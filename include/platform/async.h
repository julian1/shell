
#pragma once

#include <gtkmm.h>

#include <boost/function.hpp>

/*
	having a general purpose async abstraction that integrates with the application message pump 
	is nice.

	mostly this just presents a boost bind interface, rather than a glib one
	but it's useful to have the state.

	for both this timer tick, and managing rendering etc.
*/

struct IAsync
{
	virtual void run( const boost::function< void() > & c, int tick_interval ) = 0;  
};


struct Async : IAsync
{
	//typedef Animation this_type; 

	void run( const boost::function< void() > & c, int tick_interval ) 
	{
		Glib::signal_timeout().connect_once ( c, tick_interval );
	} 
};


