/*
	the key is to render everything in one go when we have consistent state,  
	and not spread it out over two calls. even though we have
	to have to calls (update / expose), because we need to wait for the
	os to tell us the invalid region. 
	
	in the actual expose, we just work with bitmaps, because any of the jobs could
	have had their state (bounds, active, existance) modified, between the update
	and receiving of the expose event.
*/

#include <service/renderer.h> 

#include <common/surface.h>
#include <common/timer.h>

#include <platform/render_control.h>   // bit messy, when only need render_control interface

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

static bool compare_z_order( IRenderJob *a, IRenderJob * b )
{
	// should also put the force() criteria into the sort to displace the order.

	if( a->get_z_order() < b->get_z_order() ) return true;
	if( a->get_z_order() > b->get_z_order() ) return false;

	// make it stable
	return a < b ;
}


//typedef boost::unordered_map< ptr< IKey> , ptr< IRenderJob>, Hash, Pred >		objects_t;
//typedef boost::unordered_multimap< ptr< IKey> , ptr< IRenderJob>, Hash, Pred >		objects_t;

typedef std::set<  IRenderJob * >	objects_t;
	

};

struct Inner
{
	Inner(  IRenderControl & render_control, Timer & timer  )
		: render_control( render_control),
		timer( timer),
		passive_surface(),
		active_surface( ) ,
		combine_surface( new BitmapSurface )
	{ } 

	IRenderControl					& render_control; 
	Timer							& timer;

	//private:
	//unsigned		count;
	std::vector< IRenderJob * >		passive_set;	

	BitmapSurface					passive_surface;	
	BitmapSurface					active_surface ;

	ptr< BitmapSurface>				combine_surface; 

	// last set of jobs rendered to passive buffer
	// now we create a new surface, but don't clear it or anything 

	std::vector< Rect>				active_rects; 

	objects_t						jobs;
};


	
Renderer::Renderer( IRenderControl & render_control, Timer & timer  )
	: d( new Inner( render_control, timer)  )
{ 
	// don't use render_control yet!!! it has not been instantiated!! 

	// clear the buffer ...
//		surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );
} 


Renderer::~Renderer()
{ 
	delete d;
	d = NULL;
} 



void Renderer::notify( IRenderJob & job ) 
{
	assert( d->jobs.find( &job) != d->jobs.end() ); 

	//std::cout << "render got notify" << std::endl;

	d->render_control.signal_immediate_update(); 

//	d->jobs.insert( &job );
}

void Renderer::add( IRenderJob & job ) 
{ 
	assert( d->jobs.find( &job) == d->jobs.end() ); 

	d->jobs.insert( &job );

	d->render_control.signal_immediate_update(); 
} 

void Renderer::remove( IRenderJob & job ) 
{ 
	assert( d->jobs.find( &job) != d->jobs.end() ); 

	d->jobs.erase( &job );

	d->render_control.signal_immediate_update(); 
} 


static void draw_rect( BitmapSurface::rbase_type	& rbase, const Rect & rect, const agg::rgba8 & color  )
{ 
	// helper function

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

	d->render_control.signal_immediate_update(); 
} 


void Renderer::render_and_invalidate( std::vector< Rect> & invalid_regions ) 
{ 
	assert( invalid_regions.empty() );


	RenderParams		render_params;
	render_params.dt = d->timer.elapsed();
	d->timer.restart();



	bool require_passive_redraw = false;

	std::vector< IRenderJob * >		active_set;	


	// collect elements into a set of sorted jobs
	// it would be better to use raw pointers and avoid call overhead of virtual add_ref() and release() in log( n) in sort
	// but then we would also have to 

	active_set.clear();

	std::vector< IRenderJob * >	current_set;

	// add the set of passive jobs for this render round
	foreach( IRenderJob * job , d->jobs )
	{
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
		foreach( const IRenderJob * job, current_set ) 
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

		// WHY DO WE HAVE AN OPERATION LIKE THIS ?
		// Rendering the background ought to be a job 
		
		// clear the buffer ...
	//	d->passive_surface.rbase().clear( agg::rgba8( 0xff, 0xff, 0xff ) );

		// this has to be the passive set, to respect the z_order, otherwise we would have to re-sort

		foreach( IRenderJob *job, d->passive_set )
		{
			job->render( d->passive_surface, render_params ) ;
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
	foreach( IRenderJob *job, active_set )
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


	// render the active set
	foreach( IRenderJob *job, active_set )
	{
		job->render( d->active_surface, render_params ); 
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

//	std::cout << "active_rects " << d->active_rects.size() << std::endl;

	// for each region, draw a square
	foreach( const Rect & rect, d->active_rects )
	{
		Rect	a( rect.x, rect.y, rect.w -1, rect.h - 1); 	
		//draw_rect( d->combine_surface->rbase(), a, agg::rgba8( 0, 0, 0) ); 
		draw_rect( d->combine_surface->rbase(), a, agg::rgba8( 0xff, 0, 0) ); 
	}
#endif

	return d->combine_surface; 	
}   




