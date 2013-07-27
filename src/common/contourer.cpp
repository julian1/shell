
/*
	OK, even if it is a service, the contouring, should be generalized and put in common as a common algorithm.
		just like some of the other algs.  
		load_png, load_shapefile, polygon_algebra etc. 
		probably also the geometry editing application code.
*/


/*
	- The hashing structures used in this are terrible. 
	- also probably should use boost::unordered_set as the collection type
*/
/*
	Trace contours of a grid
	Not sure if we really want this as a service 

	The only reason, is that it must run before labelling which is cross, layer. 

	We either determine deps with an algorithm, or sequence the update.
*/

#include <common/contourer.h>
//#include <common/grid.h>


#include <cassert>
#include <ostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <tr1/unordered_set>

#include <agg_path_storage.h>



// #define assert( a) (0)

// hash map gives 45ms for adding a million unsigned elements - which is much more than
// we would need to. 

/*	 ----------------------------------------
	Simply way to categorize edges is i,j and edge type (horiz, vert, diag ) 

	Edges can be traversed in the Directions
	If we have a given direction then it implies the edge type so we mostly
	just work with (i,j and dir). We project i2 j2 from i j when 
	we need the other coordinates. Dealing with the directions in 
	this way makes it easy to write the direction changing tracing
	since we can easily write the new direction and change i,j.

 (i,j)
	---0---
	|\
	1  2
    |	\

	basic algorithm is to rely on the fact that an intersection in a triangle occurs
	at two edges. so to trace we just have to select the right triangle and edge 
	-------------------------------------- 
*/


namespace { 


enum Direction 
{  
	UP, DOWN, LEFT, RIGHT, NE, SW
};

enum EdgeType 
{  
	NONE, HORIZ, VERT, DIAG
};

// should use array indexing rather than switch stmt to be guaranteed of speed

static std::ostream & operator << ( std::ostream &os, const Direction &dir)
{
	// useful for trace debugging
	switch( dir) { 
		case UP:	os << "UP"; break; 
		case DOWN:	os << "DOWN"; break; 
		case LEFT:	os << "LEFT"; break; 
		case RIGHT: os << "RIGHT"; break; 
		case NE:    os << "NE"; break; 
		case SW:	os << "SW"; break; 
		default: assert( 0);
	}
	return os;
}


static Direction opposite_dir( Direction dir)
{
	// should use array indexing rather than switch stmt to be guaranteed of speed
	// don't think this should be a separate function if only used in one place ? 

	switch( dir) {
		case UP:  	return DOWN;
		case DOWN: 	return UP; 
		case LEFT: 	return RIGHT; 
		case RIGHT: return LEFT; 
		case NE: 	return SW; 
		case SW: 	return NE; 
		default: 
			assert( 0);
	}
}	


#if 0
	the type should be part of the edge 

static EdgeType edge_from_dir( Direction dir)
{

	// should use indexing to be guaranteed of speed

	switch( dir) {
		case UP:  	return HORIZ;
		case DOWN: 	return HORIZ; 
		case LEFT: 	return VERT; 
		case RIGHT: return VERT; 
		case NE: 	return DIAG; 
		case SW: 	return DIAG; 
		default: 
			assert( 0);
	}
}	
#endif


struct Edge 
{
	// an edge and direction we are traversing through it 
	// THIS SHOULD BE RENAMED. It's more of an edge and a traversal.   

	Edge() 
		: x( -1234), y( -1234), type( NONE ) 
	{ } 
	Edge( signed x, signed y, EdgeType type )
		: x( x), y( y),	type( type ) 
	{ } 	

	signed 		x;
	signed 		y;
	EdgeType type; 
//	Direction 	dir;
};

static bool operator == ( const Edge &a, const Edge &b)
{
	return a.x == b.x
		&& a.y == b.y
		&& a.type == b.type ;		// CHANGED 
}

//void project_edge( signed x1, signed y1, Direction dir, signed *x2, signed *y2)
static void project_edge( const Edge &e, signed *x2, signed *y2)
{
	// work out the other point for the edge, given the direction.

	// given x1, y1 and a direction (which implies the edge type) then 
	// project the point x2, y2 
	switch( e.type ) {
		case HORIZ:  		// horizontal edge
			*x2 = e.x + 1; 
			*y2 = e.y; 
			break;
		case VERT:  	// vertical edge
			*x2 = e.x; 
			*y2 = e.y + 1; 
			break;
		case DIAG:  		// diagonal
			*x2 = e.x + 1;
			*y2 = e.y + 1;
			break;
		default: 
			assert( 0);
	}
}


static bool edge_within_data_boundary( const Grid &m, const Edge &e)
{
	// determine that the edge is within the data boundary
	// how would we go about handling holes - remembering that we want
	// to convert to polygons at some point ? 
	// we might actually have holds in the data as well 
	// would it be better to mark valid / boundary edges ? 

	if( e.x < 0 || e.y < 0) 
		return false; 
	
	if( e.x < (m.width() -1) && e.y < (m.height() - 1)) 
		return true; 

	signed x2, y2; 
	project_edge( e, &x2, &y2);
	
	// horizontal vert and diag edge types are constrained
	// by the bottom and rhs
	if( x2 < m.width() && y2 < m.height()) 
		return true;

	return false;
}



struct Point
{
	// point is a useful primative for recording in vector and supporting
	// reverse operation when tracing both ways
	double x, y;
	Point( double x, double y)
		: x( x), y( y)
	{ } 
};


static Point interpolate_point( 
	double value, double av, float bv,
   	const Point &a, const Point &b)  
{
	// if we ignore diag then distances will always be 1 so this
	// could be optimised to only deal with this case 
	double w = (value - bv) / (av - bv ) ;  
	Point pnt( w * (a.x - b.x) + b.x,  w * (a.y - b.y) + b.y) ; 
	return pnt ; 
};

static Point interpolate_value( const Grid &m, const Edge &e, double value)
{
	// given a point and direction and value value file the intersection
	signed x2, y2; 
	project_edge( e, &x2, &y2);

	double a = m( e.x, e.y);
	double b = m( x2, y2);
	assert( a != b);

	// distances are always going to be equal to one ? except diagonal 
	Point p = interpolate_point( value, a, b, Point( e.x, e.y), Point( x2, y2)); 
	return p;
}

static bool edge_contains_value( const Grid &m, const Edge &e, double value)
{
	// does the edge contain the search value
	signed x2, y2; 
	project_edge( e, &x2, &y2);

//	std::cout << "m.width() " << m.width() << std::endl;

	double a = m( e.x, e.y);
	double b = m( x2, y2);

	//assert( a != b);
	if( a > b) std::swap( a, b);
    return value >= a && value < b; 
}


// THE RECORD DATA STRUCTURE OUGHT TO BE ABLE TO REMOVED, and replaced with just the Edge. 
// No, because the edge may be traversed by more than one contour/isoline 

struct Record 
{
	// record element for where we have traced, we record the edge 
	// rather than direction to avoid misidentifying edges
	// traversed in an opposite direction

	Record( const Edge &edge, double value)
		: edge( edge), 
		value( value)  
	{ } 	

	Edge	edge;
	double	value; 

/*	
	Record( const Edge &e, double value)
		: x( e.x), 
		y( e.y),	
		edge( edge_from_dir( e.dir)), 
		value( value)
	{ } 	

	signed 		x;
	signed 		y;
	EdgeType	edge;
	double		value;		
*/
};

/*
	- we should compare speed with boost hash using seed.
	- boost should be faster because we don't iterate at byte level.
*/
struct hashRecord
{
	/*
		This is fundamentally wrong. we should not be iterating at byte/char level, 
		when we have ints and 64 bit int operations available. 
	*/
	template< class T> 
	unsigned f( unsigned hash, const T &value ) const
	{
 		/* The standard reference for this is Knuth's "The Art of Computer Programming", 
		volume 3 "Sorting and Searching", chapter 6.4. He recommends the hash */
		const unsigned char *p = 
			reinterpret_cast< const unsigned char *>( &value);

		for( unsigned i = 0; i < sizeof( T); ++i)
		{
			hash = ((hash<<5)^(hash>>27))^*p++;
		}
 		return hash;
	}

	unsigned operator () ( const Record & r) const
	{
		unsigned hash = 0x12345678; 
		hash = f( hash, r.edge.x); 
		hash = f( hash, r.edge.y); 
		hash = f( hash, r.edge.type ); 
		hash = f( hash, r.value); 
		// std::cout << "returning hash " << hash << std::endl;
 		return hash;
	}
};

struct equalRecord
{
	bool operator () ( const Record &a, const Record &b) const
	{
		return a.edge.x == b.edge.x
			&& a.edge.y == b.edge.y
			&& a.edge.type == b.edge.type
			&& a.value == b.value ; 
	} 
};


// record of edge traversal x*y*edge and value
typedef std::tr1::unordered_set< Record, hashRecord, equalRecord>	record_type;


struct Context
{
	// context of tracing operation

	
	Context( const Grid & m, record_type & record, double value, std::vector< Point> &path)
		: m( m)
		, record( record)
		, path( path)
//		, start( start)
		, value( value)
		, first( true)
		, closed( false)
	{ } 

	// data matrix
	const Grid			&m;

	// record of prior traversal
	record_type			&record; 

	// points in current contour that is being traced 
	std::vector< Point>	&path; 

	// starting edge - will be set when first point == true
	// ok, this means we can't use immutable 
	Edge				start;

	// first edge in traversal 
	bool 				first; 

	// was the trace a closed polygon
	bool 				closed; 

	// value we're looking for
	// should be a better name for this ? 
	// should this be part of the trace ???
	// no i think its ok here
	double				value;
};


static bool trace_contour( Context &c, const Edge &e ); 

static bool trace_boundary( Context &c, const Edge &e, Direction dir )
{
	// we want to get rid of the boundary check and instead defer to this 
	// the point should have already been recorded.

	// REMMEBER - we want this to be able to trace out the boundary if it
	// were in a hash table. 

	// fuck fuck fuck ...

	// ok, we want to call trace_contour, but that will store the bloody point ... 

	// shouldn't we be only recording the point, afterwards. the problem is 
	// it would make the line run backwards.

	
	// is there another way,   if the boundary edges were inserted and linked as a path
	// then it would be easier to follow them... rather than tracing them ????

	// also i don't think that we can just trace them ... if we don't know the actual 
	// direction of the path.  

	/*
		If we want to work with holes, then we need to respect the direction. 
	*/
	/*
		So, if we carry a path that is linked, then we can look up if we hit it.   
		then follow it. 
		We are allowed to call trace_contour, which will test the data point,   
		But we have to somehow be careful, that we don't test it on the first time.

		So, we pass a bool to indicate it's the first time ...
	*/

	/*
		The simplest mechanism is to have the path, and then just follow it around 

		VERY IMPORTANT
		We don't need recursion. 

		- We look up the segment in a table, and then just follow it, testing each time
		whether we should jump off of the path.
		- BUT we still have to know how to jump off. and that will mean determining 
		if the segment edge is vertical or horizontal. (but that is realatively easy to p1.x == p2.x etc test).

		Use points, with next links in them ?  or agg_path ?

		- the difficulty is having to code stuff to turn corners,  

		- ALSO GRIDS will potentially be incomplete, not just with holes.  

		--- OK, Don't we want to use the edge structure ??? so we can 
	*/

	return false;

	switch( dir)
	{
		case UP: 
		case LEFT: { 
			if( e.type == VERT && e.x == 0  )
			{
				if( e.y  == 0) 
				{
					// eg. top left corner we will have to go right ...

				}	

				std::cout << "whoot a boundary to trace" << std::endl;	
				// trace out the boundary and then, try to trace contour ...  
				// so we want to set the direction, 

				// OK, but then we have to reset the direction and move up ...

				return trace_boundary( c, Edge( e.x, e.y - 1, e.type), UP ); 
			}

		}

	}; 

	return false;
}



static bool trace_contour( Context &c, const Edge &e,  Direction dir )
{
	// use recursion to trace out contour - write a recursive instance and then iterative later 
	// actually recursion is probably ok here, interpolation and boundary valueing are
	// moderately expensive compared to recursion.


	/*
		JA

		- edge within data boundary can be removed, once we have boundary tracing.
		and the edge contains value, probably should be tested before we do the jump.
	
		- it simplifies and means we don't have to return values.

		NO.
		
		because we want to be able to easily try and jump to trace a contour when
		tracing the the boundary.

		- the boundary edge thing, i think should be removed. 
	*/

	// check the edge is valid inside the data boundary
	if( ! edge_within_data_boundary( c.m, e))  
	{
		return false; 
	}


	// the major test is whether the point exists within this edge or
	// the adjacent alternative edge
	if( ! edge_contains_value( c.m, e, c.value)) 
		return false;
	
	
	if( c.first)
	{
		// set the starting point
		c.start = e;
		// clear the flag
		c.first = false; 
	}
	else { 

		// encountering the starting position indicates we have traced a closed polygon
		if( e == c.start ) 
		{ 
			// assert that the first position was really recorded
			assert( c.record.find( Record( e, c.value)) != c.record.end());
			// indicate that it is closed - eg a polygon rather than polyline
			c.closed = true;
			return true;
		}
	}

	// do recording of the edge to indicate it has been processed
	//EdgeType edge = edge_from_dir( e.dir);
	if( e.type == HORIZ || e.type == VERT) 
	{ 
		// record fact that we have traversed the edge
		c.record.insert( Record( e, c.value));

		// avoiding an interpolation and creating a point for the diag improves quality 
		// a lot (the interp computation is also simpler as distance is always 1)
		Point pnt = interpolate_value( c.m, e, c.value);
		c.path.push_back( pnt);
	}

	// follow the contour around - we always have two alternative edges to 
	// test whether the contour goes

	// one of the choices will always proceed.
	// the return values are not very clear, but it doesn't matter. 

	// rather than progress outside the data bound and then checking if we are really outside, 
	// we should keep that logic in this frame of the recursion.

	switch( dir)
	{
		case UP: { 
			bool result =         trace_contour( c, Edge( e.x, e.y - 1, VERT), LEFT );
			if( !result) result = trace_contour( c, Edge( e.x, e.y - 1, DIAG), NE);
			return true;
		}
		case DOWN: { 
			bool result = 		  trace_contour( c, Edge( e.x + 1, e.y, VERT), RIGHT);
			if( !result) result = trace_contour( c, Edge( e.x, e.y, DIAG), SW);
			return true;
		}
		case RIGHT: { 
			bool result =         trace_contour( c, Edge( e.x, e.y + 1, HORIZ), DOWN);
			if( !result) result = trace_contour( c, Edge( e.x, e.y, DIAG), NE );
			return true;
		}
		case LEFT: { 
			/*
				- I believe this is correct. trace_contour adds the value, then if the 
				next point is on the data boundary, we trace that. 
				
				- we can put the trace boundary code anyway...
			*/	
			// A MORE FLEXIBLE WAY TO CHECK FOR BOUNDARIES, WOULD BE TO PUT THEM IN A HASH LOOKUP
			// THEN IT OUGHT TO ALSO WORK FOR DATA HOLES IN DATA.
			// not certain if the trace_boundary should be first or last. 
		
			// if we are on a boundary we don't want to project out of the boundary.

			bool result  = false;
			result =              trace_contour( c, Edge( e.x - 1, e.y, HORIZ), UP);
			if( !result) result = trace_contour( c, Edge( e.x - 1, e.y, DIAG), SW );
			if( !result) trace_boundary( c, e, dir );  // use existing direction	
			return true;
		}
		case NE: { 
			bool result =         trace_contour( c, Edge( e.x, e.y, HORIZ),  UP);	
			if( !result) result = trace_contour( c, Edge( e.x + 1, e.y, VERT) , RIGHT);
			return true;
		}
		case SW: { 
			bool result =         trace_contour( c, Edge( e.x, e.y, VERT), LEFT );
			if( !result) result = trace_contour( c, Edge( e.x, e.y + 1, HORIZ), DOWN);
			return true;
		}
		default:
			assert( 0);
	}	

	assert( 0);
	return true;
}


static void trace_contour1( 
	const Grid 		&m, 
	record_type 		&record, 
	double				value,
	Edge				e,
	Direction			dir,
	std::vector< Point>	&path,
	bool				&closed	)
{
	// trace a complete contour - by tracing a polygon or tracing 2 haves of
	// a polyline that is has ends bounded by the matrix dimensions

	// path should already be clear
	assert( path.empty());

	// we only start traces from horiz or vert edges
//	assert( edge_from_dir( e.dir)== HORIZ || edge_from_dir( e.dir) == VERT);
	assert( edge_contains_value( m, e, value));


	// ignore contours starting from edges that have already been included
	if( record.find( Record( e, value)) != record.end())  
		return;


	// at this point we are committed to tracing the contour 
	// set up a context for the trace
	Context	c( m, record, value, path); 

	// trace in one direction
	trace_contour( c, e, dir   );

	if( !c.closed) { 
		// if the contour was not closed - then trace in opposite direction 
		
		// reverse points
		std::reverse( c.path.begin(), c.path.end());
		
		// reverse dir
		dir = opposite_dir( dir);	

		// and trace in the opposite direction - setting the first flag is not 
		// really needed since only used for closed polygon which is not the case we are
		// dealing with
		c.first = true;
		trace_contour( c, e, dir );
	}

	// set output parameter 
	closed = c.closed; 
}


template< class T>
struct InclusiveRange
{
    // the purpose of this class is to return range of potential contour values
	// that may traverse through the two vertex values of an edge.
    // because we iterate edges linearly one after the other and given the underlying data is 
	// mostly continuous then range values will be be very near to 
	// each other over successive calls. We could use stl 
    // upper and lower_bound functions but this should be faster. 

 	const std::vector< T>   &values;
    unsigned                min_index;
    unsigned                max_index;

    InclusiveRange( const std::vector< T> &values)
        : values( values)
        , min_index( 5)
        , max_index( 5)
    {  

//		std::cout << "values.size " << values.size() << std::endl;
		assert( ! values.empty() );
		
        for( int i = 0; i < values.size() - 1; ++i) {
            assert( values.at( i) <= values.at( i + 1));
        }
    }

 	void operator() ( const T &min, const T &max, unsigned *min_idx, unsigned *max_idx) 
    {   
        // min == max is now and so are reversed ranges. if min > max - then both
        // indexes are set to the end 

        // shift min index down if we need to
        while( min_index > 0 && values.at( min_index - 1) >= min) 
            --min_index;
    
        // shift min index up if we we need to - may go one past end
        while( min_index < values.size() && values.at( min_index ) < min)
            ++min_index;

        // shift max index down if we need to
        while( max_index > 0 && values.at( max_index -1 ) > max) 
            --max_index;

        // now max index up if we need to - may go one past end
        while( max_index < values.size() && values.at( max_index ) <= max) 
            ++max_index;

        assert( min_index >= 0 && min_index <= values.size());
        assert( max_index >= 0 && max_index <= values.size());

        // set values
        *min_idx = min_index;
        *max_idx = max_index;
    }   
};
 

/*
	FOR turning polygon/lines into polygons the operations are quite different
		
		- adjacent closed polygons are *always* created in sets. 
		- where it is unclosed we need to draw around the edge to complete it.

	We can record that two adjacent polygons have been recorded against each other
	using a hash table. This ought to be very light weight. Can use polygon indices
	rather than copying data about.   

	FOR POLYLINES
		recording the end points and the (adjacent points ) we should be able
		to connect them up. relatively easisly turning them into true polygons. 

		Dont attempt to trace them out - because we could always take the wrong turn at the
		endpoints leading to deep recursion. 

*/


///////////////////////////////////////////////////////////////

}; // end of anon namespace




void make_contours2( 
	const Grid & grid, 
	const std::vector< double> & values,	
	IContourCallback	& callback
//	std::vector< ptr< Contour>  >	& contours
)  {


//	assert( contours.empty() );
	
	record_type record( 100000);

	// ok so we have to build up the list of contour values
    InclusiveRange< double>		contour_range( values);

	// we need to get rid of this outer loop and use a stepping operation
	// should potentially be in a class ? ... 
//	for( unsigned i = 0; i < values.size(); ++i)

	assert( grid.width() > 0 && grid.height() > 0);

	for( signed y = 0; y < grid.height() - 1; ++y) 
	for( signed x = 0; x < grid.width() - 1; ++x) 
	{

		// we are only doing horizontal edges at the moment  	
		// it is possible a contour will only pass through vertical and diags.
		double min = grid( x, y);			// should get values using project function
		double max = grid( x + 1, y);

		if( min > max) 
			std::swap( min, max);

//		std::cout << "-------------------------------" << std::endl;
//		std::cout << "min " << min << " max " << max << std::endl;
	
		// extract the contour range for the edge		
		unsigned cmin, cmax;
		contour_range( min, max, &cmin, &cmax);
	
		for( unsigned i = cmin; i < cmax; ++i) 
		{ 
			double value = values[ i];
			assert( value >= min);
			assert( value <= max);

			// create an edge as starting point for trace
			Edge	e( x, y, HORIZ );

			// it is probably only going to be a single line
			bool result = edge_contains_value( grid, e, value);
			if( !result)
			{
				// i think that this is correct. it's possible that the edge is in range
				// acording to the coarse inclusive range generation, but not in range
				// with our discriminating edge test.
				continue;
				// ok, i think it's a range issue ...  a < b < c or a <= b < c etc
				// it's possible we should be starting the trace at a different position
				// BUT. it may be aligned on the first one ...

				std::cout << "-------------------------------" << std::endl;
				if( value == max ) 
					std::cout << "value == max" << std::endl;

				std::cout << "value " << value << std::endl;
				std::cout << "min " << min << " max " << max << std::endl;
			
			}
			assert( result);	//hmmmmmmm

			std::vector< Point>	v; 
			bool 				closed; 	
			trace_contour1( grid, record, value, e, DOWN, v, closed);

			// we have a contour
			if( !v.empty())
			{
				// copy the trace into agg path 
				agg::path_storage	path;

				for( std::vector< Point>::const_iterator i = v.begin(); i != v.end(); ++i)
				{
					if( i == v.begin()) path.move_to( i->x, i->y);
					else 				path.line_to( i->x, i->y);
				}
				if( closed) 
				{
					path.close_polygon();
				}


				callback.add_contour( value, path ); 
//				ptr< Contour> contour = new Contour( value, path ); 
//				contours.push_back( contour ); 
			}
		}
	}
}

// the source matrix needs to be injected ...


#if 0

//int main1()
void make_contours( 
	const Grid &m, 
	const std::vector< double> &values,	
	agg::path_storage &path)
{
	
	record_type			record( 100000);

	// ok so we have to build up the list of contour values
    InclusiveRange< double>		contour_range( values);

	// we need to get rid of this outer loop and use a stepping operation
	// should potentially be in a class ? ... 
//	for( unsigned i = 0; i < values.size(); ++i)

	assert( m.width() > 0 && m.height() > 0);

	for( signed y = 0; y < m.height() - 1; ++y) 
	for( signed x = 0; x < m.width() - 1; ++x) 
	{

		// we are only doing horizontal edges at the moment  	
		// it is possible a contour will only pass through vertical and diags.
		double min = m( x, y);			// should get values using project function
		double max = m( x + 1, y);

		if( min > max) 
			std::swap( min, max);

		//	std::cout << "-------------------------------" << std::endl;
		//std::cout << "min " << min << " max " << max << std::endl;
	
		// extract the contour range for the edge		
		unsigned cmin, cmax;
		contour_range( min, max, &cmin, &cmax);
	
		for( unsigned i = cmin; i < cmax; ++i) 
		{ 
			double value = values[ i];
			assert( value >= min);
			assert( value <= max);

			// create an edge as starting point for trace
			Edge	e( x, y, DOWN);

			// it is probably only going to be a single line
			bool result = edge_contains_value( m, e, value);
			assert( result);	//hmmmmmmm

			std::vector< Point>	v; 
			bool 				closed; 	
			trace_contour1( m, record, value, e, v, closed);

			// copy the trace into agg path 
			for( std::vector< Point>::iterator i = v.begin(); i != v.end(); ++i)
			{
				if( i == v.begin()) path.move_to( i->x, i->y);
				else 				path.line_to( i->x, i->y);
			}
			if( closed) 
				path.close_polygon();
		}
	}
}

#endif

