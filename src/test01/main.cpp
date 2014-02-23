
//#define GDK_DISABLE_DEPRECATED

#include <platform/level_control.h> 
#include <platform/valid_control.h> 
#include <platform/modal_control.h> 
#include <platform/render_sequencer.h> 
#include <platform/render_resize.h> 
#include <platform/mouse.h> 
#include <platform/keyboard.h> 


#include <aggregate/cube_view.h>
#include <aggregate/mapgrid.h>
#include <aggregate/anim.h>
#include <aggregate/projection.h>
#include <aggregate/shapes.h>
#include <aggregate/raster.h>

#include <common/logger.h>
#include <common/timer.h>

#include <controller/fonts.h>
#include <controller/labels.h>

#include <platform/async.h>
#include <controller/animation.h>
#include <controller/services.h>


#include <controller/grid.h>	 // grid renderer


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

#if 0

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


#endif



/*

	The renderer, when it gets a resize will automatically redraw everything
	so this thing only needs to advertise its bounds, and doesn't need eventsc.
*/
struct ClearBackground : IRenderJob
{

	ClearBackground( IRenderer &renderer) 
		: renderer( renderer )
	{ } 

	IRenderer		&renderer; 
	Listeners		listeners;

	void register_( INotify * l) 
	{
		// we don't generate listeners here, so there's nothing really to do.
		// but if this object goes out of scope, we should release listeners
		// so should use the listeners class

		listeners.register_( l);
	} 

	void unregister( INotify * l)
	{
		listeners.unregister( l);
	}


	void pre_render( RenderParams & params ) 
	{ } 

	void render ( RenderParams & params  ) 
	{
		// we don't need to know the size in the clear. only in the get_founds call..
		std::cout << "$$$$$$$$$$$$ clearing background" << std::endl;

		params.surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );
	} 

	// change name get_render_z_order(), to distinguish from label_z_order ? 
	int get_z_order() const 
	{
		return -1;
	} 

	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		*x1 = 0; 
		*y1 = 0; 
		renderer.getsize( x2, y2 );	
	}  
};




int main(int argc, char *argv[])
{
	// Assemble the graph

	Gtk::Main kit( argc, argv);
  
	// complex objects are going to be a graph ... (or graph like) 

	Gtk::Window			window;

	Gtk::VBox			vbox; 
	Gtk::HBox			hbox;		// horizontal menu strip


	Gtk::DrawingArea	drawing_area; 	


	Logger			logger( std::cout );


	Renderer			renderer;

	RenderSequencer		render_control( drawing_area, renderer );

	Async				async;
	
	Animation			animation( async); 

	GridEditor			grid_editor;
	PositionEditor		position_editor;


	ptr< IFonts>		fonts( create_fonts_service( /* font dir */) );

	ptr< ILabels>		labels( create_labels_service() );



	ptr< ILevelController>	level_controller( create_level_controller_service() ); 
	ptr< IValidController> valid_controller( create_valid_controller_service() ); 
	//gribs_service( create_gribs_service( "data/sadis2g_soc_grib_dump.bin" ) ),

	Services services( 
		*labels,
		renderer, 
		animation,
		grid_editor, 
		position_editor,
		*fonts,
		*level_controller,
		*valid_controller
	);


	ModalControl		modal_control( services, hbox );

	GUIValidController	gui_valid_controller( hbox, valid_controller ); 

	GUILevelController	gui_level_controller( hbox, level_controller ); 

	ClearBackground		clear_background( renderer);

	// labels needs to be given the renderer so that it can get a pre_render step


	RenderResize render_resize( drawing_area, renderer );


	//TimingManager	timing_manager( render_control ); 

	Keyboard keyboard_manager( window, grid_editor, position_editor/*c$, render_control */ );

	MouseManager	mouse_manager( drawing_area, grid_editor, position_editor ); 

	//GridRenderer	grid_renderer( renderer );

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

	window.show_all_children();

#if 0

	/// ********************************************************
	// dynamic objects
	ptr< IProjectionAggregateRoot>	projection_aggregate_root; 
//	ptr< IShapesAggregateRoot>		shapes_aggregate_root; 
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

//	shapes_aggregate_root = create_shapes_aggregate_root( );
//	load_test_shapes( "data/world_adm0.shp",  shapes_aggregate_root ); 

	projection_aggregate_root = create_projection_aggregate_root(); 
	// load_test_ortho_proj( projection_aggregate_root );
	add_projection_aggregate_root( services, projection_aggregate_root ) ; 


	add_mapgrid_aggregate_root( services, mapgrid_aggregate_root,  projection_aggregate_root ); 
	add_raster_aggregate_root( services, raster_aggregate_root, projection_aggregate_root );

	// adding the mapgrid to two projections fucks it up ...
//	load_shapes_layer ( services, shapes_aggregate_root, projection_aggregate_root  ); 

	create_cube_view( services, cube, projection_aggregate_root ); 
	add_test_anim_layer( services ); 
	//add_anim_aggregate_root( services,  anim_aggregate_root ); 
	projection_aggregate_root_2 = create_projection_aggregate_root_2(); 
	add_projection_aggregate_root( services, projection_aggregate_root_2 ) ; 
//	load_shapes_layer( services, shapes_aggregate_root, projection_aggregate_root_2  ); 
	add_mapgrid_aggregate_root( services, mapgrid_aggregate_root,  projection_aggregate_root_2 ); 
	create_cube_view( services, cube , projection_aggregate_root_2 ); 

#endif

	renderer.add( clear_background );

	// add a test object	
	add_animation_object( services ); 

	/// ********************************************************

  Gtk::Main::run( window );
  return 0;

}





