
#if 0

#include <aggregate/shapes.h>
#include <aggregate/projection.h>
#include <common/projection.h>

//#include <controller/shapes.h>
//#include <controller/position.h>
#include <controller/renderer.h>
//#include <controller/projector.h>
#include <controller/services.h>

#include <controller/fonts.h>
#include <controller/labels.h>


//#include <common/path_seg_adaptor.h>
#include <common/path_reader.h>
//#include <common/update.h>
#include <common/bitmap.h>


#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
#include <agg_bounding_rect.h>

#include <vector>
#include <map>
#include <set>
#include <boost/functional/hash.hpp>	
#include <boost/variant.hpp> 


#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH



namespace {

/*
	- the Shape is no longer a wrapper or composed of internal Shape and wrapper Shape.... 
	- the full state, is part of the aggregate collection of elts. 
	- if something wants to give the aggregate a shape. then it has to give the full shape.
	- if we really don't want this and want defaults on state (eg color/style), then we impose a wrapper, but this occurs 
		outside the aggregate.
	- we are currently using this wrapper for loading right now. 	

	---
	we have also removed the shape service and updating.


	THIS IS A WRAPPED SHAPE.
		

	It has a projected path.
*/



struct ShapesAggregateRoot : IShapesAggregateRoot
{
	ShapesAggregateRoot( )
		: count( 0)
	{ }  

	unsigned			count;


	std::string			filename;

	typedef std::map< int, ptr< ISimpleShape > >	shapes_t; 
	shapes_t			shapes; 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 


	void subscribe( IShapesObserve & subscriber )
	{
		subscribers.insert( & subscriber );

		// add the current shapes
		foreach( const shapes_t::value_type pair, shapes )
		{
			subscriber.add_shape( pair.second );
		}
	}
	void unsubscribe( IShapesObserve & c )
	{
		subscribers.erase( & c);
	}

	std::set< IShapesObserve *>		subscribers;


	// the loader has to be able to access a single shape ...
	// so it can set the attributes ...

	ptr< ISimpleShape > get_shape( int id ) const
	{
		shapes_t::const_iterator i = shapes.find( id);	
		return  i != shapes.end() ? i->second : ptr< ISimpleShape>() ;  
	}

	void add_shape( const ptr< ISimpleShape > & shape )
	{
		/*  wrap it to add local state, and create an adaptor to render it */
		//WrappedShape *shape = new WrappedShape( shape ); 

		/* There is not so much point having a separate projector from renderer
		// if the projection can perform this task.
		// EXCEPT it may be faster to GROUP projection tasks, according to the limb. */

		// note that we will push the removing of duplicate geomeotry out  of this ?
		// not sure. it's easy enough either way.

		// We can make the key the inner contour, and then we wouldn't have to store anything...

		foreach( IShapesObserve *subscriber, subscribers )
		{
			subscriber->add_shape( shape );
		}

		shapes.insert( std::make_pair( shape->get_id(), shape ));

	}

	void remove_shape( const ptr< ISimpleShape > & shape )
	{


	}

	void set_filename( const std::string & _  )
	{
		filename = _ ; 
	}
};




struct ShapeAdaptor : IRenderJob//, ILabelJob 
{
	// handle projection, label and render.


	ShapeAdaptor( Services & services, const ptr< IProjectionAggregateRoot> & projection_aggregate, const ptr< ISimpleShape> & shape)
		: count( 0),
		services( services),
		projection_aggregate( projection_aggregate ),
//		root( root),
		shape( shape),
		projected_path(),
		color( 0, 0, 0xff),
		free_of_intersections_( false )
	{ } 


	unsigned					count;
	Services					& services; 
	//	ptr< IShapesAggregateRoot>	root;
	ptr< IProjectionAggregateRoot>	projection_aggregate; 

	ptr< ISimpleShape >			shape;

	agg::path_storage	projected_path;
	// line style preferences
	// styled_path
	// renderjob
	agg::rgba8	color;


	agg::path_storage			text_path;		// path of the text
	bool						free_of_intersections_;

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 



	// ok, it is not anti aliased, because it's being drawn for every shape 
	// we need to find the centre of the shape, etc and then draw it. 

	// important, we must have the shape already correctly projected before we attempt to do this.
	// which makes the coordination a bit complicated.

	// ok, ut just requires that the projection, be synchronized. 



	template< class T> 
	static void centre_of_poly( /*const*/ T &in, double *ex, double *ey )
	{
		// should use bounds - not try and weight by point density
		// but this is not correct either 

		double x1, y1, x2, y2; 
		bounding_rect_single( in, 0, &x1, &y1, &x2, &y2); 
		*ex = (x1 + x2) * .5f; 
		*ey = (y1 + y2) * .5f; 
	}

	/*
		- we did the project
		- OK the big thing is to avoid projection for normal render and label. 
		- i think it's actually ok. 
		- don't think we really want the separate projection 

		- we suppress everything. 
	*/
#if 0
	void project()
	{
		// BUT WE ALSO DON't WANT TO REPROJECT IF WE ARE GOING TO SUPPRESS ...
		// lets try this to begin with.
		// no lets move it into the label code, where we have these tests. - in a bit. 
		// return;
		// if( shape->get_invalid() ) std::cout << "shape is invalid" << std::endl;
		// if( projection_aggregate->get_invalid() ) std::cout << "projection is invalid" << std::endl;

		if( get_invalid()  )
		{
			// how and why is this being called ????
			// static int count = 0;
			// std::cout << "shape projecting " << ++count << std::endl;
			projection_aggregate->get_projection()->forward( shape->get_path(), projected_path, true  );
		}
	}
#endif

	/*
		we have a problem that we still have to compute the projection
		if the z_order changes ??
		there are too many things interacting ...

		VERY IMPORTANT.
		The renderer may ask, to render everything as it likes. it has it's own reason 
		for requiring the draw. we can't refuse. 

		render gets called any time. but including if we are suppressing ??????
	*/

	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		// we have to add the label as well ....
		//add_rect( Rect( x1, y1, x2, y2 )) ;  

		bounding_rect_single( projected_path , 0, x1, y1, x2, y2);	
	}		

	void pre_render( RenderParams & params) 
	{  }

	void render ( RenderParams & params) 
	{
		// path_reader	reader( path );
		// agg::conv_stroke< agg::path_storage >	stroke( path );
		// stroke.width( 1); 
	
		/*
			the renderer and labeller have to be allowed to run at any time
		
			two many interactions 
		*/
		if( free_of_intersections_  )
		{	
			// render the label
			agg::scanline_p8                sl;
			agg::rasterizer_scanline_aa<>   ras;
			ras.add_path( text_path );
			agg::render_scanlines_aa_solid( ras, sl, params.surface.rbase(), color );
		}

		// render the outline
		{
		path_reader	reader( projected_path );

		agg::conv_stroke< path_reader>	stroke( reader );
		stroke.width( 1); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );
		agg::render_scanlines_aa_solid( ras, sl, params.surface.rbase(), color );
		}
	}

	// ILabel
	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		// the thing is, is this always being called ????
		// it may not be if, if the label z_order is greater than cldwdwit's active ...

		// ILabel always called before IRender
		// we could do the projection of the shape here, if we wanted.  
		// we are also dependend on having the projection calculated the shape bounds also 
		// OK, it seems ok, to calculate the text_path here ... 
		// it will always be called if there's a change ??????

		// if( shape->get_invalid() ) std::cout << "shape is invalid" << std::endl;
		// if( projection_aggregate->get_invalid() ) std::cout << "projection is invalid" << std::endl;

		

		if( get_invalid() ) 
		{
		
			// construct text
			{
			text_path.free_all();
			services.fonts.set_face( "./data/Times_New_Roman.ttf", 12 ); 

			double advance = 0; 
			const std::string & text = shape->get_text(); 
			for( size_t i = 0; i < text.size(); ++i)
			{ 
				const Glyph & glyph = services.fonts.get_glyph( text.at( i) );
				// const Glyph &g = self.fonts.get_glyph(  );
				agg::path_storage letter = glyph.path; 
				letter.translate( advance, 0 ); 
				text_path.concat_path( letter );  
				advance += glyph.advance_x;
			}
			}

			// position text in centre of shape 
			{
			double ax, ay; 
			path_reader		reader( projected_path );
			centre_of_poly( reader, &ax, &ay ); 
			agg::trans_affine	a; 
			a *= agg::trans_affine_translation( ax , ay );
			text_path.transform( a); 	
			}

	
		}

		// We always have to set the text path 
		// set the bounding rect
		{
			double x1_, y1_, x2_, y2_; 
			bounding_rect_single( text_path, 0, &x1_, &y1_, &x2_, &y2_); 
			*x1 = x1_; *y1 = y1_; *x2 = x2_; *y2 = y2_;	
			// double w = (x2 - x1);
			// double h = (y2 - y1); 
		}

	}

	// ILabel
	void free_of_intersections( bool value ) 
	{
		// callback from the label service, indicating if we are free
		//std::cout << "free of intersections " << value << std::endl;
		free_of_intersections_ = value;
	}

	// both IRender and ILabel
	bool get_invalid() const 
	{
		// VERY IMPORTANT if the text changes, or the shape geom changes, or proj changes, then we have to set to true.
		// to force the labeller to update

		// we should really ask the shape, not the root. 
		// and the shape should ask the proj, ????  

		// shouldn't we just change the z_order ???

		//if( projection_aggregate->get_active() )	don't think this is right. we either suppress or move it into always draw
		//	return true;
													// we also have to be careful to avoid projections

		if( shape->get_invalid()
			|| projection_aggregate->get_invalid() )
			return true;

		return false; 
	}	

	// both IRender and ILabel
	// we can distinguish label_z_order() if we wanted
	int get_z_order() const 
	{
		// zorder is used for labelling as well ...
		// should delegate to the root. because z_order is 
		// might be a local property, if shared
		// might want to be +1, so that if we fill it appears over the top 

		int z =  projection_aggregate->get_active() ? 5 + 100 : 5; 
	
		return z; 
	};



#if 0
	void layer_update() 
	{ 
		// eg. now we do the projection in the layer update.

		if( get_invalid()  )
		{
			// how and why is this being called ????
			// static int count = 0;
			// std::cout << "shape projecting " << ++count << std::endl;
			projection_aggregate->get_projection()->forward( shape->get_path(), projected_path, true  );
		}

	} 

	void post_layer_update()
	{
		shape->clear_invalid(); 
		projection_aggregate->clear_invalid();
	}

#endif

};





struct ShapesLayer : /*ILayerJob,*/ IShapesObserve
{

	ShapesLayer( Services & services, const ptr< IShapesAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate )
		: count( 0),
		services( services),
		root( root),
		projection_aggregate( projection_aggregate )
	{ 
	} 

	~ShapesLayer()
	{ 
	} 


	void load()
	{
//		services.layers.add( ptr< ILayerJob>( this ) ); 
		root->subscribe( *this );
	}	

	void unload()
	{
		// 

		root->unsubscribe( *this );
	}

	unsigned					count;
	Services					& services; 
	ptr< IShapesAggregateRoot>	root;
	ptr< IProjectionAggregateRoot>	projection_aggregate; 


	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

	std::set< ptr< ShapeAdaptor>  >		shapes;

	void add_shape( const ptr< ISimpleShape> & shape ) 
	{
		ptr< ShapeAdaptor> w = new ShapeAdaptor( services, projection_aggregate, shape ); 

		shapes.insert( w );

	//	services.renderer.add(  *w );
//		services.labels.add(  w );
	}

	void remove_shape( const ptr< ISimpleShape > & shape ) 
	{
		assert( 0);

	}

	void modify_shape( const ptr< ISimpleShape > & shape ) 
	{ 
		// OK, this can be implemented, by injecting a ref to the aggregate root into the shape,
		// so that when it's modified. 
		// Ok, but how will this place with event sourcing as well ???
		// we want to rewrap the shape
	
	} 

#if 0
	void layer_update() 
	{ 
		/*
			we don't have to do this. 
			we could probably just project anytime.  unless need geometry for labelling. 
		*/ 
		foreach( const ptr< ShapeAdaptor> & adaptor , shapes )
		{
			adaptor->layer_update(); 
		}
	} 



	void post_layer_update() 
	{ 
		foreach( const ptr< ShapeAdaptor> & adaptor , shapes )
		{
			adaptor->post_layer_update(); 
		}
	} 
#endif

	std::string get_name() { return "shapes"; }
};




};	// anon namespace


ptr< IShapesAggregateRoot > create_shapes_aggregate_root( ) 
{
	return new ShapesAggregateRoot; 
}


void load_shapes_layer( Services & services, const ptr< IShapesAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate)
{
	ptr< ShapesLayer> layer = new ShapesLayer( services, root, projection_aggregate ); 
	layer->load();


};


/*
void remove_shapes_aggregate_root( Services & services, const ptr< IShapesAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate )
{
	assert( 0);
//	ptr< IKey> key = make_key( root, projection_aggregate ); 
//	services.post.remove( key ); 
} */

#include <sstream>

#include <common/load_shapefile.h>

namespace {



struct MySimpleShape : ISimpleShape
{
	MySimpleShape( int id, const std::string & text, const agg::path_storage & path )	// And pass a reference to the aggregate root. for the modify callback
		: id( id),
		text( text),
		path( path)
	{ } 

	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  


	void clear_invalid() 
	{ }

	bool get_invalid() const 
	{
		return false;
	}

	int get_id() const 
	{
		return id;
	}	
	const agg::path_storage & get_path() const 
	{
		return path;
	}	
	void set_text( const std::string & text_ ) 
	{
		text = text_;
	}
	const std::string & get_text() const 
	{
		return text;
	}

private:
	unsigned count;
	int id;
	agg::path_storage	path;
	std::string			text;
};



struct ShapeLoadCallback : IShapeLoadCallback
{
	ShapeLoadCallback(  const ptr< IShapesAggregateRoot> & root )
		: root( root)
	{ } 

	void add_shape( int id, const agg::path_storage & path ) 
	{
		// std::cout << "add shape id " << id << std::endl;
		/*
			These will all be setters
			does the object have to be added before we can manipulate is for ES purposes ?
		*/
		ptr< ISimpleShape> o = new MySimpleShape( id, std::string(), path ); 	
		root->add_shape( o );
	}
private:
	ptr< IShapesAggregateRoot>	root; 
};


struct ShapeAttributeLoadCallback : IShapeAttributeLoadCallback
{
	ShapeAttributeLoadCallback(  const ptr< IShapesAggregateRoot> & root )
		: root( root)
	{ } 

	void add_attr( int id, const std::string & name, const attr_t & attr ) 
	{
		// std::cout << "add attribute id " << id << " " << name << " " << attr << std::endl;
		if( name == "NAME" )
		{
			std::stringstream	ss;
			ss << attr;		// applies visitor
			//std::cout << "name " << ss.str() << std::endl;
			ptr< ISimpleShape > o = root->get_shape( id );  
			assert( o );
			o->set_text( ss.str() );
		} 
	}
private:
	ptr< IShapesAggregateRoot>	root; 
};


};	// anon namespace


void load_test_shapes( const std::string & filename, const ptr< IShapesAggregateRoot> & root )
{
	root->set_filename( filename );

	ShapeLoadCallback c( root );
	load_shapes( filename, c ); 

	ShapeAttributeLoadCallback  c2( root);	
	load_shape_attributes( filename, c2 ); 
}



/*
	ok, it is vastly simpler. and good. 1/3 the code size of having the grideditsource and grid and gridsource etc 
	
	- the adaptor wraps the entire aggregate, - it can be externally created

	- we don't have components that reference their task.

	- we've remove the boilerplate of wrapping simple objects (actually may need to do this for events).

	- it would be possible to put the event_stream in the adaptor, (no because, there maybe different controllers
		on same model, and we want the replay() on the aggregate.
	
	- we are not restricted in the number grids in the same aggregate, but we would need to create different adaptors
	that referenced them

	- it is nice how whether something is active, just gets threaded back to the aggregate

	- it's nice that the service is responsible for selecting.

	- having everything in one place (all state), is the only way to ensure we can get 
	references out to everything. 

	- we can do computed update() 
	
	- we solve the adding and removing issue

	- the grid edit doesn't care about grid source, it just gets and sets through the adaptor.

	- we can aggregate fundamentally different operations if we want (eg. project and style), but
		normally things have to be pushed out. eg. for projection we have to have fast projection.
*/



// #include <agg_conv_dash.h>
// #include <cmath>
#if 0
struct WrappedShape
{
	WrappedShape( Shape * shape )
		: count( 0), 
		shape( shape ),
		color( 0, 0, 0xff),
		active( false)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 

	ptr< Shape>			shape;	

	agg::path_storage	projected_path;

	// line style preferences
	// styled_path
	// renderjob
	agg::rgba8	color;
	bool		active; 
private:
	unsigned count;
};
#endif

/*
	ok, we have to move the shape out of the projection as well ...
*/

#if 0
struct ShapeProjectJobAdapt : IProjectJob
{
	// OK, why not make the shape carry the projection aggregate. 
	// BECAUSE THE LABELLER NEEDS IT AS WELL AS THE PROJECTOR

	// Then we can also query if the projector is active ??????????

	// NO. Because the shape wants to be able to be loaded into different projections.

	ShapeProjectJobAdapt( const ptr< IProjectionAggregateRoot> & projection_aggregate , const ptr< WrappedShape> & shape )
		: count( 0),
		projection_aggregate( projection_aggregate ) ,
		shape( shape)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

	void project()
	{
		// BUT WE ALSO DON't WANT TO REPROJECT IF WE ARE GOING TO SUPPRESS ...
		// lets try this to begin with.
		// no lets move it into the label code, where we have these tests. - in a bit. 

		return;

		if( shape->get_invalid() ) std::cout << "shape is invalid" << std::endl;
		if( projection_aggregate->get_invalid() ) std::cout << "projection is invalid" << std::endl;

		if( shape->get_invalid()
			|| projection_aggregate->get_invalid() )
		{
			// how and why is this being called ????
			static int count = 0;
			std::cout << "shape projecting " << ++count << std::endl;
			projection_aggregate->get_projection()->forward( shape->get_path(), shape->projected_path, true  );
		}
	}

	unsigned						count; 
	ptr< IProjectionAggregateRoot>	projection_aggregate ;
	ptr< WrappedShape>			shape;
};
#endif


#if 0
struct ShapePostAdapt : ILayerJob
{
	// we oguht to just about put all these operations on the actual shape. 
	// or a single shape wrapper.

	ShapePostAdapt(  const ptr< WrappedShape> & shape )
		: count( 0),
		shape( shape)
	{ } 
	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 
	void post()
	{
		shape->clear_invalid(); 
	}
	unsigned						count; 
	ptr< WrappedShape>			shape;
};
#endif

#if 0
struct ShapeRenderJobAdapt : IRenderJob
{
	ShapeRenderJobAdapt( const ptr< IShapesAggregateRoot> & root, const ptr<  WrappedShape> & shape)
		: count( 0),
//		root( root),
		shape( shape)
	{ } 

	unsigned					count;
//	ptr< IShapesAggregateRoot>	root;
	ptr< WrappedShape>		shape;

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

	void render ( Bitmap & surface, const UpdateParms & parms ) 
	{
		path_reader	reader( shape->projected_path );

		agg::conv_stroke< path_reader>	stroke( reader );
		stroke.width( 1); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), shape->color );
	}

	bool get_invalid() const 
	{
		return shape->get_invalid(); 
	}	

	int get_z_order() const 
	{
		return shape->get_z_order(); 
	};
};
#endif



/*
	communications, 


*/


#if 0
struct ShapesJobAdapt : IShapesJob
{
	/*
		This class should be depricated, and the loading moved into a command object,
		which would add to the aggregate.
	*/
	ShapesJobAdapt( ShapesAggregateRoot & root )
		: root( root)
	{ } 
	ShapesAggregateRoot & root; 

	std::string get_filename() 
	{
		return "data/world_adm0";  
	}

	/*
		VERY IMPORTANT - THESE METHODS COULD INTERCEPT THE ADDING OF THE 
		No- because we have to add default state (eg. line color ), which
		is a model/aggregate property.
	*/
	void add_shape( Shape * shape ) 
	{
		root.add_shape( shape ); 
	} 
	void remove_shape( Shape * shape ) 
	{
		root.remove_shape( shape ); 
	} 
};

#endif


#if 0
struct WrappedShape //: IKey
{
	WrappedShape( const ptr< ISimpleShape >	& shape )
		: count( 0), 
		shape( shape ),
		projected_path(),
		color( 0, 0, 0xff),
		invalid( false)
	{ } 

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 


	/*
		All of this state will need to be hidden behing accessors
		for event sourcing .
	*/
	const std::string & get_text()
	{
		return shape->get_text(); 
	}
/*
	VERY IMPORTANT WE CAN PROPAGATE
	THIS TO ENABLE GEOMETRY EDITING.

	void set_path( const agg::path_storage & path_ )
	{
		shape->set_path( path_ ); 
	}
*/

	void set_color( const agg::rgba8 & color_ )
	{
		color = color_;
	}

	const agg::path_storage & get_path() const
	{
		return shape->get_path(); ;
	}

	// projection changes, are not represented here. 
	// these are delegated to from both the shape and label rendering. 
	
	// OK, hang on, is the projection projecting everything multiple times ??????

	int get_z_order() const
	{
		// local, or keep in the object ???
		return 5; 
	}
	bool get_invalid() const 
	{
		return invalid;
	}	

	void clear_invalid()
	{
		invalid = false;	
	}

private:

	unsigned count;

	ptr< ISimpleShape >		shape; 

public:
	agg::path_storage	projected_path;


	// line style preferences
	// styled_path
	// renderjob
	agg::rgba8	color;

	bool		invalid;

//	bool		active; 
};

#endif



//	agg::path_storage	test_animation;
//	bool				test_animation_active;

// - all the state and transaction boundaries
// - the event_source reference should never leave here, as all state and modifiers to state are here.
// - and serializeable
// - and typed. 
// - the vector for isolines however is
// similar to a layer but could actually represent more than one layer.
// we don't want to put logic on here, but we have to , for . 


	// geo_reference *= agg::trans_affine_translation( 50, 50 );
	// geo_reference *= agg::trans_affine_scaling( 1.6);

	// the grid projection should be passed in along with the grid...
	// Actually NO. the grid_affine should be associated with the grid. but not the full projection
	// which extends beyond this.
	// grid_affine *= agg::trans_affine_scaling( .3 );
	// grid_projection =  new GridProjection( projection, grid_affine );


	// render and move  
	//self->services.renderer.add( 0, new RenderJobTestAnim( self->test_animation, self->test_animation_active ) );
	//self->services.position_editor.add( 0, new  PositionEditorJobTestAnim ( self->test_animation, self->test_animation_active ) ); 

	// We are grouping all the state that we can in one place, and then injecting with adaptors

	/*
		OK, I think this does allow 
			creation of isolines
			proj, rendering
			also styling
			removal of isoline (on data edit).
		
		- and I think that individual isoline edit, will work with generations
	*/
/*
	//typedef std::map< ptr< Contour>, ptr< WrappedContour > >	contours_type;			// filled in by the isoliner 
	typedef std::map< ptr< Contour>, ptr< WrappedContour > >	contours_type;			// filled in by the isoliner 
	contours_type	contours;
*/
	/*
		Extremely important.

		Note, how we have the flexibility to wrap all the shapes, and give them individual
		render adapators. Or store them, and create a single render job, that wraps the aggregate root,
		and would render all shapes in a single job. 
	*/

	/*
		OK extremely important to remove the services reference from here, 

		we should factor the add_shape() and remove_shape() functions into an adaptor 
		that carries the services. 

		Do the same for the contours stuff.
	*/	
#endif
