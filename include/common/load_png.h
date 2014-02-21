#ifndef MY_LOAD_PNG_H
#define MY_LOAD_PNG_H

#include <common/ptr.h> 
#include <string> 

struct Bitmap;

/*
	we can use an interface like this to decouple, if we want.
*/
struct ILoadPngAdaptor
{
	virtual void error( const std::string & msg ) = 0;				// we could also pass logger in 
	virtual void add_png( const ptr< Bitmap> & surface ) = 0; 
};



/*
	and an interface like this, to compose the functionality if we really wanted 
*/
struct ILoadPng
{

	virtual void load( const std::string &filename, ILoadPngAdaptor & ) = 0 ;

}; 


ptr< Bitmap> load_png_file( const std::string &filename /* ILoadPng & */ ) ;

#endif

