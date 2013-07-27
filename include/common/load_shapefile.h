/*
	USING AN INTERFACE AVOIDS HAVING TO CREATE INTERMEDIATE DATA STRUCTURES.
	- with memory handling shared or copyied etc.
	- and it provides an context for the client, to create new structures.
	- and can avoid double wrapping, because we supply the builder/factory context

	Shapefile loading should definately be a function, and if not a function then a playable command.
	This is the same as loading a png.

	It should not be a service, since it occurs once, rather than a temporal editing job.
	The maintenance of the state is for the aggregate.

	Also rather than passing complicated data structures

*/

#include <agg_path_storage.h>
#include <string>
#include <boost/variant.hpp>

// note that we can also pass a logger into these operations.
// or make them classes for seams

struct IShapeLoadCallback
{
	virtual void add_shape( int id,  const agg::path_storage & path ) = 0;
};

void load_shapes( const std::string & filename_, IShapeLoadCallback & ); 

/* 
	keep the load_attributes as a different function. There is no reason to combine at this low level.
	and we may want to load different attribute sets.
*/

struct IShapeAttributeLoadCallback
{
	typedef boost::variant< int, double, std::string>	attr_t;  
	virtual void add_attr( int id, const std::string & name, const attr_t & attr ) = 0;
};


int load_shape_attributes( const std::string & filename, IShapeAttributeLoadCallback & callback ); 



