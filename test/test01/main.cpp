
//#define GDK_DISABLE_DEPRECATED

#include <platform/level_control.h> 
#include <platform/valid_control.h> 
#include <platform/modal_control.h> 
#include <platform/render_control.h> 





/*

#include <common/update.h>
#include <common/timer.h>
#include <common/projection.h>		// ???

#include <service/grid_editor.h>
#include <service/position_editor.h>
#include <service/renderer.h>
*/

#include <aggregate/cube_view.h>
#include <aggregate/mapgrid.h>
#include <aggregate/anim.h>
#include <aggregate/projection.h>
#include <aggregate/shapes.h>
#include <aggregate/raster.h>

#include <common/ui_events.h>
#include <common/logger.h>

#include <service/fonts.h>
#include <service/labels.h>
//#include <service/projector.h>
#include <service/layers.h>

#include <service/services.h>


#include <gtkmm.h>


#include <iostream>
#include <cassert>


#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH







#include <common/read_file_into_buffer.h> 


#include <common/cube.h>

#include <data/grib_decode.h>

#include <command/load_gribs_from_gts.h>

// so this is going to be hidden behind our test aggregate interface.
// that will support level and validity operations.



struct Grib1_sections; 
struct Grid ; 
struct SurfaceCriteria; 
struct GtsHeader ; 
	

struct MyGrib
{
	MyGrib( 
		const ptr< Grib1_sections>	& grib_sections, 
		const ptr< Grid >			& grid, 
		const ptr< SurfaceCriteria>	& criteria, 
		const ptr< GtsHeader >		& gts_header 
	)
		: count( 0), 
		grib_sections( grib_sections), 
		grid( grid) , 
		criteria( criteria ), 
		gts_header ( gts_header )  
	{ } 

	ptr< Grib1_sections>	grib_sections; 
	ptr< Grid >				grid; 
	ptr< SurfaceCriteria>	criteria; 
	ptr< GtsHeader >		gts_header ; 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 
private:
	unsigned count;
};



struct MyTestCube : ICube, IDecodeGribsFromGtsCallback 
{
	/*
		A simple test cube.
	*/
	MyTestCube()
		: count( 0), 
		gribs()
	{ 
		// load gribs 
		std::vector< unsigned char>		buf; 

		read_file_into_buffer( "data/sadis2g_soc_grib_dump.bin", buf );

		std::cout << "file size " << buf.size() << std::endl;

	
		decode_gribs_from_gts( buf, *this );  

		std::cout << "gribs.size() " << gribs.size() << std::endl;
	} 

	// IDecodeGribsFromGtsCallback
	void add( 
		const ptr< Grib1_sections>	& grib_sections, 
		const ptr< Grid >			& grid, 
		const ptr< SurfaceCriteria>	& criteria, 
		const ptr< GtsHeader >		& gts_header 
	) 
	{
		gribs.push_back( new MyGrib( grib_sections, grid, criteria, gts_header ) ); 
	} 
	

//	ptr< MyTestCube>  cube(  new MyTestCube ); Closure	c( cube );



	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	// requires everything, because must be able to abstract for multiple views 

	std::vector< ptr< SurfaceCriteria> > get_available_criterias() 
	{
		std::vector< ptr< SurfaceCriteria> > criterias; 

		foreach( const ptr< MyGrib> & grib, gribs )
		{
			criterias.push_back( grib->criteria ) ; 
		}
		return criterias;
	}

	ptr< Grid > get_grid( const ptr< SurfaceCriteria> & criteria ) 
	{
		foreach( const ptr< MyGrib> & grib, gribs )
		{
			if( *grib->criteria == *criteria )
			{
				return grib->grid; 
			}  
		}

		std::cout << "criteria->valid " << *criteria->valid << std::endl;

		assert( 0 );
	}

private:
	unsigned count;
	std::vector< ptr< MyGrib > >	gribs;
};






struct MyDelegatedCube : ICube
{
	// just an example of delegating to the inner cube

	MyDelegatedCube( const ptr< ICube> & inner )
		: count( 0), 
		inner( inner)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	std::vector< ptr< SurfaceCriteria> > get_available_criterias() 
	{
		return inner->get_available_criterias() ; 
	}
	ptr< Grid > get_grid( const ptr< SurfaceCriteria> & criteria ) 
	{
		return inner->get_grid( criteria ); 
	}
private:
	unsigned count;
	ptr< ICube> inner; 
};










static void test1( Services & services )
{
	/*
		- this test should be a command that is run as a class aft. 

		- and it should be run after the gui is loaded 
	*/

	// dynamic objects
	ptr< IProjectionAggregateRoot>	projection_aggregate_root; 

	ptr< IShapesAggregateRoot>		shapes_aggregate_root; 
//	ptr< IGridAggregateRoot>		grid_aggregate_root; 
	ptr< IRasterAggregateRoot>	raster_aggregate_root; 
//	ptr< IAnimAggregateRoot>	anim_aggregate_root; 
	ptr< IMapGridAggregateRoot>	mapgrid_aggregate_root; 

	ptr< IProjectionAggregateRoot>	projection_aggregate_root_2; 



	raster_aggregate_root = create_test_raster_aggregate_root();
	mapgrid_aggregate_root = create_test_mapgrid_aggregate_root();
	//anim_aggregate_root = create_test_anim_aggregate_root();	



	ptr< ICube> cube1 = new MyTestCube ; //make_test_cube(); 

	ptr< ICube> cube = new MyDelegatedCube ( cube1 ); 

//	grid_aggregate_root  = make_test_grid_from_grib();

#if 0
	grid_aggregate_root = create_grid_aggregate_root(); 
	grid_aggregate_root->set_grid( make_test_grid( 100, 100) ); 
	agg::trans_affine	geo_ref; 
	geo_ref *= agg::trans_affine_scaling( .7 );
	grid_aggregate_root->set_geo_reference( geo_ref );
#endif


	shapes_aggregate_root = create_shapes_aggregate_root( );
	load_test_shapes( "data/world_adm0.shp",  shapes_aggregate_root ); 



	projection_aggregate_root = create_projection_aggregate_root(); 
	// load_test_ortho_proj( projection_aggregate_root );
	add_projection_aggregate_root( services, projection_aggregate_root ) ; 


	add_mapgrid_aggregate_root( services, mapgrid_aggregate_root,  projection_aggregate_root ); 

	add_raster_aggregate_root( services, raster_aggregate_root, projection_aggregate_root );

	// adding the mapgrid to two projections fucks it up ...
	load_shapes_layer ( services, shapes_aggregate_root, projection_aggregate_root  ); 


	create_cube_view( services, cube, projection_aggregate_root ); 

	
	add_test_anim_layer( services ); 

	//add_anim_aggregate_root( services,  anim_aggregate_root ); 



	projection_aggregate_root_2 = create_projection_aggregate_root_2(); 

	add_projection_aggregate_root( services, projection_aggregate_root_2 ) ; 

	load_shapes_layer( services, shapes_aggregate_root, projection_aggregate_root_2  ); 

	add_mapgrid_aggregate_root( services, mapgrid_aggregate_root,  projection_aggregate_root_2 ); 

	create_cube_view( services, cube , projection_aggregate_root_2 ); 

}
;


/*
	- It is possible that our aggregate objects should actually become entities and the 
	level of granularity is not right.

	so the load_functions would work the same, 

	So the aggregate could still be composed of any combination of entities as we like.

*/



struct Application
{

	typedef Application this_type; 

	Application()	
	: 
		window(),
		vbox(),
		hbox(),
		drawing_area(),
		logger( std::cout ),

		// if we gave these things full references, then we wouldn't
		// need to maintain instances in the Application class scope ?? 

		renderer( ),
		//projector( create_projector_service() ),
		grid_editor( ), 
		position_editor(),
		fonts( create_fonts_service( /* font dir */) ),
		labels( create_labels_service() ),
		layers( create_layers_service() ),
		level_controller( create_level_controller_service() ), 
		valid_controller( create_valid_controller_service() ), 
		//gribs_service( create_gribs_service( "data/sadis2g_soc_grib_dump.bin" ) ),

		services( 
			*layers,
//			*projector, 
			*labels,
			renderer, 

			grid_editor, 
			position_editor,
			*fonts,
		//	*gribs_service,
			*level_controller,
			*valid_controller
		),

		modal_control( services, hbox ),

		gui_valid_controller( hbox, valid_controller ), 
		gui_level_controller( hbox, level_controller ), 

		immediate_update_pending( true ),

		render_control( drawing_area, renderer )

		/*
			- Very important. Rather than passing the IProjection we should pass the projection aggregate root.  
			aggregates are permitted to trace other aggregate's references.
			- We could factor out the services reference, by making the adaptor pass the services reference
		*/
	{ 

		// complex objects are going to be a graph ... (or graph like) 

		/////////////////

		window.add( vbox );
		
		vbox.pack_start( hbox, Gtk::PACK_SHRINK  );

		vbox.pack_start( drawing_area );

		#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
			std::assert( 0);
		#endif 

        // Disable double buffering, otherwise gtk tries to update widget
    // contents from its internal offscreen buffer at the end of expose event
        // this does inhibit gtk from drawing
        drawing_area.set_double_buffered( false);



		//drawing_area.set_size_request( 1000, 900);
		drawing_area.set_size_request( 1000, 700);

		drawing_area.set_events( 
                Gdk::EXPOSURE_MASK 
                | Gdk::POINTER_MOTION_MASK 
                | Gdk::BUTTON_PRESS_MASK
                | Gdk::BUTTON_RELEASE_MASK 
                | Gdk::KEY_PRESS_MASK 
                | Gdk::KEY_RELEASE_MASK
        ); 

	
		window.set_title( "my app" );

		typedef Application this_type; 

		drawing_area.signal_motion_notify_event() .connect( sigc::mem_fun( *this, &this_type::on_motion_notify_event));
		drawing_area.signal_button_press_event() .connect( sigc::mem_fun( *this, &this_type::on_button_press_event));
		drawing_area.signal_button_release_event() .connect( sigc::mem_fun( *this, &this_type::on_button_release_event));
		window.signal_key_press_event() .connect( sigc::mem_fun( *this, &this_type::on_key_press_event));
		window.signal_key_release_event() .connect( sigc::mem_fun( *this, &this_type::on_key_release_event));

		window.show_all_children();

		/*
			extremely important.
		
			- Should the aggregate be able to be loaded more than once ????. Eg. with a separate projection. 
			- The load function would then take the projection, rather than the aggregate.
			- A bit like the way we factored out the services from the aggregate.

			If the interaction code is in the adaptors, then yes ????.

			Line color and style - need to be added, but projection clipping etc. 

			Even if we can't do this. The Aggregate should NOT take a projection reference. Instead the 
			load should take it, and associate it.

			- if a localised projection wants different line/color then it can be placed

			- Eg. View specific stuff is kept in the view adaptors, so the projection should be moved into it.

			To make it easier, the class that does the proj probably needs to handle other things as well, . 
			eg. to maintain the clip_path and perform the styling.    Eg to move the clip_path outside the aggregate.  
		*/

	/*	
		important - we can instantiate all this stuff only needing the services references, 
		meaning it can be relocated anywhere.  
	*/	

	/*
		ok, so we can't manage to show two mapgrid's at the same time.

		Simply because the pointer values of the aggregate are the same ...	
	*/

		// schedule initial deadline timeout 
		//Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & Application::on_timeout), 0 );

		immediate_update_pending = false;
//		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::immediate_update ), 0 );
		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & this_type::timer_update), 60 );


		//drawing_area.signal_expose_event() .connect( sigc::mem_fun( *this, &this_type::on_expose_event) );
		drawing_area.signal_draw() .connect( sigc::mem_fun( *this, &this_type::on_expose_event) );

		test1( services );
	} 


	Gtk::Window			window;
	Gtk::VBox			vbox;
	Gtk::HBox			hbox;		// horizontal menu strip

	Gtk::DrawingArea	drawing_area; 	

	Logger				logger; 

	// Jobs				jobs;
	//	BitmapSurface				surface;
	//	StyledContours		styled_contours;

	//Contourer			contourer;

	// if these are all services, they should be named as such ???
	Renderer			renderer;
	//ptr< IProjector>	projector;
	GridEditor			grid_editor;
	PositionEditor		position_editor;


	ptr< IFonts>		fonts; 
	ptr< ILabels>		labels; 
	ptr< ILayers>		layers; 

	ptr< ILevelController> level_controller;
	ptr< IValidController> valid_controller;

//	ptr< IGribsService>	gribs_service;  

	//ShapesProvider		shapes_provider;
	

	Services			services; 
	ModalControl		modal_control;

	GUIValidController	gui_valid_controller;
	GUILevelController	gui_level_controller;

	bool				immediate_update_pending; 

	RenderControl		render_control; 



	bool on_motion_notify_event( GdkEventMotion *event )
    {
//		std::cout << "mouse move event" << std::endl;
		grid_editor.mouse_move( event->x, event->y ); 
		position_editor.mouse_move( event->x, event->y ); 
		signal_immediate_update(  ); 
		return false;
	}
	bool on_button_press_event( GdkEventButton* event)
	{
		grid_editor.button_press( event->x, event->y ); 
		position_editor.button_press( event->x, event->y ); 
		signal_immediate_update( ); 
		return false;   
	}
	bool on_button_release_event( GdkEventButton* event)
	{
		grid_editor.button_release( event->x, event->y ); 
		position_editor.button_release( event->x, event->y ); 
		signal_immediate_update( );
		return false; 
	}
	static int translate_code( unsigned code )
	{
		int ret = 0; 
		switch( code)
		{
			// case GDK_Return: case GDK_Escape: case GDK_F1: etc
			case GDK_KEY_Shift_L  :  ret = IMyEvents ::shift_key; break; 
			case GDK_KEY_Control_L: ret = IMyEvents ::ctrl_key; break;
			default: ret = code; break;
		};
		return ret; 
	}
	// now we want keyboard events. to enable pan
	bool on_key_press_event( GdkEventKey* event )
    {
		/*
		std::cout << "keypress event " << event->hardware_keycode << " " << char( event->hardware_keycode) << std::endl;
		std::cout << "keypress event " << event->keyval << " " << char( event->keyval ) << std::endl;
		*/

		grid_editor.key_press( translate_code( event->keyval ) ); 
		position_editor.key_press( translate_code( event->keyval ) ); 
		signal_immediate_update( ); 
		return false;
	}
	bool on_key_release_event(GdkEventKey* event )
	{
		grid_editor.key_release( translate_code( event->keyval ) ); 
		position_editor.key_release( translate_code( event->keyval ) ); 
		signal_immediate_update( ); 
		return false;
	}


	void signal_immediate_update(  )
	{
		//grid_editor.signal_immediate_update( event );
		//position_editor.signal_immediate_update( event );

		if( ! immediate_update_pending )		
		{							
			immediate_update_pending = true;
			Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & Application::immediate_update ), 0 );
		}
	}

	void immediate_update( )
	{
//		std::cout << "immediate update" << std::endl;
		update();
	}

	void timer_update( )
	{
//		std::cout << "timer update" << std::endl;
		update();
		Glib::signal_timeout().connect_once ( sigc::mem_fun( *this, & Application::timer_update), 60 );
	}

	bool on_expose_event( const Cairo::RefPtr<Cairo::Context>& cr )
	{

//		std::cout << "expose event" << std::endl;

		// both this class and the render_control hook this event.
		// we do this to clear the flag after the expose event, in order to avoid scheduling another update, this
		// prevents lagging as key and update events are processed, but the image never gets drawn 
		immediate_update_pending = false;
		return false; // don't presumpt the other handler
	}
	/*
		also post expose we always clear the pending flag, but the expose might be 
		in response to a timer event, not immediate. 
	*/
	/*
		when we do the update, no more events should be able to be processed until we get the expose. 
		it is possible however, that events could be being exposed, which means the items in the
		renderer could have changed.  been made active etc.
	*/
	void update( )
	{

		// models->update(); / 

		layers->layer_update();

		// perhaps remove . 
		//projector->update();

		labels->update(); 

		render_control.update();

		// this should be delayed until the expose ? 
		layers->post_layer_update();

	}




}; 




int main(int argc, char *argv[])
{


  Gtk::Main kit( argc, argv);
  
  Application app; //std::cout, argc, argv);

  Gtk::Main::run( app.window );
  return 0;

}





#if 0
	static void blit_buffer( Gtk::DrawingArea & drawing_area, unsigned width, unsigned height, const unsigned char * buf )
	{
		// this function should be changed to actually take a ptr< surface> and pos, not a raw buf.
		// pixbuf is client side/ pixmap is server side. 

		// gtk primatives are probably pretty fast since gdk will manage its own XImage on server ?
		// in any case is a lot better than pixmap
		gdk_draw_rgb_32_image (
			drawing_area.get_window()->gobj(),
			drawing_area.get_style()->gobj()->fg_gc[ GTK_STATE_NORMAL],
			0, 0,
//					buffer->width(), buffer->height(),
			width, height, 
			GDK_RGB_DITHER_NONE,
			//(const guchar *) buffer->xptr(),
			(const guchar *) buf,
			//buffer->width() * 4
			width * 4
		);      
	}



	static void invalidate( Gtk::DrawingArea & drawing_area )
	{
		// force redraw 
		Glib::RefPtr<Gdk::Window> win = drawing_area.get_window();
		if ( win)
		{
			gdk_window_invalidate_rect( win->gobj(), NULL, FALSE); 
		}
		else { 
			//      std::cout << "no win ?" << "\n";
		}
	}

#endif


#if 0
// there are far to many buffers ...
//		std::cout << "blit buffer " << rect.x << " " << rect.y << "  " << rect.w << " " << rect.h << std::endl;

		// ok, we are actually going to have to copy the buffer here, which is a mess
		BitmapSurface		dst( rect.w, rect.h ); 

		copy_region( *surface, rect.x, rect.y, rect.w, rect.h, dst, 0, 0 ); 
	
		// OK HANG ON WE DON'T NEED THE COPY HERE .......
		// this function takes a stride argument

		gdk_draw_rgb_32_image (
			drawing_area.get_window()->gobj(),
			drawing_area.get_style()->gobj()->fg_gc[ GTK_STATE_NORMAL],
			rect.x, rect.y,
			rect.w, rect.h, 
			GDK_RGB_DITHER_NONE,
			//(const guchar *) buffer->xptr(),
			(const guchar *) dst.buf(),
			//buffer->width() * 4
			dst.width() * 4
		);      
#endif

#if 0
		agg::rendering_buffer   rbuf( 
			src.buf() + (y * src.width() * 4) + (x * 4),			// beginning of buf 
			w, h, 
			(-flip_y) * src.width() * 4 ); 
#endif
	/*
		static void update( IUpdate *n, std::map< IUpdate *, std::vector< IUpdate *> > & deps )
		{
			// OK, the deps are all recorded already ... 
			// but we cannot clear the dirty flags - because an input may be used for more than one dep .
			assert( n);
			// find deps for this node 
			const std::vector< IUpdate *>	& v = deps[ n ] ; 
			foreach( IUpdate *d, v)
			{
				// update them
				update( d, deps );
			}
			// now update this node	
			n->update();
		} 
	*/


