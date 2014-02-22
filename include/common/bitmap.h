
#pragma once

#include <agg_pixfmt_rgba.h>
#include <agg_renderer_base.h>
#include <agg_basics.h>

#include <cassert>



struct Bitmap 
{

	typedef agg::pixfmt_bgra32					pixfmt_type;
	
	typedef agg::renderer_base< pixfmt_type>	rbase_type;

private:


	unsigned				flip_y;

	/*
		if we don't use vector<>, then we should at least use a scoped_ptr. and reset
		No because it will leak if it gets new char [ sz], not an array type
	*/
	unsigned char			*buf_; 

	agg::rendering_buffer   rbuf; 
	pixfmt_type				pixf; 
	rbase_type              rbase_;

public:

	Bitmap()
		: flip_y( - 1),
		buf_( 0),
		rbuf(),
		pixf( rbuf),
		rbase_( pixf)
	{ 
		assert( rbase_.width() == 0 && rbase_.height() == 0);
	} 

	Bitmap( unsigned width, unsigned height )
		: flip_y( - 1),
		buf_( new unsigned char [ width * height * 4 ] ),
		rbuf( buf_, width, height, (-flip_y) * width * 4 ),
		pixf( rbuf),
		rbase_( pixf)
	{ 
		assert( rbase_.width() == width && rbase_.height() == height);
	} 
private:
	Bitmap( const Bitmap & ); 
	Bitmap & operator = ( const Bitmap & ); 
public:
	~Bitmap()
	{
		if( buf_) 
		{
			delete [] buf_;
		}
	}

	void resize( unsigned width, unsigned height )
	{
		if( width == rbase_.width() 
			&& height == rbase_.height()
		) return;


		if( buf_) 
		{
			delete [] buf_;
		}
		buf_ = new unsigned char [ width * height * 4 ];

		rbuf.attach( buf_, width, height, (-flip_y) * width * 4);
		pixf.attach( rbuf);
		rbase_.attach( pixf );
	}
	
	rbase_type & rbase()
	{
		return rbase_;
	}

	unsigned char * buf() const
	{
		return buf_;
	}

	unsigned width() const 
	{ 
		return rbase_.width(); 
	} 
	
	unsigned height() const 
	{ 
		return rbase_.height(); 
	} 

	void copy_from( const Bitmap & src)
	{
		if( src.width() != width() 
			|| src.height() != height())
		{
			resize( src.width(), src.height() );
		}

		unsigned n = width() * height() * 4;
		for( unsigned i = 0; i < n; ++i )
		{
			buf_[ i] = src.buf_[ i];
		}
	} 

}; 


/*
	cannot seem to make the rbase type instantiate ...
	dst_.copy_from< rbase_type>( src_ );
*/


static void copy_region( Bitmap & src, int x, int y, int w, int h, Bitmap & dst, int dst_x, int dst_y )
{
	// this is perhaps not memory safe - rendering_buffer does not enforce bounds checks on it's buffer
	// since the render_base dst uses the src render_buffer

	//	std::cout << "copy region " << x << " " << y << " " << w << " " << h << std::endl;

	// position a rendering buffer over the raw mem, at x,y with width w and h
	unsigned	flip_y( - 1); 

	// note that this can be a memory leak, if positioned in wrong place
	// ok, this works correctly
	agg::rendering_buffer   rbuf( 
		src.buf() + (y * src.width() * 4) + (x * 4),			// beginning of buf 
		w, h, 
		(-flip_y) * src.width() * 4 ); 

	// this method also takes agg::rect_i which we could use
	dst.rbase().copy_from( rbuf, NULL, dst_x, dst_y );
}

#if 0
static void blend_region( Bitmap & src, int x, int y, int w, int h, Bitmap & dst, int dst_x, int dst_y )
{
	unsigned	flip_y( - 1); 

	agg::rendering_buffer   rbuf( 
		src.buf() + (y * src.width() * 4) + (x * 4),
		w, h, 
		(-flip_y) * src.width() * 4 ); 

	typedef agg::pixfmt_rgba32					pixfmt_type;

	pixfmt_type				pixf( rbuf); 
	dst.rbase().blend_from( pixf , NULL, dst_x, dst_y );
}
#endif


