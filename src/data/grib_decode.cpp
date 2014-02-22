/*
	http://www.wmo.int/pages/prog/www/WDM/Guides/Guide-binary-2.html#Section1
*/
/*
	where we have bitmasks, we can actually now just use the bitmap reader on the inner
	byte reader. (if we want). 
	Rather than doing flag tests.

	can put a check to ensure that we are byte aligned before moving on, if we want.
*/
/*
	- design serves two goals. to decode grib and to output formatted values. hence we try to 
	have data structures that represent the mostly decoded flags, so that they can
	be dumped for debugging/understanding/formatting. 

	- the extraction of values is done with a bit of a pipeline.	
*/
/*
	VERY IMPORTANT. We can always decode a section independently of other sections, even if they
	have bitmaps/buffers, etc, because the sections all have a len field. 

	All sections can be extracted/parsed without any dependencies between parsing.

	And this obscures all the complicated calculation of n (using ni,nj or using pir etc etc), 
	until we assemble the pipeline.
*/


// source
// /home/meteo/OLD/turion_laptop/purple/gdump/ 

// gdump.cpp : Defines the entry point for the console application.
//
 




#include "data/grib_decode.h" 
#include "data/Wmo.h"

#include <vector>
#include <iostream> 
#include <string>
#include <cstdio>
#include <cmath>




/*

	need wmo.h

*/


#define IDS_SECTION_SIZE 8 //sizeof( ids)
//#define PDS_SECTION_SIZE 28
//#define GDS_SECTION_SIZE 32

#define BMS_SECTION_SIZE 6
#define BDS_SECTION_SIZE 11



#if 0
void quick_sizeof_check()
{
	/*
		this is silly we should be using uint32_t etc. where it matters 
	*/
	assert( sizeof( short int) == 2);
	assert( sizeof( int) == 4);
//	printf( "size of int64 %u\n", sizeof( int64));
//	assert( sizeof( int64 ) == 8);
	assert( sizeof( float) == 4);
	assert( sizeof( double )== 8);
}
#endif



static char *format_bits( unsigned int value, int num )
{	// string format bit represenation of value
	static char tmp[ 100];
	int i;
	char *s = tmp;
	for( i = num - 1; i >= 0; i--)  
	{
	 	*s++ = ((1 << i) & value) ? '1' : '0';	
		if( i % 8 == 0 && i != 0) 
			*s++ = ',';
	}
	*s = 0;
	return tmp;
}
 

/*
sAAAAAAA BBBBBBBB BBBBBBBB BBBBBBBB
where s = sign bit, encoded as 0 => positive, 1 => negative
A = 7-bit binary integer, the characteristic
B = 24-bit binary integer, the mantissa.
R = (-1)s x 2(-24) x B x 16(A-64)
*/
#define sign_ref_mask		020000000000u
#define exponent_ref_mask	017700000000u
#define mantissa_ref_mask	0xffffffu

// this silly fucker should we loose some accuracy here ..
// think this is right and not  0xffff instead 
#define MAX_MANTISSA 		0xffffff  
	 

static inline float ref_to_float( unsigned int v)
{

//	printf( "*REF TO FLOAT\n");
	unsigned int sign = ( sign_ref_mask & v) >> 31;
	int exponent = ( exponent_ref_mask & v) >> 24;
	unsigned int mantissa = ( mantissa_ref_mask  & v);

/*	printf( "bit value       %s\n", 	format_bits( v , 32 ));
	printf( "sign mask      '%s\n", 	format_bits( sign_ref_mask , 32 ));
	printf( "exponent  mask '%s\n", 	format_bits( exponent_ref_mask , 32 ));
	printf( "mantissaa mask '%s\n", 	format_bits( mantissa_ref_mask , 32 ));
	printf( "sign  is       %s  %u\n",  format_bits( sign, 32 ), sign);
	printf( "exponent       %s  %d\n", format_bits( exponent , 32 ) , exponent - 64);
	printf( "mant is        %s  %u\n",   format_bits( mantissa, 32 ), mantissa ); */

	float v1 = pow( -1.f, (float)sign) * pow( 2.f, -24.f) * (float)mantissa * pow( 16.f, exponent - 64);	
//	printf( "value is %f\n", v1 );
	return v1;
} 
 
// lseek( 0), means the beginning of the file/source, it's better to have the tell  

struct IReader
{
	// size_t is unsigned ?
	// eg. the filedesriptor is this class ...
	virtual size_t read( char *buf, size_t sz ) = 0; 

	virtual void lseek( int pos_ ) = 0;
	virtual int tellg() const = 0;
};


struct VectorReadAdapt : IReader
{
	// adapt a vector to support the IReader interface
	// Think this might be better with just begin and finish pointers ...
	// as the more generic interface

    VectorReadAdapt( const std::vector< unsigned char> & src ) 
        : src( src), 
        pos( 0)
    { }   

    size_t read( char *buf, size_t sz ) 
    {   
        char *p = buf;
        while( sz-- && pos < src.size()) 
        {   
            *p++ = src[ pos++];  
        }   
        return p - buf; 
    }   

	void lseek( int pos_ )
	{
		pos = pos_; 
	}

	int tellg() const
	{
		return pos;
	}
private:
    const std::vector< unsigned char> & src; 
    int pos; 
};

// OK. Are sure it might not be better to use  two range pointers as the interface ?????
// ok, now, the issue is that the bitmap reader has to be isolated.

// NO IT DOESN't. Because we now parse everything in line, we do not have to shuffle anything around.
// the bitmap reader can use the same underlying reader pos as work with the 
// EXCEPT NO. Because we need to place the reader over an existing buffer sometimes. 

// ok, but we maybe able to rewrite the Bitmap buffer just to use a reader ...   
// then we can instandiate what we need to.

class ByteReader
{
public:

	ByteReader( IReader & reader )
		: reader( reader )
	{ }
	
	void handle_read_error()
	{
		/*
			we could actually the policy as to what to do as an error
		*/
		std::cout << "buffer underflowed" << std::endl;
		// should throw !!!
		assert( 0);
	}


	IReader & inner_reader()
	{
		// expose the inner delegated reader
		// this is useful, for debugging as well, as it allows us to get position and raw_bytes 
		return reader; 
	}

	void lseek( int pos )
	{
		reader.lseek( pos );
	}

	// should we remove this and just use lseek but also return size_t ?
	int tellg() const
	{
		return reader.tellg();
	}

	inline void dec_raw( char * buf, int sz )
	{
		if( reader.read( buf, sz ) != sz ) 
			handle_read_error();	
	}

	inline unsigned int dec1()
	{ 		
		unsigned char buf[ 1];
		if( reader.read( (char *) buf, 1) != 1 )	
			handle_read_error();	
		return buf[ 0];
	}

	inline unsigned int dec2()
	{ 
		unsigned char buf[ 2];
		if( reader.read( (char *) buf, 2) != 2 )	
			handle_read_error();	

		return buf[ 1] + 256 * buf[ 0];
	}

	inline int dec2s()
	{ 
		unsigned char buf[ 2];
		if( reader.read( (char *) buf, 2) != 2)	
			handle_read_error();	
	
		int iii = buf[ 1] + 256 * (buf[ 0] & 0x7f) ;
		return (buf[0 ] & 0x80) ? -iii : iii ;
	}

	inline unsigned int dec3()
	{ 	
		unsigned char buf[ 3];
		if( reader.read( (char *) buf, 3) != 3)	
			handle_read_error();	
	
		return buf[ 2] + 256 * (buf[ 1] + 256 * buf[ 0]) ; 
	}

	inline int dec3s()
	{ 
		unsigned char buf[ 3];
		if( reader.read( (char *) buf, 3) != 3)	
			handle_read_error();	

		int iii ;
		iii = buf[2] + 256 * (buf[1] + 256 * (buf[ 0] & 0x7f)) ;
		return (buf[ 0] & 0x80) ? -iii : iii ; 
	}
	
	inline unsigned int dec4() 
	{ 
		unsigned char buf[ 4];
		if( reader.read( (char *) buf, 4) != 4 )	
			handle_read_error();	

		return buf[ 3] + 256 * (buf[ 2] + 256 * (buf[ 1] + 256 * buf[ 0]));
	}
private:
	IReader & reader; 
};




/*
	Important. There ought to be an underlying interface with seekg and tellg that can just propagate this value 
*/




class BitmapReader
{
public: 
	IReader			& reader; 
	int				t_bits; 
	unsigned int	tbits;
	
	BitmapReader( IReader & reader )
		: reader( reader ), 
		t_bits( 0 ), 
		tbits( 0 ) 
	{ }

	void handle_read_error()
	{
		std::cout << "buffer underflowed" << std::endl;
		// should throw !!!
		assert( 0);
	}
	
	unsigned int decode( int n_bits ) 
	{
		// how many bits of precision does this handle. we really need to 
		// test this a bit more. 
		unsigned int  jmask = (1 << n_bits) - 1;
      
		while (t_bits < n_bits) 
		{
			unsigned char buf[ 1];
			if( reader.read( (char *) buf, 1) != 1 )	
				handle_read_error();	

			tbits = (tbits * 256) + buf[ 0];
			t_bits += 8;
		}

		t_bits -= n_bits;
		unsigned int value = (tbits >> t_bits) & jmask;
		return value; 
	}

	// unused at moment, but potentially useful
	IReader & inner_reader()
	{
		return reader; 
	}
};




///////////////////////////////////////////////////////////

  

void Ids_section::decode( ByteReader &b   )
{
	b.dec_raw( buf, 4 ); 
	buf[ 4] = 0;

	if( std::string( buf) != "GRIB" ); // should actually do something  
										// just throw runtime exception

	len = b.dec3(); 
	edition = b.dec1();
}

//////////////////////////////////////////////////////////// 

 

void Pds_section::decode( ByteReader &b)
{			
	int pos = b.tellg(); 		

	len = b.dec3();	  
	table_version = b.dec1(); 
	center	= b.dec1(); 
	model	= b.dec1(); 	/* model id, allocated by center */
	grid	= b.dec1(); 	//* grid id, allocated by center */ 	 
	db_flag = b.dec1(); 		
	param	= b.dec1(); 

	level_flg = b.dec1(); 
	level2	=  b.dec2();   	 

	year = b.dec1();			/* reference time of data */
	month = b.dec1();
	day = b.dec1();
	hour = b.dec1();
	minute = b.dec1(); 

	

	tunit = b.dec1();		/* unit of time range, from GRIB table 4 */
//	    g1int tr[2];		/* periods of time or interval */
	tunit_0 = b.dec1();
	tunit_1 = b.dec1();
	tr_flg = b.dec1();		/* time range indicator, from GRIB table 5 */

	
	avg = b.dec2();			/* number in average, if any */
	missing = b.dec1();		/* number missing from averages or accums */

	century = b.dec1();		/* 20 for years 1901-2000 */

#if 0
	using boost::gregorian::date;
 
	if( tr_flg == 0) { 			// forecast product

		runtime = ptime( date( ((century - 1) * 100) + year, month, day), 
			hours( hour) + minutes( minute) + seconds( 0)) ;

		if( tunit == 1)  // hour
		{
			validity =  runtime + hours( tunit_0);				
		}		
		else {
			std::cout << "unknown time unit" << std::endl;		
			assert( 0);
		}
	}

	else if( tr_flg == 1) { 	// initialised analysis product

		runtime = ptime( date( ((century - 1) * 100) + year, month, day), 
			hours( hour) + minutes( minute) + seconds( 0)) ;		
		validity =  runtime;		 
	}

	else if( tr_flg == 4) { 		// accumulation p0, p1

		runtime = ptime( date( ((century - 1) * 100) + year, month, day), 
				hours( hour) + minutes( minute) + seconds( 0)) ;

		if( tunit == 1)  // hour
		{								
			validity =  runtime + hours( tunit_0);
			validity2 = runtime + hours( tunit_1);
		}		
		else {
			std::cout << "unknown time unit" << std::endl;		
			assert( 0);
		}
	}

	else {
		assert( 0);
	}
#endif

/*
	string s = str2( day) + "/" + str2( month) + "/" + str2( ((century - 1) * 100)  +year) 
		+ ":" + str2( hour, "%d") + "+" + str2( minute); 

	std::cout << s << std::endl;
	std::cout << runtime << std::endl;
*/

	subcenter = b.dec1();		/* reserved in GRIB1 standard */
	scale10 = b.dec2s();		/* (signed) units decimal scale factor */




	assert( len == b.tellg() - pos );


//	b.lseek( pos + len );

}



//////////////////////////////////////////////////////////////



void Grid_ll::decode( ByteReader &b )
{
	// ouch ni and nj - this is in the lat/lon areainate scheme 
	ni = b.dec2(); 	 
	nj = b.dec2(); 
	la1 = b.dec3s(); 
	lo1 = b.dec3s(); 
	res_flags = b.dec1(); 	
	la2 = b.dec3s(); 
	lo2 = b.dec3s();		
	di = b.dec2(); 
	dj = b.dec2();  	
	scan_mode = b.dec1();  
	// should we advance the reserved here ? 
	// b.advance( 4);
	if( ni == 0xffff) ni = nj;
	if( nj == 0xffff) nj = ni; 

//	dump();

}



void Grid_rll::decode( ByteReader &b )
{
	ni = b.dec2();
	nj = b.dec2();
	la1 = b.dec3s();
	lo1 = b.dec3s();
	res_flags = b.dec1();
	la2 = b.dec3s();
	lo2 = b.dec3s();
	di = b.dec2();
	dj = b.dec2();
	scan_mode = b.dec1();

	b.lseek( b.tellg() + 4 );
//	b.advance( 4);
	lapole = b.dec3s();
	lopole = b.dec3s(); 
	angrot = b.dec4();

	if( ni == 0xffff) ni = nj;
	if( nj == 0xffff) nj = ni;
}

 
/////////////////////////////////////////////////////////


void Gds_section::decode( ByteReader &b ) 
{

	int pos = b.tellg(); 
		
	len = b.dec3();	 
	nv = b.dec1();			/* number of vertical areainate parameters */
	pv = b.dec1();			// number of has a pir
	type = b.dec1();			/* representation type, from GRIB table 6 */
	 
	if( type == GRID_LL) 
	{ 
		ll_grid.decode( b); 		
		//ni = ll_grid.ni;
		//nj = ll_grid.nj;
	}
/** /	else if( type == GRID_OLAMBERT)		// new lithuania gribs
	{
	 
	}
/**/	else if( type == GRID_RLL)
	{
		std::cout << "got GRID_RLL" << std::endl;
		rll_grid.decode( b);
		//ni = rll_grid.ni; 
		//nj = rll_grid.nj;

		rll_grid.dump();
		std::cout << "RLL" << std::endl;
	}

/*		else if( type == GRID_LAMBERT) 
	{
		std::cout << "LAMBERT" << std::endl;
	}
	
/**/	

	else { 
		std::cout << "projection not ll type is " << type << std::endl;
		exit( 0);
	}

	if( pv != 0xff) 
	{ 
 
		if( nv == 0) 
		{ 			
			// std::cout << "pv is " << pv << std::endl; 				
		
			b.lseek( pos + pv - 1);	// -1 is endiness
			assert( type == GRID_LL);			
			int nj = ll_grid.nj ;				  
			for( int i = 0; i < nj; ++i) { 
				int value = b.dec2();					
				// std::cout << "pir " << i << " " << value << std::endl;					
				pir.push_back( value);
			} 
		}			
	}	


	// we must seek rather than adv here, because we may not have taken all the grid 
	b.lseek( pos + len );
/*	
	int adv = len - (b.pos - pos) ; 	 
//	std::cout << "@gds advance " << adv << std::endl;		
	b.advance( adv);
*/

}





Bms_section::Bms_section()
{
	len = 0; 
	nbits = 0; 
	map_flg = 0; 
	 
}

void Bms_section::decode( ByteReader &b )
{
/*
	i think that perhaps we should pass in a context
	so we can check things as we go ? 
*/
	int pos = b.tellg(); 		

	len = b.dec3(); 
	nbits = b.dec1(); 
	map_flg = b.dec2(); 


	assert( map_flg == 0);	// indicates bitmap follows

	// this is an inferred length ...  but not sure if we should not
	// be respecting ni*nj

	int n = ( len - (3 + 1 + 2)) * 8 - nbits;     // ie len - size of data * bytes - bnites
						 
	assert( include.empty());
	include.reserve( n); 

/*
	THIS INLINE USE OF THE BITMAP2 reader buffer has not been tested. 
	because we haven't had any gribs with a bms.
*/
	BitmapReader	bitmap( b.inner_reader() );

//	BitmapReader	bitmap( b.buf, b.tellg() );  // ok but we need to set pos in the buffer
//	bitmap.pos = b.pos; 		// should probably set a max size as well 

	for( int i = 0; i < n ; i++) 
	{	
		unsigned b1 = bitmap.decode( 1);	
		include.push_back( b1);  		
	}

	assert( include.size() == n);


	assert( len == b.tellg() - pos );
//	b.lseek( pos + len );		// do we need this, given the bitmap, should place us here anyway ?


#if 0
//	b.pos = bitmap.pos; 	
	b.lseek( bitmap.pos ); 	
//	int adv = len - (b.pos - pos) ; 	 
//	assert( adv >= 0);
#endif

}


static std::string show_buffer( IReader & reader, int sz )
{
	// useful for debugging
	// read sz bytes from reader, reset the reader pos, and return as string
	int pos = reader.tellg();
	std::vector< char >	buf( sz );
	int read = reader.read( &buf[ 0], buf.size() );
	reader.lseek( pos);
	return std::string( buf.begin(), buf.begin() + read );
}

void Bds_section::decode( ByteReader & r  )
{
	int pos = r.tellg();
		
	len = r.dec3();					
	flg = r.dec1(); 

	unused_bits = 0x000f & flg;	

	scale  = r.dec2s(); 	 
	ref  = r.dec4(); 
	bits = r.dec1(); 

	// calculate and cache as an optimiztion
	fref = ref_to_float( ref );  

	//	int unused_bits;  we can calculate this from the flg anytime so don't record

	// copy out the raw values that need pir, etc applied to them
	while( r.tellg() - pos < len )
	{
		buf.push_back(  r.dec1() ); 
	}

	// should be at trailing 7777 now
	// std::cout << "here -> '" << show_buffer( r.inner_reader(), 10  ) << "'" << std::endl;
}


static void decode_end_section( ByteReader &b )
{
	char buf[ 5] ;
	b.dec_raw( buf, 4 ); 
	buf[ 4] = 0;

	if( std::string( buf) != "7777" ); // should actually do something  
										// just throw runtime exception
}



////////////////////////////////////////////////////////////////////////////////


struct IGetValue
{
	virtual double get_value( ) = 0; 
};


struct DecodeBdsValuesWithBms 
{

};  

struct DecodeBdsValues : IGetValue
{
	DecodeBdsValues( const Bds_section & bds )  
		: bds( bds ), 
		reader( bds.buf),
		breader( reader  )
	{ } 

	double get_value( )  
	{
		// need to protect this if it overruns.
		unsigned int raw = breader.decode( bds.bits );
		return pow( 2.f, bds.scale) * raw + bds.fref; 
	}
	const Bds_section & bds; 
	VectorReadAdapt		reader;
	BitmapReader	breader; 
};



struct DecodePds : IGetValue
{
	// we keep this as a separate pass, just because it helps isolation, and is trivial
	// otherwise if we have a different bds reader for dealing with bms, then we would
	// have to duplicate the code (eg. DRY).

	DecodePds( IGetValue & r, const Pds_section & pds )
		: r( r),
		pds( pds)
	{ } 
	double get_value()  
	{
		return r.get_value() * pow( 10, - pds.scale10 );
	}
	IGetValue & r;
	const Pds_section & pds; 
}; 



struct ApplyScanOrder ;		// could just reverse order etc. where required. 
							// alternatively it is the main driver.


struct DecodePir : IGetValue
{
	// scan direction
	// ok, it's possible scan order has to be imposed before here, but if we 
	// have the pir areainates, then I think I think we can figure it out, 
	// and impose something. 
	// it's possible we should reverse the pir if i is different ?

	// Eg. just read pir in reverse if scanned backwards. and reverse the 
	// row after we construct if j is reversed ?????  

	DecodePir( IGetValue &r, const Gds_section & gds )  
		: r( r),
		gds( gds ),
		pir_j( 0)
	{ } 

	std::deque< double> expand_row( const std::deque< double> & src, int n ) 
	{
		std::deque< double> row( n); 
		for( int i = 0; i < row.size(); ++i )
		{        
			// now we have to map the lower index value 
			double dist = double( i) / (row.size() - 1);  
			double src_dist = dist *  (src.size() - 1); 
			int idx = floor( src_dist );  
			double remainder = src_dist - idx;  
			assert( remainder >= 0 && remainder <= 1 );  
			assert( idx >= 0 && idx < src.size());
			// std::cout << "i " << i << " dist " << dist  << " idx  " << idx << std::endl;
			if( i == row.size() - 1)    // final
			{
				row.at( i) = src.back();        
			}
			else 
			{
				row.at( i) = src.at( idx) * (1 -remainder) + src.at( idx + 1) *  remainder; 
			}
		}        
		return row;
	};


	double get_value( )  
	{
		// ok, do we decode the PIR, or else pass in the ordinary decoded structure ???
		
		if( row.empty() )
		{
			// std::cout << "row empty" << std::endl;

			// determine number of values available in row
			int n = gds.pir.at( pir_j );
			++pir_j;

			// std::cout << "n " << n << std::endl;

			// read in the source row
			std::deque< double>	src;
			for( int i = 0; i < n; ++i)
			{
				double value = r.get_value(); 
				src.push_back( value );
			}

			int ni;
			if( gds.type == GRID_LL) 
			{
				ni = gds.ll_grid.ni;
			}
			else
				assert( 0);

			row = expand_row( src, ni );
		}

		//std::cout << "row.size() " << row.size() << std::endl;

		// just return a value off of the row
		double value = row.front(); 
		row.pop_front();
		return value;
	}	

private:
	IGetValue				&r;  
	const Gds_section		& gds; 
	int						pir_j;
	std::deque< double>		row; 
	// we can work one row at a time, or else all rows 
};

/*
	no i is x.		eg ij correstponds xy
*/

#include <common/grid.h>

struct DecodeGrid 
{
	// so we will pass in the bitmap.here, most likely  
	// or return a new aggregate ...

	DecodeGrid( IGetValue &r, const Gds_section & gds )  
		: r( r),
		gds( gds )
	{ } 

	ptr< Grid>  create() 
	{

		int ni, nj;
		if( gds.type == GRID_LL) 
		{
			ni = gds.ll_grid.ni;
			nj = gds.ll_grid.nj;
		}
		else
			assert( 0);


		ptr< Grid> grid = new Grid( ni, nj );  

		for( int j = 0; j < nj; ++j )	
		{
			for( int i = 0; i < ni ; ++i )	
			{
				(*grid)( i, j) = r.get_value();
			}
		}

		return grid;
	}

private:
	IGetValue				&r;  
	const Gds_section		& gds; 
};




#include <common/desc.h>


ptr< Grib1_sections>  decode_grib_to_sections( const std::vector< unsigned char> & buf1 )
{
	ptr< Grib1_sections>	grib( new Grib1_sections ); 

	// reader

	VectorReadAdapt	r( buf1 );

	ByteReader 	b( r );

	// only used for checking
	// but we have the pos in the buffer.  
 

	// read in section 0
	grib->ids.decode( b);
	//	ids_.dump();
 
	// read in section 1
	grib->pds.decode( b);
	//	pds_.dump();
    
	
	// read in gds section
	if( grib->pds.db_flag & HAS_GDS )  
	{
		grib->gds.decode( b);
		// gds_.dump();		
	}

 	// read in optional bms
	if( grib->pds.db_flag & HAS_BMS ) 
	{
		grib->bms.decode( b);
		// bms_.dump();				
	} 

	// bds
	grib->bds.decode( b) ; 

	// 7777
	decode_end_section( b );

	return grib;
}

// should we place all this stuff in classes to make it easier to see the isolation ?


// turn the grib into a surface

// also the pds handling can be completely separated from the values handling ...
// or are we overcomplicated procedural code ...

#include <common/desc.h>


#include <boost/date_time/posix_time/posix_time.hpp>

/*
	Again. We can split the metadata decoding out from the raw grib decoding. by just using separate classes.
*/



void decode_grib_to_surface( const std::vector< unsigned char> & buf1, IDecodeGribCallback & add_grib ) 
{
	ptr< Grib1_sections> grib = decode_grib_to_sections(  buf1 ); 

	//ptr< SurfaceCriteria>	criteria( new SurfaceCriteria ); 

	ptr< Param>		param;
	ptr< Level>		level;
	ptr< Valid>		valid;
	ptr< Area>		area;


	/*
		the Decoding of the grid, should be separated from the decoding of the metadata.

	*/

	/* 
		- We can either make this stuff table driven from a file, for each centre, origin. 
		- alternatively, following dep injection just inject the param and level decoding as policy
	*/

	/*
		note that in any table driven approach, we would still, pass the centre to the 
		delegated handler, to registration for multiple centre/grid combinations.
	*/

	/*
		OK. Should we actually make the pulling of bms and pir modular in the same way ???? 
		quite possibly.

	*/

	// wants to be table driven, with deps in a class ...
	switch( grib->pds.param )
	{
		case 7: 
			param = new ParamGeopotHeight;  
			break;
		case 11: 
			param = new ParamTemp;  
			break;	
		case 33:	
			// let's make these explicit since they are so common and we will later combine
			param = new ParamGenericGrib( 33 );  
		case 34:	
			param = new ParamGenericGrib( 34 ); 
			break;
		case 52:
			param = new ParamRelativeHumdity ; 
			break;

		default:
			std::cout << "unknown param is " << grib->pds.param << std::endl;
			assert( 0 );
	}; 

	// dump info
	//grib->pds.dump();

	switch( grib->pds.level_flg )
	{
		case 100: 
		{ 
			int hpa = grib->pds.level2; 
			level = new LevelIsobaric( hpa ); 
			break;
		}
		case 6: 
			// should be generic ??, not if it's so common it's in wafc ...	
			level = new LevelMaxWindSpeed; 
			break;
		case 7: 
			level = new LevelTropopause ; 
			break;
		default:
			std::cout << "unknown level_flg is " << grib->pds.level_flg << std::endl;

			assert( 0);
	};



	switch( grib->pds.tr_flg )
	{

		// forecast product
		case 0: 
		{
				using boost::gregorian::date;

				using boost::posix_time::ptime;	 
				using boost::posix_time::hours;	 
				using boost::posix_time::minutes;	 
				using boost::posix_time::seconds;	 

				const Pds_section & pds = grib->pds; 

				// this could throw
				boost::posix_time::ptime runtime 
					= ptime( 
						date( ((pds.century - 1) * 100) + pds.year, pds.month, pds.day), 
						hours( pds.hour) + minutes( pds.minute) + seconds( 0)
					);
				/* 
					- we have to be able to record the runtime somewhere. probably with the origin/centre and grid.  
					- but it's going to be a secondary tag. 
					- or else we are going to have to wrap / expose it from an interface somehow 
				*/
				// std::cout << "runtime " << runtime << std::endl;

				if( pds.tunit == 1 )  // hour
				{
					boost::posix_time::ptime validity = runtime + hours( pds.tunit_0 );				
					// std::cout << "valid " << valid << std::endl;
					valid = new ValidSimple( validity );  
				}		
				else {
					std::cout << "unknown time unit" << std::endl;		
					assert( 0);
				}
			break;
		}

		default:
			std::cout << "unknown tr_flg" << std::endl;		
			assert( 0);
	}; 



	switch( grib->gds.type )
	{
		case GRID_LL: 
		{ 
			// there is no point recording ni, nj, since it's in the data. and derivatives are implied/redundant
			const Gds_section & gds = grib->gds; 
			const Grid_ll	& ll_grid = gds.ll_grid ; 
			area = new AreaLLGrid( ll_grid.la1, ll_grid.la2, ll_grid.lo1, ll_grid.lo2 );  
			break;	
		}

		default:;
			std::cout << "unknown projection" << std::endl;		
			assert( 0);
	}

	/*
		this extraction of values, could also be policy based. but it would
		mean exposing these decode classes. 
	*/
	
//	ptr< Data>  data ;

	ptr< Grid> grid ; 

	if( !( grib->pds.db_flag & HAS_BMS )
		&&  ! ( grib->bds.flg & BDS_PACKING )
		&& grib->gds.pir.size() ) 
	{
		// this is our pipeline ...
		DecodeBdsValues	decode_bds( grib->bds );
		DecodePir		decode_pir( decode_bds, grib->gds );
		DecodePds		decode_pds( decode_pir, grib->pds );
		DecodeGrid		decode_grid( decode_pds, grib->gds ); 

		//ptr< Grid> grid = decode_grid.create(); 
		grid = decode_grid.create(); 
//		data = new DataGrid( grid );
//		surface->data = new DataGrid( grid );
	}
	else
		assert( 0);



	ptr< SurfaceCriteria> criteria 
		= new SurfaceCriteria( 
			param,
			level,
			valid,
			area );




	add_grib.push_back( grib, grid, criteria ); 

//	return new Surface( criteria, data ); 
}



ptr< Grid> decode_grib_to_grid( const std::vector< unsigned char> & buf1 )
{
	// obsolete
	// just decode the grid data values.

	ptr< Grib1_sections> grib = decode_grib_to_sections(  buf1 ); 

	if( !( grib->pds.db_flag & HAS_BMS )
		&&  ! ( grib->bds.flg & BDS_PACKING )
		&& grib->gds.pir.size() ) 
	{
		// this is our pipeline ...
		DecodeBdsValues	decode_bds( grib->bds );
		DecodePir		decode_pir( decode_bds, grib->gds );
		DecodePds		decode_pds( decode_pir, grib->pds );
		DecodeGrid		decode_grid( decode_pds, grib->gds ); 

		ptr< Grid> grid = decode_grid.create();
		return grid;
	}
	

	assert( 0);
}






// ok, where do we actually put the grib values ???


template< class T> 
static std::string str2( const T &v, const char *format)
{
	char buf[ 1000];
	sprintf( buf, format, v);
	return buf; 
}

static std::string str2( float v) { return str2( v, "%f"); }  
static std::string str2( int v) { return str2( v, "%d"); }
static std::string str2( unsigned int v) { return str2( v, "%u"); }




/*
 * lat/lon grid (or equidistant cylindrical, or Plate Carree),
 * used when grib1->gds->type is GRID_LL
 */



template< class T> 
static void output( /* std::ostream & os */ const std::string &s1, const T &value)
{
	std::string s = "  " + s1;
	while( s.size() < 20) 
		s += " ";	
	std::cout << s << value << std::endl;
}

// we should be passing in the stream 

void output( const std::string &s1)
{
	std::cout << s1 << std::endl;
}


void Ids_section::dump()
{
	output( "Ids section"); 
	output( "char[4]", buf);
	output( "len", len);
	output( "edition", edition);
}



void Pds_section::dump()
{
	output( "PDS section");
	output( "len", len);
	output( "table_version",  table_version);
	output( "center",  center);
	output( "model", model);	 
	output( "grid", grid);	 	 

	std::string s = str2( db_flag) + " " +  format_bits( db_flag, 8) + " " +  str2(   db_flag, "%o"); 
	if( db_flag & HAS_GDS  ) s +=  " (gds included)"; 
	if( db_flag & HAS_BMS )	s +=  " (bms included)"; 
	output( "db_flag", s);		
					
	output( "param",   param  ); 
	output( "level_flg",  level_flg  ); 
	output( "level2 (OR 2*g1int)", level2 ); 


	output( "year", year ); 
	output( "month", month); 
	output( "day", day ); 
	output( "hour", hour ); 
	output( "minute", minute ); 


	output( "tunit", tunit ); 
	output( "tunit_0", tunit_0 ); 
	output( "tunit_1", tunit_1 ); 
	output( "tr_flg", tr_flg ); 

	output( "century", century ); 

	output( "subcenter", subcenter );

	output( "avg", avg );
	output( "missing", missing );
	output( "scale10", scale10 );

}


void Grid_ll::dump()
{
	output( "Grid_ll");   
	output( "ni (x)",  ni);
	output( "nj (y)",  nj);	 	
	output( "ni * nj", int (ni * nj) ); 	 
	output( "lat1/2 lon1/2", str2( la1) + " " + str2( la2) + "  " + str2( lo1) + " " + str2( lo2));		 
	output( "res_flags", str2( res_flags) + " " + format_bits( res_flags, 8));
	output( "di",  di);
	output( "dj",  dj); 	

	output( "scan_mode", str2( scan_mode) + " " + format_bits( scan_mode, 8));  /* scanning mode flags (table 8) */ 	
}



void Grid_rll::dump()
{
	output( "Grid_rll");   
	output( "ni (x)",  ni);
	output( "nj (y)",  nj);	 	
	output( "ni * nj", int (ni * nj) ); 	 
	output( "lat1/2 lon1/2", str2( la1) + " " + str2( la2) + "  " + str2( lo1) + " " + str2( lo2));		 
	output( "res_flags", str2( res_flags) + " " + format_bits( res_flags, 8));
	output( "di",  di);
	output( "dj",  dj); 	
	output( "scan_mode", str2( scan_mode) + " " + format_bits( scan_mode, 8));  /* scanning mode flags (table 8) */ 

	output( "lapole", lapole);
	output( "lopole", lopole);
	output( "angrot", angrot);
}
    
void Gds_section::dump()
{
	output( "GDS_section");
	output( "len", len );    
	output( "nv",  nv);	 
	output( "pv",   pv);	 
	output( "type (proj)",   type);  			

	if( type == 0) ll_grid.dump();
//		else if( type == GRID_OLAMBERT) ol_grid.dump();
}


void Bds_section::dump()
{
	output( "BDS section");
	output( "len",   len   );
	output( "flg", str2( flg) + " " +  format_bits(  flg,  8));	 	
	
	// first 4 bits
	std::cout << "  BDS_KIND       simple or spherical                   " << bool( flg & BDS_KIND) << std::endl; 
	std::cout << "  BDS_PACKING    simple or (spherical or second order) "  << bool( flg & BDS_PACKING)	<< std::endl;    			 		 
	std::cout << "  BDS_DATATYPE   float or integer                      " << bool( flg & BDS_DATATYPE) << std::endl; 
	std::cout << "  BDS_MORE_FLAGS additional flags for second order     " << bool ( flg & BDS_MORE_FLAGS) << std::endl;

	
	// subsequent bits - we need to left shift to use these macros
	if( flg & BDS_MORE_FLAGS) { 
		
		// see http://www.nco.ncep.noaa.gov/pmb/docs/on388/table11.html

		// 12-13 is N1 
		//bds_2nd *bds2 = & bds_section->data;
		//std::cout << " flag " << format_bits( flag, 8) << std::endl;
		//int flag = //this->flag << 4 ; 
		//std::cout << " now  " << format_bits( flag, 8) << std::endl;

		// we are refering to the wrong 4 bits ? 
		// this cannot be correct 
/*			std::cout << " ---------------------------" << std::endl;
		std::cout << " BDS_MATRIX     single or matrix                      " << bool( flag & BDS_MATRIX) << std::endl;			
		std::cout << " BDS_SECONDARY  secondary bit maps                    " << bool( flag & BDS_SECONDARY) << std::endl;		
		std::cout << " BDS_WIDTHS     constant or different widths          " << bool( flag & BDS_WIDTHS) << std::endl;
		// one bit here is reserved

		// one byte here is wrong 
	
		// we have not decoded upper bytes ? 
		std::cout << " ---------------------------" << std::endl;		
		std::cout << " BDS_MF_NON_STD mf non std 2nd order                  " << bool( flg & BDS_MF_NON_STD)	<< std::endl;				
		std::cout << " BDS_MF_BOUSTRO mf balayage boustrophedonique         " << bool( flg & BDS_MF_BOUSTRO) << std::endl;
		std::cout << " BDS_MF_SPC_DIFF_ORDER  mf differentiation spatiale   " << bool( flg & BDS_MF_SPC_DIFF_ORDER) << std::endl;
*/
	}

	/* right unused_bits is not going to be correct when bds_packing is different */


	output( "unused bits", unused_bits );
	output( "scale",  scale  );		 		
//	 	output( " ref", str2( "%x", ref)  );//
	output( "bits (prec)",  bits  );
	output( "ref", fref  );	

}

void Bms_section::dump()
{
	output( "BMS section");	 
	output( "len (bits)",  str2( len) + " " + str2( len * 8) );	 
	output( "nbits",  nbits );	 
	output( "map_flg ",  str2( map_flg) + " " +   format_bits( map_flg, 8))  ;	
	output( "include.size", include.size() ); 
//		printf( " *include_count          %d\n", include_count ) ; 
}






#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH



void output_basic_info( std::ostream & os, Grib1_sections & grib )
{
	// basic stuff 

	os << "---------------------" << std::endl;

	os << "param " << grib.pds.param << std::endl;
	//os << "ni " <<  grib.gds.ni << std::endl;
	//os << "nj " <<  grib.gds.nj << std::endl;

	os << "level_flg " << grib.pds.level_flg << std::endl; 
	os << "lev(OR 2*g1int)" << grib.pds.level2 << std::endl; 


	os << "has bms " << ( grib.pds.db_flag & HAS_BMS ) << std::endl;
	os << "has pir " << grib.gds.pir.size()  << std::endl;
	if( grib.gds.pir.size())
		os << "pir first " << grib.gds.pir.front() << " last " << grib.gds.pir.back()  << std::endl;
	os << "grib.bds.bits " << grib.bds.bits << std::endl ; 

/*
	std::vector< double> & values = grib.bds.values;

	os << "values.size " << values.size() << std::endl;
	os << "first value " << values.front()  << " last "  << values.back() << std::endl;
	
	assert( ! values.empty() );	
	os << "min " << *min_element( values.begin() , values.end() ) << "  " ;
	os << "max " << *max_element( values.begin() , values.end() ) << std::endl;


	os << "values ";
	int i = 0;
	foreach( double value, grib.bds.values ) 
	{
		if( i++ < 10 )
			os << value << ", " ;
		// os << "value " << value << std::endl;
	}
*/
	os << std::endl;
}


#if 0
class BitmapReader
{
public: 
	const std::vector< unsigned char> 	& buf; 

	int 			pos;		// i think this is in bits ?? 
	int				t_bits; 
	unsigned int	tbits;
	
	BitmapReader( const std::vector< unsigned char> &buf1 )
		: buf( buf1), 
		pos( 0 ),
		t_bits( 0 ), 
		tbits( 0 ) 
	{ }
	BitmapReader( const std::vector< unsigned char> &buf1, int pos )
		: buf( buf1), 
		pos( pos ),
		t_bits( 0 ), 
		tbits( 0 ) 
	{ }
	
	unsigned int decode( int n_bits ) 
	{
		// how many bits of precision does this handle. we really need to 
		// test this a bit more. 
		unsigned int  jmask = (1 << n_bits) - 1;
      
		while (t_bits < n_bits) 
		{
			assert( pos >= 0 && pos < buf.size()); // handle underflow
			tbits = (tbits * 256) + buf[ pos];
			t_bits += 8;
			++pos;
		}

		t_bits -= n_bits;
		unsigned int value = (tbits >> t_bits) & jmask;
		return value; 
	}
};
#endif


#if 0
	int n = 0; 
	if( grib.pds.db_flag & HAS_BMS) 
	{
		n = 0; 

		assert( grib.bms.include.size() == grib.gds.ni * grib.gds.nj);	// should be done elsewhere

		for( int i = 0; i < grib.bms.include.size(); ++i)
			n += grib.bms.include[ i];
	}
	else if( grib.gds.pir.size()) 
	{ 
		n = 0;
		for( int i = 0; i < grib.gds.pir.size(); ++i)
		{
			n += grib.gds.pir[ i]; 		 
		}
	}
	//	else if( flg & BDS_PACKING
	else 
	{ 
		n = grib.gds.ni * grib.gds.nj; 
	}
#endif
	// ok,

#if 0
static void decode_bds_with_nobms_and_pir( 
	Reader &b, 
	Bds_section & bds, 
	const Pds_section & pds, 
	const Gds_section & gds )
{
	// pds and gds are dependencies.

	// OK, and we can also handle the scale10

	// there are quite a few deps here.
	// scan direction ...
	// and the grid


	assert( !( bds.flg & BDS_PACKING));		// to know this means we must have prescanned it ??? 
	assert( !(pds.db_flag & HAS_BMS)); 
	assert( ! gds.pir.empty() ) ; 


	int pos = b.pos; 		

	bds.len = b.dec3();					
	bds.flg = b.dec1(); 
	bds.scale  = b.dec2s(); 	 
	bds.ref  = b.dec4(); 
	bds.bits = b.dec1(); 

	// the number of points is determined by the pir which gives the rows
	int n = 0;
	for( int i = 0; i < gds.pir.size(); ++i)
	{
		n += gds.pir[ i]; 		 
	}

	double fref = ref_to_float(  bds.ref );  
		
	int unused_bits = 0x000f & bds.flg;			// 0xf is 4 bytes masked. There is no endian issue inside a byte. 


	bds.values.reserve( n);

	BitmapReader	bitmap( b.buf, b.pos );  // ok but we need to set pos in the buffer

	for( int i = 0; i < n; ++i ) 
	{
		unsigned int raw = bitmap.decode( bds.bits );
		float val = ( pow( 2.f, bds.scale) * raw + fref  ) * pow( 10, - pds.scale10 );	
		// std::cout << "i " << i << " " << val  << std::endl;
		bds.values.push_back( val);
	}
	b.pos = bitmap.pos; 
	
	int adv = bds.len - (b.pos - pos) ; 
	assert( adv >= 0);	 
	//std::cout << "@bds advance " << adv << std::endl;	
	b.advance( adv);		 
	return;
};
#endif


#if 0
	grib.bds.decode( b, n ); 


	// we should actually detect this otherwise it indicates that something went wrong ...	
//	std::std::cout << std::string( b.current(), b.current() + 4 ) << std::std::endl;

	// this interface is crappy. Just pass in a byte buffer and copy it.	


	char tail[ 5]; 
	b.dec_raw( tail, 4 );
	tail[ 4] = 0;

	std::cout << "tail '" << tail << "'" << std::endl;


	std::cout << "b.pos " << b.pos << std::endl;
	std::cout << "b.buf.size() " << b.buf.size() << std::endl;


//	assert( b.pos == b.buf.size() );

/*
	int section_sum_sz = ( p - &buf1[ 0]) + sz + 4; 
	
	std::cout << "section_sum_sz " << section_sum_sz 
		<< " == ids.sz " << grib.ids.len
		<< " == buf.sz() " << buf1.size() 
		<< std::endl; 

	//	assert( section_sum_sz == ids_.len);
	//	assert( section_sum_sz == buf1.size()  );	// should be ok, 

*/

#endif



#if 0

/// should we just supply the grib. No we should supply enough   

// the problem is that there are dependencies, everywhere. 

/*
	OK WE SHOULD BE SUPPLYING THE GDS 
		
	because the calculation for n, depends on whether there is a PIR.
*/

void Bds_section::decode( Reader &b, /*bds *bds_section,*/ int n )
{
	// n is the number of points

	#if 0
	g3int len;			/* Length of section in bytes */
	g1int flg;			/* High 4 bits are flag from GRIB table 11.
				   Low 4 bits are no. of unused bits at end. */
	g2sint scale;		/* (signed) scale factor */
	g4flt ref;			/* Reference value (min of packed values) */
	g1int bits;			/* Number of bits containing each packed val */
	#endif		 

	assert( values.empty());

	int pos = b.pos; 		

	len = b.dec3();					
	flg = b.dec1(); 
	scale  = b.dec2s(); 	 
	ref  = b.dec4(); 
	bits = b.dec1(); 


	// IMPORTANT
	// why do we even bother to record the stuff that we don't actually need. 
	// why not just extract the values ????
	// eg. scale, ref, bits, len etc.

	// but this is the opposite of our actual approach of decoding values 
	// and then reconstructing, using separate context. 
	// Bloody hell.

	// but having all the values, makes it a lot easier to dump to stdout and debug 

	// OR. MAYBE THIS IS WHERE WE SHOULD BE DOING EVERYTHING. CONSTRUCTING THE GRID 
	// WITH ALL FUCKING CONTEXT AVAILABLE on scan direction, and PIR  etc.
	
	// I think that we should build a scan direction index vector. and use indirection or subscript.  
	// the same as we build a 

	// OR WE DON"T DECODE this section at all separately but together .
	
	// eg. with WITH_PIR_NOBMS


	fref = ref_to_float(  ref );  
		
	unused_bits = 0x000f & flg; 	// 0xf is 4 bytes masked - endian ? 

	// std::cout << "unused bits " << unused_bits << std::endl;

	if( !(flg & BDS_PACKING))		// ordinary packing
	{
		std::cout << "ordinary packing" << std::endl;
		values.reserve( n);

		BitmapReader	bitmap( b.buf, b.pos );  // ok but we need to set pos in the buffer

		for( int i = 0; i < n; ++i ) 
		{
			unsigned int raw = bitmap.decode( bits);
			float val = ( pow( 2.f, scale) * raw + fref  );	// need to apply dec10 
			// std::cout << "i " << i << " " << val  << std::endl;
			values.push_back( val);
		}
		b.pos = bitmap.pos; 
		
		int adv = len - (b.pos - pos) ; 
		assert( adv >= 0);	 
		//std::cout << "@bds advance " << adv << std::endl;	
		b.advance( adv);		 
		return;
	}

	assert( 0);




	if( flg & BDS_PACKING) {	// spheirc or second order and implies other bits have meaning

		std::cout << "second order or spheric" << std::endl;

		assert( flg & BDS_MORE_FLAGS);	// it could still be spherical

		// 12-13 is N1 		
		int N1 = b.dec2(); 
		int ext_flg = b.dec1();
		int N2 = b.dec2();		
		int P1 = b.dec2();
		int P2 = b.dec2();		
		b.advance( 1);	// reserved

		std::cout << "N1 " << N1 << " N2 " << N2 <<   std::endl;			
		std::cout << "P1 " << P1 << " P2 " << P2  ;	
		std::cout << "ie N1 + P1 == N2 --> " << N1 << " + " << P1 << " == " << (N1+P1) << " " << N2 << std::endl; 
		std::cout << "n is " << n << std::endl;
		std::cout << "EEE " << n - P2 << std::endl;
	
	 
		//assert( n == P2); seems to be a difference of two only
 

		if( /*true ||*/ !(ext_flg & BDS_MF_NON_STD)) { 	// normal second order packing

			std::cout << "############# normal second order packing #####" << std::endl;

			std::cout << " ---------------------------" << std::endl;
			std::cout << " BDS_MATRIX     single or matrix                      " << bool( ext_flg & BDS_MATRIX) << std::endl;			
			assert( !( ext_flg & BDS_MATRIX));	// reserved future capabiltiy

			std::cout << " BDS_SECONDARY  secondary bit maps                    " << bool( ext_flg & BDS_SECONDARY) << std::endl;		
			std::cout << " BDS_WIDTHS     constant or different widths          " << bool( ext_flg & BDS_WIDTHS) << std::endl;
			std::cout << " ---------------------------" << std::endl;		
			std::cout << " BDS_MF_NON_STD mf non std 2nd order                  " << bool( ext_flg & BDS_MF_NON_STD)	<< std::endl;				
			std::cout << " BDS_MF_BOUSTRO mf balayage boustrophedonique         " << bool( ext_flg & BDS_MF_BOUSTRO) << std::endl;
			std::cout << " BDS_MF_SPC_DIFF_ORDER  mf differentiation spatiale   " << bool( ext_flg & BDS_MF_SPC_DIFF_ORDER) << std::endl;
				
			if( ext_flg & BDS_SECONDARY) 
			{
				// ok secondary bitmap seems to work
				std::cout << "got secondary" << std::endl;
	/**/		BitmapReader 	bitmap( b.buf, b.pos );
				for( int i = 0; i < P1; ++i) 
				{		 
					int value = bitmap.decode( 1);
					std::cout << value;				
				}
				b.pos = bitmap.pos; 
	/**/		}			
			else 
				std::cout << "not secondary" << std::endl;


			if(  ext_flg & BDS_WIDTHS)		// different widths we have to take	otherwise constant
			{
				std::cout << "got changing widths" << std::endl; 
				
				int bit_sum = 0; 
				for( int i = 0; i < 71; i++) { 

					int width = b.dec1(); 
					if( i == 71) std::cout << "----" ;
					std::cout << width << ", ";//<< " format " << format_bits( width, 8) << std::endl;				
					bit_sum += 71 * width; 					
				}
				std::cout << std::endl;

				std::cout << "bitsum " << bit_sum << std::endl;
			}
			else 
				std::cout << "constant widths" << std::endl;


	//			if( ext_flg & BDS_SECONDARY) 
	//			exit( 0);			

			std::cout << std::endl;
				
		}	// standard second order

		/*	}
			

			else { 

				std::cout << "############# second order mf non stand ####" << std::endl;
			}
	*/

		n = 0 ;

	}


	assert( 0);
}
#endif




/*
static inline unsigned int Round( float v) 
{ 
	return (int)  v; 
}
*/

/*
static inline unsigned int Dec4(unsigned char* sss) 
{ 
	return sss[3] + 256 * (sss[2] + 256 * (sss[1] + 256 * sss[0])); 

}
*/

/*
static inline int dec3s(unsigned char * sss)
{ int iii ;
iii = sss[2] + 256 * (sss[1] + 256 * (sss[0] & 0x7f)) ;
iii = (sss[0] & 0x80) ? -iii : iii ; 
return (iii) ; }
*/
 
/*
static inline int dec3( unsigned char * sss)
{ int iii ;
iii = sss[2] + 256 * (sss[1] + 256 * sss[0]) ; 
return (iii) ; 
}
*/

 

/*
static inline int dec2s(unsigned char * sss)
{ int iii ;
iii = sss[1] + 256 * (sss[0] & 0x7f) ;
iii = (sss[0] & 0x80) ? -iii : iii ;
return(iii) ;
}
/** /
static inline int dec2(unsigned char * sss)
{ int iii ;
iii = sss[1] + 256 * sss[0] ;
return(iii) ;
}
 /**/


/*
static inline int dec1(unsigned char * sss)
{ int iii ;
iii = sss[0] ;
return (iii) ;
} 
*/
// i think we want a bit buffer seperate 


#if 0 
shared_ptr< Grib1_sections>	decode_headers( const vector< unsigned char> &buf1)
{
	
	shared_ptr< Grib1_sections>	ret( new Grib1_sections);

	
	
	Reader 	b;
	b.buf = buf1; 

	// read in section 0
	//Ids_section ids_;
	ret->ids = shared_ptr< Ids_section>( new Ids_section);
	ret->ids->decode( b);
	ret->ids->dump();
  	
	// read in section 1
	//p =  p + sizeof( ids); // sz ??
 
	Pds_section	pds_;
	ret->pds = shared_ptr< Pds_section>( new Pds_section);
	ret->pds->decode( b);
	ret->pds->dump();
 	//sz = pds_.len;  ;


	return ret; 

}
#endif




#if 0
	

	sz = bds_.len; 
//	std::cout << "---" << std::endl;
	
	int section_sum_sz = ( p - &buf1[ 0]) + sz + 4; 
	
	std::cout << "section_sum_sz " << section_sum_sz 
		<< " == ids.sz " << ids_.len
		<< " == buf.sz() " << buf1.size() 
		<< std::endl; 

	assert( section_sum_sz == ids_.len);
	assert( section_sum_sz == buf1.size()  ); 
// read in data
// right total data length should be
//	printf( "total data size %d %d\n", sz - sizeof( bds) + 1 , (sz - sizeof( bds) + 1) * 8);
	int ni = gds_.ni;
	int nj = gds_.nj;
	
	if( ni == 0xffff) ni = nj; 
	if( nj == 0xffff) nj = ni; 
	
	n = ni * nj;
//	printf( "number of elts %d %d = %d\n", ni, nj, n, n * 8 );
  
	int data_byte_len = 0;   

#if 0
	string g; 

	if( pds_.db_flag & HAS_BMS)  g += "has_bms";
	else g += "no_bms ";
	if( gds_.pir.size()) g += " has_pir" ; 
	else g += " no_pir "; 
	 
	std::cout << "here " << g << std::endl; 
#endif


	if( !( pds_.db_flag & HAS_BMS ) ) { 
	 
		// no bms
		if( gds_.pir.size()) { 
	 		int sum = 0; 
			for( int i = 0; i < gds_.pir.size(); ++i)
				sum += gds_.pir[ i]; 
			std::cout << "sum = " << sum << std::endl;
			n = sum ; 
		}
		else { }

		MyGrib	result ;
		result .values = new Grid( ni, nj ) ;  
		// might want to be 64 bit prec 
		int data_bit_len = ( n * bds_.bits +  bds_.unused_bits );

		std::std::cout << "------------------------------" <<  std::std::endl;
		std::std::cout << "@@@@ bds_.bits " << bds_.bits << std::std::endl;
/*
		int data_bit_modulo = data_bit_len % 8; 
		std::cout << "-modulo of bits " << data_bit_modulo << std::endl << flush;
//		assert( data_bit_modulo == 0);  // lfcr
		data_byte_len = data_bit_len / 8 ; 
*/

		std::std::cout << "p " << std::string( p, p + 4)  << std::std::endl;
		std::std::cout << "buffer pos " << std::string( b.current(), b.current() + 4) << std::std::endl;

		std::std::cout << "p - buffer pos " <<  (p - b.current() )  << std::std::endl;


		// OK THIS IS  at position 79
		unsigned char *data_start = p + BDS_SECTION_SIZE ;      // 		 	 


		std::std::cout << "bds section size " << BDS_SECTION_SIZE << std::std::endl;
		std::std::cout << "data start pos " << (data_start - & b.buf[ 0]) << std::std::endl; 


		std::std::cout << "n is " << n << std::std::endl; 

		for( int i = 0; i < n; i++) 
		{
			double raw;
			switch( bds_.bits ) 
			{ 

				case 16: 
				{
					int index = i * 2; 
					raw = (float ) dec2(  & data_start[ index ] ); break;
				}
			//	case 1: raw = (float ) dec1(  & data_start[ index ] ); break; 
				default:
					assert( 0);
			}

			// this is correct  scale10 is applied after the fref
			//float val = ( pow( 2, scale2) * raw + fref  );// * pow( 10, -scale10);	
			double val = ( pow( 2, bds_.scale ) * raw + bds_.fref  ) * pow( 10, - pds_.scale10);	
	

			(*result.values)( i) = val ; 
		}

		return result;
	}



	if( pds_.db_flag & HAS_BMS ) { 
		
		std::cout << "bms implied sz   " << bms_.include.size() << std::endl;
		std::cout << "ni * nj sz       " << ni * nj << std::endl;

//		std::cout << "bms include_count " << bms_.include_count << std::endl;   


		assert( ni * nj <= bms_.include.size());
 	
		int implied_count = 0; 
		for( int i = 0; i < bms_.include.size(); ++i)
			if( bms_.include[ i]) 
				++implied_count; 	 
		std::cout << "elts using implied bms sz " << implied_count << std::endl;

		int ninj_count = 0; 
		assert( ni * nj <= bms_.include.size());

	 
		for( int i = 0; i < ni * nj; ++i)
		{
			if( bms_.include[ i]) 
				++ninj_count; 	 
			std::cout << "elts using ni nj sz " << ninj_count << std::endl;
		} 
		// hang on we have a modulo here .... 

		int data_bit_len = ( ninj_count * bds_.bits +  bds_.unused_bits );
		int data_bit_modulo = data_bit_len % 8; 	 
		std::cout << "bms modulo " << data_bit_modulo << std::endl;
//		assert( data_bit_modulo == 0);		// lfcr should be on 16 boundary ?

		data_byte_len = data_bit_len / 8 ; 

		assert( !gds_.pir.size()) ; 
	}

	/*
		NEW. We should treat the pathways for bms and non-bms completely separately.

	*/

	
	std::cout << "-data_byte_len " << data_byte_len << std::endl; 
	std::cout << "-unused bits " << bds_.unused_bits  << std::endl;	

	unsigned char *data_start = p + BDS_SECTION_SIZE ;      // 		 	 

	int size_till_data = data_start - &buf1[ 0]; 

	std::cout << "size_till_data " << size_till_data << std::endl;
		
	int bit_section_sum_sz = size_till_data  + data_byte_len + 4;
	
	std::cout << "-bit_section_sum_sz " << bit_section_sum_sz << std::endl;
	


	int pad = ids_.len - bit_section_sum_sz; 
	std::cout << "-pad " << pad << std::endl << flush;
//	assert( pad == 0); //	lfcr


	

	// i think that data_start may be incorrect 
	// since this end of data seems to be ok 

	// so we should be able to calculate inversely from here the grib size
	// using the include_count and the bit precision

	// we can also then handle the 


#endif


 
#if 0 

typedef list< vector< unsigned char> > buf_type;

/*
	ok we do get 7777 values in the middle of files 
	unfortunately. 
	

*/
 



buf_type  split_gribs_by_scan( vector< unsigned char> &buf)
{
	std::cout << "split gribs by scan" << std::endl; 
	buf_type ret; 
	
	int i = 0; 

	while( true)
	{
		 
		// find start 
		int advance = 0; 
		while( i + 4 < buf.size()  
			&& !(buf[ i+0] == 'G' && buf[ i+1] == 'R' && buf[ i+2] == 'I' && buf[ i+3] == 'B'))  {
//		 	std::cout << "advancing" << std::endl ; 
			++advance; 
			++i;
		}

		//std::cout << "here1 " << i << std::endl;

		// find end
		int u = i;
		while( u + 4< buf.size()
			&& !(buf[ u+0] == '7' && buf[ u+1] == '7' && buf[ u+2] == '7' && buf[ u+3] == '7'))
			u++;

		u += 4; 

		if( i == buf.size())
			break; 

		vector< unsigned char>	buf2; 
		buf2.reserve( u - i);


		buf2.clear();
		for( int k = 0; k < u - i; ++k)
			buf2.push_back( buf[ i + k]);
 
		
		i = u; 

		ret.push_back( buf2);
	}

	return ret;
}



buf_type  split_gribs( vector< unsigned char> &buf)
{

	std::cout << "split gribs" << std::endl; 

	buf_type ret; 

	 

	int i = 0; 

	while( true)
	{
		
//		std::cout << "-------------------------" << std::endl;
//		std::cout << "current '"  ;
//		for( int j = i; j < i + 10 && j < buf.size(); ++j)
//			std::cout << buf[ j]; 
//		std::cout << "'" << std::endl;


		// find start 
		int advance = 0; 
		while( i + 4 < buf.size()  
			&& !(buf[ i+0] == 'G' && buf[ i+1] == 'R' && buf[ i+2] == 'I' && buf[ i+3] == 'B'))  {
		 	++advance; 
			++i;
		}

//		std::cout << "advance " << advance << std::endl;

			// we may have reached the end or have garbled stuff 
		if( i + 4 >= buf.size())
			break; 
		
		// ok we may have the indicated len or the end of the file ... 
//	 	bool got_len = false; 
		int u = 0; 

		if( buf.size() > 7 ) { 
		
			int	indicated_len = dec3((unsigned char *)& buf[ i + 4] );	

/**/		if( indicated_len == 0) { 
				std::cout << "no len in grib scanning" << std::endl;
				int e = i; 
				while( e + 4< buf.size()
					&& !(buf[ e+0] == '7' && buf[ e+1] == '7' && buf[ e+2] == '7' && buf[ e+3] == '7'))
					e++;		

				if( i + 4 >= buf.size())
					continue; // to get to main loop and then break; 

				std::cout << "got len by scanning of " << e - i << std::endl;
				u = e + 4; 
//				assert( 0);
			}
/**/			
			else { 
			//	std::cout << "using indicated len " << indicated_len << std::endl;
				int e = i + indicated_len - 4; 		
				if( buf[ e+0] == '7' && buf[ e+1] == '7' && buf[ e+2] == '7' && buf[ e+3] == '7')  
					u = e + 4; 
				else { 
					std::cout << "grib size wrong" << std::endl;
//					assert(0);
			//		continue; 
					 				
				}					
			}



		}		
		assert( u ); 

/// scanning for 7777 is problematic 

	
 
		if( i == u) {
			std::cout << "got end" << std::endl;
			break;
		}

 
		vector< unsigned char>	buf2; 
		buf2.reserve( u - i);


		buf2.clear();
		for( int k = 0; k < u - i; ++k)
			buf2.push_back( buf[ i + k]);
 

		ret.push_back( buf2);

		i = u;   
	}


	std::cout << "num gribs " << ret.size() << std::endl;

	return ret; 


}

#endif


#if 0
int main( int argc, char **argv)
{
	quick_sizeof_check();

	if( argc != 2) {
		printf( "usage: %s <filename>\n", argv[ 0] );
		exit( EXIT_FAILURE );
	}
//	int len;
	char *in_filename = argv[ 1];
//	FILE *fp = open_file( in_filename, "rb");
	//FILE *fp = fopen( in_filename, "rb");

	vector< unsigned char>	buf; 

	read_file( buf,  in_filename);


	buf_type  gribs = split_gribs(  buf);

//	buf_type gribs = split_gribs_by_scan( buf); 

	std::cout << "gribs " << gribs.size() << std::endl; 

/*
	for( buf_type::iterator ip = gribs.begin();
		ip != gribs.end(); ++ip)
	{
			
		vector< unsigned char>	&grib = *ip;		
		for( int i = 0; i < 4; ++i)
			std::cout << grib[ i];		
		for( int i = grib.size() - 4; i < grib.size(); ++i)
			std::cout << grib[ i];
	}
	*/


//	exit( 0);

//	return 0;

	for( buf_type::iterator ip = gribs.begin();
		ip != gribs.end(); ++ip)
	{
		vector< unsigned char> &buf = *ip;


		std::cout << "-------------------" << std::endl; 
		for( int i = 0; i < 4; ++i)
			std::cout << buf[ i];

		std::cout << "...";

		for( int  i = buf.size() - 4; i < buf.size() && i >= 0; ++i)
			std::cout << buf[ i];
	
		std::cout << std::endl;


		shared_ptr< Grib1_sections> grib = decode_headers( buf);
	  
//	 	decode( buf);
	  

		std::cout << std::endl; 		
	}

 

	return 0;

}
#endif

/*
FILE *open_file( char *name, char*flags)
{
	FILE *fp = fopen( name, flags);
	if( !fp) {
		printf( "could not open file '%s'\n", name, strerror( errno) );
		assert( 0); 
	}
	else if( verbose) 
		printf( "file '%s' opened\n", name);
	return fp;
}*/

