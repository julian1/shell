
#include <common/ptr.h>
#include <common/cube.h>
#include <aggregate/cube_view.h>
#include <aggregate/projection.h>

#include <common/contourer.h>			// Contour - can remove by using callback
#include <common/point_in_poly.h>
#include <common/ptr.h>
#include <common/grid.h>
#include <common/path_reader.h>
#include <common/projection.h>
#include <common/surface.h>
#include <common/path_seg_adaptor.h>

#include <common/gts_header.h>
#include <data/grib_decode.h>
#include <common/cube.h>

#include <service/services.h>
#include <service/position_editor.h>
//#include <service/contourer.h>
//#include <service/projector.h>
#include <service/renderer.h>
#include <service/grid_editor.h>
#include <service/level_controller.h>
#include <service/valid_controller.h>

#include <agg_bounding_rect.h>
#include <agg_conv_stroke.h>
#include <agg_pixfmt_rgba.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_ellipse.h>

#include <algorithm>
#include <cmath>
#include <map>

#include <boost/functional/hash.hpp>	
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH


/*
	We want a view structure. for the grid. 

	- the input grid
	- the specific contourer
	- the contour values policy.
	- the callbacks from the contourer
	- the adaptors for  
	- the style policy eg. value = modulo %  20, thick line
	- perhaps meta stuff eg. change in validity or level. we don't care at the moment.

	- and get rid of the keys.	
	- these things ought to be able to be injected in. and delegated to.  eg. 
	- and all the state for the above
	- the grid can be injected in. eg. or factored to be a separate aggregate 
	- and possibly add the event sourcing (doesn't matter). 


	- it doesn't matter how we will ultimately sequence. We will work it out.	

	- we ought to be able to get rid of the inner Contour also	
*/


namespace { 

#if 0

struct Contour
{
	/*
		we could combine this state with the ContourAdaptor. keeping it separate, means
		we can do some things more easily. 
	*/
	// This is not really a model object, since it will always be wrapped in the model.
	Contour( double value, const agg::path_storage & path)
		: count( 0),
		value( value),
		path( path)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	double				value;
	agg::path_storage	path;
private:
	unsigned count;
};

#endif


#if 0
struct GridHelper
{
	/*
		The georeference applies to the grid and is therefore a grid property 
		However, the projection is view specific. 
		The adaptors might want to create their own interface
	*/

	virtual void add_ref() = 0;
	virtual void release() = 0;

/*
	// the grid could be told about all contours if really wanted
	virtual void add_contour(  Services & services, Contour * contour, projection ) = 0;	
	virtual void remove_contour( Services & services, Contour * contour, projection ) = 0;
*/
	virtual ptr< Grid> get_grid() const = 0; 
	virtual void set_grid( const ptr< Grid> & ) = 0; 

	virtual const agg::trans_affine & get_geo_reference() const = 0;
	virtual void set_geo_reference( const agg::trans_affine & ) = 0; 

	virtual void set_active( bool ) = 0;
	virtual bool get_active() const = 0;

	// we don't expose invalid
	virtual bool get_invalid() = 0;
	virtual void clear_invalid() = 0;


	// as a test we could put level controls on here .

};
#endif

struct GridHelper //: GridHelper
{
	/*
		class needs to be combined with the CubeSelector
	*/

	/*
		this class ought to be able to be removed. it is no longer the state holder 

		the transaction boundaries should really be placed on the cube.
	*/

	// - all the state and transaction boundaries
	// - the event_source reference should never leave here, as all state and modifiers to state are here.
	// - and serializeable
	// - and typed. 
	// - the vector for isolines however is
	// similar to a layer but could actually represent more than one layer.
	// we don't want to put logic on here, but we have to , for . 

	 
	unsigned						count;

	ptr< Grid>						grid;

	agg::trans_affine				geo_reference;

	bool							active;
	bool							invalid;
/*
	// possible local state and other aggregate references
	unsigned				criteria;
	unsigned				isoline_style_preferences;
	bool					contours_acgive; 
	bool					grid_active;
*/	
	//	contours_type	contours;


	GridHelper(  )
		: count( 0),
		grid(),
		geo_reference(),
		active( false ),
		invalid( true )	
	{ }

	~GridHelper()
	{ } 

	void add_ref() {++count; } 
	void release() { if( --count == 0 ) delete this; } 

	/*
	int GridHelper::get_z_order() 
	{
		return 100;			// because new contours are generated every time ...
							
		}
	*/

	ptr< Grid> get_grid() const 
	{
		return grid;
	}

	void set_grid( const ptr< Grid> &grid_ ) 
	{
		invalid = true;
		grid = grid_; 
	}

	const agg::trans_affine & get_geo_reference() const 
	{
		return geo_reference;
	}

	void set_geo_reference( const agg::trans_affine & affine_ ) 
	{
		invalid = true;
		geo_reference = affine_;
	}
	
	void set_active( bool active_ ) 
	{
		invalid = true;
		active = active_;
	}
	bool get_active() const 
	{
		return active;
	}
	
	bool get_invalid() 
	{
		return invalid;
	}
	void clear_invalid() 
	{
		invalid = false;
	}

};



/*
	we should use combine projection for this ???.
	or keep flat ?doesn't really matter.
*/
struct GridProjection : IProjection
{



	GridProjection( const ptr< IProjectionAggregateRoot> & projection_aggregate, const ptr< GridHelper> & root  )
		: count( 0),
//		inner( inner),
		projection_aggregate( projection_aggregate),
		root( root )
	{ }  

	~GridProjection()
	{ }

	void add_ref() { ++count; }
	void release() { if( --count == 0) delete this; }

	void forward( double *x, double * y ) 
	{ 
		root->get_geo_reference().transform( x, y);
		projection_aggregate->get_projection()->forward( x, y );
	} 
	void reverse( double *x, double * y ) 
	{ 
		projection_aggregate->get_projection()->reverse( x, y );
		agg::trans_affine tmp = root->get_geo_reference();
		tmp.invert();				// very slow. but little used.
		tmp.transform( x, y );
	} 
	void forward( const agg::path_storage & src_, agg::path_storage & dst, bool clip ) 
	{ 
		agg::path_storage src = src_;
		src.transform( root->get_geo_reference() );	
		projection_aggregate->get_projection() ->forward( src, dst, clip);
	} 
	void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) 
	{ 
		projection_aggregate->get_projection()->reverse( src, dst, clip );
		agg::trans_affine tmp = root->get_geo_reference();
		tmp.invert();				// somewhat slow, but little used.
		dst.transform( tmp );
	} 
private:
	unsigned count;	
	ptr< IProjectionAggregateRoot> projection_aggregate;
//	ptr< IProjection>		inner; 
	ptr< GridHelper>	root;
};



struct ControlPoint;

struct IControlPointCallback
{
	virtual void move( ControlPoint & sender,  int x1_, int y1_, int x2_, int y2_ ) = 0;
	// control passes from the IRenderJob, therefore it's easier to request the position, although
	// it's not tell don't ask.

	virtual void get_position( ControlPoint & sender, double * x, double * y ) = 0;  
//	virtual void get_position( struct ControlPoint & sender, double * x, double * y ) = 0; 


	virtual void set_active( bool active_ ) = 0; 
}; 



struct ControlPoint 
	: IPositionEditorJob, 
	IRenderJob 
{
	ControlPoint( IControlPointCallback & callback )
		: count( 0),
		callback( callback ),
		path(),
		position_editor_active( false)
	{ 

		// don't do stuff in the constructor !!!
	} 
	
	unsigned					count;
	IControlPointCallback		& callback; 
	agg::path_storage			path;	
	bool						position_editor_active;


	void pre_render( RenderParams & render_params) {  }
/*
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine(seed, this );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< ControlPoint> arg = dynamic_pointer_cast< ControlPoint>( key ); 
		if( !arg) return false;
		return ( this == &*arg ); 
	} 
*/

	void update_path( )
	{
		// use the path to hittest ???
		double r = 5; 
		double x = 0, y = 0; 
	
		callback.get_position( *this, &x, &y ); 
		agg::ellipse e( x, y, r, r, (int) 10);
		path.remove_all();	
		path.concat_path(  e);	
	}

	// IRenderJob
	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		update_path( );

		 //bounding_rect_single( path_reader( projected_path), 0, x1, y1, x2, y2);	
		 bounding_rect_single( path , 0, x1, y1, x2, y2);	

		// we should adjust the rect by the maximum stroke.
	} 

	void render( BitmapSurface & surface, RenderParams & render_params )
	{
		if( ! position_editor_active )
			return ;

//		update_path( );
			 
		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( path );

		agg::rgba       color( .3, 0, 1, .5);
		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), color );
	}

	bool get_invalid() const 
	{
		assert( 0); // at 100
		return false;
	}	
	int get_z_order() const 
	{
		return 100;
	}; 

	// IPositionEditorJob
	void move( int x1_, int y1_, int x2_, int y2_ )  
	{
		callback.move( *this , x1_, y1_, x2_, y2_ ) ;
	}

	void set_active( bool active_ ) 
	{ 
		// the individual point	
		callback.set_active( active_ );
	}
	void set_position_active( bool active ) 
	{ 
		position_editor_active = active; 
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



struct PositionsController :  /*ILayerJob, */ IControlPointCallback
{
	/*
		this is run from the post job. but doesn't rely on its calls 
		it adds all the control points for our grid georeference 

	*/
	PositionsController( Services & services, const ptr< GridHelper> & root, const ptr< IProjection> & grid_projection  ) 
		: count( 0),
		services( services),
		root( root),
		grid_projection( grid_projection )
	{  
		/*
			why isn't the constructor being called ???!!!

			Because of the template Combine function ?
		*/

		// these are doing callbacks before they are fully constructed...
		// and this class is constructed.

		// these make callbacks, so delay until now
		top_left =  new ControlPoint( *this ) ; 
		top_right =  new ControlPoint( *this ) ; 
		bottom_left =  new ControlPoint( *this ) ; 
		bottom_right =  new ControlPoint( *this ) ; 

		services.renderer.add( *top_left  );
		services.position_editor.add( *top_left );

		services.renderer.add( *top_right  );
		services.position_editor.add( *top_right );

		services.renderer.add( *bottom_left  );
		services.position_editor.add( *bottom_left );

		services.renderer.add( *bottom_right  );
		services.position_editor.add( *bottom_right );
	} 

	~PositionsController()
	{

		services.renderer.remove( *top_left );
		services.position_editor.remove( *top_left );

		services.renderer.remove( *top_right );
		services.position_editor.remove( *top_right );

		services.renderer.remove( *bottom_left );
		services.position_editor.remove( *bottom_left );

		services.renderer.remove( *bottom_right );
		services.position_editor.remove( *bottom_right );
	}

	unsigned					count;
	Services					& services; 
	ptr< GridHelper>	root; 
	ptr< IProjection>			grid_projection ;

	ControlPoint *	top_left; 
	ControlPoint *	top_right; 
	ControlPoint *	bottom_left; 
	ControlPoint *	bottom_right; 

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

/* 
	// ILayer
	void post_update( ) { }  
	void pre_update( ) { }  
*/
	void set_active( bool active ) 
	{
		// if a control point becomes active, then set aggregate active
		// to force it to 100 level.
		// the set_active interface could
		root->set_active( active ); 
	} 

	void get_position( ControlPoint & sender, double * x, double * y ) 
	{
		if( &sender == top_left )
		{
			*x = 0;
			*y = 0; 
		}
		else if( &sender == top_right )
		{
			*x = root->get_grid()->width(); 
			*y = 0; 
		}
		else if( &sender == bottom_left )
		{
			*x = 0;
			*y = root->get_grid()->height(); 
		}
		else if( &sender == bottom_right )
		{
			*x = root->get_grid()->width(); 
			*y = root->get_grid()->height(); 
		}
		else assert( 0);

		grid_projection->forward( x, y ); 
	}


	// callback
	void move( ControlPoint & sender,  int x1_, int y1_, int x2_, int y2_ ) 
	{
		// means we have correctly hittested
		// grid projection just takes us to the affine space
		double x1 = x1_, y1 = y1_, x2 = x2_, y2 = y2_ ;  
		grid_projection->reverse( &x1, &y1 );
		grid_projection->reverse( &x2, &y2 );
		double dx = x2 - x1; 
		double dy = y2 - y1; 

		double w = root->get_grid()->width();
		double h = root->get_grid()->height();
		/*
			this is correct, it's not the point's job to know what position it is, and it's relation with the rect.
			either we route here, or we inject the dependency.
		*/
		agg::trans_affine	x;

		if( &sender == top_left )
		{
			// centre the rhs on the 0 origin
			x *= agg::trans_affine_translation( - w, -h);			
			x *= agg::trans_affine_scaling( (-dx + w ) / w , (-dy + h) / h );
			x *= agg::trans_affine_translation( + w, + h );			
		}
		else if( &sender == top_right )
		{
			x *= agg::trans_affine_translation( 0, -h);			
			x *= agg::trans_affine_scaling( (dx + w ) / w , (-dy + h) / h );
			x *= agg::trans_affine_translation( 0, + h );			
		}
		else if( &sender == bottom_left )
		{
			x *= agg::trans_affine_translation( - w, 0);			
			x *= agg::trans_affine_scaling( (-dx + w ) / w , (+dy + h) / h );
			x *= agg::trans_affine_translation( + w, 0 );			
		}
		else if( &sender == bottom_right)
		{
			//x *= agg::trans_affine_translation( 0, 0);			
			x *= agg::trans_affine_scaling( (dx + w ) / w , ( dy + h) / h );
			//x *= agg::trans_affine_translation( 0, 0 );			
		}
		else assert( 0);

		// apply existing translation
		x *= root->get_geo_reference(); 
		root->set_geo_reference( x ); 
	}


};





struct GridEditJobAdapt : IGridEditorJob
{
	// OK, this will need both the projection and the geo_reference
	// Should we just inject the grid and grid_georeference rather than pass the root ???
	// ISP etc.

	// we will have setters and getters for the root grid remember.

	GridEditJobAdapt( const ptr< GridHelper> & root, const ptr< IProjection> & grid_projection )
		: count( 0) ,
		root( root),
		grid_projection( grid_projection )
	{ } 

	unsigned					count;
	ptr< GridHelper>	root;
	ptr< IProjection>			grid_projection; 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 


	ptr< Grid> get_grid() 
	{
		return root->get_grid();
	}
	void set_grid( const ptr< Grid> & grid_ )  
	{
		root->set_grid( grid_ );
	}

	void transform( double *x, double *y ) 
	{
		grid_projection->reverse( x, y );
	}

	void set_active( bool active ) 
	{
		root->set_active( active );	
	}

};









#if 0

	do we wrap and access state. 
	or do we delegate passing down state. 

struct ContourRenderStrategy
{
	// can be injected. or copied etc. hmmm. not sure. we would have to put quite a lot of state in here ?  

	void render ( BitmapSurface & surface, const UpdateParms & parms ) 
	{

		//std::cout << "render contour" << std::endl;
		// const agg::path_storage & path = contour->projected_path ;
		path_reader	reader( projected_path );

		agg::conv_stroke< path_reader>	stroke( reader );

		if( root->get_active() )
			stroke.width( 2); 
		else
			stroke.width( 1); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		// whether the grid is active is a part of the model.
		// contour->contour->contour->grid_source ;//->get_active_state();

		/*
			agg::rgba8 c = active_state->get_active() 
				? agg::rgba8( 0, 0, 0xff ) 
				: agg::rgba8( 0xff, 0, 0) ;
		*/
		// agg::rgba8       c( 0xff, 0, 0);
		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), color );
		//std::cout << "here" << std::endl;
	}

};

#endif


struct Contour : IRenderJob // , IProjectJob
{
	// this is a wrapped contour.

	/*
		we require the proj_aggregate reference, in order to determine
		if it's invalid() and we have to re-project.
	*/
	Contour( const ptr< GridHelper> & root, 
			const ptr< IProjection>  & projection, 
			const ptr< IProjectionAggregateRoot> & projection_aggregate, 
			double value, 
			const agg::path_storage & path
	)
		: count( 0),
		root( root),
		projection( projection ),
		projection_aggregate( projection_aggregate ),

		value( value),
		path( path ),

	//	contour( contour),
		projected_path(),
		color( 0, 0, 0xff )
		//bool	active; 
	{ } 	

	unsigned					count;
	ptr< GridHelper>	root;
	ptr< IProjection>			projection;
	ptr< IProjectionAggregateRoot> projection_aggregate;

	double				value;
	agg::path_storage	path;

	// ptr< WrappedContour>		contour;
	//ptr< Contour>				contour;		// value, path
	agg::path_storage			projected_path;
	// line style preferences
	// projected_path
	// styled_path
	// renderjob
	agg::rgba8					color;
	bool						active; 


	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

//	void project() { }

	/*
		ok, this is not so hot, because it is going to generate a bounding rect for every single contour
		which could be expensive.
	*/	


	void pre_render( RenderParams & render_params ) 
	{  }

	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		 //bounding_rect_single( path_reader( projected_path), 0, x1, y1, x2, y2);	
		 bounding_rect_single( projected_path, 0, x1, y1, x2, y2);	

		// we should adjust the rect by the maximum stroke.
	} 


	void layer_update()
	{
		if( get_invalid() )
		{	
			//std::cout << "projecting  contour" << std::endl;
			projection->forward( path, projected_path, true  );
		}

	}

	void render ( BitmapSurface & surface, RenderParams & render_params )
	{



		//std::cout << "render contour" << std::endl;
		// const agg::path_storage & path = contour->projected_path ;
		path_reader	reader( projected_path );

		agg::conv_stroke< path_reader>	stroke( reader );

		if( root->get_active() )
			stroke.width( 2); 
		else
			stroke.width( 1); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		// whether the grid is active is a part of the model.
		// contour->contour->contour->grid_source ;//->get_active_state();

		/*
			agg::rgba8 c = active_state->get_active() 
				? agg::rgba8( 0, 0, 0xff ) 
				: agg::rgba8( 0xff, 0, 0) ;
		*/
		// agg::rgba8       c( 0xff, 0, 0);
		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), color );
		//std::cout << "here" << std::endl;
	}

	bool get_invalid() const 
	{
		if( root->get_invalid() ) return true;
		if( projection_aggregate->get_invalid() ) return true;
		return false;
	}	

	int get_z_order() const 
	{
		// ok, if we are changing the position/ georeference of the grid . 
		// then we need to set_active and to force it to 100 

		if( root->get_active()  ) 
			return 100;
		return 10; 
	}; 

};


// here we trace out the refernces and inject into services (
// and create a reference
struct ContoursController :  /*ILayerJob,*/ IContourCallback
{
	/*
		checks if the grid root is invalid, and generates isolines if it needs to.
	*/
	/*
		note, how we could still (if really wanted) insert all the contours into the same aggregate if we really wanted, even
		if they are in different projections. we could even add them with their  projection argument if we wanted. 
	*/
	/*
		we require the proj aggregate, to pass to the individual Contour 
	*/
	ContoursController( 
		Services & services, 
		const ptr< GridHelper> & root, 
		const ptr< IProjection> & grid_projection, 
		const ptr< IProjectionAggregateRoot> & projection_aggregate  
	)
		: count( 0),
		services( services),
		root( root),
		grid_projection( grid_projection ),
		projection_aggregate( projection_aggregate )//,
//		contours()
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 


	// callback 
	void add_contour( /* int id */ double value, const agg::path_storage & path ) 
	{
//		ptr< Contour> contour = new Contour( value, path ) ; 
//		contours.push_back( contour ); 

		//ptr< IKey>	key = make_key(  contour ); 

		//ptr< Contour> o = new Contour( root, grid_projection, projection_aggregate, contour ); 

		ptr< Contour> o = new Contour( root, grid_projection, projection_aggregate, value, path ); 

		contours.push_back( o );

//		ptr< IKey>	key = make_key(  o ); 

		services.renderer.add(  *o );
		//services.projector.add_project_job( key, o ); 
	} 

	// we have to know the contours, so that we can remove them from the services.
	std::vector< ptr< Contour> >		contours;


#if 0
/*
	We don't have a means to identify the contour, to know if it is being removed ?????
*/
	// not a callback, but just a helper.
	void remove_contour_( const ptr< Contour> & contour ) 
	{
		ptr< IKey>	key = make_key( ptr< Contour>( contour) ); 

		services.renderer.remove( key );
		services.projector.remove_project_job( key );
	}

#endif

	void layer_update() 
	{ 

		// we don't care if the projection is invalid, we only care if the grid has changed due to a grid edit
		// How do we coordinate the projection ????
		// simple, just test the projection->get_invalid()  


		if( root->get_invalid() ) 
		{
			// because the grid includes the georeference, changing the georeference will create new contours 
			// but it's ok for now
			// std::cout << "grid is invalid - recalc contours" << std::endl;
		
			// clear old contours
			foreach( const ptr< Contour> & contour, contours )
			{
//				remove_contour_( contour );
		//		ptr< IKey>	key = make_key(  contour ); 

				services.renderer.remove( *contour );
		//		services.projector.remove_project_job( key );

			}
			contours.clear() ; 


			ptr< Grid> grid = root->get_grid();
			assert( grid);

			double min = * std::min_element( grid->begin(), grid->end() );
			double max = * std::max_element( grid->begin(), grid->end() );
		
			double range = max - min;				

			std::vector< double> values;
			for( double f = min - range *.1; 
				f < max + range * .1; 
				f += range / 10 )
			{
				values.push_back( f); 
			}

#if 0
			// values
			std::vector< double> values;
			for( double f = -10; f < 10; f += 0.2 )
			{
				values.push_back( f); 
			}
#endif

			// make them
			make_contours2( *root->get_grid(), values, * this );   


			// now do our update for them
			foreach( const ptr< Contour> & contour, contours )
			{
				contour->layer_update();
			}


		}	
	} 

	void post_layer_update() 
	{ 
		root->clear_invalid();
	} 
	
//	void add_contour( Contour *contour ) {  }  
//	void remove_contour( Contour *contour ) { }  


private:
	unsigned					count;
	Services					& services;
	ptr< GridHelper>	root;
	ptr< IProjection>			grid_projection;
	ptr< IProjectionAggregateRoot> projection_aggregate; 
//	std::vector< ptr< Contour> >	contours; 
};


// the big complicated object should be assembled externally . 
//  but fundamentally what's the point ????  unless it delegates ??? 

// advantages
// we can remove the services ref from delegated to stuff, and we can get rid of the key . 
// and we can remove the root aggregate,  because we don't require the is_dirty() reference


// and it's a bit clearer what's going on.




struct CubeSelector 
	: ILevelControllerJob,
	 IValidControllerJob
{
	/*
		class needs to be combinedf with the GribHelper 

	*/

	/*
		This class should probably be injected into the other bits, that require access to grid
		Rather than having the grid injected into this and the other classes.
	*/

	/*
		Cube wrapper. or CubeSelector 
		CubeSelector 
	*/
	/*
		coordindate between the cube and the active root

		This is likely to be highly data type specific. Eg. A cube grib. needs to compare 

		Actually metadata like runtime should not be placed here.

	*/	
	CubeSelector( const ptr< ICube> & cube,  const ptr< GridHelper> & root_ ) 
		: count( 0),
		cube( cube ), 
		root( root_ )
	{ 

		// get an initial grid

		std::vector< ptr< SurfaceCriteria> > criterias = cube->get_available_criterias();

		// we have to create a new criteria because we will modify this one ... 
		current_criteria = criterias.at( 0 ); 

		ptr< Grid> grid = cube->get_grid( current_criteria ); 
		root->set_grid( grid) ;  
		
	
/*	
		assert( cube->gribs.size() );
		current = cube->gribs.at( 0); 

		root->set_grid( current->grid );

		agg::trans_affine	geo_ref; 
		//	geo_ref *= agg::trans_affine_scaling( .7 );
		root->set_geo_reference( geo_ref );
*/
	} 


	ptr< ICube>				cube;
//	ptr< Grid>				current_grid;   we don't need to maintain the current grid here. it is available   
	ptr< SurfaceCriteria>	current_criteria; 
// we should be getting rid of this. in favor of just sharing a refernce to the grid, or something 
	ptr< GridHelper>		root; 

	//IMPLEMENT_REFCOUNT ; 


	void add_ref() { ++count; }
	void release() { if( --count == 0 ) delete this; }

	bool get_active() 
	{ 
		//assert( 0 );
		return true; // ??? not being used 
	} 

	// may not be required, unless need global movement 
	// actually still need to know if the projection is active.
	void get_projection()  
	{ } 

	ptr< Level> get_level() const
	{
		return current_criteria->level;  
	} 

	void set_level( const ptr< Level> & level ) 
	{ 
		// create new criteria, using the level
		current_criteria 
			= new SurfaceCriteria( 
				current_criteria->param, 
				level ,	 
				current_criteria->valid, 
				current_criteria->area 
		); 

		std::cout << "selector to " 
			<< *current_criteria->param << ", " 
			<< *current_criteria->level << ", " 
			<< *current_criteria->valid << ", " 
			<< *current_criteria->area  
			<< std::endl; 
				
		//  get the grib and set it
		ptr< Grid> grid = cube->get_grid( current_criteria ); 
		assert( grid);
		root->set_grid( grid) ;  
	}  


	std::vector< ptr< Level> >  get_available_levels()  const
	{
		// get all the criterias
		std::vector< ptr< SurfaceCriteria> > criterias = cube->get_available_criterias();

		std::vector< ptr< Level > >		levels; 

		// find those that match the crit except for the level 

		for( int i = 0; i < criterias.size(); ++i )
		{
			const ptr< SurfaceCriteria> & criteria = criterias.at( i ); 

			if( *criteria->param == *current_criteria->param 
				&& *criteria->area == *current_criteria->area 
				&& *criteria->valid == *current_criteria->valid 
			)
			{
	//			std::cout << "'" << *criteria->param << "' level " << *criteria->level << std::endl;
				levels.push_back( criteria->level ); 		
			}
		}

		std::cout << "matching levels " << levels.size() << std::endl;
		return levels; 
	}


	///////////////////

	void set_valid( const ptr< Valid> & valid ) 
	{  
		current_criteria 
			= new SurfaceCriteria( 
				current_criteria->param, 
				current_criteria->level, 
				valid, 
				current_criteria->area 
		); 

		std::cout << "selector to " 
			<< *current_criteria->param << ", " 
			<< *current_criteria->level << ", " 
			<< *current_criteria->valid << ", " 
			<< *current_criteria->area  
			<< std::endl; 
		
		ptr< Grid> grid = cube->get_grid( current_criteria ); 
		assert( grid);
		root->set_grid( grid) ;  
	}  

	ptr< Valid> get_valid() const  
	{ 
		return current_criteria->valid;  
	} 
	
	std::vector< ptr< Valid> >  get_available_valids() const 
	{ 
		// get all the criterias
		std::vector< ptr< SurfaceCriteria> > criterias = cube->get_available_criterias();
		std::vector< ptr< Valid > >		valids; 

		foreach( const ptr< SurfaceCriteria> & criteria, criterias ) 
		{
			if( *criteria->param == *current_criteria->param 
				&& *criteria->area == *current_criteria->area 
				&& *criteria->level == *current_criteria->level 
			)
			{
				//std::cout << "'" << *criteria->param << "' valid " << *criteria->valid << std::endl;
				valids.push_back( criteria->valid ); 		
			}
		}
		std::cout << "matching valids " << valids.size() << std::endl;
		return valids; 
	}

private:
	unsigned count;
}; 






struct ViewLayer 
	: IGridEditorJob, 
	ILevelControllerJob  ,
	IValidControllerJob  //,
//	ILayerJob 
{
	ViewLayer ( 
		const ptr< ContoursController> 			& contours_controller,
		const ptr< IGridEditorJob>				& grid_edit_job_adaptor , 
		const ptr< PositionsController>			& positions_controller,
		const ptr< CubeSelector>			& cube_selector 
	)
		:  count( 0),
		contours_controller( contours_controller ),
		grid_edit_job_adaptor (  grid_edit_job_adaptor ), 
		positions_controller( positions_controller ),
		cube_selector ( cube_selector ) 
	{ 
/*
		// now this stuff  can be put in the actual view ...
		services.post.add( view ); 
		services.grid_editor.add(  view  ); 
		services.level_controller.add( view );
*/
	} 

	~ViewLayer()
	{

	}

	unsigned count;

	ptr< ContoursController> 	contours_controller;
	ptr< IGridEditorJob>		grid_edit_job_adaptor;  
	ptr< PositionsController>	positions_controller;
	ptr< CubeSelector>			cube_selector; 

	// used for all interfaces
	void add_ref() { ++count ; } 
	void release() { if( --count == 0 ) delete this ; } 


	// IGridEditorJob delegation
	ptr< Grid> get_grid() { return grid_edit_job_adaptor->get_grid(); } 
	void set_grid( const ptr< Grid> & grid )  { grid_edit_job_adaptor->set_grid( grid ); } 
	void transform( double *x, double *y )  { grid_edit_job_adaptor->transform( x, y ); }  
	void set_active( bool value ) { grid_edit_job_adaptor->set_active( value ); } 


	// ILevelControllerJob  delegation
	bool get_active() { return cube_selector->get_active(); } 
	void get_projection() { return cube_selector->get_projection(); }  

	void set_level( const ptr< Level> & level ) { cube_selector->set_level( level ) ; }  
	ptr< Level> get_level() const  { return cube_selector->get_level();  } 
	std::vector< ptr< Level> >  get_available_levels() const { return cube_selector->get_available_levels();  }

	// IValidControllerJob delgatoin
	void set_valid( const ptr< Valid> & valid ) { cube_selector->set_valid( valid ) ; }  
	ptr< Valid> get_valid() const  { return cube_selector->get_valid();  } 
	std::vector< ptr< Valid> >  get_available_valids() const { return cube_selector->get_available_valids();  }



/*
	// ILayerJob delegation
	void layer_update( ) { contours_controller->layer_update(); }  
	void post_layer_update( ) { contours_controller->post_layer_update(); }  
*/

	std::string get_name() { return "cube view"; }

};




};	// end of anon namespace


/*
	Why do we even need the grid_aggregate_root. 

	Why not just inject the grid into the controllers and adaptors
*/


/*
	change the name of this. create_new_view of the cube. 
*/

void create_cube_view( Services & services, const ptr< ICube> & cube , const ptr< IProjectionAggregateRoot> & projection_aggregate )
{
	/*
		we want this builder class. because the assembly of complex objects should be separated

	*/

	/*
		- So, the way this works, we create a wrapped projection that includes the grid georeference
		and we inject that down into the adaptors, for them to use.
		- it might be better if we just passed the projection and the root, and let them create
		their own adaptors as they like. but since they are all the same, we can drag this up and out. 	



		why not have a slightly richer interface than IProjection ???
		that will expose the projection
	*/

	/*
		VERY IMPORTANT. I suspect the level editor should be handled outside of the loading of the view. 
	*/

//	assert( cube->gribs.size() ) ; 
//	assert( cube->gribs[ 0]->grid  );

	ptr< GridHelper> root = new GridHelper ; 



	ptr< IProjection> grid_projection =  new GridProjection( projection_aggregate, root );

	ptr< ContoursController> contours_controller = new ContoursController( services, root, grid_projection, projection_aggregate ) ; 
	
	ptr< IGridEditorJob>  grid_edit_job_adaptor = new GridEditJobAdapt( root, grid_projection ); 

	ptr< PositionsController> positions_controller = new PositionsController( services, root, grid_projection ); 

	ptr< CubeSelector> cube_selector = new CubeSelector( cube, root ) ; 


	ptr< ViewLayer >  view = new ViewLayer (  
		contours_controller, 
		grid_edit_job_adaptor, 
		positions_controller, 
		cube_selector  
	); 


	// the loading and unloading could be put in the actual view class ...
	//services.layers.add( view ); 
	services.grid_editor.add(  view  ); 
	services.level_controller.add( view );
	services.valid_controller.add( view );
};


void remove_grid_aggregate_root( Services & services, const ptr< GridHelper> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate ) 
//void remove_grid_aggregate_root( Services & services, const ptr< Cube> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate ); 
{

//	ptr< IKey> key_ = make_key( root, projection_aggregate ); 

	// 

/*
	services.position_editor.remove( key_ );
	//services.grid_editor.remove( key_ ); 
	services.post.remove( key_ ); 
	services.post.remove( key_ );
*/
	assert( 0 );
}



#if 0

ptr< GridHelper> create_grid_aggregate_root( ) 
{
	return new GridHelper;

#if 0
	ptr< GridHelper> o = new GridHelper;

	o->set_grid( make_test_grid( 100, 100) ); 

	agg::trans_affine	geo_ref; 

	geo_ref *= agg::trans_affine_scaling( .7 );
		
	o->set_geo_reference( geo_ref );

	return o; 
#endif
}

#endif

#if 0

struct WrappedContour
{
	WrappedContour( const ptr< Contour> & contour )
		: count( 0), 
		contour( contour ),
		color( 0, 0, 0xff),
		active( false)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	// ALL state should appear here, both computed state and true model state
	// this is local to the aggregate root.

	//ptr< Contour>	contour;	// value, path
	ptr< Contour>		contour;	// value, path

	agg::path_storage	projected_path;

	// line style preferences
	// projected_path
	// styled_path
	// renderjob
	agg::rgba8	color;
	bool		active; 


private:
	unsigned count;
};

#endif

/*
	ok, we can combine the WrappedContour state ?
*/

/*
	OK, at the moment every contour gets an individual render job,  
	we certainly have to wrap the contourer.
	This maybe ok	
*/

/*
struct ContourProjectJobAdapt : IProjectJob
{
	ContourProjectJobAdapt( const ptr< IProjection>  & projection, const ptr< WrappedContour> & wrapped_contour )
		: count( 0),
		projection ( projection),
		wrapped_contour( wrapped_contour)
	{ } 

	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  

	void project()
	{
		projection->forward( wrapped_contour->contour->path, wrapped_contour->projected_path, true  );
	}
	unsigned				count;
	ptr< IProjection>		projection;
	ptr< WrappedContour>	wrapped_contour;
};

*/



/*
	Ok, ContourJob provides us with our contours, 

	We are going to have to maintain the services reference.
	and when we get a contour,  we wil

	We will create the wraped contour in here, and then inject it into the renderer service. 	

	It's a job delegating to a job - which is interesting.
*/


#if 0
struct ContourKey : IKey
{
	ContourKey( Contour *contour ) 
		: count( 0),
		contour( contour )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine(seed, contour );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< ContourKey> arg = dynamic_pointer_cast< ContourKey>( key ); 
		if( !arg) return false;
		return contour == arg->contour ; 
	}  
private:
	unsigned	count;
	Contour		*contour;
};
#endif



/*
template< class A, class B> 
struct CombineKey : IKey
{
	// it would be possible to delegate to manage multiple combining
	// no, because it's too messy to dynamic cast into the child for the equality 

	CombineKey( const ptr< A> & a, const ptr< B> & b)
		: count( 0),
		a( a),
		b( b )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine(seed, &*a );
	    boost::hash_combine(seed, &*b );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 

		ptr< CombineKey> arg = dynamic_pointer_cast< CombineKey>( key ); 
		if( !arg) return false;
		return a == arg->a && b == arg->b;
	}  
private:
	unsigned	count;
	ptr< A>		a;
	ptr< B>		b;
};

template< class A, class B> 
ptr< IKey> make_key( const ptr< A> & a, const ptr< B> & b)
{
	return new CombineKey< A, B>( a, b);
}
*/

#if 0
	double hit_test( unsigned x, unsigned y ) 
	{
		// we should do this when set_grid() is called in the model, 
		// and make the path a member of the model 

		return 1234;	// never test
	}
	void set_active( bool active_ ) 
	{ }
	
	void set_position_active( bool active ) 
	{ 

		// THE PROBLEM IS THAT WE ADD NEW ENTRIES

		// IT IS ITERATOR INVALIDATION ...


		if( active)
		{
				}
		else
		{
			}
	} 	
#endif

