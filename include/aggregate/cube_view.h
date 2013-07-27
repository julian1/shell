
#ifndef MY_CUBE_VIEW_H
#define MY_CUBE_VIEW_H

#include <common/ptr.h>


struct Services; 
struct IProjectionAggregateRoot;
//struct CubeGrib; 
struct ICube; 

void create_cube_view( Services & services, const ptr< ICube> & root , const ptr< IProjectionAggregateRoot> & projection_aggregate );
void remove_grid_aggregate_root( Services & services, const ptr< ICube> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate ); 


struct CubeAnalysis; 

void add_analysis_cube( Services & services, const ptr< CubeAnalysis> & root , const ptr< IProjectionAggregateRoot> & projection_aggregate );

#endif



/*
	The event boundary is also the lifetime boundary, is the point that we want to serialize/unserialize
	eg. the aggregate	

	I think it should be a no argument constructor.  arguments can be given at load time (eg, the projection to use). 
*/

/*
	- VERY IMPORTANT - THE AGGREGATE ROOT 
	does not need a reference to services.
	
	- Instead if Wrappers want services then give it to them, when the wrappers are created
	- Except the add_contour() and remove_contour()  have to be able to handle this ... 
*/

//void create_cube_view( Services & services, const ptr< IGridAggregateRoot> & root , const ptr< IProjectionAggregateRoot> & projection_aggregate );
//void remove_grid_aggregate_root( Services & services, const ptr< IGridAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate ); 



//	int get_z_order() ; 
//private:
//	boost::scoped_ptr< struct GridAggregateRootImpl > self;

	/*
		we really want to try to factor out, the projection, but keep the line style preferences as model.
	*/
//	virtual ptr< IProjection> get_grid_projection() const = 0; 

/*	
	virtual ptr< IProjection> get_projection() const = 0; 
*/

// THere is no reason for the aggregate to expose the services.
//	Services & get_services() const ; 

	// these should be methods, because the set, has to generate an event
	// the grid trans_affine	georeference


