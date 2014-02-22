
#include <controller/fonts.h>
#include <common/load_font.h>

#include <map>
//#include <utility>

/*
	ok, we are still likely to load the entire face at a single point in time... 
*/

namespace {

typedef	std::map< int, Glyph >			glyph_map_t; 

typedef std::pair< std::string, int>	face_desc_t;  

typedef std::map< face_desc_t , glyph_map_t >	face_map_t;  



struct LoadFace : IFontLoadCallback 
{
	glyph_map_t	& glyph_map; 

	LoadFace( glyph_map_t & glyph_map)  
		: glyph_map( glyph_map )
	{ } 

	void add_glyph( int code, const agg::path_storage & path, double advance_x )  
	{
		glyph_map.insert( std::make_pair( code, Glyph( code, path, advance_x ))); 
	}  
};


struct Fonts : IFonts
{
	Fonts()
		: count( 0), 
		loaded_faces(),
		current_face_iterator( loaded_faces.end() )
	{ } 

	void add_ref() { ++count; }
	void release() { if( --count == 0) delete this; }

	const Glyph & get_glyph( int code ) 
	{
		assert( current_face_iterator != loaded_faces.end()); 

		const glyph_map_t & glyph_map = current_face_iterator->second; 
	
		// how do we indicate the failure to find the glyph ????????
		glyph_map_t::const_iterator i = glyph_map.find( code );  
		assert( i != glyph_map.end() );
	
		return i->second; 	
	}
	
	void set_face( const std::string & name, int sz ) 
	{
		face_desc_t		face_desc( name, sz );

		current_face_iterator = loaded_faces.find( face_desc );  
		if( current_face_iterator == loaded_faces.end())
		{ 	
		//	std::cout << "loading new face " << std::endl;

			glyph_map_t		glyph_map; 
			LoadFace		callback( glyph_map ); 
			load_face( name, sz,  callback ); 
			loaded_faces.insert( std::make_pair( face_desc, glyph_map )); 

			current_face_iterator = loaded_faces.find( face_desc);  
			assert( current_face_iterator != loaded_faces.end()); 
		}
	} 

	face_map_t				loaded_faces;
	face_map_t::iterator	current_face_iterator ; 

private:
	unsigned count;
};


};	// end anon namespace


ptr< IFonts> create_fonts_service()
{
	return new Fonts; 
}


