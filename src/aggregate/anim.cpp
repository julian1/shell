
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

		// notify();
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



// ok, we just need to give the control point a position...
// and then hook the event, ... 
// that's ok, we set the position property, which will fire an event and
// the renderer will know to update. 
// The issue will be propagating activeness - maybe just propagate out. 

struct ControlPoint : IPositionEditorJob, IRenderJob
{
	ControlPoint( Services & services, int x, int y )
		: services( services ),
		x( x),
		y( y),
		r( 15),
		path(),
		active( false)
	{ 
//		double r = 15; 
//		double x = 125, y = 125; 


		show( true );
	} 

	Services					& services ;
	Listeners					listeners;

	agg::path_storage			path;	
	bool						active;
	int							x, y;
	double						r; 

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
		path.free_all();
		agg::ellipse e( x, y, r, r, (int) 20);
		path.concat_path( e );

//		agg::trans_affine affine;
//		affine *= agg::trans_affine_translation( x, y );
//		agg::path_storage tmp = path ; 
//		tmp.transform( affine );	
//		path = tmp; 
//		path.transform( affine );	

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
		x += x2 - x1;
		y += y2 - y1; 
	
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


// OK, there's an issue, that the control points are going to be moved... 

struct MyObject : /*IPositionEditorJob,*/ IRenderJob, IAnimationJob 
{
	// Assemble the object graph

	typedef MyObject	this_type;

	Services			& services ;

	bool				is_active;
	agg::path_storage	path; 

	// Helpers
	Render				renderer;
//	Editor				editor;

	int					x1, y1, x2, y2;
	ControlPoint		top_left;
	ControlPoint		top_right;

	Listeners			listeners;
	

	MyObject( Services & services )
		: services( services),
		is_active( false ),
		path(),
		renderer( is_active, path), 
//		editor( is_active, path),
		
		x1( 20), y1( 20),
		x2( 100), y2( 100),
		top_left( services, x1, y1 ),
		top_right( services, x2, y1 )
	{ 

		top_left.register_( make_adapter( *this, & this_type::on_control_point_changed )); 
		top_right.register_( make_adapter( *this, & this_type::on_control_point_changed )); 


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
//			services.position_editor.add( *this );
			services.animation.add( *this );
		}
		else {
			services.renderer.remove( *this  );
//			services.position_editor.remove( *this );
			services.animation.remove( *this );
		}
	}

	void remove()
	{
		show( false ); 
		// push ourselves onto the undo/restore queue.
	}


#if 0
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
#endif
	
	// IAnimationJob 
	void tick() 
	{
		//std::cout << "whoot we are getting an animation event " << dt_ << std::endl;
		// we notify that our state has changed even if we haven't calculated it
		// yet.
		notify( "change");
	}


	void on_control_point_changed( const Event &e )
	{
		std::cout << "control point changed" << std::endl;

		// we have to know which object, the event came from, update that, 
		// then set all the other objects...
		// and suppress feedback of events...

		// Ok, one of the points has changed --- but how do we know if
		// we should change the top_left or what-ever ?

		// easy - we can just test it...		 

		std::cout << "object " << &e.object << std::endl;

		ControlPoint &src = dynamic_cast< ControlPoint &>( e.object ); 
		if( &src == &top_left)
		{
			x1 = src.x;
			y1 = src.y;
		}
		else if( &src == &top_right)
		{
			x2 = src.x;
			y1 = src.y;
		}


		// and broadcast
		notify( "change");
	}


	// IRenderJob 
	void pre_render( RenderParams & params ) 
	{  }

	void render ( RenderParams & params ) 
	{
/*
		agg::rounded_rect   r( x1, y1, 100, 100, 10);

		path.free_all();
		path.concat_path( r);
*/
		renderer.render( params);

	}

	void get_bounds( int *x1_, int *y1_, int *x2_, int *y2_ ) 
	{
		agg::rounded_rect   r( x1, y1, x2, y2, 10);

		path.free_all();
		path.concat_path( r);

		renderer.get_bounds( x1_, y1_, x2_, y2_);
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

