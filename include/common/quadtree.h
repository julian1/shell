/*
	it seems quite a bit slower, having introduced the ref counted items.
	Would a simple scoped_ptr<> work ????
	
	It would require rewriting all the assignments to use .reset()
*/

// g++ -Wall main.cpp -I /opt/boost/include/ /opt/boost/lib/libboost_test_exec_monitor.a 
// g++ main.cpp -I /opt/boost/include/

/*
	- we almost certainly want to use std::pair<> rect and user value
	the QuadRect structure doesn't even need to be exposed coudl do
	
	find( x1,x2,y1,y2 );


	we don't push objects that cannot fit into a subnode, into a subnode
	to be effecient we would have to mark, to avoid 

	we also don't want to keep creating subchild, to exactly wrap the size
	of something. we only want to subdivide if there are too many entries

	this this code is very self contained and potentially useful in other places - 
	it probably makes sense to remove the boost dependency

	note it is normal that all subtrees are made or not at all, but we avoid this
	for cases where nothign will be added. (don't know if advantage or not - since
	we avoid calculating the subnodes on each split ? )	
*/

#include <common/ptr.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <iostream>
#include <cassert>
//#include <vector>
#include <vector>

// we either put the rect inside the node	 (advantage is that its template able )
// or 

// or we use underscores and leave out , then typedef in the class ? 

struct QuadRect
{
	explicit QuadRect( double x1, double y1, double x2, double y2)
		: x1( x1), y1( y1), x2( x2), y2( y2)
	{ 
		// orientate
		if( x2 > x1)
		{
			// how are we allowed to do this if they're const ?
			std::swap( x1, x2);
			std::swap( y1, y2);
		}
	} 
	// should potentially rename top, bottom, left,right etc
	// min
	double x1, y1; 
	double x2, y2; 
private:
//	QuadRect( const QuadRect &);
};

#if 0
static std::ostream & operator << ( std::ostream &os, const QuadRect &rect )
{
	os << rect.x1 << "," << rect.y1 << " " << rect.x2 << "," << rect.y2 ; 
	return os;
}
#endif

// these functions are useful generally 

static bool contains( const QuadRect &parent, const QuadRect &child)
{
	// better names for params ?
	// is child contains within rect of parent
	return child.x1 > parent.x1 && child.x2 < parent.x2
		&& child.y1 > parent.y1 && child.y2 < parent.y2 ; 
}

static bool intersects( const QuadRect &a, const QuadRect &b)
{
	// assumes top,bottom, left,right orientation of x1,y1 etc
	if( a.y2 < b.y1) return false;
	if( a.y1 > b.y2) return false;
	if( a.x2 < b.x1) return false;
	if( a.x1 > b.x2) return false;
	return true;
}

// problem is namespace

// ok so we want to template it ??
// with a value_type ?

template< class T> 
struct Node
{

	// this is wrong and causing the problems, 
	// value_type is not value_type but the T ??
	// actuall it's correct
	// except in a std::map it is the pair

	typedef std::pair< QuadRect, T>	value_type;

	Node( const QuadRect &rect)
		: count( 0 ),
		rect( rect)
	{ 
/*		child[ 0] = NULL;
		child[ 1] = NULL;
		child[ 2] = NULL;
		child[ 3] = NULL; */
	} 

	unsigned count; 

	// the rect of this node
	const QuadRect	rect; 

	// the child nodes
	ptr< Node>	child[ 4];

	// items that cannot be moved to children because too big at this depth in tree
	// we keep in seperate vector to avoid rechecking each time when split() called
	std::vector< value_type >	big_items;	
	
	// current items of this node (generally max fixed size - but we use vector)
	std::vector< value_type >	items;

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 

private:
	Node( const Node &);
	Node & operator =( const Node &);
};


template< class T> ptr< Node< T> > 
copy_tree( const ptr< Node< T> > &src )
{
	// copy and return an identical tree 

	ptr< Node< T> >  p = new Node< T> ( src->rect );   
	p->big_items = src->big_items;
	p->items = src->items;
	
	for( size_t i = 0; i < 4; ++i)
	{
		if( src->child[ i] )
		{
			p->child[ i] = copy_tree( src->child[ i] );
		}
	}
	return p;
}



/*
template< class T> 
struct QuadTree
{
	// actually a region quadtree
*/
	// key type


//	ptr< Node>	root;
//	Node	root;

template< class T> 
void split( const ptr< Node< T> >  &node ) 
{
	//typedef typename Node< T> :: value_type	value_type; 
	typedef std::pair< QuadRect, T >	value_type; 

	// this is recursive
	//std::cout << "split " << node->items.size() << std::endl;
	// the quads may already exist ... we shouldn't be calculating if
	// we have the children ?? 

	if( !node->child[ 0])
	{
		double	centre_x( ( node->rect.x1 + node->rect.x2) * .5); 
		double	centre_y( ( node->rect.y1 + node->rect.y2) * .5); 

		// we can use fixed size allocator if we want
		// raw speed
		node->child[ 0] = new Node< T> ( QuadRect( node->rect.x1, node->rect.y1, centre_x, centre_y));	// 0,0 5,5
		node->child[ 1] = new Node< T> ( QuadRect( centre_x, node->rect.y1, node->rect.x2, centre_y)); // 5,0 10,5
		node->child[ 2] = new Node< T> ( QuadRect( node->rect.x1, centre_y, centre_x, node->rect.y2));	// 0,5 5,10
		node->child[ 3] = new Node< T> ( QuadRect( centre_x, centre_y, node->rect.x2, node->rect.y2)); // 0,5 5,10
	}


	foreach( const value_type &item, node->items)
	{
		// could fit in child rect
		bool could_fit = false;
		for( size_t i = 0; i < 4; ++i)
		{
			// std::cout << "item -> " << item << " in " << quadrant[ i] << std::endl;
			if( contains( node->child[ i]->rect, item.first ))  
			{
				insert( node->child[ i], item );	
				could_fit = true;
				break;
			}
		}
		if( !could_fit)
		{
			// if object didn't fit in child insert to big_items
			// std::cout << "not could_fit" << std::endl;
			node->big_items.push_back( item );
		}
	}

	// clear out node items
	node->items.clear();
	// hand on inserting items needs to be recursive
}


template< class T> 	
void insert( const ptr< Node< T> > &node, const std::pair<  QuadRect, T > & value ) 
{
	const double spill_factor = 10; 

	node->items.push_back( value );
	// we spill over - then try to insert to sub node
	if( node->items.size() > spill_factor )
	{
		// try to split node up 
		split( node);
	}
}

template< class T> 
void candidate_intersections( const ptr< Node< T> > &node, const QuadRect &rect, std::vector< std::pair<  QuadRect, T > > & result ) 
{
	typedef std::pair< QuadRect, T >	value_type; 

	// return the number of nodes visited in search path
	// which is a useful metric

	/*
		the logic of these functions is not good. (almost wrong) 
		the intersect functions should be placed here
		in the top two foreach loops. to be more selective.

		eg. we can be more selective, by applying the rect argument here
		before adding to the result vector.

		note how the test
	*/


	// insert all items from normal and fixed vector
	foreach( const value_type &item, node->items)
		result.push_back( item);

	foreach( const value_type &item, node->big_items)
		result.push_back( item);

	// now check quads we have to descent into 
	for( size_t i = 0; i < 4; ++i)
	{
		if( node->child[ i]) 
		{
			if( intersects( node->child[ i]->rect, rect))
			{
				candidate_intersections( node->child[ i], rect, result ); 
			}
		}
	}
}


template< class T> 
size_t candidate_intersections_count( const ptr< Node< T> > &root,  const QuadRect &rect ) 
{
	// return a count of the number of intersections. Avoids overhead of pushing
	// the intersections into the vector
	// this should be rewritten, but there's a problem, in testing the top most node.

	// we should be avoiding instantiating the vector.

	typedef std::pair< QuadRect, T >	value_type; 

	size_t n = 0;
	// for faster operation we can return immediately while inside the recursion
	std::vector< value_type > result; 
	candidate_intersections( root, rect, result);

	foreach( const value_type &candidate, result)
	{
		if( intersects( rect, candidate.first ))
			++n;
	}
	return n;
}

template< class T> 
size_t node_count( const ptr< Node< T> > &node  ) 
{
	size_t n = 0; 
	for( size_t i = 0; i < 4; ++i)
		if( node->child[ i])
		{
			++n; 
			n += node_count( *node->child[ i]);
		}
	return n;
}


template< class T> 
size_t size(  const ptr< Node< T> > &node ) 
{
	// change name size()
	// how many items
	// change name to n_
	size_t n = 0; 
	n += node->items.size();
	n += node->big_items.size();
	for( size_t i = 0; i < 4; ++i)
		if( node->child[ i] )
			n += size( *node->child[ i]);
	return n;
}



/*
	QuadTree( const QuadTree &rect); 
	QuadTree & operator = ( const QuadTree &rect); 
public:

	QuadTree( const QuadRect &rect)
//		: root( rect ) //new Node( rect ) )
	{ 
		root = new Node( rect );
	} 
	~QuadTree( )
	{ } 

	// we haven't actually provided a function to return the intersections
	// 


	void clear( const QuadRect &rect)
	{
		// clear and reinit
		// memory leak
		// this is all incredibly hackish
		root->clear();// = Node( rect);
	}

	void insert( const QuadRect &rect, const T& value ) 
	{
		// convenience
		insert( value_type( rect, value) ) ; 
	}

	void insert( const value_type & value ) 
	{
		insert( root, value ) ; 
	}
	size_t candidate_intersections( const QuadRect &rect, std::vector< value_type > &candidates ) const
	{
		return candidate_intersections( root, rect, candidates);
	}	

	size_t nodes_visited( const QuadRect &rect ) const
	{
		std::vector<QuadRect > candidates;
		return candidate_intersections( root, rect, candidates );
	}
	size_t size() const
	{
		return size( root);
	}
*/


//static void insert( Node &node, const QuadRect &item ); 




// these are just some interesting / useful functions
// we want early bail out operations in recursion when detect a match, and possibility
// to avoid having to push matches into vector (allocation is expensive) 

// we want to count total nodes




// it would be really good - if we could keep the base functionality of 
// the tree separate from extended functions and walks
// eg. if resize the top level node


// almost certain we want a vector as the container structure, to reduce 
// allocation -- except that we only have 5 or 10 items in the node.  
// so we could preallocate ?  except that big_items are unknown





#if 0

#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>




BOOST_AUTO_TEST_CASE( MyTestCase1)
{
	// add 5000 test cases, check that's how many ended up in the tree

	QuadTree< int>	tree( QuadRect( 0, 0, 1000, 1000)); 
	//typedef QuadTree< int>::value_type value_type;

//	Node	node( QuadRect( 0, 0, 1000, 1000));
	for( size_t i = 0; i < 5000; ++i)
	{
		int x = rand() % 990; 
		int y = rand() % 990; 
		int w = rand() % 10; 
		int h = rand() % 10; 

		QuadRect	r( x, y, x + w, y + h); 
		tree.insert( r, 123  );
	}
	BOOST_CHECK( tree.size() == 5000 );
}


BOOST_AUTO_TEST_CASE( MyTestCase2)
{
	// check that manually checking all intersections produces same
	// result as using the lookup tree 
	QuadTree< int>	tree( QuadRect( 0, 0, 1000, 1000)); 
	//Node	node( QuadRect( 0, 0, 1000, 1000));
	QuadRect	area( 10,10, 50, 50);
	size_t n = 0; 
	for( size_t i = 0; i < 5000; ++i)
	{
		int x = rand() % 990; 
		int y = rand() % 990; 
		int w = rand() % 10; 
		int h = rand() % 10; 
		QuadRect	rect( x, y, x + w, y + h);
		// std::cout << "object " << object << std::endl;
		tree.insert( rect, 123  );
		if( intersects( rect, area)) 
			++n;
	}
	// std::cout << "n " << n << " n3 " << n3 << std::endl;
	BOOST_CHECK( tree.intersections_count( area ) == n );
}



BOOST_AUTO_TEST_CASE( MyTestCase3)
{
	// check that the number of candidates that require to check
	// stays the same, - note this depends on deterministic rand() function
	
	QuadTree< int>	tree( QuadRect( 0, 0, 1000, 1000)); 

	//Node	node( QuadRect( 0, 0, 1000, 1000));
	for( size_t i = 0; i < 5000; ++i)
	{
		int x = rand() % 990; 
		int y = rand() % 990; 
		int w = rand() % 10; 
		int h = rand() % 10; 
		QuadRect	rect( x, y, x + w, y + h);
		tree.insert( rect, 123 );
	}

	std::vector< QuadTree< int>::value_type > candidates; 
	tree.candidate_intersections(  QuadRect( 10,10, 50, 50), candidates);
	// std::cout << "candidates.size() " << candidates.size() << std::endl;
	BOOST_CHECK( candidates.size() == 146 );
}

#endif


#if 0
#endif

#if 0
int main()
{
	Node	node( QuadRect( 0, 0, 1000, 1000));

	QuadRect	area( 10,10, 50, 50);

	std::vector< QuadRect>	test_set; 

	for( size_t i = 0; i < 5000; ++i)
	{
		int x = rand() % 990; 
		int y = rand() % 990; 
		int w = rand() % 10; 
		int h = rand() % 10; 

		QuadRect	object( x, y, x + w, y + h);
		// std::cout << "object " << object << std::endl;
		insert( node, object);
		test_set.push_back( object);
	}

	// so we have to do a double loop
	size_t n = 0;
	foreach( const QuadRect &r, test_set ) 
	{
		if( intersects( r, area)) 
			++n;
	}	

	std::cout << "elts                    " << test_set.size() << std::endl;
	std::cout << "elts in tree            " << size( node)  << std::endl;
	std::cout << "intersections (slow)    " << n << std::endl;

	std::vector<QuadRect > candidates; 
	candidate_intersections( node, area , candidates);

	std::cout << "candidates.size()       " << candidates.size() << std::endl;
	std::cout << "intersections           " << intersections( node, area ) << std::endl;
	std::cout << "node count              " << node_count( node) << std::endl;
	std::cout << "nodes visited in search " <<  nodes_visited( node, area ) << std::endl;
	return 0;
//	std::cout << "has intersections " << has_intersections( node, QuadRect( 0,0, 2, 1) ) << std::endl;
}

#endif


	// this should be able to be coded a lost faster

/*
	rather than assembling this crap here, why don't we just
	do the split. and store them in the child nodes ?
	eg we can avoiding doing this. 

	this also means we avoid carrying around the centre_x as we
	can just compute it once for all values 
*/


#if 0
int main()
{
	Node	node( QuadRect( 0, 0, 10, 10));


	insert( node, QuadRect( 2,2,4,4));
	insert( node, QuadRect( 7,7,8,8));

	std::cout << "count " << count( node) << std::endl;

	std::vector<QuadRect > candidates; 
	candidate_intersections( node, QuadRect( 2,3,7,7), candidates);

	std::cout << "candidates.size() " << candidates.size() << std::endl;

	std::cout << "has intersections " << has_intersections( node, QuadRect( 0,0, 2, 1) ) << std::endl;
}
#endif

// we don't want to create the children unless we have to
// but, should we only want to create the sizes once - eg we don't
// want to be always calculating 
// also when traversing down - so should precalculate ?

#if 0

static int child_index( Node &node /*, const QuadRect &item*/)
{
	// we want a contains
	// i think we want to create the childing	

//	assert( contains( node.rect, item));

	QuadRect	quad1( node.rect.x1, node.rect.y1, node.centre_x, node.centre_y);	// 0,0 5,5
	std::cout << quad1 << std::endl;

	QuadRect	quad2( node.centre_x, node.rect.y1, node.rect.x2, node.centre_y); // 5,0 10,5
	std::cout << quad2 << std::endl;

	QuadRect	quad3( node.rect.x1, node.centre_y, node.centre_x, node.rect.y2);	// 0,5 5,10
	std::cout << quad3 << std::endl;

	QuadRect	quad4( node.centre_x, node.centre_y, node.rect.x2, node.rect.y2);	// 0,5 5,10
	std::cout << quad4 << std::endl;



#if 0
	if( contains( quad1, item)) 	
	{
		// quadrant 0
		return 0;
	}
#endif
}  
#endif
