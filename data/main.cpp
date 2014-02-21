/*
	- ok, we need to convert the grib to a grid. 
	- we could wrap the data behind a grid aggregate ?	

	- or else just convert to a grid ?
*/

#include <data/grib_decode.h>

#include <algorithm> 
#include <fstream> 
#include <iostream> 



#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH



static void read_file( std::vector< unsigned char> & buf, const std::string &filename ) 
{ 
	std::fstream is( filename.c_str(), std::ios::binary | std::ios::in ); 
	assert( is.is_open());
	is.seekg( 0, std::ios::end);
	buf.resize(  is.tellg());
	is.seekg( 0, std::ios::beg);
	is.read( (char *)& buf[ 0], buf.size());
} 


static bool compare (double A, double B) 
{
	double EPSILON = .001;
	double diff = fabs( A - B );
	// std::cout << "diff " << diff << std::endl;
	return (diff < EPSILON);  
}

#include "data/Wmo.h"



#if 0
BOOST_AUTO_TEST_CASE( MyTestCase1 )
{
	std::vector< unsigned char>	buf; 
	read_file( buf, "test/data/ecmf_160000.bin" );
	Grib1 grib; 
	decode( buf, grib ); 

	output_basic_info( std::cout, grib ); 

	BOOST_CHECK( grib.gds.ni == 37 );  
	BOOST_CHECK( grib.gds.nj == 37 );  
	BOOST_CHECK( grib.gds.nj * grib.gds.ni == grib.bds.values.size() );  
	BOOST_CHECK( compare(  grib.bds.values.at( 0), 0.853409 ));  
	BOOST_CHECK( compare( grib.bds.values.back() , -6.40636 )); 
}
#endif

BOOST_AUTO_TEST_CASE( MyTestCase2 )
{
	std::vector< unsigned char>	buf; 
	read_file( buf, "test/data/egrr_160000.bin" );
//	Grib1 grib; 
	ptr< Grid> grid = decode_grib_to_grid( buf ); 

#if 0
	output_basic_info( std::cout, grib ); 

	BOOST_CHECK( grib.gds.ni == 73 );  
	BOOST_CHECK( grib.gds.nj == 73 );  
//	BOOST_CHECK( grib.gds.nj * grib.gds.ni == grib.bds.values.size() );		// fails ?????
	BOOST_CHECK( compare(  grib.bds.values.at( 0), 241.667 ));  
	BOOST_CHECK( compare( grib.bds.values.back() , 211.042  )); 
#endif
}

#if 0
BOOST_AUTO_TEST_CASE( MyTestCase3 )
{
	std::vector< unsigned char>	buf; 
	read_file( buf, "test/data/lfpw_160000.bin" );
	Grib1 grib; 
	decode( buf, grib ); 

	output_basic_info( std::cout, grib ); 


	BOOST_CHECK( grib.gds.ni == 126 );  
	BOOST_CHECK( grib.gds.nj == 66 );  
	BOOST_CHECK( grib.gds.nj * grib.gds.ni == grib.bds.values.size() );		// fails ?????
//	BOOST_CHECK( compare(  grib.bds.values.at( 0), 241.667 ));  
//	BOOST_CHECK( compare( grib.bds.values.back() , 211.042  )); 

}
#endif







#if 0
	buf_type  gribs = split_gribs(  buf);

//	buf_type gribs = split_gribs_by_scan( buf); 

	std::cout << "gribs " << gribs.size() << std::endl; 

/*
	for( buf_type::iterator ip = gribs.begin();
		ip != gribs.end(); ++ip)
	{
			
		vector< unsigned char>	&grib = *ip;		
		for( int i = 0; i < 4; ++i)
			cout << grib[ i];		
		for( int i = grib.size() - 4; i < grib.size(); ++i)
			cout << grib[ i];
	}
	*/


//	exit( 0);

//	return 0;

	for( buf_type::iterator ip = gribs.begin();
		ip != gribs.end(); ++ip)
	{
		std::vector< unsigned char> &buf = *ip;


		std::cout << "-------------------" << std::endl; 
		for( int i = 0; i < 4; ++i)
			std::cout << buf[ i];

		std::cout << "...";

		for( int  i = buf.size() - 4; i < buf.size() && i >= 0; ++i)
			std::cout << buf[ i];
	
		std::cout << std::endl;


//		shared_ptr< Grib1> grib = decode_headers( buf);
	  
	 	decode( buf);
	  

		std::cout << std::endl; 		
	}

 
#endif


