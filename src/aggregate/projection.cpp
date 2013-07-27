
#include <aggregate/projection.h>

#include <common/projection.h>
#include <common/polygon_algebra.h>
#include <common/surface.h>
#include <common/point_in_poly.h>
#include <common/path_seg_adaptor.h>

#include <service/services.h>
#include <service/position_editor.h>
#include <service/position_editor.h>
#include <service/renderer.h>

#include <service/layers.h>


#include <agg_trans_affine.h>
#include <agg_path_storage.h>

#include <agg_pixfmt_rgba.h>
#include <agg_ellipse.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_bounding_rect.h>


#include <boost/functional/hash.hpp>	

#include <cmath>
#include <sstream>

#include <proj_api.h>


namespace {


struct XControlPoint; 

struct IXControlPointCallback
{
	virtual void move( const ptr< XControlPoint>  & sender,  int x1_, int y1_, int x2_, int y2_ ) = 0;
	// control passes from the IRenderJob, therefore it's easier to request the position, although
	// it's not tell don't ask.
	virtual void get_position( const ptr< XControlPoint> & sender, double * x, double * y ) = 0; 

	virtual void set_control_point_active( bool ) = 0;
}; 


/*
	-right so if we have two classes sharing the same name, then the symbols get fucking merged	
	on the virtual interface. 
	
	- this is a real mess.
*/

// how can it render, if render is never called. ???

struct XControlPoint : IPositionEditorJob, IRenderJob//, IKey
{
	XControlPoint( IXControlPointCallback & callback )
		: count( 0 ),
		callback( callback ),
		path(),
		active( false)
	{ } 

	~XControlPoint()
	{ }

	unsigned					count;
	IXControlPointCallback		& callback; 
	agg::path_storage			path;	
	bool						active		;

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

/*
	// IKey
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine(seed, this );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< XControlPoint> arg = dynamic_pointer_cast< XControlPoint>( key ); 
		if( !arg) return false;
		return ( this == &*arg ); 
	} 
*/

	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		if( active )
		{
			double r = 5; 
			double x = 0, y = 0; 
			callback.get_position( this, &x, &y );			// this get_position is getting called multiple times.

			path.remove_all();	
			agg::ellipse e( x, y, r, r, (int) 10);
			path.concat_path( e );	

			bounding_rect_single( path , 0, x1, y1, x2, y2);	
		}
	}	

	// IRenderJob
	void render( BitmapSurface & surface, const UpdateParms & parms /* x, y */ ) 
	{
		// remember the path is used for both, render and hittesting. 
		// we calculate at render time.

		if( active )
		{
			agg::scanline_p8                sl;
			agg::rasterizer_scanline_aa<>   ras;
			ras.add_path( path );

			agg::rgba       color( .3, 0, 1, .5);
			agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), color );
		}
	}
	bool get_invalid() const 
	{
		assert( 0);	// at 100
		return false;
	}	
	// IRenderJob
	int get_z_order() const
	{
		return 100;
	}

	// IPositionEditorJob
	void move( int x1_, int y1_, int x2_, int y2_ )  
	{
		callback.move( this , x1_, y1_, x2_, y2_ ) ;
	}

	void set_active( bool active_ ) 
	{ 
		callback.set_control_point_active( active_ ) ;
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




/*
	there's a problem about who owns the memory.
*/
struct PositionEditorJobProjection : /* IPositionEditorJob,*/ IXControlPointCallback
{
	// OK we have to manipulate the projection, 
	// which means we have to have the details of the projection.
	// which means having the affine that's injected into the projection

	/*
		This only really needs the limb, (and the means to project the limb if not in screen space)
	*/
	/*
		we could inject the points, or inject the type of dot to render etc.
		or just instantiate here, because coupling is not really avoidable.
	*/
	PositionEditorJobProjection(  Services & services, const ptr< IProjectionAggregateRoot> & root )
		: services( services ),
		root( root),
		top_left( new XControlPoint( *this ) ), 
		top_right( new XControlPoint( *this ) ),
		bottom_left( new XControlPoint( *this ) ),
		bottom_right( new XControlPoint( *this ) )
	{ 
	

		services.renderer.add( top_left  );
		services.position_editor.add( top_left );

		services.renderer.add( top_right  );
		services.position_editor.add( top_right );

		services.renderer.add( bottom_left  );
		services.position_editor.add( bottom_left );

		services.renderer.add( bottom_right  );
		services.position_editor.add( bottom_right );
/**/
	} 

	~PositionEditorJobProjection()
	{

		services.renderer.remove( top_left );
		services.position_editor.remove( top_left );

		services.renderer.remove( top_right );
		services.position_editor.remove( top_right );

		services.renderer.remove( bottom_left );
		services.position_editor.remove( bottom_left );

		services.renderer.remove( bottom_right );
		services.position_editor.remove( bottom_right );

	}
 
	Services& services; 

	ptr< IProjectionAggregateRoot>	root;

	ptr< XControlPoint> top_left; 
	ptr< XControlPoint> top_right; 
	ptr< XControlPoint> bottom_left; 
	ptr< XControlPoint> bottom_right; 

#if 0
	void set_position_active( bool active ) 
	{ 
		std::cout << "$$$ set_position_active " << active << std::endl;
		if( active)
		{ }
		else
		{ }

		// THE PROBLEM IS THAT THE POINT CANNOT BE PROJECTED, if it's outside the proj bounds ....
		// I think maybe it doesn't matter.
		// ok, so to get the coordinates  we have to get all the points ...
		// remember it's a square so we only need 4 values
		// left_x, right_x,  top_y, bottom_y 
		// RIGHT I THINK IT'S SIMPLE - WE GET THE LIMB AND PROJECT IT INTO SCREEN SPACE AND THEN WORK FROM THERE...
		// SO EVERYTHING IS ACTUALLY HANDLED IN SCREEN SPACE 
	} 	
#endif

	void set_control_point_active( bool active ) 
	{
		// set the projection to be active
		root->set_active( active );	
	}


	// in screen space
	double left_x, right_x, top_y, bottom_y ; 

	/*
		we may want a pre_get_position, and post_get_position ? 
		which could enable caching ? 
	*/
	void get_position( const ptr< XControlPoint>  & sender, double * x, double * y ) 
	{

		/*
			we have to avoid recalculating all this
		*/

		// WHY NOT JUST INJECT THESE POINTS INTO THE CONTROL BY REFERENCE ???????

		agg::path_storage path;

		root->get_projection()->forward( root->get_limb(), path, false ); 

		agg::bounding_rect_single( path, 0, & left_x, &top_y, & right_x, & bottom_y );


		if( sender == top_left )
		{
			*x = left_x;//-180;
			*y = top_y;//-90; 
		}
		else if( sender == top_right )
		{
			*x = right_x;//+180 ;
			*y = top_y;//-90; 
		}
		else if( sender == bottom_left )
		{
			*x = left_x;//-180;
			*y = bottom_y;//90; 
		}
		else if( sender == bottom_right )
		{
			*x = right_x;//+180; 
			*y = bottom_y;//90; 
		}
		else assert( 0);

//		std::cout << "get position for projection " << x << " "  << y << std::endl;

		/*
			We would want to avoid the projection.
		*/
		//root.get_projection()->forward( x, y ); 
	}

	//	struct IXControlPointCallback
	void move( const ptr< XControlPoint>  & sender,  int x1_, int y1_, int x2_, int y2_ ) 
	{

//		agg::trans_affine u = root->get_affine(); 
//		u.invert();


		/*
			OK, we've changed this to work completely in screenspace. we can do this, 
			because the affine is the last element of the object projection. 
		*/
		// means we have correctly hittested
		// grid projection just takes us to the affine space
		double x1 = x1_, y1 = y1_, x2 = x2_, y2 = y2_ ;  
//		u.transform( &x1, &y1 ) ; 
//		u.transform( &x2, &y2 ) ; 
//		root->get_projection()->reverse( &x1, &y1 );
//		root->get_projection()->reverse( &x2, &y2 );
		double dx = x2 - x1; 
		double dy = y2 - y1; 

//		double w = root.get_grid()->width();
//		double h = root.get_grid()->height();
		/*
			this is correct, it's not the point's job to know what position it is, and it's relation with the rect.
			either we route here, or we inject the dependency.
		*/
		agg::trans_affine	a;

	
		/*
			The width should always be the same. 	eg. approax 360. 
			Both the grid and how this used to work.

			the issue is that right_x and left_x etc are all in screen space, because we forward project them

			But we can't actually project the points. 
			So we have to project imaginary points ?

			BUT WE DON"T HAVE TO PUSH IT THROUGH THE PROJ. ONLY THROUGH THE INVERSE OF THE AFFINE
		*/

		/*
			OK. Maybe we should do everything in screen coordinates. 
			then invert, then apply. 
			????
		*/
	
		/*
			no, i really don't understand why it doesn't work all in screen.
			perhaps we should should use the original 
		*/

/*
		double right_x_ = right_x;
		double left_x_  = left_x; 
		double top_y_   = top_y;
		double bottom_y_  = bottom_y;

		u.transform( &left_x_, &top_y_  ) ; 
		u.transform( &right_x_, &bottom_y_) ; 
*/

		double width = (right_x - left_x ) ;//360;
		double height = ( bottom_y - top_y ) ;

		assert( width >= 0 );
		assert( height >= 0 );

		// the translation has put the opposite point on the origin of the original affine
		agg::trans_affine u = root->get_affine(); 
		u.invert();


		if( sender == top_left )
		{
			//std::cout << "*** top left" << std::endl;

			// this point lies outside the proj, and in screen space.
			double x = right_x;//180;
			double y = bottom_y;//90;
			u.transform( & x, & y );

			// centre tye rys on tye 0 origin
			a *= agg::trans_affine_translation( -x, -y);			
			a *= agg::trans_affine_scaling( (-dx + width) / width, (-dy + height) / height );
			a *= agg::trans_affine_translation( + x, + y );			
		}
		else if( sender == top_right )
		{
			//std::cout << "*** top right " << std::endl;

			double x = left_x;//-180;
			double y = bottom_y;//90;
			u.transform( & x, & y );

			a *= agg::trans_affine_translation( -x, -y);			
			a *= agg::trans_affine_scaling( (+dx + width) / width, (-dy + height) / height );
			a *= agg::trans_affine_translation( +x, + y );			
		}
		else if( sender == bottom_left )
		{
			//std::cout << "*** bottom left " << std::endl;

			double x = right_x;//180;
			double y = top_y;//-90;
			u.transform( & x, & y );

			a *= agg::trans_affine_translation( -x, -y);			
			a *= agg::trans_affine_scaling( (-dx + width) / width, (+dy + height) / height );
			a *= agg::trans_affine_translation( + x, + y );			
		}
		else if( sender == bottom_right)
		{
			//std::cout << "*** bottom right " << std::endl;

			double x = left_x;//-180;
			double y = top_y;//-90;
			u.transform( & x, & y );

			a *= agg::trans_affine_translation( -x, -y);			
			a *= agg::trans_affine_scaling( (+dx + width) / width, (+dy + height) / height );
			a *= agg::trans_affine_translation( +x, + y );		
		}
		else assert( 0);

//		a.invert();

		// apply eaisting translation
		a *= root->get_affine(); 
		root->set_affine( a ); 
	}

#if 0
	double hit_test( unsigned x, unsigned y ) 
	{
		// WE SHOULD RETURN A NEGATIVE NUMBER FOR NON-MATCHING.
		return 1234;
	}

	void set_active( bool active_ ) 
	{ 
		std::cout << "PROJ POSITION EDITOR set active" << std::endl;

	}

	void move( int x1, int y1, int x2, int y2 ) 
	{ }

	void finish_edit() 
	{ }
#endif
};




struct AffineProjection : IProjection
{
	// a simple projection we will use to put georef coordinates on screen

	AffineProjection ( agg::trans_affine & affine )
		: count( 0),
		affine( affine)
	{ 
/*
		affine *= agg::trans_affine_scaling( 1, -1 );				// flip
		affine *= agg::trans_affine_translation( 180, 90 );		// align edges
		affine *= agg::trans_affine_scaling( 2 );				// scale
*/
	} 
	~AffineProjection ()
	{ } 
	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	void forward( double *x, double * y ) 
	{
		affine.transform( x, y ); 
	} 
	void reverse( double *x, double * y ) 
	{
		agg::trans_affine	tmp = affine;
		tmp.invert();
		tmp.transform( x, y );	
	}
	void forward( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		dst = src;
		dst.transform( affine );
	} 
	void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		agg::trans_affine	tmp = affine;
		tmp.invert();
		dst = src;
		dst.transform( tmp );
	} 
private:
	unsigned count;
	agg::trans_affine	& affine;
};







struct ForwardTransform 
{
	ForwardTransform( const projPJ & proj  )
		: proj( proj)
	{ } 
	void transform( double *x, double *y ) const
	{
		assert( proj);
		projUV g;       
		// should be removed, handled in 
		g.u = agg::deg2rad( *x);                // -180 to +180
		g.v = agg::deg2rad( *y);                // -90 to +90           
		projUV h = pj_fwd( g, proj); 

		// not sure, if we shouldn't use HUGE_VAL directly, 

		if( h.u == HUGE_VAL || h.v == HUGE_VAL) {
			//  assert( 0); // should be assert
//			std::cout << "proj failed " << *x << " " << *y << std::endl;
			*x = 0; 
			*y = 0; 
			// return false;
		}       
		else { 
			// *x = h.u * .00001;  
			// *y = -h.v * - .00001; // reverse y for convenience  ie flip_y 
			*x = h.u ;  
			*y = h.v ; 
		}       
	}
private:
	const projPJ & proj; 
};


struct InverseTransform
{
	InverseTransform( const projPJ & proj  )
		: proj( proj)
	{ } 
	void transform( double *x, double *y ) const
	{
		projUV g;       
		g.u = *x; 
		g.v = *y; 
		projUV h = pj_inv( g, proj); 

		if( h.u == HUGE_VAL || h.v == HUGE_VAL) {
			//  assert( 0); // should be assert
			*x = 0; 
			*y = 0; 
			// return false;
		}       
		else { 
			// *x = h.u * .00001;  
			// *y = -h.v * - .00001; // reverse y for convenience  ie flip_y 
			*x = agg::rad2deg( h.u );  
			*y = agg::rad2deg( h.v ); 
		}       
	}
private:
	const projPJ & proj; 
};


struct PjProjProjection : IProjection
{
	// projection using pj_proj 

	PjProjProjection ( projPJ & proj ) 
		: count( 0),
		proj( proj),
		forward_( proj ),
		inverse_( proj )
	{ } 
	
	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	void forward( double *x, double * y ) 
	{
		forward_.transform( x, y );
	} 
	void reverse( double *x, double * y )	// change name to inverse rather than reverse ???
	{
		inverse_.transform( x, y );
	}
	void forward( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		dst = src;
		dst.transform( forward_ ); 
	} 
	void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		dst = src;
		dst.transform( inverse_ );
	} 
private:
	unsigned count;
	projPJ             & proj; 
	ForwardTransform	forward_;
	InverseTransform	inverse_;	
};


/*
	Very Important. 

	The Clip operation could be placed in the pipeline as well as the proj and the affine !!! 
	We could also pass booleans down, or a struct with some flags (eg. don't clip ).  
	- An issue is that, we only handle polygons, and not polylines in the clipper.

	note sure if pipeline is best, or injected deps, or another structure that does the
	composing. 
*/

/*
	We should rename IProjection to ITransform. it is only really a projection when put together. 
	Not really 'projection' is used in relational theory.
*/

struct ClipProjection : IProjection
{
	ClipProjection ( const agg::path_storage & limb ) 
		: count( 0),
		limb( limb )
	{ } 
	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	void forward( double *x, double * y ) 
	{ } 
	void reverse( double *x, double * y )	// change name to inverse rather than reverse ??? - quite like reverse
	{ }
	void forward( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		if( clip )
			calculate_intersection( limb, src, dst );
		else
			dst = src;
	} 
	void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{
		dst = src;
		// ?
	} 
private:
	unsigned count;
	const agg::path_storage & limb; 
};



/*
	Very Important.

	Am not sure if we really want to combine the projection in a tree.

	It may be better to have a flat structure that contains all the transforms, and
	perform the chaining.  And keep the types. 

	**Eg. we are getting close to coding the procedural steps as a tree graph, however this
	is more complicated than flattening it out and wrapping **

	This would be easier to modify.

	Eg. there is quite a bit of cross-over between the 'aggregate-root' and 

	Then we can avoid having 
	-----	
	We have the very big issue, of knowing how to efficiently project and clip multiple items and tagging them.
	remembering that the limb has 4000 points or so, and is inefficient to sort.

	The limb is part of the projection aggregate, and so we could actually use a merge sort, and keep the limb in 
	sorted condition and use dirty flags. - potentially.
*/

struct CombineProjections : IProjection
{
	/*
		I think it would be better to combine all transforms in one wrapper class, rather
		than encode the procedural evaluation as a nested tree.
	*/
	CombineProjections( const ptr< IProjection> & a, const ptr< IProjection> & b )
		: a( a),
		b( b ),
		count( 0)
	{ }  

	void add_ref() { ++count; }
	void release() { if( --count == 0) delete this; }

	void forward( double *x, double * y ) 
	{ 
		a->forward( x, y ); 
		b->forward( x, y );
	} 
	void reverse( double *x, double * y ) 
	{ 
		b->reverse( x, y );
		a->reverse( x, y );
	} 
	void forward( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{ 
		assert( & src != & dst );
		// do we have to copy here ???
		agg::path_storage tmp; 
		a->forward( src, tmp, clip );
		b->forward( tmp, dst, clip );
	} 
	void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{ 
		assert( & src != & dst );
		agg::path_storage tmp; 
		b->reverse( src, tmp, clip );
		a->reverse( tmp, dst, clip );
	} 
private:
	unsigned count;	
	const ptr< IProjection>		a; 
	const ptr< IProjection>		b; 
};







struct point_x_less
{
	// how do we give this thing static linkage ??? anonymouse namespace
	// by putting it in the Impl ? 
	typedef std::pair< double, double > point_t;
	inline bool operator () ( const point_t &a, const point_t &b) const
	{
		return a.first < b.first;
	}
};

static void create_clip_path_for_ortho( agg::path_storage & clip_path, const InverseTransform & it )
{
	/*
		REMEMBER THE CLIPPING SOLUTION HERE, IS TO REMOVE OR REWRITE THE CLIPPING
		in PJ_PROJ AT THE BOUNDARY, SO it doesn't matter if the projected point is behind the
		limb.	

	*/
	// now we want to try and create the proj bounds .	
	typedef std::pair< double, double >      point_t;
	std::vector< point_t >     v;
	// InvTransform	it( c.proj); 
	// Transform		t( c.proj); // /*proj*/;  t.proj = proj;
	const double pi = 3.14159265358979323846;
	double r = 6378000; // double r = 6000000;
    double inc = (2 * pi) / 2000;	// 1000, 4000
	for( double t = -pi; t < pi ; t += inc)
	{
		double x = cos( t) * r;
		double y = sin( t) * r;
		it.transform( &x, &y );	// to lat/lon  
		assert( x != 0 && y != 0 );
		v.push_back( std::make_pair( x, y ));
		// convert to lat lon
		// bool result = trans_inverse_.transform( &x, &y);
		// v.push_back( point( x , - y));      // negative y because of our inverse proj
		// t.transform( &x, &y );	// check no points fail
	}
	// std::cout << "### v.size() " << v.size() << std::endl;
	/*
	we this std::sort. instead we should just overlay the trace of the limb 
	and the box from -200 + 200.
	*/
	// sort by lon
	std::sort( v.begin(), v.end(), point_x_less());
	// add to the two paths the oversized clip and the drawable regions 
	clip_path. free_all();
	for( std::vector< point_t>::iterator i = v.begin(); i != v.end(); ++ i )
	{
		if( i == v.begin()) clip_path.move_to( i->first, i->second);
		else clip_path.line_to( i->first, i->second);
		// if( i == v.begin()) limb.move_to( i->x, i->y);
		// else limb.line_to( i->x, i->y);
	}
	clip_path.line_to( 200, 100);
	clip_path.line_to( -200, 100);
	clip_path.close_polygon(); 
}



struct ProjectionAggregateRoot : IProjectionAggregateRoot 
{
	unsigned			count;

	agg::path_storage	limb;

	projPJ              proj; 

	agg::trans_affine	affine;			// accessed with get_affine() and set_affine()

	ptr< IProjection>	simple_projection;

	bool				active;			// being edited etc.

	bool				invalid;

	ProjectionAggregateRoot( /*Services & services */ /* proj prefs */ ) 	
		: count( 0),
		limb(),
		proj( NULL),
		affine(),
		simple_projection(),
		active( false ),
		invalid( true )
	{ }
	
	~ProjectionAggregateRoot()
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

/*
	there should be a set_Pj_Proj here
*/
	ptr< IProjection> get_projection() const
	{
		return simple_projection;
	}

	const agg::trans_affine & get_affine () const 
	{
		return affine;
	}

	void set_affine( const agg::trans_affine & affine_ )
	{
		invalid = true;
		affine = affine_;
	}

	const agg::path_storage & get_limb() const 
	{
		return limb;
	}

	void set_active( bool _ ) 
	{
		std::cout << "PROJECTION setting active " << _ << std::endl;

		invalid = true;
		active = _;
	}

	bool get_active() const 
	{
		return active;
	}


	bool get_invalid() const 
	{
		return invalid;
	}
	void clear_invalid()
	{
		invalid = false;
	}

/*
	// IKey
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine( seed, this );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< ProjectionAggregateRoot> arg = dynamic_pointer_cast< ProjectionAggregateRoot>( key ); 
		if( !arg) return false;
		return ( this == &*arg ); 
	} 
*/


};



/*
	Get rid of this class. 
*/
struct ProjectionLayer : ILayerJob
{
	ProjectionLayer(  const ptr< IProjectionAggregateRoot> & root )
		: count( 0),
		root( root)
	{ } 
	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 
	void layer_update() { } 
	void post_layer_update()
	{
		root->clear_invalid(); 
	}

	std::string get_name() { return "projection control points?"; }

	unsigned						count; 
	ptr< IProjectionAggregateRoot>	root;
};


}; // end anon namespace



void add_projection_aggregate_root( Services & services, const ptr< IProjectionAggregateRoot> & root ) 
//void add_projection_aggregate_root( Services & services, ProjectionAggregateRoot & root ) 
{
	void *key = & *root;
	// add a position edit for the projection (this shouldn' really be here
	// YES IT's for the projection ... 
//	services.position_editor.add( key , new PositionEditorJobProjection( services, root) ); 


	// noone owns the memory for this at the moment !!!!.

	new PositionEditorJobProjection( services, root); 


//	services.post.add( root, new ProjectionLayer( root )); 


	services.layers.add(  new ProjectionLayer( root )); 

}

#if 0
void remove_projection_aggregate_root( Services & services, const ptr< IProjectionAggregateRoot> & root ) 
//void remove_projection_aggregate_root( Services & services, ProjectionAggregateRoot & root ) 
{
	void *key = & *root;
//	services.position_editor.remove( key );


//	services.post.remove( root); 
}
#endif




/*
	Ok, I think the Pj proj just wants to implement the same interface IProjection interface ...
	for operations. Then we can compose it.

	Remember that we are going to have handle clipping and stuff

	The pj proj should be injected only - to enable us to modify it in the aggregate.
*/


ptr< IProjectionAggregateRoot>	create_projection_aggregate_root()
{
	ptr< ProjectionAggregateRoot> o = 	new ProjectionAggregateRoot;
	// initialize pj-proj
	double lon = 10;
	double lat = 30;

	// set up ortho proj 
	std::stringstream       config;
	config  << " +proj=" << "ortho"
			<< " +lon_0=" << lon //10e"
			<< " +lat_0=" << lat    // 30
			<< " +ellps=WGS84 +datum=WGS84 +no_defs";   

	o->proj = pj_init_plus( config.str().c_str() );
	assert( o->proj);        

	ptr< IProjection>  pjproj = new PjProjProjection( o->proj ); 
	
	// create clip path
	create_clip_path_for_ortho( o->limb, InverseTransform( o->proj ) ); 

	ptr< IProjection>	clip = new ClipProjection( o->limb );
	
	ptr< IProjection>	combined = new CombineProjections( clip, pjproj ); 

	// initialize the affine
	o->affine *= agg::trans_affine_scaling( 1, -1);               // flip y
	o->affine *= agg::trans_affine_scaling( .00007 );     // scale down from metre unit
	o->affine *= agg::trans_affine_translation( +300, +500);      // translate to screen origin

	ptr< IProjection> a = new AffineProjection( o->affine ); 
	o->simple_projection = new CombineProjections( combined , a); 
	return o;
}


ptr< IProjectionAggregateRoot>	create_projection_aggregate_root_2()
{
	ptr< ProjectionAggregateRoot> o = 	new ProjectionAggregateRoot;
	// initialize pj-proj
	double lon = 10;
	double lat = 30;

	// set up ortho proj 
	std::stringstream       config;
	config  << " +proj=" << "ortho"
			<< " +lon_0=" << lon //10e"
			<< " +lat_0=" << lat    // 30
			<< " +ellps=WGS84 +datum=WGS84 +no_defs";   

	o->proj = pj_init_plus( config.str().c_str() );
	assert( o->proj);        

	ptr< IProjection>  pjproj = new PjProjProjection( o->proj ); 
	
	// create clip path
	create_clip_path_for_ortho( o->limb, InverseTransform( o->proj ) ); 

	ptr< IProjection>	clip = new ClipProjection( o->limb );
	
	ptr< IProjection>	combined = new CombineProjections( clip, pjproj ); 

	// initialize the affine
	o->affine *= agg::trans_affine_scaling( 1, -1);               // flip y
	o->affine *= agg::trans_affine_scaling( .00003 );     // scale down from metre unit
	o->affine *= agg::trans_affine_translation( +800, +200);      // translate to screen origin

	ptr< IProjection> a = new AffineProjection( o->affine ); 
	o->simple_projection = new CombineProjections( combined , a); 
	return o;
}






/*
	- shouldn't really have any deps
	- and it maintains its state
	- and can be deserialized, and events replayed etc.
*/
 


/*
	should be operations like set_affine
	or set_Pj_proj 

void ProjectionAggregateRoot::set_projection( const ptr< IProjection> & )
{

}
*/

#if 0
		double dx = x2 - x1;
		double dy = y2 - y1;

		agg::trans_affine affine = root.get_affine(); 

		affine *= agg::trans_affine_translation( dx, dy );		// align edges

		root.set_affine( affine );
#endif
#if 0
		// OK, this is the projection, which means a square ...  -180, +180, +90, -90 etc.

		// what is easiest way to do this, create a path and then test it ??? 
		// and then use the segment iterator ???
		// if we had a simple square (like an ellipse) then we wouldn't need.
		// it could be ...
		// it doesn't matter we just need to try and get something working .  
		// UGGHH - we actually want this to be 
		// *** AHHHH - we need to project it - therefore we should use a path...
		agg::path_storage	path;
		path.move_to( -180, -90 );
		path.line_to( +180, -90 );
		path.line_to( +180, +90 );
		path.line_to( -180, +90 );
		path.close_polygon();

		// project.
		path.transform( root.get_affine() );		
		path_seg_adaptor< agg::path_storage> segs( path);

		double dist = 1234;
		int cmd;
		double x1, y1, x2, y2; 
		while( !agg::is_stop( cmd = segs.segment( &x1, &y1, &x2, &y2)) )
		{
			dist = std::min( dist, pointToLineDistance( x1, y1,  x2, y2,  x, y ) ); 
		} 
		//std::cout << "dist " << dist << std::endl;
		return dist;
#endif

