
#pragma once

#include <string>

struct GtsHeader // : Desc
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
	) : count( 0) ,
		ttaaii( ttaaii ),
		cccc( cccc),
		yygggg( yygggg),
		bbb( bbb)
	{ } 

	const std::string	ttaaii;
	const std::string	cccc;
	const std::string	yygggg;
	const std::string	bbb;

	void add_ref() { ++count; }  
	void release() { if( --count == 0) delete this; }  

private:
	GtsHeader( const GtsHeader & ) ; 
	GtsHeader & operator = ( const GtsHeader & ) ; 
	unsigned count; 
};


