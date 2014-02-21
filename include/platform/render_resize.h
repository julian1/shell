
#pragma once

#include <service/renderer.h>
#include <service/labels.h>

#include <gtkmm.h>



// we should'nt be feeding objects in here, just
// so we can know if the screen has resized.

// The ClearBackground needs to hook an event on the renderer
// and catch the resize event. 

// The event sequence should be linear.
// GTk event -> RenderResize -> Renderer.resize -> ClearBackground event


struct RenderResize 
{
	typedef RenderResize this_type; 

	RenderResize( Gtk::DrawingArea & drawing_area, IRenderer &renderer, IResizable	& resizable )
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


