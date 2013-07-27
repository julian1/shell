
/*
	Ok, complicated file loading stuff, should be put in a command. not pushed into common 
	- not certain.
	- whatever deal with it later

	// the png stuff has an awful interface.
*/

// g++ main.cpp  libpng.a -lm -lz


#include <common/ptr.h>
#include <common/surface.h>



#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#include <stdexcept>

//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//#include "libpng.h"		// our interface
//#include "renderable.h"		
//#include "timer.h"

#define PNG_DEBUG 3
#include <png.h>


void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}



ptr< BitmapSurface>  load_png_file( const std::string &filename )
//void read_png_file( const char *filename, Renderable &buffer)
{
	//start_timer();

	png_structp		png_ptr;
	png_infop		info_ptr;
	int number_of_passes;


	char header[8];	// 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen( filename.c_str(), "rb");
	if (!fp) { 
		//	abort_("[read_png_file] File %s could not be opened for reading", filename);
		throw std::runtime_error( "could not open file");
	}	
	size_t n = fread(header, 1, 8, fp);

	if (png_sig_cmp( (png_byte *)header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", filename.c_str());

	/* initialize stuff */
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");


	// we need gamma handling of color space somewhere 

	// ?? 
	png_init_io(png_ptr, fp);


	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);


	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	// bloody hell what a messy interface 
	// does it always have an alpha or not an alpha ??
	// we want to initialize the io with a pure memory buffer
	// the thing is not alpha channedled 

	// this is not critical 
	assert( sizeof( png_bytep) == 8);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during read_image");

#if 0
	// we have the bitdepth ... ie 4 or 8
	// but how do we classify -- ugghhh 
	std::cout << "------------------------"  << "\n"; 
	std::cout << "width/height " << info_ptr->width << ", " << info_ptr->height  << "\n" ; 
	std::cout << "color_type   " << (unsigned)info_ptr->color_type << "\n"; 
	std::cout << "bit_depth    " << (unsigned)info_ptr->bit_depth << "\n"; 
	std::cout << "rowbytes     " << info_ptr->rowbytes  << std::endl;
#endif

	// what type of structure are we going to use ??
	std::vector< png_byte >		buf( info_ptr->height * info_ptr->rowbytes);	
	std::vector< png_byte*>		rows( info_ptr->height);	

	for( unsigned y = 0; y < rows.size(); ++y)
		rows[ y] = & buf[ y * info_ptr->rowbytes];

	png_read_image( png_ptr, &rows[ 0]);


    fclose(fp);


	ptr< BitmapSurface> buffer( new BitmapSurface( info_ptr->width, info_ptr->height )  ); 
//	boost::shared_ptr< Renderable>	buffer( new Renderable); 


//	buffer->resize( info_ptr->width, info_ptr->height );


	for( unsigned y = 0; y < rows.size(); ++y)
	{
		png_byte *row = rows[ y];

		unsigned x = 0;

		if( info_ptr->color_type == PNG_COLOR_TYPE_RGB 
			&& info_ptr->bit_depth == 8) {  
			for( unsigned i = 0; i < info_ptr->width * 3; i += 3)
			{
				assert( i + 2 < info_ptr->rowbytes);
				
				// potentially need to adjust bitdepth to our 0xff range
				unsigned char r = row[ i]; 
				unsigned char g = row[ i + 1]; 
				unsigned char b = row[ i + 2]; 
				unsigned char a = 0xff;

				buffer->rbase().copy_pixel( x, y, agg::rgba8( r, g, b, a)); 
				++x;
			}
		}
		else if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY
			&& info_ptr->bit_depth == 8)
		{
			for( unsigned i = 0; i < info_ptr->width ; i ++)
			{
				// potentially need to adjust bitdepth to our 0xff range
				unsigned char r = row[ i]; 
				unsigned char a = 0xff;

				// there's not enough range ?
				// r = (r / 2) + 0x7f; 

				buffer->rbase().copy_pixel( x, y, agg::rgba8( r, r, r , a)); 
				++x;
			}
		}
		else assert( 0);
	}



//	std::cout << "file '" << filename << "' load time " << elapsed_time() << "ms\n"; 


	return buffer;
}


#if 0

void write_png_file(char* filename)
{
	int x, y;

	int width, height;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep * row_pointers;



	/* create file */
	FILE *fp = fopen(filename, "wb");
	if (!fp)
		abort_("[write_png_file] File %s could not be opened for writing", filename);


	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		abort_("[write_png_file] png_create_write_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[write_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during init_io");

	png_init_io(png_ptr, fp);


	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");

	png_set_IHDR(png_ptr, info_ptr, width, height,
		     bit_depth, color_type, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing bytes");

	png_write_image(png_ptr, row_pointers);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during end of write");

	png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
	for (y=0; y<height; y++)
		free(row_pointers[y]);
	free(row_pointers);

        fclose(fp);
}

#endif

#if 0
int main( int argc, char **argv)
{
	assert( argc > 1);
	const char *filename = argv[ 1];// "../../gtktest/shapes/Blue_Marble_Geographic.png";

	Renderable	buffer;

	read_png_file( filename, buffer);
}
#endif

