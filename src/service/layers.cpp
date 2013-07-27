
#include <common/ptr.h>
#include <service/layers.h>

#include <set>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH




// not a multimap 
// typedef boost::unordered_multimap< ptr< IKey> , ptr< ILayerJob>, Hash, Pred >	objects_t;
typedef std::set<  ptr< ILayerJob> >	objects_t;



struct Layers : ILayers
{
	Layers()
		: count( 0),
		jobs()
	{ } 


	void add_ref() { ++count;  } 
	void release() { if( --count == 0) delete this; } 

	void add(  const ptr< ILayerJob>  & job )
	{
		assert( jobs.find( job) == jobs.end() ); 
		jobs.insert( job ); 
	}

	void remove( const ptr< ILayerJob>  & job  )
	{
		assert( jobs.find( job) != jobs.end() ); 
		jobs.erase( job );
	}

	void layer_update()
	{
		foreach( const ptr< ILayerJob>  & job , jobs )
		{
			job->layer_update();
		}
	}


	void post_layer_update()
	{
		foreach( const ptr< ILayerJob>  & job , jobs )
		{
			job->post_layer_update();
		}
	}
private:
	unsigned		count;
	objects_t		jobs;
};	

ptr< ILayers> create_layers_service()
{
	return new Layers; 
}



