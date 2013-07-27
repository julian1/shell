/*

	THIS IS CRAP. level stuff should be in level.h or level_desc.h to avoid having to associate crap.
			remember the level_editor knows nothing about anything other than the level.
	
	- actually it has to match validity. No that is done in the adaptor.

	Very Important. All this stuff is Search Criteria. 

	- IT COULD ACTUALLY BE USEFUL TO HAVE A VISITOR HOWEVER . Eg. for printing. No just use a fucking switch
*/
#pragma once

#include <cassert>

#include <common/ptr.h>

/*
	- we basically try for algebraic typing. 
	- everything descends from common defs, so that we can treat it dynamically (vector) or statically.
	- we don't throw away static typing unless we can help it. 
	- we need a common way to describe dimensions of data independent of type (grib, analysis, plot) etc.
*/

/*
	Model and origin, probably don't even need to be stored at all. 
*/
struct Desc
{
	typedef enum { 
			none,
			// origin_grib, origin_form, origin_cube, origin_analysis,	// not sure about this ..., but we could treat like this 

			// misc
			// gts_header,   really don't think this should be part of anything

			// levels - order is used, but shouldn't be !!
			level_tropopause, level_max_wind_speed, level_isobaric, level_surface,  level_isobaric_range, //level_section_hpa,
			// params
			param_generic_grib, param_temp, param_relative_humidity, param_geopot_height, param_wind,

			// validity
			valid_simple, valid_range, 

			// area 
			area_ll_grid 

	}	type_t;


	Desc( Desc::type_t type ) 
		: count( 0 ),
		type( type)
	{ } 

	virtual ~Desc() 
	{ } 

//	IMPLEMENT_REFCOUNT
#if 1
	void add_ref()  { assert( count >= 0 ); ++count; } 
	void release()  { assert( count > 0 ); if( --count == 0 ) delete this; } 
//private:
	int count; 

	const type_t type;
#endif

private:
	Desc( const Desc & ); 
	Desc & operator = ( const Desc & ); 
};

///////////////////////////////////////////////////////



///////////////////////////////////////////////////////

// Levels
// change ordering of names eg. SurfaceLevel to LevelSurface

struct Level : Desc
{ 
	Level( Desc::type_t type ) 
		: Desc( type ) { } 
private:
	Level( const Level & ); 
	Level & operator = ( const Level & ); 
} ; 

struct LevelSurface : Level 
{ 
	LevelSurface() : Level( level_surface) { } 
};

struct LevelIsobaric : Level 
{
	LevelIsobaric( int hpa) : Level( level_isobaric), hpa( hpa ) { } 
	const int		hpa;
};

struct LevelMaxWindSpeed : Level
{
	// grib table 3a. code 6
	LevelMaxWindSpeed( ) : Level( level_max_wind_speed ) { } 
}; 

struct LevelTropopause : Level
{
	// grib table 3a. code 7 
	LevelTropopause() : Level( level_tropopause) { } 
};

struct LevelIsobaricRange: Level 
{
	LevelIsobaricRange( int hpa1, int hpa2) : Level( level_isobaric_range), hpa1( hpa1 ), hpa2( hpa2) { } 

	const int hpa1, hpa2; 
}; 

#if 0
struct SectionHpaLevel : Level 
{ 
	// for sections not layers.   
	// specifying a section level, is neat, and might work
	// std::vector< double>	hpas;
};
#endif

bool operator == ( const Level & a, const Level & b ) ; 

// this is just a useful default, but we can format locally however we wish. 
std::ostream & operator << ( std::ostream & os, const Level & level ) ; 

// should we have a default sort, probably not as it's a very local type of operation 



///////////////////////////////////////////////////////

// shouldn't need this here...
#include <boost/date_time/posix_time/posix_time.hpp>



struct Valid : Desc
{ 
	Valid( Desc::type_t type ) 
		: Desc( type ) { } 

private:
	Valid( const Valid & ); 
	Valid & operator = ( const Valid & ); 

} ;

struct ValidSimple : Valid 
{
	// change name to instant ?

	ValidSimple( const boost::posix_time::ptime & valid )
		: Valid( valid_simple ),
		valid( valid )
	{ } 

	const boost::posix_time::ptime	valid;	// change name to a or value or time  or instant ?
}; 

struct ValidRange : Valid 
{
	ValidRange( const boost::posix_time::ptime & start, const boost::posix_time::ptime & finish )
		: Valid( valid_range ),
		start( start) ,
		finish( finish ) 
	{ } 

	const boost::posix_time::ptime	start, finish;
}; 



bool operator == ( const Valid & a, const Valid & b ) ; 

std::ostream & operator << ( std::ostream & os, const Valid & valid ) ; 






///////////////////////////////////////////////////////


// Parameters
// ???? EACH Parameter should be a separate class.
// or only specialize some, eg. 

/*
	I DON"T THINK THIS IS CORRECT. 

	THe normal Parameter wants to be of the form Param( "temp", "degc" ) for easy use with formula calculation.
	as well as conversion from satellite vectors etc etc.

	Note that grib parameter decoding can/could be hidden behind the icube plane interface.
*/

struct Param : Desc
{ 
	Param( Desc::type_t type ) 
		: Desc( type ) { } 
private:
	Param( const Param & ); 
	Param & operator = ( const Param & ); 
};  

struct ParamGenericGrib : Param 
{
	ParamGenericGrib( int code ) 
		: Param( param_generic_grib ),
		code( code ) 
	{ } 

	// used for unknown gribs or we don't particularly care 
	const int code; 
};


struct ParamTemp : Param
{
	ParamTemp() 
		: Param( param_temp ),
		unit( degc )
	{  }

	typedef enum {	degc, f }   unit_type;
	const unit_type	unit;
};

struct ParamGeopotHeight : Param
{
	// grib. table 2. code 7.  unit Gpm

	ParamGeopotHeight() 
		: Param( param_geopot_height )
	{  }
};

struct ParamRelativeHumdity : Param  
{ 
	ParamRelativeHumdity() 
		: Param( param_relative_humidity )
	{  }

	// unit is %
}; 

struct ParamWind : Param 
{
	ParamWind() 
		: Param( param_wind ),
		unit( mps )
	{  }
	// doesn't exist until we potentially combine u and v
	typedef enum {	mps, knots }   unit_type;
	const unit_type	unit;
};

/*
	we should get rid of ParamUWind and ParamVWind ...
	
	The criteria should be high level description. 
	if its grib U and V, then record as generic grib parameter 
*/
#if 0
struct ParamUWind : Param 
{
	ParamUWind() : unit( mps) { }  

	typedef enum {	mps, knots }   unit_type;
	unit_type	unit;
};

struct ParamVWind : Param 
{
	ParamVWind() : unit( mps) { }  

	typedef enum {	mps, knots }   unit_type;
	unit_type	unit;
};
#endif


/*
struct VWindParam : Desc
{
	typedef enum {	mps }   unit_type;
	unit_type	unit;
}; 
*/
/*
struct ParamBufr : Desc
{
	// we can distinguish this if we wanted
	int code;
};

struct ParmGeneric: Param 
{
	// non-common param, which we can't be bothered distinguishing
	std::string		name;	
	std::string		unit;	
};
*/




bool operator == ( const Param & a, const Param & b ) ;

std::ostream & operator << ( std::ostream & os, const Param & level ) ; 



///////////////////////////////////////////////////////
// georef 

// or just 'geo'
// or area  Area system.
// or Projection
// projection doesn't cover it, because a set of waypoints. 

struct Area : Desc
{ 
	Area( Desc::type_t type ) 
		: Desc( type ) { } 

private:
	Area( const Area & ); 
	Area & operator = ( const Area & ); 
} ;

struct AreaLLGrid : Area 
{
	AreaLLGrid( double top, double bottom, double left, double right ) 
		: Area( area_ll_grid  ),
		top( top), 
		bottom( bottom),
		left( left),
		right( right)
	{ } 

	const double top, bottom, left, right;	// maybe lamin, lamax, lonmin, lonmax 

};




bool operator == ( const Area & a, const Area & b ) ;


std::ostream & operator << ( std::ostream & os, const Area & area ) ; 

#if 0

struct AreaAll : Area 
{
	// might distinguish this case, because stuff like isolining has to be treated differently 
	// entire earth, so that it's wrapped, might be useful to distinguish
};

struct AreaStation : Area 
{
	// a single point observation
	double x, y; 
	// elevation ??. no. But, maybe station_id
	//int station_id;
};

struct AreaWaypoints : Area 
{
	// specify georeferencing for a section.
	// agg::path_storage	path; 
}; 

struct AreaPoints : Area 
{
	// only needs to be enough, for us to perform the analysis
	// just point data, before an analysis has been done
	//	std::vector< std::pair< double, double> >	points;
}; 

#endif

struct SurfaceCriteria : RefCounted		// or call it a SurfaceCriteria ???
{
	/*
		we should change the name of this.  because it is confused with a surface on the surface.

	*/

	/*
		This is enough to info to abstract, analysis, formula, grib, sat etc. 

		slice can use LevelRange

		gts headers, etc, 
	*/

	/*
		We have to be very careful with copying operations here.

		we can't make things const, because the ref counting accessors are non-const. 

		one way to protect is to force creation of a new object, through the constructor
	*/ 

	SurfaceCriteria(
		const ptr< Param> & param,
		const ptr< Level> & level,
		const ptr< Valid> & valid,
		const ptr< Area> & area
	): 	param( param),
		level( level),
		valid( valid),
		area( area )
	{ 

		assert( param && level && valid && area );
	}  

	const ptr< Param>		param;
	const ptr< Level>		level;
	const ptr< Valid>		valid;
	const ptr< Area>		area;

//	IMPLEMENT_REFCOUNT ; 

private:
	SurfaceCriteria(  const SurfaceCriteria & ) ; 
	SurfaceCriteria & operator = (  const SurfaceCriteria & ) ; 
};




bool operator == ( const SurfaceCriteria & a, const SurfaceCriteria & b ) ;



#if 0
struct GtsHeader : Desc
{
	// a result of decoding a gts header ...
	// could be useful . 
	// should it be made explicit, or a generalized tag ???? 

//	GtsHeader() { } 

	GtsHeader(
		const std::string & ttaaii,
		const std::string & cccc,
		const std::string & yygggg,
		const std::string & bbb
	) : Desc() ,
		ttaaii( ttaaii ),
		cccc( cccc),
		yygggg( yygggg),
		bbb( bbb)
	{ } 

	std::string	ttaaii;
	std::string	cccc;
	std::string	yygggg;
	std::string	bbb;
};


struct Grib1___ : Desc
{
	// not certain, if should be a type of tag 
	int centre; 
	int processing_id; 
}; 

#endif



// if it's an analysis, we probably still want access to the points (No, use a visitor, or interface at the appropriate point
// eg. there is a need to be able to edit the stations.
// ) 


/////////////////////////////////////////////////////////////


/*
	OK. Very important. 

	DATA should not be mentioned here. 

	eg. there should be not Surface

*/

#if 0
struct Grid; // forward

struct Data : Desc
{ } ;

struct DataValue : Data 
{
	// could work for a station observation, if we really wanted.
	double value;
};

struct DataGrid : Data
{
	DataGrid(  const ptr< Grid > & grid )
		: grid( grid )
	{ } 
	ptr< Grid >	grid;
};

struct DataUVGrid : Data 
{
	ptr< Grid>	u, v;
}; 
#endif

// we could inherit this from Desc as well ? to support serialization

// OK are we sure that we want to erase the fact that something is a grib. 
// or do we want to write a wrapper so that it can conform to an interface, ...




/*
	OK metadata like the grib runtime and origin should not be placed here. 

		eg. analysis or computed validity etc etc. 

	They should be typed and typed separately. and propagated or injected someother way

	
	**** Tags FORCE US TO USE and inherit from the generic Desc. 

*/
	// tags.
	// gts, other grib data, ?? 	
	// Or do we wrap the surface in a secondary structure. 
	// or apply a visitor. or optionally static in this structure.
//	std::vector< ptr< Desc> >	tags;

//IMPLEMENT_REFCOUNT ; 


/*
	Do we really want the 
*/

// should we just use a tuple ???
// std::pair< ptr< SurfaceCriteria>, ptr< Data> >     

#if 0
struct Surface  : RefCounted
{
	Surface ( const ptr< SurfaceCriteria> & criteria, const ptr< Data> & data )
		: criteria( criteria ),
		data( data)
	{ } 

	ptr< SurfaceCriteria>	criteria;
	ptr< Data>				data;
};

#endif

#if 0
	ptr< Param>		param;
	ptr< Level>		level;
	ptr< Valid>		valid;
	ptr< Area>		area;

	std::vector< ptr< Desc> >	tags;

// missing values in a grid should be explicit, then we can still compute using interpolated values, 
// but then when displaying we know the areas, and can show holes etc.
// this should be put in the georef. as point locations. 
// might making combining of areas a lot easier.
// actually also might be better to use Nan or similar in the grid. 
// no, because it's too hard to propagate through formula calculations. 
//	std::vector< double> 

	Surface() : count( 0) { } 
	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 
private:
	unsigned count;
	Surface( const Surface & ); 
	Surface & operator = ( const Surface & ); 
};
#endif

// these ought to be placed in common. 

// actually many of these operations would be performed on grid ?
// not sure.

// we might have grid versions as well. eg. the metadata is secondary.  

#if 0

Surface interpolate_grid( Surface & a, int ni, int nj ); 

Surface fast_interpolate_grid( Surface & a, int ni, int nj ); 

Surface combine_areas( Surface & a, Surface & b ); 

Surface compute_section( std::vector< Surface>  & a ); 

Surface apply_grid_edit( Surface & a  );	// can be applied singlly or used by cube

Surface interpolate_validity( Surface  & a ); 

Surface decode_a_grib( /* IReader  */ );


Surface compute_analysis( std::vector< Surface> & );


Surface compute_formula( std::vector< Surface> & );

Surface compute_point_forecast( std::vector< Surface> & );

#endif


#if 0
struct Dims
{
	// should we have an intermediate type, to derive from to restrict this stuff ???
	// probably...

	Param	param;
	Level	level;
	Valid	valid;
	Area	area;

};

struct Surface
{
	// and the actual data. 
	// Or should it be separated.
	Dims	dims;
	Desc	data;
};
#endif



#if 0

/////
//// do we wish to distinguish the surface and section and analysis types ???
////

struct Section
{
	// will have a waypoint rather than a grid ... but it's still a georef.
	// it will also have a level type... eg it's on hpa levels ...
	// but we will have to supply the set of levels for the vertical areainates
};

struct Analysis
{
	Desc	param;
	Desc	level;
	Desc	valid;
	Desc	grid;
};
#endif





