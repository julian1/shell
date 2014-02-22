/*
	we should have a couple of simple functions for    ymax( edge)   that will compute
*/

/*
	mechanisms to speed up further (after switching to straight vector)
		(1) use/get to work   asio/threading    (is agg using globals ?) 
		(2) bounding boxes of subjects,(and simplified clip region geom)  and simple topolgy rules for inclusion
		(3) presorted geometry structures. (and merge from two sorted lists) 	
		(4) potentially use a priorty queue for removal.
		(5) switching to simplified view when panning in projection.

	at the moment, we really need to profile. what is the cost of the 
		linear traversal for the intersection testing.
			vs
		linear sweep for removal

	be
*/

/*

	choice at moment.
		(1) maintain a heap for max-y over vector<> for order_set, and do linear scan on vector<> for intervals. 
			(one datastructure), fast insertion and removal. 
		(2) the above without the heap. just use simple vector and scan (remove/remove_if) for both interval and y removal operations.

	considerations, 
		- remove y has to be a scan because there can be more than one element (ie we cannot just record max y from previous).
			- a heap would work, we can just loop the pop() until no more to be removed. 
			- eg. so use scan for intervals, but keep the thing as a stl heap. 
			- DON'T use 2 data structures. just use one for the y removal. 
			- computing intersections must use random removal (but we can call make_heap again).

		- i think we really must have only one structure. 

		- the complexity of both bubble sort and insertion sort, when already sorted is at least O(n).
			Eg. this is the case when we add each elt in order.  therefore there is no advantage relative 
			to a linear scan, which we do anyway. Heap is still better for y removal. 

		- intervals have to be a scan if don't have interval tree because we can only maintain order on 
			one dimension (eg. xmin), and therefore must can at least half the elements to ensure get xmax)

		- computing intersections, requires random removal, and reinsertion. (but if this is relatively rare, 
			it may still be good to a heap, and just call make_heap after these operations ).


		- if sweepline processing is linear in complexity. then the cost for processing all edges should 
			be about the same as the sort (if the average sweepline size is log( n) ). it seems to be
			actually less than this so it should be faster. 


	sweepline datastructures choices
		0 - bubble sort or insertion sort (eg very fast when only one element to add - no allocation ? )
		1 - set<>,map<>, interval tree (what we use now)
		2 - simple vector<> with linear scan.
		3 - stl heap (advantage, they don't require allocation - head could only be used on y remove)
		4 - boost icl interval data structures. 

	- extremely important - It would be possible to combine the two linear scans to compute intersections
		and record the index of the y max element. (no because we potentially have to remove more than one
		so we would have to test again ).  

	- advantage of an unsorted vector, 
			- can the same structure (vector) for both the intersection testing
			and the y removal, rather than 2 separate structures manipulated in parallel.
			- no allocation/ deallocation (for tree nodes)
			- we are likely to have smaller sweepline sizes if we do geometry separately.

	- note when doing the scan we may as well just put the loop where the intersection tests are
		using begin() and end(), and then have a simple x bounds check that call continue() to reject, and
		only doing the math when necessary. 
	
	- Performance of bubble sort over an already-sorted list (best-case) is O(n)	
	so it's no better than just scanning out the elements ?

	- for the intervals (if don't use an interval tree) - we have to linear scan from xmin to the end because
	to find the xmax. this is already O( n / 2 ) so we may as well scan the entire thing.
	
	
	5000 items * 100 elements.

		ln( 5000) * 5000 = 8.5 * 5000 = 42000
			* 100 = 4.2 million.

	
		ln( 500000 ) * 500000 
			=  13 * 500000 = 6 million 

		- there's not a lot of difference between combining the geometry into one set and running once, or splitting and processing each individually .
			- an interval tree should work better, where the internal size of a sweepline starts getting large however ?

	v.erase( remove( v.begin(), v.end(), 5 ), v.end() );
 
	v.erase( remove_if( v.begin(), v.end(), Pred() ), v.end() );
	

*/
/*
	for testing we can just output to a ppm to verify that it works.
	but we would really require serialization.    eg shapefile to 
*/
/*
	having a single data structure that is linearly scanned, would also remove all the 
	testing that the two data structures are the same size. 
*/

/*
	it would be interesting to know  the average number of elements in the sweepline
	- it is likely to be small ?
	- we also really need to run a profiler - on the algorithm.
		we could export to ppm to ensure that it's working correctly, the same as normal		

	we can use the vector<> swap and pop trick, for removal.	
	and using a vector<> will not require any allocation.

*/





/*
	http://cs1.bradley.edu/public/jcm/weileratherton.html
	http://en.wikipedia.org/wiki/Weiler-Atherton
	http://cs.fit.edu/~wds/classes/graphics/Clip/clip/clip.html
	http://inkscape.svn.sourceforge.net/viewvc/inkscape/inkscape/trunk/src/2geom/shape.cpp?revision=20938&view=markup
	http://en.wikipedia.org/wiki/Point_in_polygon
	http://paulbourke.net/geometry/insidepoly/
	http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
	http://en.wikipedia.org/wiki/Curve_orientation
*/

#include <memory>
#include <algorithm>
#include <iostream>
//#include <queue>

#include <vector>
#include <list>

#include <map>
#include <set>

#include "itree.h"

// must be last
//#include <common/assert.h>

#include <cassert>
//#include <controller/itimer.h>

/*
#undef assert
#define assert( a) (0)
*/

struct Ring; 

struct Point
{
	double x, y;
	Point *next, *prev; 
	Point *join;				// we don't actually have to record this here ... 
								// computing intersectionso
	
								// except when we propagate entering/exiting, we need a fast mechanism to
								// to test whether the point is a join point - and we want to avoid doing 
								// set<> lookup 

	Ring * ring;				// only valid during an operation. (should always be valid now)
};


struct Ring
{
	// may not be a closed ring.
//	Point *head; 
//	Point *tail; 

	enum type_t {  type_none, type_a, type_b }  type;		// change name class ? 

	enum orientation_t {  orientation_none, cw, ccw }  orientation;  

	enum topology_t   { topology_none, shared, interior, exterior  } topology;  

	Point * topology_point; 

	int	intersections;	// count

	bool	is_polyline;

	Ring()
		: type( Ring::type_none ),
		orientation( Ring::orientation_none ), 
		topology( Ring::topology_none ), 
		topology_point( NULL),
		intersections( 0),
		is_polyline( false )
	{ }
};



template< class T> 
struct pool_allocator
{
	/*
		fairly fast wrapper over a standard allocator, that deletes all chunks on destruction. 
	*/
	typedef T value_type;
	std::allocator< char>  alloc; 

	std::vector< value_type  *>	v; 

	value_type	*finish; 
	value_type	*current; 

	
	pool_allocator( const std::allocator< char> & alloc = std::allocator< char>() )
		: alloc( alloc), 
		current( 0 ),
		finish( 0)
	{ }

	~pool_allocator()
	{
		// ????  //	for( std::vector< T *>::iterator i ; ; )
		for( unsigned i = 0; i < v.size(); ++i)
		{
			char *p = ( char *) v[ i];  
			alloc.deallocate( p, 1);
		}
	}

	value_type * allocate( size_t sz )	// must be one at the moment
	{
		/*
			normal case only requires pointer comparison and increment.
		*/
		if( current >= finish )
		{
			current = ( T *) alloc.allocate( 1001 * sizeof( T) ); 
			finish = current + 1000;
			v.push_back( current);
		}
		return current++;  
	}	
};




static Point * make_point( pool_allocator< Point> & alloc, double x, double y)
{
	Point *p = alloc.allocate( 1); 
	
	p->x = x; 
	p->y = y; 
	p->ring = NULL;
	p->next = NULL; 
	p->prev = NULL; 
	p->join = NULL;
	//p->inter_prev = NULL;
	return p;
}


static void append_point( Point *head, Point *p )
{
	assert( 0);
	/*
		for test code only, not production 
		complexity is order O( n^2 )
	*/
	assert( head);
	assert( !p->next && ! p->prev );  

	while( head->next )
		head = head->next;
	
	p->prev = head;	
	head->next = p;	

}

static void close( Point * head )	// close_ring
{
	/*
		for test code only, not production 
		complexity is order O( n^2 )
	*/
	assert( head);

	// loop to the end
	Point *tail = head; 
	while( tail->next )
		tail = tail->next;

	head->prev = tail;
	tail->next = head; 
}


std::ostream & operator << ( std::ostream &os , Ring:: type_t type )
{
	// enum type_t {  type_none, type_a, type_b }  type;		// change name class ? 
	switch( type)
	{
		case Ring::type_none: os << "type_none"; break;
		case Ring::type_a: os << "type_a"; break;
		case Ring::type_b: os << "type_b"; break;
		default: assert( 0);
	};
	return os; 
}

std::ostream & operator << ( std::ostream &os , Ring::orientation_t orientation )
{
	//	enum orientation_t {  orientation_none, cw, ccw }  orientation;  
	switch( orientation)
	{
		case Ring::orientation_none: os << "orientation_none"; break;
		case Ring::cw: os << "cw "; break;
		case Ring::ccw: os << "ccw"; break;
		default: assert( 0); 
	}; 
	return os; 	
}

std::ostream & operator << ( std::ostream &os , Ring::topology_t topology )
{
	//	enum topology_t   { topology_none, shared, interior, exterior  } topology;  
	switch( topology)
	{
		case Ring::topology_none: os << "topology_none" ; break; 
		case Ring::shared: os <<   "shared  " ; break; 
		case Ring::interior: os << "interior" ; break; 
		case Ring::exterior: os << "exterior" ; break; 
	}; 
	return os;
}

std::ostream & operator << ( std::ostream &os , Ring & ring )
{
	os << "*type       " << ring.type << std::endl;; 
	os << "is_polyline " << ring.is_polyline << std::endl;; 
	os << "topology    " << ring.topology << std::endl;; 
	os << "orientation " << ring.orientation << std::endl;; 
	os << "top point   " << ring.topology_point << std::endl; 
	os << "intersects  " << ring.intersections << std::endl; 
	return os;
}

std::ostream & operator << ( std::ostream &os , Point * e )
{
	assert( e && e->next ); 
	assert( e->next->prev == e ); 

	assert( e->ring );

	Point *e2 = e->next; 	

	os << e->ring->type << " "; 

	os << (void *)(e) << " " << e->x << " " << e->y << ", ";  
	os << (void *)(e2) << " " << e2->x << " " << e2->y ; 

	if( e->join ) os << " (join " << (void *)( e->join) << ")";

	if( e->ring->topology_point == e)
		os << " *";

	return os; 
} 

#if 0

/*
	- these complicated data structures, didn't yield much improvement over linear scanning
	the sweepline, so they are commented for now.
	- if the normal sweepline contained more elements then they would become more desirable.
*/
interval set is only used one place so lets remove it.

struct IntervalSet
{
	/*
		maintains an interval tree for edges min x to max x 
		we support adding and removing
	*/

private:
	struct Key
	{
		typedef double Underlying_type; 
		double start, stop;

		// we have to give all constructors and assign if we just give Key( double, double )
		// so avoid
	
		Key( )
		{ } 
		explicit Key( double start, double stop)
			: start( start), stop( stop)
		{ } 
	}; 

    typedef Itree::itree< Key, Point *>    tree_type;
    typedef tree_type::iterator             iterator;

	tree_type	tree; 

public:
    typedef tree_type::range_iterator       range_iterator;


	void insert( Point * e )
	{
		assert( e && e->next ); 
		double min = std::min( e->x, e->next->x );
		double max = std::max( e->x, e->next->x );
		assert( min <= max ); 
	    tree.insert(  Key( min, max), e );
	}

	void remove( Point * e )
	{

		//std::cout << "remove " << e << std::endl;

		// what if there are several edges (no pointer guarentees unique ? )
	    // find it
		assert( e && e->next ); 
		double min = std::min( e->x, e->next->x );
		double max = std::max( e->x, e->next->x );
		assert( min <= max ); 

		iterator i = tree.find(  Key( min, max) );

        // may need to iterate on identical intervals to match correct one based on p
		// the unique pointer values of p1, p2
		// since edge could have identical cartesian coords
        while( i != tree.end()) {

			if( e == *i )
			{
				tree.erase( i);
				return ; 
			}
            ++i;
        }
		assert( 0); 
	}

	std::size_t size()
	{
		return tree.size();
	}

	range_iterator find_intervals( double start, double stop )
    {
		assert( start <= stop );
        return tree.in_range( Key( start, stop ) ); 
    } 

    iterator end()
    {
        return tree.end();
    }
};

#endif

#if 0
struct OrderSet
{
	/*
		a set of edges organized on the max y point of an edge
		allows us to lookup what is the next edge that should be removed from the sweepline
		without having to linear scan the sweepline which is expensive.

		we can't use a std::priority_queue because we want ability to randomly remove (eg. when 
		we are splicing in intersection points) as well as from the ends.

		we also have being() end() iterators to get all the edges in the sweepline, in order
		to support topology classification.
	*/

private:
	struct Compare
	{
		bool operator () ( Point *e1, Point *e2 ) const
		{
			// distinguish on max y
			double ay = std::max( e1->y, e1->next->y );
			double by = std::max( e2->y, e2->next->y ); 
			if( ay < by ) return true; 
			if( ay > by ) return false; 

			// (to diambiguate explicit edges)
			// distinguish on p1 pointer value 			
			if( e1 < e2 ) return true;	
			if( e1 > e2 ) return false;

			return false; 
		}
	}; 

	typedef std::multiset< Point *, Compare  >	map_t ; 

	map_t  	m; 

public:
	typedef std::multiset< Point *, Compare  >::const_iterator const_iterator; 

	const_iterator begin() const
	{
		return m.begin();
	}

	const_iterator end() const
	{
		return m.end();
	}

	void insert( Point *e )
	{
		map_t::const_iterator i = m.find( e ); 
		assert( i == m.end() ) ; 
		m.insert( e );
	}

	void remove( Point *e )
	{
		map_t::const_iterator i = m.find( e ); 
		assert( i != m.end() ) ; 
		m.erase( i);
	}

	Point * back() const
	{
		// rather than saying back(), would it be easier to implement std::vector 
		// with an iterator ? 

		// remember we will have direct access to the point vector, so we can directly perform the scan

		/*
			having just this is a lot better interface, than the remove_to_y() 
			because removing explicit edges, allows us to distinguish on point values
			rather than just their max y, avoiding linear scan to remove explict edges
			when max y is identical.
		*/
		assert( ! m.empty()  );
		return * m.begin(); 
	}

	std::size_t size() const
	{
		return m.size();
	}

	bool empty() const
	{
		return m.empty();
	}	
};
#endif

/*
	very important, rather than record intersections, or returning values, 
	indicating what is going on, we should just calculate everything 
	and do the appropriate insertions .
*/


struct CalculateIntersections
{
	/*
		if the class resepecting is the only difference for A/B operations versus
		testing self-intersections, then we should make the operation controlled with
		a boolean. then the same unit-tests can be used.  
	*/
	typedef pool_allocator< Point> point_allocator_type;  

	CalculateIntersections( point_allocator_type & alloc, /*OrderSet & order_set */ /*, IntervalSet & interval_set */ std::vector< Point *> & sweepline )
		: alloc( alloc),
//		order_set( order_set), 
//		interval_set(interval_set )//,
		sweepline( sweepline )
	{ } 

private:
	point_allocator_type  &alloc; 
//	OrderSet		& order_set; 
//	IntervalSet		& interval_set;
	std::vector< Point *> & sweepline;  
//	int				count;

	static void find_intersections( 
		point_allocator_type  &alloc, 
		Point * edge, 
		std::vector< Point *>	& to_add,
//		OrderSet		& order_set, 
		std::vector< Point *> & sweepline 
//		IntervalSet		& interval_set
	)
	{
		/*
			recursive function that removes candidates from the sweepline, and stores the 
			required addition edit in to_add to avoid subdivided edges from rematching. 
		*/

		assert( edge->ring->type == Ring::type_a
			|| edge->ring->type == Ring::type_b ); 
		
		// the recusion must re-lookup for every subdivided edge
		// loop through potential candidates for this edge 

		double xmin = std::min( edge->x, edge->next->x );
		double xmax = std::max( edge->x, edge->next->x );

	

//		for( IntervalSet::range_iterator i = interval_set.find_intervals( xmin, xmax ); 
//			i != interval_set.end(); ++ i)

		for( std::vector< Point *>::iterator i = sweepline.begin(); i != sweepline.end(); ++i)
		{ 
			Point *candidate = *i; 

			/*
				we should control this testing. with a boolean respect_class 
			*/
			assert( candidate->ring->type == Ring::type_a
				|| candidate->ring->type == Ring::type_b ); 

			// only test against other edges in the opposite class 
			if( edge->ring->type == candidate->ring->type )
				continue;


			// filter
			if( std::min( edge->x, edge->next->x) >  std::max( candidate->x, candidate->next->x)) 
				continue;

			if( std::max( edge->x, edge->next->x) <  std::min( candidate->x, candidate->next->x)) 
				continue;

			double x1 = edge->x;
			double y1 = edge->y;
			double x2 = edge->next->x;
			double y2 = edge->next->y;

			double x3 = candidate->x;
			double y3 = candidate->y;
			double x4 = candidate->next->x;
			double y4 = candidate->next->y;

			// demonimator is gcd presumably
			double ua_numerator = ((x4 - x3) * (y1 - y3)) - ((y4 - y3) * (x1 - x3));
			double ub_numerator = ((x2 - x1) * (y1 - y3)) - ((y2 - y1) * (x1 - x3));
			double denominator =  ((y4 - y3) * (x2 - x1)) - ((x4 - x3) * (y2 - y1));

			// these are actually our alpha s ...
			double ua = ua_numerator / denominator;    // this is actually alpha ...
			double ub = ub_numerator / denominator;    // this is actually alpha ...

			if( denominator == 0)
			{
				if( ua_numerator == 0 && ub_numerator == 0) {
					// can be just a single point
				std::cout << " got cooincident " << edge << " " << candidate << std::endl;
			
					// continue (not return) at the moment, because we don't process it.
				}
				// parallel is not an intersection
				// we want to continue (not return)
				//std::cout << " parallel " << std::endl;
			}

			else if( (ua > 0 && ua < 1) && (ub > 0 && ub < 1))
		//  if( (ua >= 0 && ua <= 1) && (ub >= 0 && ub <= 1))
			{
				// std::cout << " got intersect" << std::endl;
		
				// accessing the ring's allocator is crap. 

				// remove the particular candidate so we won't inadvertantly rematch the subdivided edge in recursion
				//interval_set.remove( candidate ); 
				// order_set.remove( candidate ); 
/*
	iterator invalidation ?
	no - it's ok.
*/
				sweepline.erase( i );


				// create point and link in a new edge 
				Point *edge2 = make_point( alloc, x1 + ua * (x2 - x1 ), y1 + ua * ( y2 - y1 ) );
				edge2->ring = edge->ring ;
				splice_point ( edge, edge2 ); 


				// and for the candidate
				Point * candidate2 = make_point( alloc, x3 + ub * (x4 - x3 ), y3 + ub * ( y4 - y3));
				candidate2->ring = candidate->ring ;
				splice_point ( candidate, candidate2 ); 

				// cross link the join points
				edge2->join = candidate2; 
				candidate2->join = edge2; 
//				candidate2->inter_prev = edge2; 

				// and record in the work to be done vector 
				to_add.push_back( candidate ); 
				to_add.push_back( candidate2 ); 

				// record count
				edge->ring->intersections ++;
				candidate->ring->intersections ++;

				// now RECURSE, for each side of the split edge
				find_intersections( alloc, edge, to_add, sweepline );//order_set, interval_set ); 
				find_intersections( alloc, edge2, to_add, sweepline ); //order_set, interval_set  ); 

				/*
					we only have to match against one target. because after that we use 
					the recusion (not the loop) to check for further subdivision
				*/
				// YES IMMEDIATELY RETURN HERE, 
				return; 
			}
		}
	
		// we are at the leaf of the recursion and no (more) intersections can be found, 
		// so just record the required addtion of the current edge to the sweep

		to_add.push_back( edge ); 

		return; 
	}	

	static void splice_point ( Point * edge, Point * point )
	{
		assert( !point->next && !point->prev ); 
		Point *edge2 = edge->next; 

		edge2->prev = point; 
		point->next = edge2;

		point->prev = edge ; 
		edge->next = point; 

		/*assert( edge2->next->prev == edge2); 
		assert( edge->next->prev == edge); 
		assert( point->next->prev == point ); */
	}

public:
	void calculate( Point * edge )
	{
		// add the edge

		// new y of the sweepline
		double y = std::min( edge->y, edge->next->y ); 
	
		// vector of potential intersected edge edits that will be found
		// as well as edges that get removed in the recursion, and then have to be 
		// added back
		std::vector< Point *>	to_add; 
	//	to_add.reset();	// has no affect on speed

		find_intersections( alloc, edge, to_add, sweepline );//order_set, interval_set  ); 
//		assert( interval_set.size() == order_set.size() ); 

		// add the new edges forming intersections into the sweepline structures
		for( size_t i = 0; i < to_add.size(); ++i)
		{
//			interval_set.insert( to_add[ i]  );  
//			order_set.insert( to_add[ i] );  
			sweepline.push_back( to_add[ i] );
		}
//		assert( interval_set.size() == order_set.size() ); 

	}
};


struct DetectOrientation
{
	/*
		Determine if rings are cw or ccw, according to their first sorted edge that appears.
	
		If we have a cheap mechanism of determining orientation, then it is nice to 
		be able to rely on it rather, than having to pass in the orientation of each
		path separately. 

		method, if have a point, known to be on the hull
		http://en.wikipedia.org/wiki/Curve_orientation
	*/
	DetectOrientation( ) 
	{ } 
	
	void detect( const Point * edge )
	{
		/*
			**** THIS RELIES on edge introduction being ordered by y, then x, then set or ring. ***

			this assumes that the first edge given of a particular ring is on the convex hull 
			eg. all edges are sorted by y, then x

			- IMPORTANT - because we force the sort on x, it means that this will always work
			and we shouldn't ever need the area test.  	

			we avoid  o--o---o--o---o   scenarios where we cannot determine.

				o---o		eg. middle point is always to left, meaning lhs will have different y.
			   /
              o
		*/
		
		Ring & ring = * edge->ring; 
		if( ring.orientation != Ring::orientation_none )
			return ; 

		// if it's a polyline - then it doesn't have an orientation
		// the polyline classification is wrong ???
		if( ring.is_polyline )
			return;

		assert( ! ring.is_polyline );

		// std::cout << " classify orientation " << std::endl;
	
		// how do we know that the edge is on the convex hull ? 
		// we would have to know, where it sits  ?????
		assert( edge->next && edge->prev  ); 


		const Point *a = edge->prev;  	
		const Point *b = edge;  	
		const Point *c = edge->next;  	
		assert( a->next == b ); 

		// http://en.wikipedia.org/wiki/Curve_orientation 
		// For numerical reasons, the following equivalent formula for the determinant is commonly used:
		// det(O) &= (x_B-x_A)(y_C-y_A)-(x_C-x_A)(y_B-y_A) \end{align} 

		double det = (b->x - a->x ) * (c->y - a->y ) - (c->x - a->x ) * ( b->y - a-> y);  
		//std::cout << " det is " << det << std::endl;

		// If the determinant is negative, then the polygon is oriented clockwise. If the 
		// determinant is positive, the polygon is oriented counterclockwise.	
		if( det < 0 ) ring.orientation = Ring::cw ; 	
		else ring.orientation = Ring::ccw ; 
#if 0
		else if( det > 0) ring.orientation = Ring::ccw ; 
		else 
		{
			// maybe o=o=o  completely horizontal circle. 
			// or   o=o---o  (no this can be detected)
			//         \o/ 
			// no - both of these can be detected
			// if det == 0, it doesn't even matter.
			
			// no, it should still see o->o->  order 
			// see discussion here http://cgafaq.info/wiki/Simple_Polygon_Orientation

			// except if the points are all on the same point - it won't work (eg possible degenerate cases
			// from prior polygon operations). 

			// use area method - which requires linear scan of all polys
			// http://cgafaq.info/wiki/Polygon_Area
			// http://paulbourke.net/geometry/clockwise/
			double area = 0.0;
			const Point *p1 = edge; 
			do
			{
				// std::cout << " " << p1 << std::endl;
				Point *p2 = p1->next;
				area += p1->x * p2->y - p1->y * p2-> x;  
				p1 = p2; 
			} while( p1 != edge); 
			area *= 0.5f; 
			// std::cout << "area " << area << " " ;
			if( area < 0 ) ring.orientation = Ring::cw; 
			else ring.orientation = Ring::ccw; 
		}
#endif
	}
};


struct DetectTopology
{
	/*
		This doesn't work as written, because it misses opportunities for detection
		we should wait, until all items in opposite class are present before attempting

		Also DetectOrientation requires sort order using x. may be conflict
	*/

	/*
		find a point on the ring, and establish whehter it is exterior or interior of the opposite set of polygons.
		if it is shared, we will wait for another edge to distinguish 

		this works by taking all the edges in the sweep, and sorting them on their x intersection
		with the introduced edge. we then use even_odd classification to determine whether interior/exterior  
	*/
//	DetectTopology( OrderSet & order_set )
//		: order_set( order_set)
	DetectTopology( const std::vector< Point *>	& sweepline )
		: sweepline(  sweepline )
	{ } 

	// these should be const
//	OrderSet		& order_set; 

	const std::vector< Point *>	& sweepline; 

	// opposing intersection points
	std::vector< double>	xs; 


/*
	trying to determine topology might be failing, because of cut lines ?	
	eg when we cut, we slightly change what might be removed from the sweepline ?
*/
	void detect( const Point * edge )
	{
		Ring & ring = * edge->ring; 

		// if we already have determined for the ring, ignore
		// if we have shared, then we will try again.
		if( ring.topology == Ring::exterior 
			|| ring.topology == Ring::interior )
			return; 

		// std::cout << " process topology " << std::endl;

		// sort order guarntees, y, x, then specific polygon. 
		// if edges are added according to y, then x, it should be ok to count from the left 	
		// this applies, if we got a shared, and are trying again ?

		// we have to collect all the edges - and compute their intersections. 
		// and add them to a sort vector.

		// also we have to ignore everything, that's not from our own set.
		// if the edge is horizontal, then we kind of want to know.

/*
	ok there are 3 edges in there

	is the issue to do with a split point - yes one of the edges is a splice ... 
	and maybe isn't properly yet in the sweepline ...

	so we should properly compute whether there is an intersection.

	ahh the reason is that it is part of a join ? 
	
*/

		//std::cout << "-----------------" << std::endl;

		xs.clear();

		for( std::vector< Point *>::const_iterator i = sweepline.begin(); 
			i != sweepline.end(); ++i)
		{
			const Point *candidate = *i; 

			
			// only test/ collect against other edges in the opposite class 
			if( edge->ring->type == candidate->ring->type )
				continue;

			/*
				- we only test topology (for a ring or polyline) against a ring. 
				- eg rings against rings, or polylines against rings, but not rings against polylines.
					because a ring could be simultaneously interor and exterior two diffreent polylines of the same class,
					and it doesn't make any sense.
			*/
			if( candidate->ring->is_polyline )
				continue;
			
#if 0
			// this is our sweepline (which we use to classify)
			double y = std::min( edge->y, edge->next->y );
			double x1 = -200.f; 
			double y1 = y;
			double x2 = +200; 
			double y2 = y;

			double x3 = candidate->x;
			double y3 = candidate->y;
			double x4 = candidate->next->x;
			double y4 = candidate->next->y;

			// demonimator is gcd presumably
			double ua_numerator = ((x4 - x3) * (y1 - y3)) - ((y4 - y3) * (x1 - x3));
			double ub_numerator = ((x2 - x1) * (y1 - y3)) - ((y2 - y1) * (x1 - x3));
			double denominator =  ((y4 - y3) * (x2 - x1)) - ((x4 - x3) * (y2 - y1));

			// these are actually our alpha s ...
			double ua = ua_numerator / denominator;
			double ub = ub_numerator / denominator;

			if( denominator == 0)
			{
				if( ua_numerator == 0 && ub_numerator == 0) {
					std::cout << " got cooincident " << edge << " " << candidate << std::endl;
			
				}
				// parallel is not an intersection
				// we want to continue (not return)
				std::cout << " parallel " << std::endl;
			}

			else if( (ua > 0 && ua < 1) && (ub > 0 && ub < 1))
			{
				std::cout << "here got intersect" << std::endl;
				double x = x3 + ub * (x4 - x3 ); 
				xs.push_back( x );
			}
#endif
	
#if 1
			double y = std::min( edge->y, edge->next->y );
	//		double ymax = std::max( edge->y, edge->next->y );

			double x1 = candidate->x; 
			double y1 = candidate->y; 
			double x2 = candidate->next->x; 
			double y2 = candidate->next->y; 


			/*	
				there may be some edges in the sweepline that don't actually intersect on edge.min( y)
				because they are the result of previously constructed edge intersections. 
				therefore filter them
			*/
//			if( !( y >= std::min( y1, y2)) && y <= std::max( y1, y2))
//			if( !( y >= std::min( y1, y2)) && y < std::max( y1, y2))	// should be non-inclusive a >= b < c 

			/*
				o
				|
				o   we have to avoid taking both of these edges, or it will make the entering/exiting wrong.
				|
				o
			*/
			if( !( y > y1 && y <= y2 ) && !( y > y2 && y <= y1 ) ) 
			{
				// std::cout << "filter edge" << std::endl;
				continue; 
			}	



			if( y1 == y2)
			{
				// horizontal edge
				// this is not correct, we don't know if it intersects or not ...
				// yes, it should have been removed from the sweep ?????
				// NO, because it is not until we process to sweepline that we clear from the sweep
				// can we clear first.  

				//std::cout << "add horizonal" << std::endl;

				assert( edge->y == y1 ); 
				xs.push_back( x1 ); 
				xs.push_back( x2 ); 
			}
			else if( x1 == x2)
			{
				//vertical - probably faster for common case
				//std::cout << "add vertical" << std::endl;
				xs.push_back( x1);
			}
			else
			{
				// WE MUST CHANGE THIS TO GET RID OF THE DIVISION . 
				// because orientation of calculation, could affect result.
				//std::cout << "add normal  " << x1 << " " << y1 << ", " << x2 << " " << y2 << std::endl;
				//std::cout << "candidate " << *candidate->ring << std::endl;
				double x = ( y - y1) * (x2 - x1) / (y2 - y1) + x1;
			
				//std::cout << "x is " << x << std::endl;

				xs.push_back( x);
			}
#endif
		}

		// this test is correct - conceptually. A line through a ring must intersect an even
		// number of times. We already filter for polylines. And avoid coincident points being
		// included more than once

		if( !( xs.size() % 2 == 0))
		{
			// this won't be satisfied for a horizontal - yes it will, because there will be only
			// one horizontal, rather than two non-horizontals.	
			std::cout << "xs.size() " << xs.size() << std::endl;
			assert( 0);
		}

		// check
		assert( xs.size() % 2 == 0);

		// sort the vector.
		std::sort( xs.begin(), xs.end() );

		// i think lower bound is binary search - otherwise would use find 
		// note it returns next higher value,  however indexing from 0 using std::distance is good 
		std::vector< double>::iterator i =    
			std::lower_bound( xs.begin(), xs.end(), edge->x);

		if( i != xs.end() && *i == edge->x) 
		{
			// if we are shared with another edge we cannot discriminate
			ring.topology = Ring::shared;  
			ring.topology_point = (Point *) edge;  
		}
		else
		{
			// else use even_odd rule from beginning
			unsigned dist = std::distance( xs.begin(), i); 
			// std::cout << " dist " << dist << std::endl;
			ring.topology = ( dist % 2 == 0) ? Ring::exterior : Ring::interior;   
			ring.topology_point = (Point *)edge;  
		}
	}
};


// ok, so we can use virtual functions or static parameterization
// virtual would probably be quite fast enough, but lets wait - 
// till have it running better before test cost

struct CalculateStatistics
{
	const std::vector< Point *>  & sweepline ;
//	OrderSet		& order_set; 
	int		n; 
	int		sum; 
	int		max; 

//	CalculateStatistics( OrderSet & order_set )
//		: order_set( order_set),
	CalculateStatistics( const std::vector< Point *>  & sweepline ) 
		: sweepline( sweepline ), 
		n ( 0),
		sum( 0),
		max( 0)
	{ } 

	~CalculateStatistics()
	{
#if 0
		std::cout << "sweepline size average=" << ( double( sum) / n) ; 
		std::cout << " max=" << max ;  
		std::cout << std::endl;
#endif
	}

	void calculate( const Point * edge )
	{
#if 0
		max = std::max( max, int( sweepline.size()) );
		sum += sweepline.size();
		++n ;
#endif
	}
}; 

struct RemoveOld
{
/*
	remove old edges that have fallen out of the sweepline
	this avoids us having to pass the sweepline vector to 
	the edge event processor

	actually this might as well just go in find intersections
	except it might be a heap operation.

	adding an edge would have to be 
*/
	std::vector< Point *>  & sweepline ;


	RemoveOld( std::vector< Point *>  & sweepline ) 
		: sweepline( sweepline )
	{ } 

	struct Pred
	{
		Pred( double y)
			: y( y)
		{ } 
		double y; 

		bool operator () ( const Point *candidate ) const
		{
			return std::max( candidate->y, candidate->next->y ) < y; 
		}
	}; 

	// change the interface to just use y
	void remove( const Point * edge )
	{
		// use the y, to remove items
		double y = std::min( edge->y, edge->next->y ); 

		sweepline.erase( std::remove_if( sweepline.begin(), sweepline.end(), Pred( y)), sweepline.end()); 
	}
};

/////////////////////////////////////////////////////////


#include <agg_path_storage.h>


struct ProcessEdgeEvents2
{
	/*
		maintains state, and has functions to add paths to the core.
		also drives the algorithm, by sorting up the edges and looping the edge events
	*/

	typedef pool_allocator< Point>	point_allocator_type;

	ProcessEdgeEvents2(  
		point_allocator_type	& alloc,
		RemoveOld	& remove_old,
		DetectOrientation & detect_orientation, 
		DetectTopology & detect_topology,
		CalculateIntersections & calculate_intersections, 
		CalculateStatistics  & calculate_statistics
	)

		: alloc( alloc), 
		remove_old( remove_old ),
		calculate_intersections( calculate_intersections),
		detect_orientation( detect_orientation ),
		detect_topology( detect_topology ), 
		calculate_statistics(  calculate_statistics )
	{ } 
private:



	point_allocator_type	& alloc; 

	RemoveOld	& remove_old;
	DetectOrientation & detect_orientation; 
	DetectTopology & detect_topology; 
	CalculateIntersections	& calculate_intersections; 
	CalculateStatistics  & calculate_statistics; 


	struct Compare
	{
		/*
			Comparator for main y edge sort.
		*/
		bool operator () ( Point *e1 , Point * e2 ) const
		{
			// sort on min( y), the criteria for introducing new edges
			// into the sweepline  
			double e1_min = std::min( e1->y, e1->next->y );
			double e2_min = std::min( e2->y, e2->next->y );
			if( e1_min < e2_min ) return true;
			if( e1_min > e2_min ) return false;

/*

			THIS IS NOT ENOUGH - CLASSIFICATION REQUIRES ALL OPPOSING POINTS FOR THE y TO HAVE 
			BEEN ADDED, OTHERWISE WE MISS OPORTUNITY TO CLASSIFY.

			- we have to wait until y advances, and then loop all points.

			// additionally sort on the ring type (eg a or b), so that edges from the same polygon/ring
			// will be introduced immediately next to each other, to help interior/exterior classification is good. 
			if ( e1->ring->type < e2->ring->type )
				return true;
			if ( e1->ring->type < e2->ring->type )
				return false;
*/

			// additionally sort on min( x), for stability 
			// and required to always ensure we can do simple orientation test
			// and also the topology tests
			e1_min = std::min( e1->x, e1->next->x );
			e2_min = std::min( e2->x, e2->next->x );
			if( e1_min < e2_min ) return true;
			if( e1_min > e2_min ) return false;



			/*
				EXTREMELY IMPORTANT 
				extremely important - we should get all of the a's in before doing b's
				otherwise making the sweep satisfy the %2 == 0 can be wrong when testing.

				e1->ring.type < e2->ring.type
		
				but distinguishing just on the rings should be enough
			*/
			return e1->ring  < e2-> ring ; 
			//	return false; 
		}
	}; 

public:

	// ok rather than take points, should we take paths as inputs

	// note we only have to add the edges ...
	// do we want our segment adapator ????
	// no because we want proper point linking. 

	/*
		ok the problem is there may be several polys in the path.
		and we can only represent one. 

		so we either push immediately into a ring.
		or we return a vector< Point *>		re. 
	*/


	/*
		It's not convert it's an add function.
	*/
	template< class PathType >
	static void add_path( 
		point_allocator_type & alloc, 
	//	agg::path_storage & path, 
		PathType			& path,
		Ring::type_t			type,
		std::list< Ring >		& rings, 
		std::vector< Point * >	& edges 
	) {

//		std::cout << "-------------------------" << std::endl;
//		std::cout << "convert path " << std::endl;

		double x, y;
		unsigned cmd;

		path.rewind( 0);

		// we can only do one ring at a time. 
		Point *head = NULL; 
		Point *tail = NULL; 

		/*
			doing the insertion and then modifying in place is horible.
	
			it would be much better with a current_ring reference that points
			at the back.
		*/
		while( ! agg::is_stop( cmd = path.vertex( &x, &y) ) )
		{ 	
			if( agg::is_move_to( cmd ))
			{
//				std::cout << "got move_to" << std::endl;
	
				// push a new ring
				rings.push_back( Ring() );
				Ring & ring = rings.back();
				ring.type = type;
				// set to polyline, unless we determine later it's closed
				ring.is_polyline = true;

				// create the point
				Point *p = make_point( alloc, x, y ); 
				// ref the ring
				p->ring = & rings.back(); 

				// link it
				tail = p;
				head = p;
			}
			else if ( agg::is_line_to( cmd))
			{
		//		std::cout << "got line_to" << std::endl;
			
				// create point	
				Point *p = make_point( alloc, x, y ); 
				assert( ! rings.empty());
				// ref the ring
				p->ring = & rings.back(); 

				// link it
				tail->next = p; 
				p->prev = tail; 
				tail = p;

				// record the edge (p, p->next)
				edges.push_back( p->prev );  
			}
			else if( agg::is_end_poly( cmd))
			{
//				std::cout << "got close" << std::endl;

				// link the close
				tail->next = head;
				head->prev = tail;

				edges.push_back( tail );  

				Ring & ring = rings.back();
				ring.is_polyline = false;
			} 
			// ending a polyline doesn't require anything
			
			else assert( 0); 
		}


	}

	/*
		THERE IS NO LARGE DATA STORED IN THE RINGS, we should just use a vector<>
		rather than list push_back().

		it's 32 bytes.
		
		No. The reason we use list is so the position doesn't change. 
		Instead allocate dynamically.
	*/	
	
	template< class PathType >
	void run( PathType &a, PathType &b, std::list< Ring >	 & rings ) 
	{
		//Timer	timer; 
		//timer.restart();

	//	std::list< Ring >		rings; 

		std::vector< Point * >	edges;

		// add the paths to the rings and edges
		add_path( alloc,  a, Ring::type_a, rings, edges ); 
		add_path( alloc,  b, Ring::type_b, rings, edges ); 

/*
		std::cout << "----------------------------" << std::endl;
		std::cout << "sizeof( Ring) " << sizeof( Ring ) << std::endl;
		std::cout << "edges.size() " << edges.size() << std::endl;
		std::cout << "rings.size() " << rings.size() << std::endl;
		std::cout << std::endl; 

		for( std::list< Ring>::iterator i = rings.begin(); i != rings.end(); ++ i)
		{
			std::cout << *i << std::endl;
		} 
*/

//		std::cout << "convert " << timer.elapsed() << "ms" << std::endl;
//		timer.restart();

		// sort the edges, 
		std::stable_sort( edges.begin(), edges.end(), Compare() );

/*
		std::cout << "sort " << timer.elapsed() << "ms" << std::endl;
		timer.restart();
*/

		for( unsigned i = 0; i < edges.size(); ++i)
		{
			Point * edge = edges[ i]; 

			assert( edge->ring->type == Ring::type_a 
				|| edge->ring->type == Ring::type_b );

			double y = std::min( edge->y, edge->next->y ); 

/*
			std::cout 
				<< "----------------------------------" << std::endl 
				<< i 
				<< " add edge " << (edge->ring->type == Ring::type_a ? "a" : "b" ) 
				<< "  " << edge  
				<< " y is " << y  
				<< "  sweepline.size " << order_set.size() <<  std::endl;
*/

/*
	we should make a class/ process remove old edges

	then we wouldn't need to inject the sweepline in here.
*/

			remove_old.remove( edge );
			detect_orientation.detect( edge ); 
			detect_topology.detect( edge ); 
			calculate_intersections.calculate( edge );
			calculate_statistics.calculate( edge );
		}	
/*
		std::cout << "sweep " << timer.elapsed() << "ms" << std::endl;
		timer.restart();
*/

	}

};



struct CalculateIntersectingPolys
{
	/*
		there is currently no state here - it doesn't really need to be a class 
	*/

	void run( const std::list< Ring > & rings, agg::path_storage & result )
	{
		
		// create two sets entering and exiting and fill them in

		typedef std::set< Point * >	joins_t; 
		joins_t	entering_; 
		joins_t	exiting_; 

		for( std::list< Ring >::const_iterator i = rings.begin(); i != rings.end(); ++i )
		{
			const Ring & ring = *i; 

			if( ring.is_polyline )
				continue;

			assert( ring.topology_point );
			assert( ring.topology == Ring::interior || ring.topology == Ring::exterior  );
			assert( ring.orientation == Ring::cw || ring.orientation == Ring::ccw ); 


			if( ring.intersections == 0)
				continue;

//			std::cout << "ring " << 	ring.topology  << " " << ring.orientation << " " << ring.type << " cuts " << ring.intersections << std::endl; 

			enum transition_t { entering, exiting } ;  
			transition_t	current; 

			if( ring.topology == Ring::exterior )
				current = exiting; 
			else if ( ring.topology == Ring::interior )
				current = entering;
			else 
				assert( 0);
		/*
			VERY IMPORTANT 
				
				the entering / exiting distinction already includes information about cw / ccw
				because it will be marked (entering/exiting) according to the cw / ccw, because
				we always follow next.

				the cw/ccw by itself is only useful for working with non-intersecting holes.
		*/


			Point *p = ring.topology_point;  
			do 
			{
				// a join point, that is not a polyline
				if( p->join  &&  ! p->join->ring->is_polyline )
				{


					if( current == entering )
					{
						current = exiting; 
						exiting_.insert( p); 
					}
					else if ( current == exiting )
					{
						current = entering; 
						entering_.insert( p); 
					}
					else assert( 0);
				}
				p = p->next; 
			}
			while( p != ring.topology_point ); 
		}	

		assert( entering_.size() % 2 == 0 );
		assert( exiting_.size() % 2 == 0 );
		assert( entering_.size() == exiting_.size() );


//		std::cout << "entering_: " << entering_.size() << std::endl;
//		std::cout << "exiting_ : " << exiting_.size() << std::endl;
		//for( joins_t::const_iterator i = entering_.begin(); i != entering_.end(); ++i)
		//	std::cout << *i << " " << std::endl;  
//		std::cout << std::endl;


		// creating two seperate lists probably is simpler. 
		// we only really need one, but we can do better checks with 2 and there is no cost
		
		enum direction_t { left, right } ;  
		direction_t		direction; 

		// now process the entering list, and trace 

		while( ! entering_.empty())
		{
			// pop a point
			// hang on we have to immediately determine the direction ...

			Point * head = * entering_.begin(); 
			direction = right; 
			entering_.erase( head);
			
			Point *p = head; 
			result.move_to( p->x, p->y ); 	
			do
			{
				if( direction == right)
					p = p->next; 
				else if( direction == left)
					p = p->prev;
				else assert( 0);

				result.line_to( p->x, p->y ); 


				// hang on - we know our very first point is a join - we want to stay on next
				// before swapping

				if( p->join && ! p->join->ring->is_polyline  )		 
				{
					// we always take the join ??? 
					p = p->join; 
					assert( p->join );

					if( entering_.find( p ) != entering_.end() )
					{
						// following next would be entering, therefore follow next
						direction = right; 
						entering_.erase( p);
					}
					else if( exiting_.find( p) != exiting_.end() )
					{
						// following next would be exiting, therefore follow prev
						direction = left; 
						exiting_.erase( p);
					}
					else if( p == head)
					{
						// we came full circle
						break;
					}
					else assert( 0);
				}
	

			} while( true);

			result.close_polygon();
		}
	}
};



struct CalculateNonIntersectingPolys
{
	/*
		there is currently no state here - it doesn't need to be a class 


		we should have a test, that the points taken are the same as the input agg points

		we - just take the ring if it is interior with respect to the other poly class 
		a hole of a poly, or an island in a hole will still be regarded as exterior.
	*/


	void run( const std::list< Ring > & rings, agg::path_storage & result )
	{
		// ok - exterior and interior apply to the whole ring.		
		
		for( std::list< Ring >::const_iterator i = rings.begin(); i != rings.end(); ++i )
		{
			const Ring & ring = *i; 
			if( ring.is_polyline )
				continue;

			assert( ring.topology_point );
			assert( ring.topology == Ring::interior || ring.topology == Ring::exterior  );
			assert( ring.orientation == Ring::cw || ring.orientation == Ring::ccw ); 

			/*
			note that we need to trace to respect the direction. (except it's always next).
			eg if cw then p->next is cw. if ccw then p->next is ccw 
			*/
			if( ring.intersections != 0)
				continue;
		
			if( ring.topology == Ring::interior )
			{
				Point *p = ring.topology_point;  
				result.move_to( p->x, p->y ); 
				p = p->next; 
				do {
					result.line_to( p->x, p->y ); 
					p = p->next;
				}
				while( p != ring.topology_point ); 
				result.close_polygon();
			}

		}

	}
};


struct CalculatePolylines 
{
	/*
		ok, we have to know whether it's exterior or interior, and then just count. 
	*/
	void run( const std::list< Ring > & rings, agg::path_storage & result )
	{
		for( std::list< Ring >::const_iterator i = rings.begin(); i != rings.end(); ++i )
		{
			const Ring & ring = *i; 

			//std::cout << "ring.is_polyline=" <<  ring.is_polyline << " topology=" << ring.topology << std::endl;

			// only polylines
			if( ! ring.is_polyline )
				continue;



			assert( ring.topology_point );

			// ok, the topology point perhaps can't be used as the start and finish.
			// so we have to move to the beginning or the end, and then try to 
			// trace it out.

			enum transition_t { entering, exiting } ;  
			transition_t	current; 

			if( ring.topology == Ring::exterior )
				current = exiting; 
			else if ( ring.topology == Ring::interior )
				current = entering;
			else 
				assert( 0);

			// std::cout << "---------------" << std::endl;

			// rewind back to the start of the polyline, keeping track
			Point *p = ring.topology_point;  
			while( p->prev )					// need to check to stop at intersections as well...
			{
				if( p->join )		
				{
					if( current == exiting) 
						current = entering;
					else if( current == entering ) 
						current = exiting;
					else assert( 0);
				}
				p = p->prev;
			}

			// now we have to raise an lower the pen...

			if( current == entering )
			{
				//std::cout << "initial state is entered " << std::endl;
				result.move_to( p->x, p->y ); 
			}

			// iterate over the line, putting pen down or up
			while( p)
			{
				if( current == entering )
					result.line_to( p->x, p->y ); 

				if( p->join )		
				{
					//std::cout << "got join " << p->x << " " << p->y << std::endl;

					if( current == exiting) 
					{
						// put pen down
						result.move_to( p->x, p->y ); 
						current = entering;
					}
					else if( current == entering ) 
					{
						current = exiting;
					}
				}


				p = p->next;
			}
	

#if 0
			result.move_to( p->x, p->y ); 
			p = p->next; 
			do {								// should be a while
				result.line_to( p->x, p->y ); 
				p = p->next;
			}
			while( p );
#endif
		}
	}

};





#include <common/path_reader.h>

void calculate_intersection( const agg::path_storage &a1, const agg::path_storage &b1, agg::path_storage & result )
{
	/*
		- all of this stuff should be instantiated in the ProcessEdgeEvents class rather than here.	
		- not sure if we want to inject loggers etc, this might be the place
	*/

	path_reader		a( a1); 
	path_reader		b( b1); 


	pool_allocator< Point > 	alloc; 
	 
	//IntervalSet			interval_set; 
	//OrderSet			order_set ; 

	std::vector< Point *>	sweepline; 

	RemoveOld		remove_old( sweepline ) ;

	DetectOrientation	detect_orientation ;  

	DetectTopology		detect_topology( sweepline ); 

	CalculateIntersections	calculate_intersections( alloc, sweepline ); 

	CalculateStatistics		calculate_statistics( sweepline ); 	

	ProcessEdgeEvents2	process_edge_events( 
		alloc,
		remove_old,
		detect_orientation, 
		detect_topology, 
		calculate_intersections, 
		calculate_statistics
	); 
	

	// rather than producing the result this should produce the rings, 
	// eg it does the sorts and builds the rings.

	std::list< Ring >	 rings;  

	process_edge_events.run( a, b, rings ) ; 


	CalculateIntersectingPolys	calculate_intersecting;	
	calculate_intersecting.run( rings, result );	

	CalculateNonIntersectingPolys	calculate_non_intersecting;
	calculate_non_intersecting.run( rings, result );	

	CalculatePolylines  calculate_polylines;
	calculate_polylines.run( rings, result );

}



//////////////////////////////////////////////////////////////////







#if 0
	void run( Point * a, Point * b, agg::path_storage & result )
	{
		std::list< Ring >		rings; 

		std::vector< Point * >	edges;

		rings.push_back( Ring() );
		rings.back().type = Ring::type_a ;

		Point *pa = a; 
		do 
		{
			// set the ring
			pa->ring = & rings.back(); 

			if( pa->next)
				edges.push_back( pa);  
				
			pa = pa->next; 
		} while( pa != a); 


		rings.push_back( Ring() );
		rings.back().type = Ring::type_b;
		Point *pb = b; 
		do 
		{
			pb->ring = & rings.back(); 
			assert( pb->ring->type == Ring::type_b );

			if( pb->next)
				edges.push_back( pb);  
				
			pb = pb->next; 
		} while( pb != b ); 
#endif



#if 0
	std::cout << "here sweepline " << sweepline.size() << std::endl;
	std::cout << "here order_set " << order_set.size() << std::endl;

	// Ok, now hang on, 
	// so we detect an edge, then pull it out, 
	// put in the new edges and carry on.

/*	
	for( IntervalSet::range_iterator i = sweepline.find_intervals( 20, 30 ); i != sweepline.end(); ++i )
	{ 
		const Edge &edge = *i; 	

		double min = std::min( edge.p1->x, edge.p2->x );
		double max = std::max( edge.p1->x, edge.p2->x );

		assert( min <= 30  );
		assert( max >= 20  );
		std::cout << min << "->" << max  << std::endl;
	}
*/

	while( ! order_set.empty() )
	{
		const Edge & edge = order_set.back () ; 
		std::cout << edge << " max y " << std::max( edge.p1->y, edge.p2->y ) << std::endl;

		order_set.remove( order_set.back() );
	}


/*
	for( unsigned i = 0; i < edges.size(); ++i)
	{
		const Edge & edge = edges[ i] ;
		bool ret; 
		ret = sweepline.remove( edge ); 
		assert( ret);

		ret = order_set.remove( edge ); 
		assert( ret);
	}	
*/
	std::cout << "sweepline " << sweepline.size() << std::endl;

/*
	so if we have two sets of geometries, then how do we merge them ?

	1) one edge at a time from one set.
	2) then take an edge from the other set and see if it could intercept  
	
	- the first set we keep in the sweepline
	- the second we just peel them off.

	- so we should be able to write the first loop fairly simply

*/


#endif
#if 0
struct Ring /* Polygon */
{
	Point	*head; 	
	Point	*tail; 	
	// possibly some information.
	// bool closed ? 
};

static void appen( Ring *ring, Point *point)
{


}
#endif


#if 0

// a fascade for such simple operations is just unwanted coupling

struct Sweepline
{
	/*
		- the sweepline just organizes the interval_set and order_set 
		and simplified access
		- not certain whether this is creating coupling.	
	*/
	IntervalSet & interval_set; 
	OrderSet & order_set; 


	Sweepline(  IntervalSet & interval_set, OrderSet & order_set )
		: interval_set( interval_set),
		order_set( order_set )
	{ } 

	void insert( const Edge & edge )
	{
		assert( interval_set.size() == order_set.size() ); 
		interval_set.insert( edge);
		order_set.insert( edge); 
	}

	void remove( const Edge & edge )
	{
		bool ret = interval_set.remove( edge) ;
		assert( ret); 
		ret = order_set.remove( edge); 
		assert( ret);
	}

	std::size_t size() const
	{
		return interval_set.size() ; 
	}
}; 
#endif

/*
	ok, but hang on. if we create additional edges, when computing intersections
	
	will this stuff up the edge list processing ? 

	i think it may be ok.

*/


		/*
			******************************************
			record_intersections chops up the edge, so we cannot just use it again.
			instead we have to get the two edges and reapply.

			OR WHY NOT JUST PUSH BACK ONTO THE EVENT LIST.

			rather than have a vector, maintain a set. - not sure if 
				
			use a set rather than a sort()		  
		
			eg. 
				
				do the edit on the edge and target. 	

				then push the new edges back into the event list
				where they will get put into the sweepline again. 

				No- because we don't want to test against the same target twice.
		*/

		/*  if we split an edge 
			we should then retest each half of that edge. (use recursion)
			also we shouldn't retest on the target, so it would probably help to remove it.

		*/


		/*
			ok, the same edge is being manipulated ... 	
			therefore it is different each time. 

			eg. if the edge intersects with two different targets - then it will get chopped.
			by the first.

			perhaps the logic of getting all the targets is wrong. 

			if we chop the edge up into two pieces then don't we have to check those
			two pieces against all the targets ? 
		*/


/*

	Ok it cannot find the element in the interval_tree despite it being there. 
		either bacause
		(1) the comparator is wrong.	
		(2) comparator wrong because of math error
		(3) the element has been changed out from under us - and its postion in the set tree is wrong.
			
			but we are always removing before we reinsert with changed p.

		remember the remove is being called from the main edge event vector.
		but this is presorted because we do manipulate underneath it.
		ok, it's a brand new edge that is a freaking split point.


		---------------------------------------

		the next pointer has been changed to something else .	
			therefore the y position in the ordered set is going to be wrong.
	
		create the 	
		 edge intersect splice {0xcc17c0 271.405 62.2898, 321 159},
		and insert it.

		then the next pointer has been changed to something else ???
		because its the target of something.

		back of order_set {0xcc17c0 271.405 62.2898, 311.88 141.216}, 
		  removing (20)    {0xcc17c0 271.405 62.2898, 311.88 141.216},

	
	it did get removed ok, from the interval tree. but the interval tree hard codes its nodes.
*/

#if 0

				{
					Point * split = make_point( alloc, x3 + ub * (x4 - x3 ), y3 + ub * ( y4 - y3));
					splice_point ( target, split ); 

					std::cout << "intersect target " << target << " splice " << split <<  std::endl; 

					// add to the sweepline
					interval_set.insert( target); 
					order_set.insert( target ); 
					assert( interval_set.size() == order_set.size() ); 

					interval_set.insert( split); 
					order_set.insert( split); 
					assert( interval_set.size() == order_set.size() ); 
				}
				
				// and create our new edge ( ok this is still complicated ).  

				{
					// we want a splice function
					// create the split point
					Point * split = make_point( alloc, x1 + ua * (x2 - x1 ), y1 + ua * ( y2 - y1 ) );
					splice_point ( edge, split ); 

					std::cout << "intersect edge " << edge << " splice " << split <<  std::endl; 

					// add to the sweepline
					interval_set.insert( edge ); 
					order_set.insert( edge); 
					assert( interval_set.size() == order_set.size() ); 

					interval_set.insert( split); 
					order_set.insert( split); 
					assert( interval_set.size() == order_set.size() ); 
				}

#endif

		// will have to be passed through the recursion  

		// also we are removing the current element ????
		// when we remove the candidate ??
		// no we can just return from the recusions - whether it was added or not.

/*
		we shoudn't be manipulating the set as we go. 
		but how ?

		OR - MAYBE WE DO HAVE TO COPY the targets, so that we can pass them through 
		all the recursions, and edit, and add and remove ? 

		-- there are two operations 
			(1) splitting the edge and adding. Which fucks up the next test.
			(2) splitting the target (which we . 

		-- If we record the candidates (in a vector), then we can freely edit the sweepline
	
		when we split our edge. we want to re-do the candidate lookup again (except without that particular candidate).  	
	
		but in a recursion, if we lookup again	

		**** we don't really want to edit the candidates vector, until everything is processed ????
		the problem is flagging.

		or the sweepline needs a flag, on edges whether it's being processed. 
*/





#if 0 
static void append_point( Point *head, Point *p)
{
/*
	for test code only, not production 
	coplexity is order O = o2
*/
	assert( head);
	assert( !p->next && ! p->prev );  

	while( head->next )
		head = head->next;
	
	head->next = p;	
	p->prev = head;	
}
#endif




#if 0

// must get rid of this. 
// should change name to append_point to distinguish from edge operations
static Point * append( Point *a, Point *b)
{
	assert( ! a->next && ! b->next ); 
	a->next = b; 
	b->prev = a; 
	return b; // tail (not head)
}
#endif


/*
static void append( Ring & ring, double x, double y)
{
	Point *p = ring.alloc.allocate( 1); 

	p->ring = & ring;

	p->x = x; 
	p->y = y; 
	p->next = NULL; 
	p->prev = NULL; 

	if( !ring.tail)
	{
		assert( ! ring.head);
		ring.head = p; 
		ring.tail = p; 
	}
	else 
	{
		assert( ring.head);
		p->prev = ring.tail;
		ring.tail->next = p;  
		ring.tail = p;
	}
}
*/

/*
	am a bit unsure about copy-constructing the Ring object
*/

//typedef std::vector< boost::variant< Ring, Point, Line>  >	Geometry; 

//typedef std::vector< Ring>	Geometry; 




/*
std::ostream & operator << ( std::ostream & os, Point *p )
{
	do {
		os << "{" ;  
		os << p->x << " " << p->y ;  
		os << "}, ";
		p = p->next; 
	}
	while( p); 			
	return os; 
}*/ 

#if 0
		if( ring.type == Ring::type_a )
		{
			// goes straight into the sweepline


	
			// now add to the sweepline	
			interval_set.insert( edge);  
			order_set.insert( edge );  
		}	

		else if( ring.type == Ring::type_b )
		{
			// do intersections against type_a

			std::vector< Point *>	to_add; 

			find_intersections( alloc, edge, to_add, order_set, interval_set  ); 
			assert( interval_set.size() == order_set.size() ); 

			// now edit the new edges into the sweep
			for( size_t i = 0; i < to_add.size(); ++i)
			{
				interval_set.insert( to_add[ i]  );  
				order_set.insert( to_add[ i] );  
			}
			assert( interval_set.size() == order_set.size() ); 
		}
		else 
		{
			assert( 0);
		}
#endif
#if 0
//template<class VC> 
//unsigned path_base<VC>::perceive_polygon_orientation(unsigned start,
//													 unsigned end)

static bool perceive_polygon_orientation( Point *p1 )
{
	// http://cgafaq.info/wiki/Polygon_Area
	// http://paulbourke.net/geometry/clockwise/
	double area = 0.0;
	Point *head = p1; 
	do
	{
		std::cout << " " << p1 << std::endl;
		Point *p2 = p1->next;
		area += p1->x * p2->y - p1->y * p2-> x;  
		p1 = p2; 
	} while( p1 != head); 
	area *= 0.5f; 
	std::cout << "area " << area << " " ;
	if( area < 0 ) std::cout << "(cw)"; 
	else std::cout << "ccw "  ; 
	std::cout << std::endl;

//	return (area < 0.0) ? path_flags_cw : path_flags_ccw;
}
#endif


#if 0
		for( std::list< Ring >::const_iterator i = rings.begin(); i != rings.end(); ++i )
		{
			const Ring & ring = *i; 
			std::cout << "ring " << 	ring.topology  << " " << ring.orientation << " " << ring.type << " cuts " << ring.intersections << std::endl; 
		
			// ccw and exterior and turn left -> union
			// ccw and interior and turn right -> intersection

			if( ring.orientation == Ring::cw && ring.topology == Ring::interior ) 
				// || ring.orientation == Ring::cw && ring.topology == Ring::exterior ) 
			{
		//		std::cout << "got ccw " << std::endl;

				Point *p = ring.topology_point; 
				 result.move_to( p->x, p->y );
				do 
				{
					std::cout << "p " << p << std::endl;

					// the ordering of these might be important ...
					if( p->join)
						p = p->join; 
					else 
						p = p->next; 

					result.line_to( p->x, p->y ); 



				} while( p != ring.topology_point );
		
				// close	
				result.close_polygon();
			}

		}
#endif
