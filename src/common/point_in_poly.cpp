/*
	- This algorithm is efficient - is linear with vertex count, not log behaviour. 
	- the sort() is required onl for the set of vertices crossing a y sweepline, it n=2 for poly without holes
	- Handles polys with holes correctly
	- Note the really useful path_seg_adaptor, which gives us clean segments rather than vertices

	FIXED
		Previously failed to include the final segment of a polygon (eg when agg::cmd == close)
		This would have made a lot of projection configuration calculation wrong

		another variation for point in polygon - see http://www.antigrain.com/demo/interactive_polygon.cpp.html
*/

#include <common/point_in_poly.h>
#include <common/path_seg_adaptor.h>


#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>		// std::distance()

#if 0

struct path_seg_adaptor
{
	/*
		This is an incredibly useful adapator/abstraction over a path.
		An interface that provides access to all the segments for polygons, or polylines, rather than the vertices
	*/
	agg::path_storage &path;
	double x_last, y_last;
	double x_begin, y_begin;

	path_seg_adaptor( agg::path_storage &path)
		: path( path)
	{
		//	this->path.rewind( 0);
		//
		// keep compiler happy about initializion
		x_begin = -1;
		y_begin = -1;
		x_last = -1;
		y_last = -1;
	}

	int segment( double *x1, double *y1, double *x2, double *y2)
	{
		double x, y;
		unsigned cmd = path.vertex( &x, &y); 

		if( agg::is_move_to( cmd))
		{
			assert( agg::is_vertex( cmd));  
			// grab another point
			double xx, yy;
			cmd = path.vertex( &xx, &yy);	// cmd is now agg::line_to

			// TODO if they are just points - then avoid returning full segments 	
			assert( agg::is_line_to( cmd));  
			*x1 = x;
			*y1 = y;
			*x2 = xx;
			*y2 = yy;

			x_begin = x;
			y_begin = y;

			x_last = xx;
			y_last = yy;
		}
		else if( agg::is_line_to( cmd) )
		{
			assert( agg::is_vertex( cmd));  
			*x1 = x_last;
			*y1 = y_last;
			*x2 = x;
			*y2 = y;

			x_last = x;
			y_last = y;
		}
		else if( agg::is_close( cmd))
		{
			*x1 = x_last;
			*y1 = y_last;
			*x2 = x_begin;
			*y2 = y_begin;
		}
		else if( agg::is_stop( cmd)) 
		{
			*x1 = -1;
			*y1 = -1;
			*x2 = -1;
			*y2 = -1;
		} 
		else 
			assert( 0);
		// 
		return cmd;
	}

	void rewind()
	{
		path.rewind( 0);
	}
};
#endif

static bool classify_point_inside_sweep( double x, const std::vector< double> &xs) 
{

	// TODO - this is not quite correct
	// xs.size() may equal 1 in which case we are tangent to the point. 
	// this is unlikely to occur, due to fp inaccuracy of line intersection calculation

	// from sweepline
	// classify whether point value x is interior/exterior/shared in v
	// v must be sorted

	if( xs.size() % 2 != 0)
	{
		std::cout << "*** classify_point_in_sweep problem xs.size() " << xs.size() << std::endl;
		return false; 
//		assert( 0);
	}
	
	assert( xs.size() % 2 == 0);
/*
	// proj limb fuckout
	if( v.size() % 2 != 0) 
		return false;
*/
	// i think lower bound is binary search - otherwise would use find 
	// note it returns next higher value,  however indexing from 0 using std::distance is good 
	std::vector< double>::const_iterator i = 	
		std::lower_bound( xs.begin(), xs.end(), x);

	// coincident ? on line -- if point is found
	// classification::shared;
	if( i != xs.end() && *i == x) 
		return true; 

	unsigned index = std::distance( xs.begin(), i);

	return index % 2 != 0;
}


static void record_y_intersections( double py, agg::path_storage &path1, std::vector< double> &xs) 
{
	path_seg_adaptor< agg::path_storage > 	path( path1);
	path.rewind();

	// std::cout << "---------------------------" << std::endl;

	while( true)
	{
		double x1, y1, x2, y2;
		unsigned cmd = path.segment( &x1, &y1, &x2, &y2); 

		if( agg::is_stop( cmd)) 
			break;
	
		// std::cout << x1 << " " << y1 << " " << x2 << " " << y2 << std::endl;

		double ymin = std::min( y1, y2);
		double ymax = std::max( y1, y2); 

		// does our segment span the y axis 
		if( py >= ymin && py <= ymax) 
		{
			// calculate x intersection of point value of y						
			double xx = ( py - y1) * (x2 - x1) / (y2 - y1) + x1; 
			// and record
			xs.push_back( xx);
		}
	}
}

bool point_in_poly( double px, double py, agg::path_storage &path) 
{
	std::vector< double>	xs; 

	record_y_intersections( py, path, xs);

/*
	std::cout << "total_vertices " << path.total_vertices() << " " ;
	std::cout << "sweepline points " << xs.size() << " " ;
*/
	std::sort( xs.begin(), xs.end()); 

	bool ret = classify_point_inside_sweep( px, xs);
/*
	std::cout << " ret " << ret;
	std::cout << std::endl;
*/
	return ret;
}









#if 0 
static void record_y_intersections( double py, agg::path_storage &path, std::vector< double> &xs) 
{
	// x value of segment intersections according to y 
	// 
	path.rewind( 0);

	double x_last, y_last;
	bool begin = true;

	double x_begin, y_begin;

	while( true)
	{
		double x, y;
		unsigned cmd = path.vertex( &x, &y); 

		// end of poly - TODO should start again
		if( agg::is_stop( cmd)) 
			break;

		// close of poly, then we need to create segment from begin
		if( agg::is_close( cmd))
		{
			x = x_begin;
			y = y_begin;
			// fallthrough 
		}	

		// a regular segment
		if( agg::is_vertex( cmd) || agg::is_close( cmd)) 
		{ 
			if( !begin) 
			{
				double y1 = std::min( y, y_last); 
				double y2 = std::max( y, y_last); 
				double x1 = x;
				double x2 = x_last;

				// does our segment span the y axis 
				if( py >= y1 && py <= y2) 
				{
					// calculate x intersection of point value of y						
					double xx = ( py - y1) * (x2 - x1) / (y2 - y1) + x1; 
					// and record
					xs.push_back( xx);
				}
			}
			else 
			{
				begin = false; 	
				x_begin = x;
				y_begin = y;
			}	
			x_last = x; 
			y_last = y;
		}
	}
}
#endif



