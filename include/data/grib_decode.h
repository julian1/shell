
/*
	http://www.wmo.int/pages/prog/www/WDM/Guides/Guide-binary-2.html#Section1
*/
/*
	if we have modular meta decoders for different centres, then we do need to expose these
	basic structures here, to support writing them.
*/
/*
	VERY IMPORTANT. We can always decode a section independently of other sections, even if they
	have bitmaps/buffers, etc, because the sections all have a len field. 

	All sections can be extracted/parsed without any dependencies between parsing.
*/

/*
	when we combine gribs together, potentially we should include  
*/
// original source - /home/meteo/OLD/turion_laptop/purple/gdump/ 


// should poentially be able to treat sections optionally. if it doesn't exist then it should be missing 
// and if we don't want to parse it. that means that we have to return the values as functions
// because we don't want to instantiate if it's missing.  

/*
	- ok, a point about using a vector based char buf, is that it's very easy to reposition the bit buffer
	exactly where we want it.

	A fully decoded grib. should probably still hide it's internals ??? 

	Remember our wrapper concept. Will be applied. to data. 

	BUT FOR ANY FORMULA WE HAVE TO HAVE A georef DESCRIPTION OF THE GRID, and use pds pattern matching, to determine the parameter etc.

		
	struct IGrib1_sections
	{
		add_ref()
		release_ref()

		const & Pds_section get_pds();
		const & Pds_section get_grid();

		ptr< Grid> get_grid_data()
	};

	making it flexible is going to be fairly effective. 

	EXTREMELY IMPORTANT. 
		we can put a dynamic tag system wrapper over the top of the grid if we want.

	- OK. WE PROBABLY WANT A SLIGHTLY SIMPLIFIED GRID Description.  where we don't need the derivatives etc.
	Except this should probably be created by the intermediate layer ???

	- Because remember that we want to be able to dump all the parameters,  

	- Do we use ref counting on these structures, or stupid copying. copying is the wrong default.

	- or just use a scoped_ptr ??. 
*/

#pragma once

#include <vector>

/*
// shouldn't need this here...
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time ;	 
*/

class ByteReader; 

/*
	- We don't have to expose all of these interfaces. 
	- actually we almost certainly do have to expose the pds, and the grid. 
	- and the decoded grid itself.
*/

struct Ids_section
{
public:
	char	buf[ 5];		// change to string ? 
	int		len; 
	int		edition; 

	void decode( ByteReader &b );
	void dump();
};


class Pds_section
{
public:

	int len;  
	
	// meta
	int table_version;  
	int center; 
	int model; 	/* model id, allocated by center */
	int grid; 	//* grid id, allocated by center */ 	 

	int db_flag ; 		 // what is this.

	int param;

	int level_flg ; 
	int level2;

	// reference time of data
	int year;			/* reference time of data */
	int month;
	int day;
	int hour;
	int minute; 

	// more time stuff
	int tunit;		/* unit of time range, from GRIB table 4 */
	//	    g1int tr[2];		/* periods of time or interval */
	int tunit_0;
	int tunit_1;
	int tr_flg ;		/* time range indicator, from GRIB table 5 */

	int century;		/* 20 for years 1901-2000 */

	int subcenter; 
	
	int avg  ;	 
	int missing  ;	 		
 	int scale10;
#if 0  
    unsigned char reserved1[12];	/* reserved; need not be present */
    unsigned char reserved2[GRIB_ARB]; /* reserved for local center use; */
#endif	

	void decode( ByteReader &b);	
	void dump() ;

}; 



class Grid_ll
{
public:
    int	ni;			/* number of points along a parallel */
    int	nj;			/* number of points along a meridian */
    int la1;			/* latitude of first grid point */
    int lo1;			/* longitude of first grid point */
    int res_flags;		/* resolution and component flags (table 7) */
    int la2;			/* latitude of last grid point */
    int lo2;			/* longitude of last grid point */
    int di;			/* i direction increment */
    int dj;			/* j direction increment */
    int scan_mode;		/* scanning mode flags (table 8) */
	#if 0    
		unsigned char reserved[4];
		union {			/* need not be present */
		g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
		g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
		} vn;
	#endif 

	void decode( ByteReader &b );
	void dump();
};

// OK we should place stuff in common in a common entry ...
// and then build up a vector representing the order of extraction. 


class Grid_rll
{
	// don't think that this is actually used at the moment.

public:
    int ni;			/* number of points along a parallel */
    int nj;			/* number of points along a meridian */
    int la1;			/* latitude of first grid point */
    int lo1;			/* longitude of first grid point */
    int res_flags;		/* resolution and component flags (table 7) */
    int la2;			/* latitude of last grid point */
    int lo2;			/* longitude of last grid point */
    int di;			/* i direction increment */
    int dj;			/* j direction increment */
    int scan_mode;		/* scanning mode flags (table 8) */
	//  unsigned char reserved[4];
    int  lapole;		/* latitude of southern pole */
    int  lopole;		/* longitude of southern pole */
    unsigned int angrot;		/* angle of rotation */
	#if 0
		union {			/* need not be present */
		g4flt vertc[GRIB_ARB];	/* vertical coordinate parameters */
		g2int npts[2*GRIB_ARB];	/* list of number of points in each row */
		}
	#endif

	void decode( ByteReader &b );
	void dump();
};

/*
	We need to create default constructor values for all these variables.
*/

class Gds_section
{
public:
	int		len; 
	int		nv; 
	int		pv; 
	int		type; // projection/ coordinate scheme
 	
	// don't record ni/nj here 

	std::vector< int>	pir;		// points in row
	std::vector< int>	vertical_coordinate_parameters;

	// should use a boost::variant for the different grid combinations ?
	Grid_ll	ll_grid; 
	Grid_rll rll_grid;

	void decode( ByteReader &b );
	void dump();
};



class Bms_section
{
public:

	int len; 
	int nbits; 

	/* 0 if bit map follows, otherwise catalogued
				   bit map from center */
	int map_flg; 

	// we can completely decode this without reference to other sections, so do so.
	// change name to values.	
	// or buf, or precision etc. 
	std::vector< unsigned char>	include; 
 

	Bms_section(); 
	void dump(); 
	void decode( ByteReader &b );
};


/*
	- ALL SECTIONS SHOULD BE ABLE TO BE PARSED INDENDEPENTLY. IF THERE ARE DEPENDENCIES,
	Then it means we are trying to decode too much information.
	- after we have the basic data. we can extract all values. 
	- I THINK THAT THIS CONCEPT SHOULD PROBABLY ALSO BE APPLIED TO THE date/time handling as well.
*/

class Bds_section
{
	// OK we always have the length field in the bds which means, we can extract the bitmap buffer 
	// without having to know about the gds and ni nj etc. 
public:
	int		len; 
	/* High 4 bits are flag from GRIB table 11.
		   Low 4 bits are no. of unused bits at end. */
	int		flg; 
	int		unused_bits; 
	int		scale; 
	unsigned int ref; 
	float	fref; 
	int		bits; 

	// the bitmap buffer
	std::vector< unsigned char>		buf; 

	void decode( ByteReader &b );
	void dump();
};


class End_section
{
	// be explicit, as it aids formatting of fields
	char buf[ 5] ;
};


#include <common/ptr.h>
#include <common/desc.h>	// Surface
#include <common/grid.h>


/*
	Very important the sections should have a visitor, which could be used for formatting, 
	actually don't even really need this 
*/

struct Grib1_sections : RefCounted
{
	Ids_section		ids;	
	Pds_section		pds;
	Gds_section		gds; 
	Bms_section		bms;
	Bds_section		bds; 
//	End_section		end;		not implemented yet
};

struct IDecodeGribCallback 
{
	// important. the decoding of meta data. could be made into a completely separate class. which
	// is probably useful, because of additional table like dependencies. 
	
	virtual void push_back( 
		const ptr< Grib1_sections> &, 
		const ptr< Grid > &, 
		const ptr< SurfaceCriteria> &  
	) = 0;     
};

void decode_grib_to_surface( const std::vector< unsigned char> & buf1, IDecodeGribCallback &  ); 






// typedef std::pair< ptr< SurfaceCriteria>, ptr< Data> >   crit_data_t;  
//ptr< Surface> decode_grib_to_surface( const std::vector< unsigned char> & buf1 ); 


// REMOVE THESE 2.

#if 0
// first grib pass, useful for debuggin
ptr< Grib1_sections>  decode_grib_to_sections( const std::vector< unsigned char> & buf1 ); 

// useful for testing, and avoids desc.h dependencies
ptr< Grid> decode_grib_to_grid( const std::vector< unsigned char> & buf1 ); 


struct IAddGrib1Sections
{

}; 
#endif

// this needs to use an interface or a tuple to return the grid and the criteria. 
// decode meta information about grib
// why not use an interface ????

// we also don't have to fiddle with the different data types (point, section), but can specifically return a grid.. 

//struct IGribCallback

#if 0
struct DecodeGrib
{

	virtual void add_grib( const ptr< Grib1_sections> &, const ptr< Grid > &, const ptr< SurfaceCriteria> &  ) = 0;     

};

struct DecodeGrib
{
	// the point is to have a shim. so that it can be composed. but but but, then we actually want 
	// a real interface. and  
	// what's better an interface? or just creating the structure, and a vector. ??
	// creating the decoder, then using it ... will need add_ref/release ..!!! ugghhh.

	// no, this isn't correct. the 

	struct ICallback 
	{
		virtual void add_grib( const ptr< Grib1_sections> &, const ptr< Grid > &, const ptr< SurfaceCriteria> &  ) = 0;     
	};

	void decode( const std::vector< unsigned char> & buf1 ) = 0; 
};
#endif


void format_grib_sections( std::ostream & os, const ptr< Grib1_sections> & grib ); 



/*
	we want to do in 3 stages. 
		1 - raw values.
		2 - pipeline to extract
		3 - meta data, eg. standard level, parameter, grid  -( surface, layer aggregates ). 

	surely we could actually organize. the 2nd and 3rd could actually be  

	plane == surface is any type of cut. 0ww 

	We could actually combine

	- Remember the point is to be able to work easily with these. 
	- analysis, formula, combine areas, section, roaming tephigram., 
*/


