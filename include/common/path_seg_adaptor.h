/*
	FIXED
		Previously failed to include the final segment of a polygon (eg when agg::cmd == close)
		This would have made a lot of projection configuration calculation wrong

		another variation for point in polygon - see http://www.antigrain.com/demo/interactive_polygon.cpp.html

	fixed bug with returning cmd, rather than cmd2 when dealing with move_to
*/

#ifndef PATH_SEG_ADAPTOR_H
#define PATH_SEG_ADAPTOR_H

#include <cassert>
#include <agg_path_storage.h>


template< class T> 
struct path_seg_adaptor
{

private:
	typedef T path_type; 

	path_seg_adaptor( const path_seg_adaptor &);
	path_seg_adaptor & operator = ( const path_seg_adaptor &);

	/*
		This is an incredibly useful adapator/abstraction over a path.
		An interface that provides access to all the segments for polygons, or polylines, rather than the vertices
	*/
	path_type  &path;
	double x_prev, y_prev;
	double x_begin, y_begin;


public:
	path_seg_adaptor( path_type &path)
		: path( path)
	{
		//	this->path.rewind( 0);
		//
		// keep compiler happy about initializion
		x_begin = -1;
		y_begin = -1;
		x_prev = -1;
		y_prev = -1;
	}

	int segment( double *x1, double *y1, double *x2, double *y2)
	{
//		double x, y;
		unsigned cmd = path.vertex( x1, y1); 

		if( agg::is_move_to( cmd))
		{
			assert( agg::is_vertex( cmd));  
			// grab another point
			//double xx, yy;
			unsigned cmd2 = path.vertex( x2, y2);	// cmd is now agg::line_to

			// TODO if they are just points - then avoid returning full segments	
			assert( agg::is_line_to( cmd2));  
//			*x1 = x;
//			*y1 = y;
//			*x2 = xx;
//			*y2 = yy;

			x_begin = *x1;
			y_begin = *y1;
			x_prev = *x2;
			y_prev = *y2;
			return cmd;
		}
		else if( agg::is_line_to( cmd) )
		{
			assert( agg::is_vertex( cmd));  
			*x2 = *x1;
			*y2 = *y1;
			*x1 = x_prev;
			*y1 = y_prev;
			x_prev = *x2;
			y_prev = *y2;
			return cmd;
		}

		// i think the close needs to be treated differently
		else if( agg::is_close( cmd))
		{
			*x1 = x_prev;
			*y1 = y_prev;
			*x2 = x_begin;
			*y2 = y_begin;
			return cmd;
		}
		else if( agg::is_stop( cmd)) 
		{
			*x1 = -1;
			*y1 = -1;
			*x2 = -1;
			*y2 = -1;
			return cmd;
		} 
		else 
			assert( 0);
		// 

		assert( 0);
		return cmd;
	}

	void rewind()
	{
		path.rewind( 0);
	}
};

#endif

