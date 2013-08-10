
#include <aggregate/anim.h>


#include <service/services.h>
#include <service/position_editor.h>
#include <service/renderer.h>

#include <service/animation.h>

#include <common/path_seg_adaptor.h>
#include <common/path_reader.h>
//#include <common/update.h>
#include <common/surface.h>


#include <agg_bounding_rect.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
#include <agg_conv_dash.h>

#include <agg_rounded_rect.h>


#include <boost/functional/hash.hpp>	
#include <cmath>

namespace {

static double pointToLineDistance( double Ax, double Ay,  double Bx, double By, double Px, double Py )
{
	// belive this is not correct. hittesting extends beyond the line.

	// http://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
	double normalLength = std::sqrt((Bx - Ax) * (Bx - Ax) + (By - Ay) * (By - Ay));
	return std::abs((Px - Ax) * (By - Ay) - (Py - Ay) * (Bx - Ax)) / normalLength;
}





#if 0
struct AnimAggregateRoot 
{
	AnimAggregateRoot(  )
	{ 
			}  


};
#endif

struct MyObject ; 

struct IMyPeer
{
	virtual void add( MyObject & o ) = 0;
	virtual void notify( MyObject & o ) = 0;
	
};


struct MyObject : IPositionEditorJob, IRenderJob, IAnimationJob 
{
	// assemble the state

	MyObject( IMyPeer & peer )
		: peer( peer), 
		offset( 0),
		dt( 0),
		test_animation_active( false )
	{ 
		agg::rounded_rect   r( 20, 20, 100, 100, 10);

		path.free_all();
		path.concat_path( r);
	}  

	IMyPeer				& peer; 

	double				offset; 
	int					dt; 
	agg::path_storage	path; 
	bool				test_animation_active;

	void notify()
	{
		peer.notify( *this );
	}


	// IPositionEditorJob
	void add_ref()  { } 
	void release()  { } 

	// - return distance so that position editor can choose when several options 
	// - this can handle its own projection needs, and a projection wrapper can 
	// be added. also georef etc. eg. it can delegate
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
		test_animation_active = active_;	
		notify();
	}
	void set_position_active( bool ) 
	{ } 	

	void move( int x1, int y1, int x2, int y2 ) 
	{
		double dx = x2 - x1; 
		double dy = y2 - y1; 

		agg::trans_affine	affine;
		affine *= agg::trans_affine_translation( dx, dy );

		agg::path_storage tmp = path ; 
		tmp.transform( affine );	
		path = tmp; 

		notify();
	}

	void finish_edit() 
	{
		// make the model fire an event 
	}
	
	
	// IAnimationJob 

	void animate( int dt_ ) 
	{
		std::cout << "whoot we are getting an animation event " << dt_ << std::endl;
		dt = dt_; 
		notify();
	}


	// IRenderJob 

	void pre_render() {  }

	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		bounding_rect_single( path, 0, x1, y1, x2, y2);	

		*x1 -= 2; 
		*y1 -= 2; 
		*x2 += 2; 
		*y2 += 2; 
	}
	

	void render ( BitmapSurface & surface ) 
	{
		//std::cout << "render test job " << parms.dt << std::endl;

//		path_reader	reader( root->get_path() );

		offset += dt * 0.020;

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

		if( test_animation_active  )
			stroke.width( 4);
		else	
			stroke.width( 2);
#if 0
		agg::conv_stroke< path_reader>	stroke( reader );
		stroke.width( 1); 
#endif
		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), agg::rgba8( 0xff, 0, 0 ) );
	}

	bool get_invalid() const 
	{
		assert( 0);	// shouldn't ever be called because at 100
		return false;
	}	
	int get_z_order() const 
	{
		// should delegate to the root. because z_order is 
		// animation job ...
		return 100;
	}; 

};




// when we create the object ...
// why not let the individual objects register what they want ...

// there's a problem if the top level object doesn't implement all the interfaces...
// any point in the object has to be able to pass itself ...

// the root needs to broadcast the event ... 
// peer.notify( *this );

// notify has to be able to be broadcast to everyone ...


struct MyPeer : IMyPeer
{
	MyPeer ( Services & services )
		: services( services)
	{ } 

	Services & services;

	void create_object()
	{
		MyObject *o = new MyObject( *this ); 
		add( *o );	// object could also add itself ...
					// would be consistent with the destructor ...
	}

	void delete_object()
	{
		// could use this... but probably better to rely on the 
		// destructor
		// since the entire point is broadcast from the object itself.
	}

	void add( MyObject & o )  
	{
		services.renderer.add( o );
		services.position_editor.add( o );
		services.animation.add( o );
	}

	void remove( MyObject & o )  
	{
		services.renderer.remove( o );
		services.position_editor.remove( o );
		services.animation.remove( o );
	}

	void notify( MyObject & o)
	{
		services.renderer.notify( o );
	}
};


}; // anon namespace

void add_test_anim_layer( Services & services )
{
	MyPeer *peer = new MyPeer( services ); 	
	peer->create_object(); 
}





#if 0
////


struct MyLayer //: ILayerJob 
{
	Services & services;  
	ptr< RenderAnim>			render_anim;
	ptr< PositionEditorAnim>	position_editor; 

	MyLayer( 
		Services			& services,
		const ptr< RenderAnim>			render_anim,
		const ptr< PositionEditorAnim>	position_editor ) 
		: services( services) ,
		render_anim( render_anim ),
		position_editor( position_editor )
	{ } 


	void load() 
	{
//		services.layers.add( ptr< ILayerJob>( this) ); 
		services.renderer.add( * render_anim ); 

		services.animation.add( * render_anim );
		services.position_editor.add(  position_editor );
	}
	void unload() 
	{
		services.position_editor.remove(  position_editor );

		services.animation.remove( * render_anim );
		services.renderer.remove( * render_anim ); 
//		services.layers.remove( ptr< ILayerJob>( this) ); 
	}
	void add_ref() { } 
	void release() { } 

/*
	void layer_update( ) { }   
	void post_layer_update( ) { }   
*/
	std::string get_name() { return "anim"; }
};
#endif


//ptr< IAnimAggregateRoot> create_test_anim_aggregate_root(); 

// It should also be loaded into the layers ...

#if 0

void add_test_anim_layer( Services & services )
{
	ptr< IAnimAggregateRoot> root = create_test_anim_aggregate_root();	

	MyLayer * layer = new MyLayer ( services, new RenderAnim( root ), new  PositionEditorAnim ( root ) ); 
	layer->load();

};
#endif

#if 0

//void add_anim_aggregate_root( Services & services, const ptr< IAnimAggregateRoot> & root )

void remove_anim_aggregate_root( Services & services, const ptr< IAnimAggregateRoot> & root )
{
	assert( 0 ); 
}
#endif




