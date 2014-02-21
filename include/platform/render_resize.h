
#include <common/surface.h>

#include <service/renderer.h>
#include <service/labels.h>

#include <gtkmm.h>

/*
//#include <iostream>
#include <cassert>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

*/

struct RenderSizeControl 
{
	typedef RenderSizeControl this_type; 

	RenderSizeControl( Gtk::DrawingArea & drawing_area, IRenderer &renderer, IResizable	& resizable )
		: drawing_area( drawing_area),
		renderer( renderer ),
		resizable( resizable)
	{	

		drawing_area.signal_size_allocate() .connect( sigc::mem_fun( *this, &this_type::on_size_allocate_event));
	}

	Gtk::DrawingArea	& drawing_area; 
	IRenderer			& renderer;
	IResizable			& resizable; 

	void on_size_allocate_event( Gtk::Allocation& allocation)
	{
		int w = allocation.get_width(); 
		int h = allocation.get_height(); 

		// eg. the background object ...
		// wouldn't it be better if the renderer had an event itself ??
		resizable.resize( w, h ) ; 

		renderer.resize( w, h ); 
	}
};


