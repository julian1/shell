/*
	the key is to render everything in one go when we have consistent state,  
	and not spread it out over two calls. even though we have
	to have to calls (update / expose), because we need to wait for the
	os to tell us the invalid region. 
	
	in the actual expose, we just work with bitmaps, because any of the jobs could
	have had their state (bounds, active, existance) modified, between the update
	and receiving of the expose event.
*/

#include <common/ptr.h> 
#include <service/renderer.h> 

#include <common/surface.h>


#include <algorithm>
#include <vector>
#include <map>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH


#include <set> 
//#include <boost/unordered_map.hpp>

/*
	changed to multimap, which means everything will the same key will be removed ...
	This is really not very nice.
*/


namespace { 

static bool compare_z_order( const ptr< IRenderJob> & a, const ptr< IRenderJob>  & b )
{
	// should also put the force() criteria into the sort to displace the order.

	if( a->get_z_order() < b->get_z_order() ) return true;
	if( a->get_z_order() > b->get_z_order() ) return false;

	// make it stable
	return a < b ;
}


//typedef boost::unordered_map< ptr< IKey> , ptr< IRenderJob>, Hash, Pred >		objects_t;
//typedef boost::unordered_multimap< ptr< IKey> , ptr< IRenderJob>, Hash, Pred >		objects_t;

typedef std::set<  ptr< IRenderJob> >	objects_t;
	

};

struct Inner
{
	Inner()
		: passive_surface(),
		active_surface( ) ,
		combine_surface( new BitmapSurface )
	{ } 

	//private:
	//unsigned		count;
	std::vector< ptr< IRenderJob> >	passive_set;	

	BitmapSurface					passive_surface;	
	BitmapSurface					active_surface ;
	ptr< BitmapSurface>				combine_surface; 

	// last set of jobs rendered to passive buffer
	// now we create a new surface, but don't clear it or anything 

	std::vector< Rect>				active_rects; 

	objects_t						jobs;
};


Renderer::Renderer()
	: d( new Inner )
{ 
	// clear the buffer ...
//		surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );
} 


Renderer::~Renderer()
{ 
	delete d;
	d = NULL;
} 



void Renderer::add( const ptr< IRenderJob> & job ) 
//void Renderer::add( const ptr< IKey> & key, const ptr< IRenderJob>  & job )
{ 
	assert( d->jobs.find( job) == d->jobs.end() ); 

	d->jobs.insert( job );
} 

void Renderer::remove( const ptr< IRenderJob> & job ) 
//void Renderer::remove( const ptr< IKey> & key )
{ 
	assert( d->jobs.find( job) != d->jobs.end() ); 

	d->jobs.erase( job );
} 


static void draw_rect( BitmapSurface::rbase_type	& rbase, const Rect & rect, const agg::rgba8 & color  )
{ 
	//BitmapSurface::rbase_type	& rbase = active_surface->rbase(); 
	// std::cout << "line " << rect.x << " " << rect.y << " " << rect.w << " " << rect.h << std::endl;

	//void copy_hline(int x1, int y, int x2, const color_type& c)

	rbase.copy_hline( rect.x, rect.y, rect.x + rect.w, color );	
	rbase.copy_hline( rect.x, rect.y + rect.h, rect.x + rect.w, color );	
	
	rbase.copy_vline( rect.x, rect.y, rect.y + rect.h, color );	
	rbase.copy_vline( rect.x + rect.w, rect.y, rect.y + rect.h, color );	
}



void Renderer::resize( int w, int h ) 
{
	d->passive_surface.resize( w, h );

	d->active_surface.resize( w, h );

	d->combine_surface->resize( w, h );

	// clear the set, to force complete redraw
	d->passive_set.clear();
} 


void Renderer::update_render(  const UpdateParms & parms, std::vector< Rect> & invalid_regions ) 
{ 
	assert( invalid_regions.empty() );

	bool require_passive_redraw = false;

	std::vector< ptr< IRenderJob> >		active_set;	


	// collect elements into a set of sorted jobs
	// it would be better to use raw pointers and avoid call overhead of virtual add_ref() and release() in log( n) in sort
	// but then we would also have to 

	active_set.clear();

	std::vector< ptr< IRenderJob> >	current_set;

	// add the set of passive jobs for this render round
//	foreach( objects_t::value_type & pair , d->jobs )
	foreach( const ptr< IRenderJob > & job , d->jobs )
	{
//		const ptr< IRenderJob>  & job = pair.second; 
		int z = job->get_z_order(); 
		if(  z < 100 )
		{
			current_set.push_back( job);
		}
		else
		{
			active_set.push_back( job );
		}
	}
	
	// sort them according to their z_order
	// we want it always sorted (even if we don't test), because it becomes
	// the old vector in the next render round.
	std::sort( current_set.begin(), current_set.end(), compare_z_order ); 

	// also sort the active set for later
	std::sort( active_set.begin(), active_set.end(), compare_z_order ); 


	// check if any jobs have invalid flag set.
	if( ! require_passive_redraw)
	{	
		foreach( const ptr< IRenderJob> & job, current_set ) 
		{
			if( job->get_invalid() )
			{
				require_passive_redraw = true;
				break;
			}
		}
	}

	// check if jobs or ordering might have changed ordering
	if( ! require_passive_redraw)
	{	
		require_passive_redraw = current_set != d->passive_set ; 
	}


	// now that we've compared, set the passive_set to be the current_set
	d->passive_set = current_set ; 



	// perhaps we should pass a boolean for the area ??? and pass it back again ???
	// ok, now we invalidate the areas
	if( require_passive_redraw )
	{

		// invalidate the entire background
		invalid_regions.push_back( Rect( 0, 0 , d->passive_surface.width(), d->passive_surface.height() ));

//			std::cout << "@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;	
//			std::cout << "has changed - redrawing passive "  << passive_set.size() << std::endl; 
		
		// clear the buffer ...
		d->passive_surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );

		// this has to be the passive set, to respect the z_order, otherwise we would have to re-sort

		foreach( const ptr< IRenderJob> & job, d->passive_set )
		{
			job->render( d->passive_surface, parms ) ;
		}

	}



	// ok, now invalidate the active regions of the last draw round, because the
	// item might have moved.
	foreach( const Rect & rect, d->active_rects )
	{
		if( ! require_passive_redraw )  
		{
			invalid_regions.push_back( rect ) ;
		}
	}	

	// clear to recalculate the new bits
	d->active_rects.clear(); 


	// ok, we have to keep a list of active regions ...	

	// invalidate the active items
	foreach( const ptr< IRenderJob> & job, active_set )
	{
		double x1, x2, y1, y2; 
		job->get_bounds( &x1, &y1, &x2, &y2 ) ;  

		Rect	rect( x1, y1, x2 - x1, y2 - y1 ); 

		d->active_rects.push_back( rect) ; 

		// if we have done a passive, then the entire region is already invalided
		if( ! require_passive_redraw )  
		{
			invalid_regions.push_back( rect ) ;
		}

		// we have to copy across *all* the backgrounds in one go, before doing any of the foreground 

		copy_region( d->passive_surface, rect.x, rect.y, rect.w, rect.h, d->active_surface, rect.x, rect.y ); 
	}


	foreach( const ptr< IRenderJob> & job, active_set )
	{
		job->render( d->active_surface, parms ); 
	}

} 


/*
	IMPORTANT we can avoid double handling here, and eliminate the combine surface.
	
	we need a blit interface and then we can blit the passive and active 
	directly into the gtk. rather than into the combine and then into the gtk surface
*/

// ok, for the old regions, it ought now to 	
// return a surface, that is sufficient to cover the invalid regions argumnet

ptr< BitmapSurface> Renderer::update_expose( const std::vector< Rect> & invalid_regions ) 
{ 
	// the regions may be a superset of the regions we invalided, therefore we have to blit all passive
	// and then all active, even though everything in active is likely to be sufficient. 
	// but it should allow the convenience to also easily handle old regions by just also invaliding them
	// rather than having to calculate

	// now we work with the combined buffer.	

	// for each region copy the passive into the active 
	foreach( const Rect & rect, invalid_regions )
	{
		// std::cout << "copying region of passive into combine " << rect.x << " " << rect.y << " " << rect.w << " " << rect.h << std::endl;
		copy_region( d->passive_surface, rect.x, rect.y, rect.w, rect.h, *d->combine_surface, rect.x, rect.y ); 
	}

	// now blend over the top, the active elts
	foreach( const Rect & rect, d->active_rects )
	{
		//blend_region( active_surface, rect.x, rect.y, rect.w, rect.h, *combine_surface, rect.x, rect.y ); 
		copy_region( d->active_surface, rect.x, rect.y, rect.w, rect.h, *d->combine_surface, rect.x, rect.y ); 
	}

//	std::cout << "here1 " << combine_surface->width() << " " << combine_surface->height() << std::endl;
#if 1
	// for each region, draw a square
	// foreach( const Rect & rect, invalid_regions )
	foreach( const Rect & rect, d->active_rects )
	{
		Rect	a( rect.x, rect.y, rect.w -1, rect.h - 1); 	
		draw_rect( d->combine_surface->rbase(), a, agg::rgba8( 0, 0, 0) ); 
	}
#endif

	return d->combine_surface; 	
}   






//}; // namespace



/*
ptr< IRenderer>	create_renderer_service( ) 
{
	return new Renderer; 
}
*/


#if 0


template< class T> 
static bool ranges_are_identical( 
	T start1, const T & finish1,    
	T start2, const T & finish2) 
{     
	if( std::distance( start1, finish1) != std::distance( start2, finish2))
		return false;
	return std::equal( start1, finish1, start2 ); 
}	

#endif


/* i don't think this is right. jobs should invalidate themselves to the renderer, not the other way around.
void Renderer::invalidate()
{
	self->passive_set.clear(); 
}
*/

#if 0
	void update( const UpdateParms & parms) 
	{ 
		// collect elements into a set of sorted jobs
		// it would be better to use raw pointers and avoid call overhead of virtual add_ref() and release() in log( n) in sort
		// but then we would also have to 
		std::vector< ptr< IRenderJob> >	current_set;

		// add the set of passive jobs for this render round
		foreach( objects_t::value_type & pair , jobs )
		{
			const ptr< IRenderJob>  & job = pair.second; 
			int z = job->get_z_order(); 
			if(  z < 100 )
			{
				current_set.push_back( job);
			}
		}
		
		// sort them according to their z_order
		// we want it always sorted (even if we don't test), because it becomes
		// the old vector in the next render round.

		std::sort( current_set.begin(), current_set.end(), compare_z_order ); 

		bool require_passive_redraw = false; 

		// check if any jobs have invalid flag set.
		if( ! require_passive_redraw)
		{	
			foreach( const ptr< IRenderJob> & job, current_set ) 
			{
				if( job->get_invalid() )
				{
					require_passive_redraw = true;
					break;
				}
			}
		}

		// check if jobs or ordering might be different
		if( ! require_passive_redraw)
		{	
			require_passive_redraw = current_set != passive_set ; 
		}

		// check if the surface might have changed (should not be necessary ) 
		if( ! require_passive_redraw)
		{
			if( last_surface.width() != surface.width() 
				|| last_surface.height() != surface.height() ) 
			{
				require_passive_redraw = true;
			}
		}

		// if change in the passive items, then we redraw
		if( require_passive_redraw)
		{
			std::cout << "has changed - redrawing passive"  << std::endl;	

			last_surface.resize( surface.width(), surface.height() );
			
			// clear the buffer ...
			last_surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );

			// render the current_set jobs
			foreach( const ptr< IRenderJob> & job , current_set )
			{
				job->render( last_surface, parms ) ;
			}

			// and record the new passive job set
			passive_set = current_set;	
		}

		// ideally, we'd like not to have to copy as much, but could detect the size
		// of all the items greater than 100 and take more care blitting.
		// copy into new buffer.
		surface.copy_from(  last_surface );


		// draw the rest of the high z_order jobs 
		foreach( objects_t::value_type & pair , jobs )
		{
			const ptr< IRenderJob> & job = pair.second; 
			if( job->get_z_order() >= 100 )
			{
				job->render( surface, parms ) ;
			}
		}

		// ok, the isolines are all new.
	} 
#endif	

			// ok, we have now got a white background,   DO WE EVEN HAVE TO USE A BLEND ??? 
			// this should have worked ...
#if 0
			typedef agg::pixfmt_rgba32					pixfmt_type;
			typedef agg::renderer_base< pixfmt_type>	rbase_type;

			unsigned	flip_y( - 1); 

			agg::rendering_buffer   rbuf( 
				active_surface.buf() + ( r.y * active_surface.width() * 4) + ( r.x * 4),
				r.w, r.h, 
				(-flip_y) * active_surface.width() * 4 ); 

			pixfmt_type		pixf( rbuf);
			rbase_type		rbase( pixf);

			rbase.clear( agg::rgba8( 0xff, 0xff, 0xff ) );
#endif

