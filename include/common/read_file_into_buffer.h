
#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <cassert>

static void read_file_into_buffer( const std::string &filename, std::vector< unsigned char> & buf ) 
{ 
	std::fstream is( filename.c_str(), std::ios::binary | std::ios::in ); 
	assert( is.is_open());
	is.seekg( 0, std::ios::end);
	buf.resize(  is.tellg());
	is.seekg( 0, std::ios::beg);
	is.read( (char *)& buf[ 0], buf.size());
}


