
#pragma once

#include <service/renderer.h>
#include <service/labels.h>

#include <gtkmm.h>

// resizes the renderer in response to a change in the drawing area

struct RenderResize 
{
	typedef RenderResize this_type; 

	RenderResize( Gtk::DrawingArea & drawing_area, IRenderer &renderer )
		: drawing_area( drawing_area),
		renderer( renderer )
	{	

		drawing_area.signal_size_allocate() .connect( sigc::mem_fun( *this, &this_type::on_size_allocate_event));
	}

	Gtk::DrawingArea	& drawing_area; 
	IRenderer			& renderer;

	void on_size_allocate_event( Gtk::Allocation& allocation)
	{
		int w = allocation.get_width(); 
		int h = allocation.get_height(); 

		renderer.resize( w, h ) ; 
	}
};


