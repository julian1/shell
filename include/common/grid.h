/*
	would be better if named to Grid, because not associated with linear algebra 
*/


#pragma once

#include <vector>
#include <common/ptr.h>



struct Grid
{
	// a (mostly) immutable matrix for holding source data
	// cannot change dims.
public:
	Grid( unsigned w, unsigned h)
		: count( 0),
		nx( w), 
		ny( h),
		buf( w * h )//new double [ w * h ] )
	{ 
		// to support begin()
		assert( !buf.empty() );
	} 

	~Grid()
	{
		//delete [] buf; 
	}

	void add_ref() 
	{ 
		++count; 
	} 

	void release() 
	{ 
		if( --count == 0 ) 
			delete this; 
	} 

	// useful to expose begin(),end() this for std::algorithm etc.
	// Using a simple buf, makes it easier ...

	const double * begin() const
	{
		return & buf[ 0]; 
	}
	const double * end() const
	{
		return & buf[ 0] + (nx * ny ); 
	}

	// change name get_width() ?
	unsigned width() const 
	{
		return nx; 
	}

	unsigned height() const 
	{
		return ny; 
	}

	// it's much harder to use these, when we are dealing with a pointer
	// maybe better to just have .at so that can use m->at( x, y ) rather than (*m)(x, y )

/*
	don't use this, use begin(), end() instead

	inline double & operator () ( unsigned i )
	{
		assert( i < nx * ny );
		return buf[ i];
	}
*/

	inline double & operator () ( unsigned x, unsigned y)
	{
		//return buf.at( y * nx + x );
		return buf[  y * nx + x ];
	}
	inline double operator () ( unsigned x, unsigned y) const
	{
		//return buf.at( y * nx + x );
		return buf[  y * nx + x ];
	}

private:
	Grid ( const Grid &m ); 
	Grid & operator = ( const Grid &m ); 

	unsigned	count;	// ref count
	unsigned	nx;
	unsigned	ny;
//	double		*buf;
	std::vector< double> buf;
};



// this function really shouldn't be defined here 

ptr< Grid> make_test_grid( unsigned w, unsigned h ); 





