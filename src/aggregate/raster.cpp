
#include <aggregate/raster.h>
#include <aggregate/projection.h>

#include <common/load_png.h>
#include <common/projection.h>
#include <common/path_reader.h>

#include <service/renderer.h>
#include <service/services.h>

/*
	ok, we need a limb as well ???
*/

#include <agg_bounding_rect.h>
#include <agg_rasterizer_scanline_aa.h>
//#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_u.h>

#include <boost/functional/hash.hpp>	

namespace {


// so the fill will take the root to get the image and the proj, and an affine ??? 
// actually the affine could be locally generated 
// note that we want to avoid wrapping.

struct RenderJob : IRenderJob
{
	RenderJob( const ptr< IRasterAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate )
		: count( 0),
		root( root),
		projection_aggregate( projection_aggregate )
	{ } 

	unsigned						count;
	ptr< IRasterAggregateRoot>		root;
	ptr< IProjectionAggregateRoot>	projection_aggregate ; 

	agg::trans_affine				affine;	// should be passed explicitly ??

//	Fill						fill;
	/*agg::path_storage	& path;
	bool				& active;
	double				offset;  */

	// ok, should we try to get some scaling and sampling working ?

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 


	void blend( BitmapSurface::rbase_type & dst, int x_, int y_, int len, agg::int8u *covers ) const
	{
		//std::cout << "whoot blend y=" << y << " x=" << x << " len=" << len << std::endl;
		//my_projection.transform ; 

		ptr< IProjection> proj = projection_aggregate->get_projection();

		//affine.
		BitmapSurface::rbase_type & src = root->get_surface()->rbase();

		for( unsigned i = 0; i < len; ++i)
		{
			double x = x_ + i; 
			double y = y_; 
			proj->reverse( &x, &y );
			// so it should be lat-long now

//			std::cout << "x " << x << " " << y << y << std::endl;
			affine.transform( &x, &y );

//			std::cout << "after x " << x << " " << y << std::endl;

			agg::rgba8 color = src.pixel( x, y );

			dst.blend_pixel( x_ + i, y_, color, covers[ i]);
		}
#if 0
		for( unsigned i = 0; i < len; ++i)
		{
			agg::rgba8	color( 0x7f, 0x7f, 0x7f, 0x3f );	
			rbase.blend_pixel( x + i, y, color, covers[ i]);
		}
#endif
	}



	void output_scanlines( 
		agg::rasterizer_scanline_aa<>	&ras, //		Rasterizer& ras, 
		agg::scanline_u8				&sl,  // Scanline& sl, 
		// const T1 &screen_to_latlon,
		// const T2 &latlon_to_data,
		// const Renderable &image_src, 
		BitmapSurface::rbase_type & rbase		//	BaseRenderer& ren,
		//const IFill  & filler,
		//int interlace
	) {
		// we could actually put the rasterizer and sweepline in here if we really wanted. 
		if(ras.rewind_scanlines())
		{
			sl.reset( ras.min_x(), ras.max_x());

			while( ras.sweep_scanline(sl))
			{
				int y = sl.y();

				// agg doesn't have a nice normal end() iterator, eg this doesn't work,
				// for( agg::scanline_u8::const_iterator span = sl.begin(); span != sl.end(); ++span );

				unsigned num_spans = sl.num_spans();

				//typename agg::scanline_u8::const_iterator span = sl.begin();
				agg::scanline_u8::const_iterator span = sl.begin();
	#if 0 
				// ugghhhh - if we call this with two threads ... 
				if( interlace >= 0 && y % 2 != interlace) 
					continue;
	#endif
				for(;;)
				{
					int x = span->x;
					if(span->len > 0)
					{
						// ok, rather than using this loop, this is where we factor the interface 
						// VERY IMPORTANT - WE DON"T EVEN NEED THE FUCKING RENDERER IN HERE
						// Instead 
						/*filler.*/blend( rbase, x, y, span->len, span->covers  );	
					}
					else
					{
						assert( 0);
						// seems that only the packed sl calls this 
						// ren.blend_hline(x, y, (unsigned)(x - span->len - 1), ren_color, *(span->covers));
					}
					if(--num_spans == 0) break;
					++span; // span is an iterator 
				}
			}
		}
	}



	void get_bounds( double *x1, double *y1, double *x2, double *y2 ) 
	{
		
		agg::path_storage	proj_limb ;

		projection_aggregate->get_projection()->forward( 
			projection_aggregate->get_limb(), proj_limb, false );			// should flag, not to clip

		bounding_rect_single( proj_limb, 0, x1, y1, x2, y2);	
	}


	void render ( BitmapSurface & surface, const UpdateParms & parms ) 
	{
//		std::cout << "whoot render raster" << std::endl;
//		return;

		// set up the affine
		ptr< BitmapSurface> src = root->get_surface();

		//std::cout << "src width " << src->width() << "    " << src->height() << std::endl;

		affine.reset();

		affine *= agg::trans_affine_scaling( 1., -1.  ); 

		affine *= agg::trans_affine_translation( 180., 90. ); 
		affine *= agg::trans_affine_scaling( src->width() /  360. ,   src->height() /  180.  ); 


		agg::path_storage	proj_limb ;

		projection_aggregate->get_projection()->forward( 
			projection_aggregate->get_limb(), proj_limb, false );			// should flag, not to clip

		path_reader	path( proj_limb );

		agg::rasterizer_scanline_aa<> ras;
		ras.clip_box( 0, 0, surface.width(), surface.height());	

		ras.add_path( path );
		agg::scanline_u8 sl;
	
		output_scanlines( ras, sl, surface.rbase()/*, fill*/ /*, 0*/ );  

#if 0
		ptr< BitmapSurface>	x = root->get_surface();
		assert( x);

		BitmapSurface::rbase_type & src = x->rbase(); 
		BitmapSurface::rbase_type & dst = surface.rbase(); 

		// this is a very naive approach direct blit
		for( unsigned h = 0; h < src.height(); ++h)
		for( unsigned w = 0; w < src.width(); ++w)
		{
			dst.copy_pixel( w, h, src.pixel( w, h) );
		}
#endif

		// ok, so we have to put together stuff with the georeference and the projection.
		// but can we just try to test blit this image.
	}


	// ok, changing the z_order will likely force a redraw  
	// but there is an added problem. 
	// it doesn't force redraws at other times. 
	// therefore we need to set the 	


	bool get_invalid() const 
	{
		/*
			this ought to be sufficient - to always force redraw. 	
			But, it means everything gets redrawn at every animation step which is a bit nasty. 
				- but it's up to the render to structure this stuff.

			- we have to do the same invalid thing, for the contours when they are active.

			
		*/	
		if( projection_aggregate->get_active() )
			return true;

		return false;
	}
	
	int get_z_order() const  
	{
		return 0; 
/*
		if( projection_aggregate->get_active() )
			return -100;
		else 
			return 0;//root->get_z_order();
*/
	}

};


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

}; // namespace


void add_raster_aggregate_root( Services & services, const ptr< IRasterAggregateRoot> & root , const ptr< IProjectionAggregateRoot> & projection_aggregate )
//void add_raster_aggregate_root( Services & services, const ptr< IRasterAggregateRoot> & root, const ptr< IProjection> & proj, const agg::path_storage & limb  )
{
	//std::cout << "add raster agg root" << std::endl;

	//ptr< IKey> key = make_key( root, projection_aggregate ); 
	//void *key = &*root;

	services.renderer.add( new RenderJob( root, projection_aggregate ) ); 

}

void remove_raster_aggregate_root( Services & services, const ptr< IRasterAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate  )
{
	assert( 0 ); 
	//ptr< IKey> key = make_key( root, projection_aggregate ); 
	//void *key = &*root;

//	services.renderer.remove( key); 
}



struct RasterAggregateRoot : IRasterAggregateRoot
{
	RasterAggregateRoot() 
		: count( 0),
		surface(),
		affine()
	{ 
	//	std::cout << "raster agg root constructor" << std::endl;
	} 

	void add_ref() 
	{ 
		++count;
	} 

	void release() 
	{  
		if( --count == 0)
			delete this;
	}

	ptr< BitmapSurface> get_surface() const 
	{  
		return surface;
	}  

	void set_surface( const ptr< BitmapSurface> & surface_ ) 
	{  
		surface = surface_;
	}  

	void set_geo_reference( const agg::trans_affine & affine_ ) 
	{  
		affine = affine_;
	}  

private:
	unsigned		count;
	ptr< BitmapSurface>	surface; 
	agg::trans_affine affine;
};



ptr< IRasterAggregateRoot> create_test_raster_aggregate_root()
{

	ptr< IRasterAggregateRoot> root = new RasterAggregateRoot;

	ptr< BitmapSurface> texture = load_png_file( "./data/BlueMarble4096x2048.png" );
	root->set_surface( texture );

	return root;

	// ok, so we have to load an image into the surface, before we can display, let alone handle the projection

}




/*
struct IFill 
{
	virtual	void blend( BitmapSurface::rbase_type & rbase, int x, int y, int len, agg::int8u *covers )  = 0; 
};
*/

#if 0
struct Fill //: RasterRenderComponent::IFiller
{
	Fill(/* MyProjection & my_projection */)  
//			: my_projection( my_projection)
	{ } 

//		MyProjection & my_projection;

	void blend( BitmapSurface::rbase_type & rbase, int x, int y, int len, agg::int8u *covers ) const
	{
		//std::cout << "whoot blend y=" << y << " x=" << x << " len=" << len << std::endl;

		//my_projection.transform ; 

		for( unsigned i = 0; i < len; ++i)
		{
			agg::rgba8	color( 0x7f, 0x7f, 0x7f, 0x3f );	
			rbase.blend_pixel( x + i, y, color, covers[ i]);
		}
	}
}; 
#endif



#if 0
         /*result =*/ read_png_file( "shapes/Blue_Marble_Geographic.png", latlon_image);
./old/old2/renderer1.cpp.cpy2:          /*result =*/ read_png_file( "shapes/BlueMarble4096x2048.png", latlon_image);
./old/old2/renderer1.cpp.cpy2://                /*result =*/ read_png_file( "shapes/DEM_ACE_30sec_RA_or.png", latlon_image);
./old/old2/renderer1.cpp.cpy2://                /*result =*/ read_png_file( "shapes/elev_bump_8k.png", latlon_image);
#endif


