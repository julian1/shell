
#pragma once

#include <common/ptr.h>
#include <common/timer.h>
#include <common/update.h>
#include <common/surface.h>

#include <service/renderer.h>

#include <service/layers.h>
#include <service/labels.h>

#include <gtkmm.h>


#include <iostream>
#include <cassert>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH




struct RenderControl
{

	RenderControl( Gtk::DrawingArea & drawing_area, IRenderer &renderer )
		: drawing_area( drawing_area),
		renderer( renderer ),
		timer()
	{	}

	Gtk::DrawingArea	& drawing_area; 
	IRenderer			& renderer;

	Timer				timer;		// used for animation, not performance, change name animation_timer

	void resize( int w, int h )
	{
		renderer.resize( w, h ); 
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

	void blit_stuff( const Cairo::RefPtr<Cairo::Context>& cr )
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
   //     return false;   // dont presumpt
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


/*
	- It is possible that our aggregate objects should actually become entities and the 
	level of granularity is not right.
	so the load_functions would work the same, 
	So the aggregate could still be composed of any combination of entities as we like.
*/

struct ISignalImmediateUpdate 
{
	virtual void signal_immediate_update() = 0;
};

struct IResizable
{
	virtual void resize( int w, int h ) = 0; 
};


/*
	This RenderManager class can be removed, when we get the 
	
	RenderControl class working better.

	actually not sure. 
*/

struct RenderManager : ISignalImmediateUpdate 
{

	typedef RenderManager this_type; 

	RenderManager( Gtk::DrawingArea	& drawing_area,  
		ptr< ILayers>	&layers,
		ptr< ILabels>	&labels, 
		RenderControl	& render_control ,
		IResizable		& resizable

   ) 	
		: drawing_area( drawing_area ),
		layers( layers),
		labels( labels ),
		render_control( render_control ),
		resizable( resizable),
		immediate_update_pending( false )
	{
		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), 60 );

		drawing_area.signal_draw() .connect( sigc::mem_fun( *this, &this_type::on_expose_event) );

		drawing_area.signal_size_allocate() .connect( sigc::mem_fun( *this, &this_type::on_size_allocate_event));
	}

	Gtk::DrawingArea	& drawing_area; 	
	ptr< ILayers>		& layers;			// should be removed ...
	ptr< ILabels>		& labels; 
	RenderControl		& render_control ; 
	IResizable			& resizable; 

	bool	immediate_update_pending; 

	void signal_immediate_update(  )
	{
		//grid_editor.signal_immediate_update( event );
		//position_editor.signal_immediate_update( event );

		if( ! immediate_update_pending )		
		{							
			immediate_update_pending = true;
			Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::immediate_update ), 0 );
		}
	}

	/*
		this timer update stuff can be factored out later, when 
		the renderer is capable of doing things.
	*/

	void immediate_update()
	{
//		std::cout << "immediate update" << std::endl;
		update();
	}

	void timer_update( )
	{
//		std::cout << "timer update" << std::endl;
		update();
		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), 60 );
	}

	void on_size_allocate_event( Gtk::Allocation& allocation)
	{
		int w = allocation.get_width(); 
		int h = allocation.get_height(); 
		render_control.resize( w, h ) ; 

		// eg. the background object ...
		// wouldn't it be better if the renderer had an event itself ??
		resizable.resize( w, h ) ; 
	}
	

	bool on_expose_event( const Cairo::RefPtr<Cairo::Context>& cr )
	{
		// this is really crappy. this event is already being caught elsewher 


//		std::cout << "expose event" << std::endl;

		// both this class and the render_control hook this event.
		// we do this to clear the flag after the expose event, in order to avoid scheduling another update, this
		// prevents lagging as key and update events are processed, but the image never gets drawn 


		render_control.blit_stuff( cr ); 

		immediate_update_pending = false;
		return false; // don't presumpt the other handler
	}
	/*
		also post expose we always clear the pending flag, but the expose might be 
		in response to a timer event, not immediate. 
	*/

	void update( )
	{

		// what does this even do ???
		layers->layer_update();

		labels->update(); 

		render_control.update();

		// this should be delayed until the expose ? 
		layers->post_layer_update();
	}
};


