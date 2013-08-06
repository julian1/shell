/*
	a simple way to handle multiple labels, is just to give two options, and let the labeller
	make a choice. Or rather than get bounds we get a set of options.
*/
/*
	labeling will work almost exactly the same as the renderer.
*/
//#include <common/key.h>

#include <common/quadtree.h>
#include <service/labels.h>

#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
//#include <boost/unordered_map.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH



namespace { 


static bool compare_z_order( const ptr< ILabelJob> & a, const ptr< ILabelJob>  & b )
{
	// should also put the force() criteria into the sort to displace the order.

	if( a->get_z_order() < b->get_z_order() ) return true;
	if( a->get_z_order() > b->get_z_order() ) return false;

	// make it stable
	return a < b ;
}

// multimap . uggh.
//typedef boost::unordered_multimap< ptr< IKey> , ptr< ILabelJob>, Hash, Pred >	objects_t;


typedef std::set<  ptr< ILabelJob> >	objects_t;


struct Labels : ILabels
{
	Labels()
		: count( 0),
		jobs(),
		last_set(),
		//quad_tree( QuadRect( -5000, -5000, 5000, 5000) )
		root()
	{ } 


	void add_ref() { ++count;  } 
	void release() { if( --count == 0) delete this; } 


	void add( const ptr< ILabelJob> & job) 
	//void add( const ptr< IKey> & key, const ptr< ILabelJob>  & job )
	{
		assert( jobs.find( job) == jobs.end() ); 
		jobs.insert( job );
	}

	void remove( const ptr< ILabelJob> & job ) 
	//void remove( const ptr< IKey> & key )
	{
		assert( jobs.find( job) != jobs.end() ); 
		jobs.erase( job );
	}

	void update() 
	{
		//std::cout << "@@@@   labels update" << std::endl;

		// vector for sorted jobs
		std::vector< ptr< ILabelJob> >	current_set;

		// add the set of passive jobs for this render round
		foreach( const ptr< ILabelJob>  & job , jobs )
		{
	//		const ptr< ILabelJob>  & job = pair.second; 
			int z = job->get_z_order(); 
			if(  z < 100 )	// i think we want this test. if it's > 100, we don't want to recalculate the label
										// as part of the passive set. 
			{
				current_set.push_back( job);
			}
		}
		
		// sort them
		// we want it always sorted (even if we don't test), because it becomes
		// the old vector in the next render round.
		std::sort( current_set.begin(), current_set.end(), compare_z_order ); 

		bool has_changed = false; 

		// check if any jobs have invalid flag set.
		if( ! has_changed)
		{	
			foreach( const ptr< ILabelJob> & job, current_set ) 
			{
				if( job->get_invalid() )
				{
					has_changed = true;
					break;
				}
			}
		}

		// check if jobs or ordering might be different
		if( ! has_changed)
		{	
			has_changed = current_set != last_set ; 
		}
/*
		// check if the surface might have changed (should not be necessary ) 
		if( ! has_changed)
		{
			if( last_surface.width() != surface.width() 
				|| last_surface.height() != surface.height() ) 
			{
				has_changed = true;
			}
		}
*/

		// if change in the passive items, then we redraw
		if( has_changed)
		{
			//std::cout << "---------------" << std::endl;	
			std::cout << "label set has changed - recalculating all labels "  << jobs.size() << " jobs" << std::endl;	

			// clear the old tree
			//quad_tree.clear( QuadRect( -5000, -5000, 5000, 5000) );
			root = new Node< int>( QuadRect( -5000, -5000, 5000, 5000) );


			// so we will regenerate the quad tree with the items. 

			foreach( const ptr< ILabelJob> & job , current_set )
			{
				int x1 = 0, y1 = 0, x2 = 0, y2 = 0; 
				job->get_bounds( &x1, &y1, &x2, &y2 ) ; 

				//std::cout << "bounds " << x1 << " " << y1 << " " << x2 << " " << y2 << std::endl;

                // no intersections then we are good
				//size_t nnn = quad_tree.intersections_count( QuadRect( x1, y1, x2, y2) ) ; 
				size_t nnn = candidate_intersections_count( root, QuadRect( x1, y1, x2, y2) ) ; 
			
				// std::cout << "intersections " << nnn << std::endl;
	
                if( nnn == 0)
                {
					job->free_of_intersections( true );	
					//quad_tree.insert( QuadRect( x1, y1, x2, y2), 1234 ) ; 
					insert( root, std::make_pair( QuadRect( x1, y1, x2, y2), 1234 ) ) ; 
                }
				else
				{
					/*
						OK, very important. If free of intersections is false We could request a new bounds. 
					*/
					job->free_of_intersections( false );	
				}
			}

			// and record the new passive job set
			last_set = current_set;	

		}
		else
		{
			// no change
		}

		/*
			OK THE PROBLEM IS HERE. 
	
			WE CAN'T JUST COMPUTE LABELS OVER THE TOP, WE WOULD HAVE TO COPY THE EXITING
			QUADTREE.

			OTHERWISE IT JUST PERSISTS.
		*/

		{
			if( ! root )
			{
				//std::cout << "WHAT IS THIS ? !!!" << std::endl; 
				return;
			}

			// copy the quadtree ...
			assert( root );
			ptr< Node< int> > root2 = copy_tree( root );  

			// now we compute all other labels 
			// where the z_order is > 100


			foreach( const ptr< ILabelJob>  & job , jobs )
		//	foreach( objects_t::value_type & pair , jobs )
			{
		//		const ptr< ILabelJob>  & job = pair.second; 
				int z = job->get_z_order(); 
				if( z >= 100 )	
				{
					int x1 = 0, y1 = 0, x2 = 0, y2 = 0; 
					job->get_bounds( &x1, &y1, &x2, &y2 ) ; 

					// no intersections then we are good
					//size_t nnn = quad_tree.intersections_count( QuadRect( x1, y1, x2, y2) ) ; 
					size_t nnn = candidate_intersections_count( root2, QuadRect( x1, y1, x2, y2) ) ; 
					if( nnn == 0)
					{
						job->free_of_intersections( true );	
						//quad_tree.insert( QuadRect( x1, y1, x2, y2), 1234 ) ; 
						insert( root2, std::make_pair( QuadRect( x1, y1, x2, y2), 1234) ) ; 
					}
					else
					{
						// OK, very important. If free of intersections is false We could request a new bounds. 
						job->free_of_intersections( false );	
					}
				}
			}

		}	

	} 

private:
	unsigned		count;
	objects_t		jobs;
	std::vector< ptr< ILabelJob> >		last_set;	// set, when last calculated
	//QuadTree< int>	quad_tree;
	ptr< Node< int> >		root;
};


}; // namespace


ptr< ILabels> create_labels_service()
{
	return new Labels; 

}


