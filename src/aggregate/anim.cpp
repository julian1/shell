
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

#if 0
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
#endif


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
		r( 6),
		path(),
		active( false)
	{ 
		show( true );
	} 

	Services					& services ;
	Listeners					listeners;
	agg::path_storage			path;	
	bool						active;
	int							x, y;
	double						r; 

	void set_position( int x_, int y_)
	{
		if( x_ == x && y_ == y)
			return;

		x = x_;
		y = y_;
		notify( "change");
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

	// IObject
	void register_( INotify * l) 
	{
		listeners.register_( l);
	} 

	void unregister( INotify * l)
	{
		listeners.unregister( l);
	}


	// IRenderJob
	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		path.free_all();
		agg::ellipse e( x, y, r, r, (int) 20);
		path.concat_path( e );

		bounding_rect_single( path , 0, x1, y1, x2, y2);	
	}	

	void pre_render( RenderParams & params) 
	{  }

	void render( RenderParams & params ) 
	{
		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( path );

		agg::rgba       color( .3, 0, 1, 1);
		agg::render_scanlines_aa_solid( ras, sl, params.surface.rbase(), color );
 	}

	int get_z_order() const
	{
		// always in top ? 
		return 101;
	}

	// IPositionEditorJob
	void move( int x1, int y1, int x2, int y2 )  
	{
		set_position( x + x2 - x1, y + y2 - y1 );
	}

	void set_active( bool active_ ) 
	{ 
		//callback.set_control_point_active( active_ ) ;

		// this needs to push an event

		std::cout << "control point active " << active_ << std::endl;
		if( active_ )
			notify("active");
		else
			notify("inactive");
	}

	void set_position_active( bool active_ ) 
	{ 
		// must change the name of this ( position_controller_active() )
		// WHY IS THIS 
		active = active_;
	}  

	void finish_edit() 
	{ }

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


// we should be propagating whether the thing is active, from the  ...
// also we should be injecting the renderer thing ?

// OK, there's an issue, that the control points are going to be moved... 


// Ok, MyObject Is now something of a framing/ hosting object for something ...

// stuff like the Renderer should be being passed to it however ?

// Ok, but hang on, how do construct the real object,


// we can construct an aggregated object. - it's the inner objects that will communicate
// with the interfaces. also events could even be forwared. 


/*
	OK, actually the renderering thing with the path ought into a frame subject 

*/

struct IFrameSubject
{
	// framesubject can be completely self contained. 

	virtual void set_position( int x1, int y1, int x2, int y2) = 0; 

	// set_active() ...
	virtual void set_active( bool active_ ) = 0; 

};



struct Frame  
{
	typedef Frame		this_type;

	IFrameSubject		& frame_subject;

	// Helpers
	int					x1, y1, x2, y2;
	ControlPoint		top_left;
	ControlPoint		top_right;
	ControlPoint		bottom_left;
	ControlPoint		bottom_right;

	Frame( Services & services, IFrameSubject & frame_subject )
		: frame_subject( frame_subject),
//		is_active( false ),
//		path(),
		x1( 20), y1( 20),
		x2( 100), y2( 100),
		top_left( services, x1, y1 ),
		top_right( services, x2, y1 ),
		bottom_left( services, x1, y2 ),
		bottom_right( services, x2, y2 )

	{ 
		// frame subject is composed yet
		// possibly we should be reading the location from the inner object ...
		frame_subject.set_position( x1, y1, x2, y2 ); 

		top_left.register_( make_adapter( *this, & this_type::on_control_point_changed )); 
		top_right.register_( make_adapter( *this, & this_type::on_control_point_changed )); 
		bottom_left.register_( make_adapter( *this, & this_type::on_control_point_changed )); 
		bottom_right.register_( make_adapter( *this, & this_type::on_control_point_changed )); 
	}  

	~Frame()
	{
		top_left.unregister( make_adapter( *this, & this_type::on_control_point_changed )); 
		top_right.unregister( make_adapter( *this, & this_type::on_control_point_changed )); 
		bottom_left.unregister( make_adapter( *this, & this_type::on_control_point_changed )); 
		bottom_right.unregister( make_adapter( *this, & this_type::on_control_point_changed )); 
		//show( false);
	}


	void on_control_point_changed( const Event &e )
	{
		ControlPoint &src = dynamic_cast< ControlPoint &>( e.object ); 

		if( std::string( e.msg) == "active" )		
			frame_subject.set_active( true );
		else if( std::string( e.msg) == "inactive" )		
			frame_subject.set_active( false );
			

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
		if( &src == &bottom_left)
		{
			x1 = src.x;
			y2 = src.y;
		}
		else if( &src == &bottom_right)
		{
			x2 = src.x;
			y2 = src.y;
		}

		top_left.set_position( x1, y1);
		top_right.set_position( x2, y1);
		bottom_left.set_position( x1, y2);
		bottom_right.set_position( x2, y2);

		frame_subject.set_position( x1, y1, x2, y2 );
	}
};




struct Render
{
	// A helper class for simple dashed line animation renderer
//	bool				& is_active; 
//	agg::path_storage	& path; 
	double				offset; 

public:
//	Render( bool & is_active, agg::path_storage	& path )  
	Render() 
	:	offset( 0)
	{ } 

	void render ( RenderParams & params,
		agg::path_storage	& path,
		bool				& is_active
	) 
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


	void get_bounds(
		agg::path_storage	& path,
		 int *x1, int *y1, int *x2, int *y2 ) 
	{
		bounding_rect_single( path, 0, x1, y1, x2, y2);	

		*x1 -= 2; 
		*y1 -= 2; 
		*x2 += 2; 
		*y2 += 2; 
	}

};



struct FrameAnimator : IRenderJob, IAnimationJob, IFrameSubject
{
	// animate the path... 
	// framesubject can be completely self contained. 

	Services			& services ;
	Render				& renderer;

	Listeners			listeners;
	int					x1, y1, x2, y2;
	bool				is_active;
	agg::path_storage	path; 

	FrameAnimator( Services & services, Render	& renderer)
		: services( services),
		renderer( renderer)
	{
		show( true);
	}

	// IFrameSubject
	void set_active( bool active_ ) 
	{
		is_active = active_;
	} 

	void set_position( int x1_, int y1_, int x2_, int y2_) 
	{
		if( x1 == x1_ && y1 == y1_ && x2 == x2_ && y2 == y2_ )
			return;

		x1 = x1_;
		y1 = y1_;
		x2 = x2_;
		y2 = y2_;
		notify( "change");
	} 

	void notify( const char *msg )
	{
		listeners.notify( *this, msg ); 
	}

	void show( bool u )
	{
		if( u) {		
			services.renderer.add( *this  );
			services.animation.add( *this );
		}
		else {
			services.renderer.remove( *this  );
			services.animation.remove( *this );
		}
	}

	// IObject
	void register_( INotify * l) 
	{
		listeners.register_( l);
	} 

	void unregister( INotify * l)
	{
		listeners.unregister( l);
	}

	// IAnimationJob 
	void tick() 
	{
		// notify that our state has changed even if we haven't calculated it yet.
		notify( "change");
	}

	// IRenderJob 
	void get_bounds( int *x1_, int *y1_, int *x2_, int *y2_ ) 
	{
		agg::rounded_rect   r( x1, y1, x2, y2, 10);
		path.free_all();
		path.concat_path( r);
		renderer.get_bounds( path, x1_, y1_, x2_, y2_);
	}

	void pre_render( RenderParams & params ) 
	{ }

	void render ( RenderParams & params ) 
	{
		renderer.render( params, path, is_active );
	}

	int get_z_order() const 
	{
		return 102;
	}; 
};



// Ok now we want a frame multiplexer...


struct FrameMultiplexer : IFrameSubject
{
	IFrameSubject & a;
	IFrameSubject & b;

	FrameMultiplexer( IFrameSubject & a, IFrameSubject & b ) 
		: a( a),
		b( b)
	{ } 

	void set_position( int x1, int y1, int x2, int y2) 
	{
		a.set_position( x1, y1, x2, y2);
		b.set_position( x1, y1, x2, y2);
	} 

	void set_active( bool active ) 
	{
		a.set_active( active);	
		b.set_active( active);	
	} 
};


struct Y : IFrameSubject
{
	void set_position( int x1, int y1, int x2, int y2) 
	{
		std::cout << "Whoot Y set_position()" << std::endl;
	} 

	void set_active( bool active ) 
	{ }
};

// This is our aggregated object...

struct MyObject2
{
	Render			renderer; 
	FrameAnimator	frame_animator;
	Y				y;	
	FrameMultiplexer multi;
	Frame			frame;

	MyObject2( Services & services )
		: renderer(),
		frame_animator( services, renderer),
		y(),
		multi( frame_animator, y),
		frame( services, multi )
	{ } 

};


}; // anon namespace


void add_animation_object( Services & services )
{

	// the renderer, ought to be being passed by reference to this thing.

	new MyObject2( services);
}

