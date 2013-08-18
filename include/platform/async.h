
#pragma once

#include <gtkmm.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>


struct Async
{
	//typedef Animation this_type; 

	struct X
	{
		template< class C> 
		X( const C & c ) 
			: f( c)
		{ } 

		boost::function< void()>	f;	

		void operator () ()
		{
			f();
		}
	};

	template< class C> 
	void run( const C & c, int tick_interval ) 
	{
//		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), tick_interval );

		X	x( c);

		Glib::signal_timeout().connect_once ( x, tick_interval );
	}  

};


