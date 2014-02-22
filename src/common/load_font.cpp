/*
	Loading a font is basically an atomic operation, let's try to keep it one.

	On windows we may want, non-freetype control, and ability to inject.

	if we want persistance, then we can wrap in a class, or put in service etc.   	

	
*/

/*
	we have never flattened out the beziers ? 
*/

// freetype include path must be explicit
// even though the ft2build.h is in /usr/include not freetype2
// g++ -lfreetype -I/usr/include/freetype2/ text.cpp
// g++ -lfreetype -I/usr/include/freetype2/ -Iagg/include/ text.cpp

//
// THESE SHOULD BE REMOVED AND HANDLE EXCPETIONS


#include <common/load_font.h>

//#include <freetype/font_service.h>
//#include <controller/ilogger.h>


#include <iostream>
#include <cassert>

#include <map>


#include <ft2build.h>
#include FT_FREETYPE_H

/// ok so what do we want here ... here want to call this once 
/// and return a font path 

#include <agg_path_storage.h>
#include <agg_conv_curve.h>




static inline double int26p6_to_dbl(int p)
{
	return double(p) / 64.0;
}

/*
static inline int dbl_to_plain_fx(double d)
{
	return int(d * 65536.0);
}
*/ 

/*
static inline int dbl_to_int26p6(double p)
{
	return int(p * 64.0 + 0.5);
}
*/

//------------------------------------------------------------------------


static bool decompose_ft_outline(
	const FT_Outline& outline,
	bool flip_y,
	agg::path_storage &path)
{   

	FT_Vector   v_last;
	FT_Vector   v_control;
	FT_Vector   v_start;
	double x1, y1, x2, y2, x3, y3;

	FT_Vector*  point;
	FT_Vector*  limit;
	char*       tags;

	int   n;         // index of contour in outline
	int   first;     // index of first point in contour
	char  tag;       // current point's state

	first = 0;

	for(n = 0; n < outline.n_contours; n++)
	{
		int  last;  // index of last point in contour

		last  = outline.contours[n];
		limit = outline.points + last;

		v_start = outline.points[first];
		v_last  = outline.points[last];

		v_control = v_start;

		point = outline.points + first;
		tags  = outline.tags  + first;
		tag   = FT_CURVE_TAG(tags[0]);

		// A contour cannot start with a cubic control point!
		if(tag == FT_CURVE_TAG_CUBIC) return false;

		// check first point to determine origin
		if( tag == FT_CURVE_TAG_CONIC)
		{

			// first point is conic control.  Yes, this happens.
			if(FT_CURVE_TAG(outline.tags[last]) == FT_CURVE_TAG_ON)
			{
				// start at last point if it is on the curve
				v_start = v_last;
				limit--;
			}
			else
			{
				// if both first and last points are conic,
				// start at their middle and record its position
				// for closure
				v_start.x = (v_start.x + v_last.x) / 2;
				v_start.y = (v_start.y + v_last.y) / 2;

				v_last = v_start;
			}
			point--;
			tags--;
		}

		x1 = int26p6_to_dbl(v_start.x);
		y1 = int26p6_to_dbl(v_start.y);
		if(flip_y) y1 = -y1;
//		mtx.transform(&x1, &y1);
//		path.move_to(value_type(dbl_to_int26p6(x1)), 
//					 value_type(dbl_to_int26p6(y1)));
		path.move_to( x1, y1); 

		while(point < limit)
		{
			point++;
			tags++;

			tag = FT_CURVE_TAG(tags[0]);
			switch(tag)
			{
				case FT_CURVE_TAG_ON:  // emit a single line_to
				{
					x1 = int26p6_to_dbl(point->x);
					y1 = int26p6_to_dbl(point->y);
					if(flip_y) y1 = -y1;
//					mtx.transform(&x1, &y1);
//					path.line_to(value_type(dbl_to_int26p6(x1)), 
//								 value_type(dbl_to_int26p6(y1)));
					path.line_to( x1, y1);
					//path.line_to(conv(point->x), flip_y ? -conv(point->y) : conv(point->y));
					continue;
				}

				case FT_CURVE_TAG_CONIC:  // consume conic arcs
				{
					v_control.x = point->x;
					v_control.y = point->y;

				Do_Conic:
					if(point < limit)
					{
						FT_Vector vec;
						FT_Vector v_middle;

						point++;
						tags++;
						tag = FT_CURVE_TAG(tags[0]);

						vec.x = point->x;
						vec.y = point->y;

						if(tag == FT_CURVE_TAG_ON)
						{
							x1 = int26p6_to_dbl(v_control.x);
							y1 = int26p6_to_dbl(v_control.y);
							x2 = int26p6_to_dbl(vec.x);
							y2 = int26p6_to_dbl(vec.y);
							if(flip_y) { y1 = -y1; y2 = -y2; }
//							mtx.transform(&x1, &y1);
//							mtx.transform(&x2, &y2);
/*							path.curve3(value_type(dbl_to_int26p6(x1)), 
										value_type(dbl_to_int26p6(y1)),
										value_type(dbl_to_int26p6(x2)), 
										value_type(dbl_to_int26p6(y2)));
*/
							path.curve3( x1, y1, x2, y2);

							continue;
						}

						if(tag != FT_CURVE_TAG_CONIC) return false;

						v_middle.x = (v_control.x + vec.x) / 2;
						v_middle.y = (v_control.y + vec.y) / 2;

						x1 = int26p6_to_dbl(v_control.x);
						y1 = int26p6_to_dbl(v_control.y);
						x2 = int26p6_to_dbl(v_middle.x);
						y2 = int26p6_to_dbl(v_middle.y);
						if(flip_y) { y1 = -y1; y2 = -y2; }
//						mtx.transform(&x1, &y1);
//						mtx.transform(&x2, &y2);
/*						path.curve3(value_type(dbl_to_int26p6(x1)), 
									value_type(dbl_to_int26p6(y1)),
									value_type(dbl_to_int26p6(x2)), 
									value_type(dbl_to_int26p6(y2)));
*/
						path.curve3( x1, y1, x2, y2);

						//path.curve3(conv(v_control.x), 
						//            flip_y ? -conv(v_control.y) : conv(v_control.y), 
						//            conv(v_middle.x), 
						//            flip_y ? -conv(v_middle.y) : conv(v_middle.y));

						v_control = vec;
						goto Do_Conic;
					}

					x1 = int26p6_to_dbl(v_control.x);
					y1 = int26p6_to_dbl(v_control.y);
					x2 = int26p6_to_dbl(v_start.x);
					y2 = int26p6_to_dbl(v_start.y);
					if(flip_y) { y1 = -y1; y2 = -y2; }
//					mtx.transform(&x1, &y1);
//					mtx.transform(&x2, &y2);
/*					path.curve3(value_type(dbl_to_int26p6(x1)), 
								value_type(dbl_to_int26p6(y1)),
								value_type(dbl_to_int26p6(x2)), 
								value_type(dbl_to_int26p6(y2)));
*/
					path.curve3( x1, y1, x2, y2);

					//path.curve3(conv(v_control.x), 
					//            flip_y ? -conv(v_control.y) : conv(v_control.y), 
					//            conv(v_start.x), 
					//            flip_y ? -conv(v_start.y) : conv(v_start.y));
					goto Close;
				}

				default:  // FT_CURVE_TAG_CUBIC
				{


					FT_Vector vec1, vec2;

					if(point + 1 > limit || FT_CURVE_TAG(tags[1]) != FT_CURVE_TAG_CUBIC)
					{
						return false;
					}

					vec1.x = point[0].x; 
					vec1.y = point[0].y;
					vec2.x = point[1].x; 
					vec2.y = point[1].y;

					point += 2;
					tags  += 2;

					if(point <= limit)
					{
						FT_Vector vec;

						vec.x = point->x;
						vec.y = point->y;

						x1 = int26p6_to_dbl(vec1.x);
						y1 = int26p6_to_dbl(vec1.y);
						x2 = int26p6_to_dbl(vec2.x);
						y2 = int26p6_to_dbl(vec2.y);
						x3 = int26p6_to_dbl(vec.x);
						y3 = int26p6_to_dbl(vec.y);
						if(flip_y) { y1 = -y1; y2 = -y2; y3 = -y3; }
//						mtx.transform(&x1, &y1);
//						mtx.transform(&x2, &y2);
//						mtx.transform(&x3, &y3);
/*						path.curve4(value_type(dbl_to_int26p6(x1)), 
									value_type(dbl_to_int26p6(y1)),
									value_type(dbl_to_int26p6(x2)), 
									value_type(dbl_to_int26p6(y2)),
									value_type(dbl_to_int26p6(x3)), 
									value_type(dbl_to_int26p6(y3)));
*/
						path.curve4( x1, y1, x2, y2, x3, y3);
						//path.curve4(conv(vec1.x), 
						//            flip_y ? -conv(vec1.y) : conv(vec1.y), 
						//            conv(vec2.x), 
						//            flip_y ? -conv(vec2.y) : conv(vec2.y),
						//            conv(vec.x), 
						//            flip_y ? -conv(vec.y) : conv(vec.y));
						continue;
					}

					x1 = int26p6_to_dbl(vec1.x);
					y1 = int26p6_to_dbl(vec1.y);
					x2 = int26p6_to_dbl(vec2.x);
					y2 = int26p6_to_dbl(vec2.y);
					x3 = int26p6_to_dbl(v_start.x);
					y3 = int26p6_to_dbl(v_start.y);
					if(flip_y) { y1 = -y1; y2 = -y2; y3 = -y3; }
//					mtx.transform(&x1, &y1);
//					mtx.transform(&x2, &y2);
//					mtx.transform(&x3, &y3);
/*					path.curve4(value_type(dbl_to_int26p6(x1)), 
								value_type(dbl_to_int26p6(y1)),
								value_type(dbl_to_int26p6(x2)), 
								value_type(dbl_to_int26p6(y2)),
								value_type(dbl_to_int26p6(x3)), 
								value_type(dbl_to_int26p6(y3)));
*/
					path.curve4( x1, y1, x2, y2, x3, y3);

					//path.curve4(conv(vec1.x), 
					//            flip_y ? -conv(vec1.y) : conv(vec1.y), 
					//            conv(vec2.x), 
					//            flip_y ? -conv(vec2.y) : conv(vec2.y),
					//            conv(v_start.x), 
					//            flip_y ? -conv(v_start.y) : conv(v_start.y));
					goto Close;
				}
			}
		}

		path.close_polygon();

   Close:
		first = last + 1; 
	}

	return true;
}


/*
	Best reference is here.

	http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html
*/

/*
	Ok, there is a problem in knowing the code ... unicode or whatever ...
	
	There must be a means to get all possible glyphs, by getting a list of whats 
	
	available.

	Rather than iterating them all. 	

	OR WE JUST Get the glyph index and reverse map the code .  <<-- very important. 

	yes the font includes a mapping, and most fonts include unicode.	
	FT_ENCODING_UNICODE
*/



	 

static void load_glyph( FT_Face	&  face, unsigned code, IFontLoadCallback & callback )
{
	// std::cout << "load glyph " << code << std::endl;
	// ok, it's possible that the paths may repeat for the same code ...

	//	unsigned code = '@';
	// note - does not return an error code
	unsigned glyph_index = FT_Get_Char_Index( face, code);
	//std::cout << "m_glyph_index " << glyph_index << std::endl; 
	if( glyph_index == 0 )
	{
		// not found
		return; 
	}


	//std::cout << "m_glyph_index " << glyph_index << std::endl; 

	FT_Error error;

	/*
		- Believe that Light-hinting is vertical only - in any case it works, 
		and we can snap glyphs vertically when rendering. 
		- It probably also forces auto-hinting
	*/

	/*
		It seems that freetype is completely decomposing the glyphs, so 
		that there are no beziers. 
	*/
	// FT_LOAD_DEFAULT 
	//	error = FT_Load_Glyph( self.face, glyph_index, FT_LOAD_NO_HINTING); 
	error = FT_Load_Glyph( face, glyph_index,  FT_LOAD_TARGET_LIGHT ); 
	if ( error ) 
	{ 
		std::cout <<  "failed to load glyph" << std::endl;
		assert( 0);
	}

	// do we need to close the glyph ???
	// can't see a close here http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html

	agg::path_storage	path; 

	decompose_ft_outline( face->glyph->outline, true, path);

	// i don't think we 
	typedef agg::conv_curve< agg::path_storage >	curve_type; 
	curve_type	 curve_path( path );

	agg::path_storage	path2; 

	// transform it
	path2.concat_path( curve_path );


	int advance_x = int26p6_to_dbl( face->glyph->advance.x);  


	callback.add_glyph( code, path2, advance_x )  ;  

}



void load_face( const std::string & name, int size, IFontLoadCallback & callback )
{
//	std::string name = "./data/Times_New_Roman.ttf";
//	int size = 10;
	// we could use the same callback mechanism ...

	FT_Library		library; 

	FT_Error error = FT_Init_FreeType( & library ); 

	if ( error ) 
	{ 
		std::cout <<  "failed to initialize font service" << std::endl;
		assert( 0);
	}

	FT_Face			face;

	//FT_Error error = FT_New_Face( library, "LucidaBrightRegular.ttf", 0, &face ); 
	error = FT_New_Face( library, name.c_str(), 0, & face ); 

	if ( error == FT_Err_Unknown_File_Format ) 
	{ 
		std::cout <<  "font format appears unsupported" << std::endl;
		assert( 0);
	} 
	else if ( error) 
	{ 
		std::cout <<  "failed to open font file or read" << std::endl;
		assert( 0);
	}	

	/*
		it is a bit weird that the face is opened - but not the sizes ??
	*/
	// presumably dpi needs to be set to extract hinted outlines 
	// we have to call this before we can access a glyph
	error = FT_Set_Char_Size( 
		face,			/* handle to face object */  
		0,				/* char_width in 1/64th of points */  
		64 * size,		/* char_height in 1/64th of points */  // ie font size
		96,				/* horizontal device resolution */  
		96 );			/* vertical device resolution */ 

	if ( error) 
	{ 
		std::cout <<  "failed to set font face char size" << std::endl;
		assert( 0);
	}	


	// now we want to do something ...

	// really don't understant this
	for( int i = 0; i < 256; ++i )
	//for( int i = 0; i < 1024; ++i )
	{
		load_glyph( face, i, callback  ); 
	}


	error = FT_Done_Face( face);
	if ( error) 
	{ 
		std::cout <<  "failed to done face" << std::endl;
		assert( 0);
	}	



	error = FT_Done_FreeType( library);
	if ( error ) 
	{ 
		std::cout << "error closing freetype" << std::endl;  
		assert( 0);
	}

}



/*
    agg::rect_d bnd  = path.bounding_rect();
	int m_data_size = path.byte_size();
	int m_bounds_x1 = int(floor(bnd.x1));
	int m_bounds_y1 = int(floor(bnd.y1));
	int m_bounds_x2 = int(ceil(bnd.x2));
	int m_bounds_y2 = int(ceil(bnd.y2));
	int m_advance_x = int26p6_to_dbl( face->glyph->advance.x);
	int m_advance_y = int26p6_to_dbl( face->glyph->advance.y);
*/
	//return m_cur_face->ascender * height() / m_cur_face->height;


// FT_Done_Face( m_faces[i]);
// FT_Done_FreeType( m_library);

/*
	std::cout << "is scalable " << FT_IS_SCALABLE( face) << std::endl;
	std::cout << "has kerning " << FT_HAS_KERNING( face) << std::endl;
*/

