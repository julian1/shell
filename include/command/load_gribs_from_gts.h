
#pragma once

// decoding a grib might be a common operation or decoding job. but loading from a gts dump file as a test, has to be 
// a command that can be run. it's a verb rather than static functionality that sits around to be run. 

// we could give it access to the active aggregate ????
// or inject the aggregate ??

// as a command it should really be run against an inbox ?

#include <common/gts_header.h>
#include <data/grib_decode.h>


struct IDecodeGribsFromGtsCallback 
{
	virtual void add( 
		const ptr< Grib1_sections>	& grib_sections, 
		const ptr< Grid >			& grid, 
		const ptr< SurfaceCriteria>	& criteria, 
		const ptr< GtsHeader >		& gts_header 
	) = 0; 
};

void decode_gribs_from_gts(  const std::vector< unsigned char> & buf1, IDecodeGribsFromGtsCallback &  );  


#if 0
struct MyGrib
{
	ptr< Grib1_sections>	grib_sections; 
	ptr< Grid >				grid; 
	ptr< SurfaceCriteria>	criteria; 
	ptr< GtsHeader >		gts_header; 
};
#endif



#if 0
struct LoadGribsFromGts // : ICommand
{
	std::string filename ;


	LoadGribsFromGts( const std::string & filename )
		: filename( filename )
	{ }  
};
#endif


/*
struct LoadGribsFromGts
{
	struct ICallback
	{
		virtual void push_back( const MyGrib & x ) = 0; 
	};

	void load( const std::vector< unsigned char> & buf, ICallback & ); 
};
*/



