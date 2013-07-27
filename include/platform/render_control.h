
#pragma once

#include <common/ptr.h>
#include <common/timer.h>
#include <common/update.h>
#include <common/surface.h>

#include <service/renderer.h>


#include <gtkmm.h>


#include <iostream>
#include <cassert>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH




struct RenderControl
{
	// the motivation for this class is to separate the actual renderering performed by the renderer, 
	// with the OS specific blitting and general ui updating events 

	// in reality we want events to be delegated to the renderer, and the
	// renderer would manipulate the size. because the renderer has to know the
	// or else we would just inject width and height, and the renderer would 
	// examine.	 

	typedef RenderControl this_type; 

	RenderControl( Gtk::DrawingArea	& drawing_area, IRenderer &renderer )
		: drawing_area( drawing_area ),
		renderer( renderer ),
		timer()
	{	

		//drawing_area.signal_expose_event() .connect( sigc::mem_fun( *this, &this_type::on_expose_event) );
		drawing_area.signal_draw() .connect( sigc::mem_fun( *this, &this_type::on_expose_event) );
		drawing_area.signal_size_allocate() .connect( sigc::mem_fun( *this, &this_type::on_size_allocate_event));
	}

	Gtk::DrawingArea	& drawing_area; 	
	IRenderer			& renderer;


	Timer				timer;	// used for animation, not performance, change name animation_timer

	void on_size_allocate_event( Gtk::Allocation& allocation)
	{
		renderer.resize( allocation.get_width(), allocation.get_height() ) ; 
	} 

	/*
		- ok, previosly the dispatch, was quite fast. and it generated an event that was pushed on the queue, 
		therefore it didn't slow anything 
		- i think what we need is to create a message to update, so that it ends up on the back of the queue, not the front. 
		We have to run update, in order to know what areas to invalidate. but jjjjjjjjjjjjjjjj 
	*/


	void update()
	{
		unsigned elapsed = timer.elapsed();
		timer.restart();
		UpdateParms	parms;
		parms.dt = elapsed; 

		std::vector< Rect> regions; 

		renderer.update_render( parms, regions ) ; 

		foreach( const Rect & rect, regions)
		{
			invalidate( drawing_area, rect.x, rect.y, rect.w, rect.h ); 
		}
	}

	bool on_expose_event( const Cairo::RefPtr<Cairo::Context>& cr )
//	bool on_expose_event( GdkEventExpose* event)
	{

		std::vector< Rect> regions; 

		std::vector< Cairo::Rectangle > rectangles;
		cr->copy_clip_rectangle_list ( rectangles); 	

		for( int i = 0; i < rectangles.size(); ++i)
		{
			Cairo::Rectangle & rect = rectangles[ i] ; 
			regions.push_back( Rect( rect.x, rect.y, rect.width, rect.height) ); 
		}
#if 0
		// should call them rects, because regions are something a bit different in gtk documentation

//		gdk_cairo_get_clip_rectangle()

		//cairo_rectangle_int_t *rectangles;
		gint n, n_rectangles;
		//- gdk_region_get_rectangles (region, &rectangles, &n_rectangles);
		n_rectangles = cairo_region_num_rectangles ( cr.get_context() /* event->region */);
		//rectangles = g_new (cairo_rectangle_int_t, n_rectangles);
		for (n = 0; n < n_rectangles; n++) 
		{
			cairo_rectangle_int_t	rect;

			//cairo_region_get_rectangle ( event->region, n, &rectangles[n]);
			cairo_region_get_rectangle ( event->region, n, &rect );

			regions.push_back( Rect( rect.x, rect.y, rect.width, rect.height) ); 
		}
#endif
		// get the rendered regions	
		ptr< BitmapSurface> surface = renderer.update_expose( regions ); 


		int region_area = 0; 
		foreach( const Rect & region, regions)
		{
			region_area += region.w * region.h ; 
		}
		double percent = double( region_area) / ( surface->width() * surface->height() ) * 100.; 

		// std::cout << "-------- blitting regions " << regions.size() << " percent " << percent << std::endl;


		// and blit them
		foreach( const Rect & rect, regions )
		{
			blit_buffer( drawing_area, rect, surface ); 
		}
        return false;   // dont presumpt
	}



	/*
		ok, there is a problem at the moment. we do the update() sequence as part of the expose.  	 
		but we want the update(), to trigger the expose.
	*/


	static void blit_buffer( Gtk::DrawingArea & drawing_area, const Rect & rect, ptr< BitmapSurface> & surface_ )
	{
		unsigned char *data = surface_->buf() + (rect.y * surface_->width() * 4) + (rect.x * 4); // beginning of buf 

		cairo_t *cr = gdk_cairo_create( drawing_area.get_window()->gobj() );// drawable );

		cairo_surface_t *surface = cairo_image_surface_create_for_data(
					   data, CAIRO_FORMAT_ARGB32, rect.w, rect.h, surface_->width() * 4 );

//					   data, CAIRO_FORMAT_RGB24, rect.w, rect.h, rect.w * 4 );

		cairo_set_source_surface( cr, surface, rect.x, rect.y );
		cairo_paint( cr );
		cairo_destroy( cr );
		cairo_surface_destroy( surface );


#if 0
		gdk_draw_rgb_32_image (
			drawing_area.get_window()->gobj(),
			drawing_area.get_style()->gobj()->fg_gc[ GTK_STATE_NORMAL],
			rect.x, rect.y,		// target pos
			rect.w, rect.h,		// target pos 
			GDK_RGB_DITHER_NONE,
			(const guchar *)  buf,
			surface->width() * 4
		); 
#endif
	}


	static void invalidate( Gtk::DrawingArea & drawing_area, unsigned x, unsigned y, unsigned w, unsigned h )
	{
		// force redraw 
		Glib::RefPtr<Gdk::Window> win =  drawing_area.get_window();
		if ( win)
		{
			Gdk::Rectangle r( x, y, w,  h  );
			win->invalidate_rect( r, false);
		}
		else { 
		//      std::cout << "no win ?" << "\n";
		}
	}

};

