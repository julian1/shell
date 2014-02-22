
#if 0

/*
	We create this as a way to do async stuff that will help to synchronize
	animation stuff.

	ok, there's something funny. our animation service 
	peers are not services ...
*/

#pragma once



//#include <set> 
#include <map> 
#include <controller/renderer.h> 
/*
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
*/



struct IGridRendererJob 
{
	// ok, this could just be a get data interface ...
	// rather than a full blown thing ...

	//virtual void set_size( int w, int h ) = 0; 
	//virtual void set_data( int w, int h, float * ) = 0; 

	virtual void get_data( int w, int h, float *data ) = 0; 

//	virtual void get_geo_extent () 

	// No, rather than putting a projection here ... 
	// we should associate it with the current_projection ...
	// so this job has two dependencies the renderer and the list of projections ...

	// eg. we set the active projection, and add the grid, 
	// likewise set the styling ... 
	// if parameter is temp, then we set the default styling and then add the grid ...
};


struct MyJob : IRenderJob
{
	MyJob(  IGridRendererJob & inner ) 
		: inner( inner) 
	{ } 
	
	IGridRendererJob & inner;  

	// isoliner to use...
	// style
	// color 
	// thickness 

	void pre_render( RenderParams & params ) 
	{ } 

	void render ( RenderParams & params) 
	{ } 

	int get_z_order() const 
	{ }

	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{ }

};

// 

// in practice rather than rendering, this would just generate geometry I think ...
// and then another follower would generate styled geometry using default styling and render it? 


struct GridRenderer //: IGridRenderer 
{
	typedef GridRenderer this_type; 
	
	typedef std::map< IGridRendererJob *, MyJob *>		jobs_t;
	jobs_t								jobs;

	Renderer & renderer ; 

	// Projections & projections;	// to get the active projeciton to associate ... 
	// Grid & editor ;				// to add it to the grid editor ... 
	// Default & style ; 

	GridRenderer( Renderer & renderer )  
		: renderer( renderer )
	{  }	

	void add( IGridRendererJob & job ) 
	{
		MyJob *j = new MyJob( job ); 
		jobs.insert( std::make_pair( &job, j) );
		renderer.add( *j ); 	
	} 

	void set_active( IGridRendererJob & job )
	{
		// make the job change it's z_order in the renderer ...
		jobs_t::iterator i = jobs.find( & job );
		assert( i != jobs.end());
	//	MyJob & j = *i->first; 
	
	}

	void remove( IGridRendererJob & job ) 
	{
		jobs_t::iterator i = jobs.find( & job );
		assert( i != jobs.end());
		renderer.remove( *i->second ); 
		jobs.erase( i );
	}
	void notify( IGridRendererJob & job ) 
	{
		jobs_t::iterator i = jobs.find( & job );
		assert( i != jobs.end());
		renderer.notify( *i->second ); 
	}
};

#endif

