
#include <aggregate/anim.h>


#include <service/services.h>
#include <service/position_editor.h>
#include <service/renderer.h>
#include <service/animation.h>

#include <common/path_seg_adaptor.h>
#include <common/path_reader.h>
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
		double dx = x2 - x1; 
		double dy = y2 - y1; 

		agg::trans_affine	affine;
		affine *= agg::trans_affine_translation( dx, dy );

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


struct MyObject : IPositionEditorJob, IRenderJob, IAnimationJob 
{
	// Assemble the object graph

	Services			& services ;

	bool				is_active;
	agg::path_storage	path; 

	// Helpers
	Render				renderer;
	Editor				editor;


	MyObject( Services & services )
		: services( services),
		is_active( false ),
		renderer( is_active, path), 
		editor( is_active, path)
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


	void show( bool u )
	{
		if( u) {		
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

	void notify()
	{
		services.renderer.notify( *this );
	}


	// IPositionEditorJob
	double hit_test( unsigned x, unsigned y ) 
	{
		return editor.hit_test( x, y ); 
	}

	void set_active( bool active ) 
	{
		editor.set_active( active); 
		notify();
	}

	void set_position_active( bool ) 
	{ } 	

	void move( int x1, int y1, int x2, int y2 ) 
	{
		editor.move( x1, y1, x2, y2 );
		notify();
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
		notify();
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
		// should delegate to the root. because z_order is 
		// animation job ...
		return 100;
	}; 

	bool get_invalid() const 
	{
		assert( 0);	// shouldn't ever be called because at 100
		return false;
	}	
};



}; // anon namespace

void add_animation_object( Services & services )
{
	new MyObject( services);
}







