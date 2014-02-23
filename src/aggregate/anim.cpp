
#include <common/events.h>
#include <aggregate/anim.h>


#include <controller/services.h>
#include <controller/position.h>
#include <controller/renderer.h>
#include <controller/animation.h>

#include <common/path_seg_adaptor.h>
#include <common/path_reader.h>
#include <common/bitmap.h>


#include <common/point_in_poly.h>

#include <agg_bounding_rect.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
#include <agg_conv_dash.h>

#include <agg_rounded_rect.h>


#include <cmath>

namespace {

/*
	This is a clean example of how we aggregate interfaces for complicated functionality
	but then delegate to separate objects responsible for tasks. 

*/

struct Render
{
	// A helper class for simple dashed line animation renderer
	bool				& is_active; 
	agg::path_storage	& path; 
	double				offset; 

public:
	Render( bool & is_active, agg::path_storage	& path )  
		: is_active ( is_active),
		path( path),
		offset( 0)
	{ } 

	void render ( RenderParams & params ) 
	{
		// std::cout << "render test job " << parms.dt << std::endl;
		// path_reader	reader( root->get_path() );

		offset += params.dt * 0.020;

		if( offset > 20)  
			offset -= 20;

		typedef agg::conv_dash< agg::path_storage>              dash_type;
		typedef agg::conv_stroke< dash_type>                    stroke_type;

		// very important dash is different from stroking ?? 
		// should they be distinguished for purpose of an animation ?
		dash_type               d( path ); 
		d.add_dash( 10, 10);
		d.dash_start( offset); 

		stroke_type             stroke( d); 

		if( is_active  )
			stroke.width( 4);
		else	
			stroke.width( 2);

		// agg::conv_stroke< path_reader>	stroke( reader );
		// stroke.width( 1); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		agg::render_scanlines_aa_solid( ras, sl, params.surface.rbase(), agg::rgba8( 0xff, 0, 0 ) );
	}

	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		bounding_rect_single( path, 0, x1, y1, x2, y2);	

		*x1 -= 2; 
		*y1 -= 2; 
		*x2 += 2; 
		*y2 += 2; 
	}

};


struct Editor
{
	bool				& is_active; 
	agg::path_storage	& path ; 

	Editor( bool & is_active, agg::path_storage & path ) 
		: is_active( is_active),
		path( path)
	{ } 

	double hit_test( unsigned x, unsigned y ) 
	{
		path_seg_adaptor< agg::path_storage> segs( path  );
		double dist = 1234;
		int cmd;
		double x1, y1, x2, y2; 
		segs.rewind();
		while( !agg::is_stop( cmd = segs.segment( &x1, &y1, &x2, &y2)) )
		{
			dist = std::min( dist, pointToLineDistance( x1, y1,  x2, y2,  x, y ) ); 
		} 
		return dist;
	}

	void set_active( bool active_ ) 
	{
		is_active = active_;	
//		notify();
	}

	void set_position_active( bool ) 
	{ } 	

	void move( int x1, int y1, int x2, int y2 ) 
	{
		agg::trans_affine	affine;
		affine *= agg::trans_affine_translation( x2 - x1, y2 - y1 );

		agg::path_storage tmp = path ; 
		tmp.transform( affine );	
		path = tmp; 


//		notify();
	}

	void finish_edit() 
	{
		// make the model fire an event 
	}

	static double pointToLineDistance( double Ax, double Ay,  double Bx, double By, double Px, double Py )
	{
		// belive this is not correct. hittesting extends beyond the line.

		// http://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
		double normalLength = std::sqrt((Bx - Ax) * (Bx - Ax) + (By - Ay) * (By - Ay));
		return std::abs((Px - Ax) * (By - Ay) - (Py - Ay) * (Bx - Ax)) / normalLength;
	}
};






struct ControlPoint : IPositionEditorJob, IRenderJob//, IKey
{
	ControlPoint( Services & services )
		: services( services ),
		path(),
		active( false)
	{ 
		double r = 15; 
		double x = 125, y = 125; 
		//			callback.get_position( this, &x, &y );			// this get_position is getting called multiple times.

		path.free_all();

		agg::ellipse e( x, y, r, r, (int) 20);
		path.concat_path( e );	

		show( true );
	} 

	Services					& services ;
	Listeners					listeners;

	agg::path_storage			path;	
	bool						active		;

	void register_( INotify * l) 
	{
		listeners.register_( l);
	} 

	void unregister( INotify * l)
	{
		listeners.unregister( l);
	}

	void notify( const char *msg )
	{
		listeners.notify( *this, msg ); 
	}


	void show( bool u )
	{
		if( u) {		
			services.renderer.add( *this  );
			services.position_editor.add( *this );
		}
		else {
			services.renderer.remove( *this  );
			services.position_editor.remove( *this );
		}

	}

	// IRenderJob
	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		bounding_rect_single( path , 0, x1, y1, x2, y2);	
	}	

	void pre_render( RenderParams & params) 
	{  }

	void render( RenderParams & params ) 
	{
		//std::cout << "&&& render ControlPoint" << std::endl;

		// remember the path is used for both, render and hittesting. 
		// we calculate at render time.

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( path );

		agg::rgba       color( .3, 0, 1, 1);
		agg::render_scanlines_aa_solid( ras, sl, params.surface.rbase(), color );
 	}

	// IRenderJob
	int get_z_order() const
	{
		// always in top ? 
		return 101;
	}

	// IPositionEditorJob
	void move( int x1, int y1, int x2, int y2 )  
	{
		agg::trans_affine	affine;
		affine *= agg::trans_affine_translation( x2 - x1, y2 - y1 );

		agg::path_storage tmp = path ; 
		tmp.transform( affine );	
		path = tmp; 

		// need to notify renderer
		notify( "change");
	}

	void set_active( bool active_ ) 
	{ 
		//callback.set_control_point_active( active_ ) ;
	}

	void set_position_active( bool active_ ) 
	{ 
		// must change the name of this ( position_controller_active() )
		// WHY IS THIS 
		active = active_;
	}  

	void finish_edit() { }

	double hit_test( unsigned x, unsigned y ) 
	{
		if( point_in_poly( x, y, path))
		{
			// eg. dist == 0	
			return 0;
		} 
		return 1234;
	}
};










struct MyObject : IPositionEditorJob, IRenderJob, IAnimationJob 
{
	// Assemble the object graph

	Services			& services ;

	bool				is_active;
	agg::path_storage	path; 

	// Helpers
	Render				renderer;
	Editor				editor;

	ControlPoint		control_point;

	Listeners			listeners;
	

	MyObject( Services & services )
		: services( services),
		is_active( false ),
		renderer( is_active, path), 
		editor( is_active, path),
		control_point( services )
	{ 
		agg::rounded_rect   r( 20, 20, 100, 100, 10);

		path.free_all();
		path.concat_path( r);

		show( true);
	}  

	~MyObject()
	{

		show( false);
	}

	void register_( INotify * l) 
	{
		listeners.register_( l);
	} 

	void unregister( INotify * l)
	{
		listeners.unregister( l);
	}
	
	void notify( const char *msg )
	{
		listeners.notify( *this, msg ); 
	}

	// the this address is different to the IRenderjob address 
	// even though they are the same object. 

	// If the event is going to be common. Think we have to have a common
	// event thing for jobs. then we can downcast without dynamic_cast.   


	// Not sure the issue is the notify. 

	// is there a way to embed a cast in the template,,, 
	// to indicate what actual object we want ????

	// eg. we have all these things that are typed, and listening.. 

	void show( bool u )
	{
		if( u) {		

			std::cout << "show " << this << std::endl;
			std::cout << "show as render interface " << (IRenderJob *)this << std::endl;

			services.renderer.add( *this  );
			services.position_editor.add( *this );
			services.animation.add( *this );
		}
		else {
			services.renderer.remove( *this  );
			services.position_editor.remove( *this );
			services.animation.remove( *this );
		}
	}

	void remove()
	{
		show( false ); 
		// push ourselves onto the undo/restore queue.
	}



	// IPositionEditorJob
	double hit_test( unsigned x, unsigned y ) 
	{
		return editor.hit_test( x, y ); 
	}

	void set_active( bool active ) 
	{
		editor.set_active( active); 
		notify( "change");
	}

	void set_position_active( bool ) 
	{ } 	

	void move( int x1, int y1, int x2, int y2 ) 
	{
		// should test if pos is the same, and avoid firing the event

		editor.move( x1, y1, x2, y2 );
		notify( "change");
	}

	void finish_edit() 
	{
		editor.finish_edit();
		// make the model fire an event 
	}

	
	// IAnimationJob 
	void tick() 
	{
		//std::cout << "whoot we are getting an animation event " << dt_ << std::endl;
		// we notify that our state has changed even if we haven't calculated it
		// yet.
		notify( "change");
	}

	// IRenderJob 
	void pre_render( RenderParams & params ) 
	{  }

	void render ( RenderParams & params ) 
	{
		renderer.render( params);
	}

	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		renderer.get_bounds( x1, y1, x2, y2);
	}

	int get_z_order() const 
	{
		return 102;
	}; 


};



}; // anon namespace

void add_animation_object( Services & services )
{
	new MyObject( services);
}







