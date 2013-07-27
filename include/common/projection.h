
#ifndef MY_PROJECTION_H
#define MY_PROJECTION_H

/*
	The projection (that has the algorithm that actually does the work of project points and shapes / cf sequencing)
	has to be independent, so that it can be passed out to systems that need it for stuff like hittesting, geom editing.
*/
// Projection should normally just be an adaptor that wraps an aggregate root.
// it doesn't require a .cpp file.

// if the flags are const then we can't modify ??...

#include <agg_path_storage.h>

#if 0
struct ProjectionFlags
{
	ProjectionFlags()
		: clip( false)
	{ } 

	bool clip;
};
#endif

		//root->get_grid_projection()->forward( contour->contour->path, contour->projected_path, true  );
/*
	Change name to ITransform to avoid confusion with the projection aggregate.
	This thing is an entity 
*/
struct IProjection
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void forward( double *x, double * y ) = 0; 
	virtual void reverse( double *x, double * y ) = 0; 
/*, const ProjectionFlags & flags*/
	virtual void forward( const agg::path_storage & src, agg::path_storage & dst, bool clip ) = 0; 
	virtual void reverse( const agg::path_storage & src, agg::path_storage & dst, bool clip ) = 0; 
	// reverse
};




#endif

