
#pragma once

#include <common/ptr.h>
#include <agg_path_storage.h>

//struct F0

// so we can have a get_glyph() change_face( font, size )  interface ...
// or else return the entire font set for a size.  eg. std::map< int, Glyph >  

// propagating the get_glyph() interface is better, because it may be easier
// with freetype, doing exactly the same thing, meaning we don't have to load
// the entire thing.  

// will the services be & references, or will they be ptr<>  ???

struct Glyph
{
	Glyph( int code, const agg::path_storage &path, double advance_x ) 
		: code( code),
		path( path ),
		advance_x( advance_x )
	{ } 

	int					code; 
	agg::path_storage	path;
	double				advance_x; 
};


struct IFonts
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual const Glyph & get_glyph( int code ) = 0;
	virtual void set_face( const std::string & name, int sz ) = 0; 
};

ptr< IFonts>	create_fonts_service();		// the dependency will be the font paths. 





