
#pragma once

// sequences the rendering with gtk event loop

#include <common/events.h>
#include <common/bitmap.h>

#include <controller/renderer.h>

#include <gtkmm.h>
#include <iostream>
#include <cassert>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH





struct RenderSequencer
{
	typedef RenderSequencer this_type;

	RenderSequencer( Gtk::DrawingArea & drawing_area, IRenderer &renderer )
		: drawing_area( drawing_area),
		renderer( renderer ),
		immediate_update_pending( false ),
		x( "change", *this, & this_type::on_renderer_change )
	{
		drawing_area.signal_draw().connect( sigc::mem_fun( *this, &this_type::on_expose_event) );


		renderer.register_( x );
	}

	~RenderSequencer( )
	{
		// VERY IMPORTANT
		// why don't we support copy assignment of the closure/ binder ...
		// then we can just do,
		// renderer.register( EventAdapter( "change", *this, &this_type::on_renderer_change ))

		// and the same for removal...

		renderer.unregister( x );
	}


	EventAdapter		x;

	Gtk::DrawingArea	& drawing_area;
	IRenderer			& renderer;
	bool				immediate_update_pending;

	// keep this around, since resizing it is expensive
	Bitmap				surface; 

	void on_renderer_change( const Event & e )
	{
		//std::cout << "got renderer change event" << std::endl;
		if( ! immediate_update_pending )
		{
			immediate_update_pending = true;
			Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::update ), 0 );
		}
	}


	void update()
	{
		std::vector< Rect> regions;

		renderer.render( regions ) ;

		foreach( const Rect & rect, regions)
		{
			invalidate( drawing_area, rect.x, rect.y, rect.w, rect.h );
		}
	}

	bool on_expose_event( const Cairo::RefPtr<Cairo::Context>& cr )
	{
		blit_stuff( cr );

		immediate_update_pending = false;
		return false; // don't presumpt the other handler
	}

	void blit_stuff( const Cairo::RefPtr<Cairo::Context>& cr )
	{

		// Shuffle the data into our own structures
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
		renderer.update_expose( regions, surface );

#if 0
		int region_area = 0;
		foreach( const Rect & region, regions)
		{
			region_area += region.w * region.h ;
		}
		double percent = double( region_area) / ( surface->width() * surface->height() ) * 100.;
		std::cout << "-------- blitting regions " << regions.size() << " percent " << percent << std::endl;
#endif
		// and blit them
		foreach( const Rect & rect, regions )
		{
			blit_buffer( drawing_area, rect, surface );
		}
	}

	/*
		ok, there is a problem at the moment. we do the update() sequence as part of the expose.
		but we want the update(), to trigger the expose.
	*/
	static void blit_buffer( Gtk::DrawingArea & drawing_area, const Rect & rect,  Bitmap & surface_ )
	{
		unsigned char *data = surface_.buf() + (rect.y * surface_.width() * 4) + (rect.x * 4); // beginning of buf

		cairo_t *cr = gdk_cairo_create( drawing_area.get_window()->gobj() );// drawable );

		cairo_surface_t *surface = cairo_image_surface_create_for_data(
			data, CAIRO_FORMAT_ARGB32, rect.w, rect.h, surface_.width() * 4 );

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



