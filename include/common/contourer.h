/*
	Calculating contour lines is an atomic operation (like loading_shapes or loading_png) . 
	It should not be factored into service, unless required for sequencing.
*/

#ifndef COMMON_CONTOURER_H
#define COMMON_CONTOURER_H


#include <common/ptr.h>
#include <common/grid.h>

#include <vector> 
#include <agg_path_storage.h>

/*
	Potentially we should have a simple callback interface (Like loading shapefiles), and we could get rid of the
	Contour structure, but 

	instead of a structure combining value and path fnd need or mem management and wrapping. We instead use simple callback mechanism 
	Anything else builds on top of the basic interface to the algorithm

	the grid should probably be a ptr ?? 
*/


struct IContourCallback
{
	virtual void add_contour( /* int id */ double value, const agg::path_storage & path ) = 0; 
};

void make_contours2( const Grid & grid, const std::vector< double> & values, IContourCallback & callback );   

// this is a really crap interface ...
// actually 
// the dependency on a grid is ugly too ...

struct ContourData
{
	double *data;  
	unsigned w; 
	unsigned h; 
	std::vector< double> values; 
	std::vector< agg::path_storage > paths; 
};

struct IMakeContours
{
	virtual void make_contours( ContourData & ) = 0  ; 
};


#endif

