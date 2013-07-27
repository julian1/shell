
#pragma once

/* 
	This is not an aggregate it has no state. should be in /common or /data, leave in common because
		it is used a lot  

	the cube should abstract gribs/analysis/formula for the purposes of the cube_view and presentation.
	eg. runtime, model is irrelevant.

	An icube interface can delegate to other icubes. to enable cross model/ cross analysis formula etc. 
	caching, db etc is also abstracted

	The cube_view probably should take 2 interfaces. the icube for data, and a meta description 	
*/

#include <common/ptr.h>

#include <vector>

struct SurfaceCriteria;
struct Grid;

struct ICube
{
	virtual void add_ref() = 0; 
	virtual void release() = 0; 

	// requires everything, because must be able to abstract for multiple views 
	virtual std::vector< ptr< SurfaceCriteria> > get_available_criterias() = 0;

	virtual ptr< Grid > get_grid( const ptr< SurfaceCriteria> & /*, meta */  ) = 0;  

	// virtual bool get_active() = 0;  not sure ... different planes might be active in different situations
};


/* 
	we could have another Cube, for pointwise data that would work for station reports.

struct IPointCubeModel
{
	get_points( crit ); 

};

*/
