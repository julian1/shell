
#include <aggregate/mapgrid.h>
#include <aggregate/projection.h>

#include <common/surface.h>
#include <common/projection.h>
//#include <common/key.h>

#include <service/services.h>
#include <service/renderer.h>

#include <agg_bounding_rect.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>
//#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
//#include <agg_conv_dash.h>



#include <boost/functional/hash.hpp>	
#include <boost/unordered_map.hpp>
#include <map>


namespace
{

struct Render : IRenderJob
{
	Render( const ptr< IMapGridAggregateRoot > & root, const ptr< IProjectionAggregateRoot> & projection_aggregate )
		: count( 0),
		root( root),
		projection_aggregate( projection_aggregate )
	{ } 

	unsigned						count;	
	ptr< IMapGridAggregateRoot >	root;
	ptr< IProjectionAggregateRoot>	projection_aggregate ;

	void add_ref() { ++count; } 
	void release() { if( --count == 0 ) delete this; } 


	void pre_render( RenderParams & render_params  ) { } 

	void get_bounds( int *x1, int *y1, int *x2, int *y2 ) 
	{
		agg::path_storage path;		// this is inefficient. (but we have to copy, because we are going to transform).
									// it should be cached. 
									// but what if the proj changes ???. 
									// WHY BOTHER CACHING. THE RENDERER PROVIDES GOOD CACHING ALREADY, AND AVOIDS CALLING render() unless necessary.

		projection_aggregate->get_projection()->forward( root->get_path(), path, true ); 	

		bounding_rect_single( path, 0, x1, y1, x2, y2);	
	}




	void render ( BitmapSurface & surface, RenderParams & render_params ) 
	{
		agg::path_storage path;		// this is inefficient. (but we have to copy, because we are going to transform).
									// it should be cached. 
									// but what if the proj changes ???. 
									// WHY BOTHER CACHING. THE RENDERER PROVIDES GOOD CACHING ALREADY, AND AVOIDS CALLING render() unless necessary.

//		std::cout << "$$$$$ ok, mapgrid render" << std::endl;
//		return;	
		projection_aggregate->get_projection()->forward( root->get_path(), path, true ); 	
	
//		std::cout << "$$$$ finished projected mapgrid" << std::endl;
//		exit( 0);

		typedef agg::conv_stroke< agg::path_storage>                    stroke_type;
		stroke_type             stroke( path); 

		agg::scanline_p8                sl;
		agg::rasterizer_scanline_aa<>   ras;
		ras.add_path( stroke );

		agg::render_scanlines_aa_solid( ras, sl, surface.rbase(), agg::rgba8( 0, 0, 0 ) );
	}
	bool get_invalid() const 
	{
		return projection_aggregate->get_active(); 	
			
	}	
	int get_z_order() const 
	{
		// should delegate to the root. because z_order is 
		// animation job ...
		return 4;
	}; 
};



struct MapGridAggregateRoot : IMapGridAggregateRoot
{
	MapGridAggregateRoot()
		: count( 0)
	{ } 

	void add_ref() 
	{
		++count;
	}
	void release() 
	{
		if( --count == 0) delete this;
	}
	const agg::path_storage & get_path() const 
	{
		return path;
	}
	void set_path( const agg::path_storage & _ )
	{
		// we potentially want this. If someone changes the style of the map etc.
		// it should be event sourced
		path = _;
	}

	unsigned			count;
	agg::path_storage	path;

//	virtual void set_active( bool ) = 0;
//	virtual bool get_active() const = 0;
};


/*
template< class A, class B> 
struct CombineKey : IKey
{
	// it would be possible to delegate to manage multiple combining
	// no, because it's too messy to dynamic cast into the child for the equality 

	CombineKey( const ptr< A> & a, const ptr< B> & b)
		: count( 0),
		a( a),
		b( b )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
	    boost::hash_combine(seed, &*a );
	    boost::hash_combine(seed, &*b );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< CombineKey> arg = dynamic_pointer_cast< CombineKey>( key ); 
		if( !arg) return false;
		return a == arg->a && b == arg->b;
	}  
private:
	unsigned	count;
	ptr< A>		a;
	ptr< B>		b;
};

template< class A, class B> 
ptr< IKey> make_key( const ptr< A> & a, const ptr< B> & b)
{
	return new CombineKey< A, B>( a, b);
}
*/




};	// anonymous namespace


ptr< IMapGridAggregateRoot> create_test_mapgrid_aggregate_root()
{
	ptr< IMapGridAggregateRoot> o = new MapGridAggregateRoot;

	agg::path_storage	path;

	// will it work with staggered lines ? - that require interior/exterior classification.

	unsigned count = 0;

#if 1
	for( int y = -80; y <= 80; y += 10 )
//	int y = 0;
	{
		for( int x = -180 ; x < 180; x += 10 ) 
		{
			
			bool first = true;
			for( int x1 = x -2 ; x1 <= x + 2; ++x1 )
			{
				if( first)
				{
					++count;
					path.move_to( x1, y );
					first = false;
				}
				else
					path.line_to( x1, y);
			}  
		}
	}
#endif


	std::cout << "count is " << count << std::endl;

#if 1
	for( int x = -180 ; x < 180; x += 10 ) 
//	int y = 0;
	{
		for( int y = -80; y <= 80; y += 10 )
		{
			bool first = true;
			for( int y1 = y - 2; y1 <= y + 2; ++y1 )
			{
				if( first) 
				{
					path.move_to( x, y1  );
					first = false;
				}
				else
					path.line_to( x, y1);
			}  
		}
	}
#endif

#if 0
	for( int y = -80; y < 80; y += 10 )
	{
		// test equator	
		path.move_to( -180, y );
		for( int x = -180 + 1; x < +180; ++x )
		{
			path.line_to( x, y);
		} 
	}
#endif

	o->set_path( path );
	return o;
}




void add_mapgrid_aggregate_root( Services & services, const ptr< IMapGridAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & proj)  
{

//	m.insert( std::make_pair( make_key( root, proj) , 123 ));

/*
	std::size_t seed = 0;
	boost::hash_combine(seed, &*root );
	boost::hash_combine(seed, &*proj );
*/
	//void *key = (void *)seed ; 
//	ptr< IKey> key = make_key( root, proj );

	services.renderer.add( * new Render( root, proj )); 
}

void remove_mapgrid_aggregate_root( Services & services, const ptr< IMapGridAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & proj)  
{
	assert( 0 );

}




/*
	ok, so, we have an issue.  
	If the key is just the root, then it's getting added multiple times. and std::set<> can only take the item once. 
	It could be added, by combining the root and the proj which would be unique.

	// just something that implemented a operator <.    except make it virtual, and ref countable ?  

	// Or RATHER THAN USING A MAP. 

	Why not use a std::set. and put the comparison operator on the wrapper IRenderer
	This seems a lot cleaner.  

	Because we don't want to have to reconstruct an IRenderer key to remove the item.
	
	- or do we jjjjj
	- The problem is  		
	----------------------------------------
		- the choice is to create another structure, and persist it. containing all the added items, eg if we instantiate a Render keys. 
		- Or to have a key that we can generate, 
*/
#if 0
struct IKey
{
	virtual void add_ref() = 0;
	virtual void release() = 0;
	// we keep this named, 
	virtual bool less_than ( const ptr< IKey> & key ) = 0; 
};

bool compare_keys( const ptr< IKey> & a, const ptr< IKey> & b )
{
	return a->less_than( b);
}

/*
	ok, this is a lot of work, for stuff like isolines that are created and removed very frequently 
	Remember, everything will have to perform the dynamic_cast, for alternate key types.

	A lighter weight solution would be much nicer, if possible ?
	It requires new. and dynamic_cast<> 
*/
template< class A, class B> 
struct Key : IKey
{
	Key( const ptr< A> & a, const ptr< B> & b)
		: count( 0),
		a( a),
		b( b )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
	bool less_than ( const ptr< IKey> & key ) 
	{
		ptr< Key> arg = dynamic_pointer_cast< Key>( key ); 
		if( !arg)
		{
			// what do we do here, to ensure that everything is unique ???
			//return operator < ( ptr< IKey>( this), key ) ; 
			return ( this < &*key ); 
		}
		if( a < arg->a ) return true; 
		if( arg->a < a ) return false; 
		if( b < arg->b ) return true; 
		if( arg->b < b ) return false; 
		return false;	
	}  
private:
	unsigned	count;
	ptr< A>		a;
	ptr< B>		b;
};


template< class A, class B> 
struct HKey 
{
	HKey( const ptr< A> & a, const ptr< B> & b)
		: count( 0),
		a( a),
		b( b )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  

	std::size_t hash() const
	{
		return &* a; 
	}  

	bool equal( const ptr< HKey> & key ) const
	{
		ptr< HKey> arg = dynamic_pointer_cast< HKey>( key ); 
		if( !arg)
		{
			return false ;  
		}
		return a == arg.a && b = arg.b ; 
	}  
private:
	unsigned	count;
	ptr< A>		a;
	ptr< B>		b;
};

void func2()
{
	ptr< IKey>	x = new Key< IMapGridAggregateRoot, IProjectionAggregateRoot> ( root, proj ); 

	std::map< ptr< IKey>, IRenderJob *,
		bool (*)( const ptr< IKey> & a, const ptr< IKey> & b )
	>		mymap( compare_keys );


	mymap.insert( std::make_pair( x, new Render( root, proj )));
}
#endif

/*
	Using a hashtable still requires the dynamic_cast operation, which is messy. 
		- But it only gets called once and not for every comparison.
*/

#if 0
struct IKey 
{
	virtual void add_ref() = 0; 
	virtual void release() = 0;
	virtual std::size_t hash() = 0 ; 
	virtual bool equal_to( const ptr< IKey> & key ) = 0; 
};	
#endif

#if 0
struct TestKey : IKey 
{
	TestKey()
		: count( 0)
	{ } 

	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
	std::size_t hash()		
	{ 
		// boost::hash< IKey *> hash;
		std::size_t seed = 0;
	    boost::hash_combine(seed, this );
	    boost::hash_combine(seed, this );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		ptr< TestKey> arg = dynamic_pointer_cast< TestKey>( key ); 
		if( !arg) return false;
	}  
private:
	unsigned count;
};	
#endif

// Ok, it's one stupid class.

/*
	OK, there is no reason not to have a structure that does it.
	kthe void * can be filled with zero if unused.

	std::pair< void *, void *> 
	Or, 
	std::vector< > 
	filled with the void pointers.
	
*/

#if 0
template< class A> 
struct CombineKey : IKey
{
	// it would be possible to delegate to manage multiple combining
	// no, it doesn't work, because we can't peer inside the other key to get
	// the child for the equal_to

	CombineKey( const ptr< A> & a, const ptr< IKey> & child )
		: count( 0),
		a( a),
		b( b )
	{ } 
	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  
  	std::size_t hash()		
	{ 
		std::size_t seed = 0;
		if( child) child->hash( seed);
	    boost::hash_combine(seed, &*a );
		return seed;
	}  
	bool equal_to( const ptr< IKey> & key ) 
	{ 
		if( child && child->equal_to( key);

		ptr< CombineKey> arg = dynamic_pointer_cast< CombineKey>( key ); 
		if( !arg) return false;
		return a == arg->a && b == arg->b;
	}  
private:
	unsigned	count;
	ptr< A>		a;
	ptr< IKey>	child;
};
#endif


/*
	- we could just hash the two values, and use std::size_t as the key ?
	- but it wouldn't be guaranteed unique  	
*/	
/*
	OK, hang on. Why not use a set. but derive the IRender from IKey ?????
	
	Then it's easy to construct a different key ?
	because we have to duplicate the thing
*/
/*
    template <
        class Key, class Mapped,
        class Hash = boost::hash<Key>,
        class Pred = std::equal_to<Key>,
        class Alloc = std::allocator<Key> >
    class unordered_map;
*/



/*
	Even if using a set, means we have to construct the key. it might be easier. 
	No, it's all awful.
	If we are going to construct all the crap. then just use a set 0ww. 
	// hash the value in a unique way and add it.
*/

// it would likely be easier to do this with a hash table, where only need to support a 4 bit hash and a comparison 
// operation. 

// Ok but this is still a mess, because the less_than is supposed to peer.  

/*
	we want to add according to the root and the proj, and remove both together. 
*/
/*
	We want to store a value, in respect to two other values.
*/

