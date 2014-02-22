
#pragma once

#include <string>
#include <agg_path_storage.h>

/*
	it's no different to pass a vector in by ref that has a push_back(), than to use the interface
	the adv. of the interface, is that we avoid choosing the datastructure to hold the items until
	instead using the function arguments (tuple). 
	and we are not prematurely locked to a vector.

	- we also have the ability to communicate an error
*/

struct IFontLoadCallback	// change name load_face_callback 
{
	virtual void add_glyph( int code, const agg::path_storage & path, double advance_x )  = 0 ;  
};

void load_face( const std::string & name, int size, IFontLoadCallback & callback ); 


