
#include <command/load_gribs_from_gts.h>


#include <common/parser.h>
#include <data/grib_decode.h>



namespace { 

/*
	remember we should be delaying the processing until after the parse.  
	because the parse could be constructed, to require multiple gribs etc.

	also we don't have a context in which to do it.
*/

struct parse_grib1
{
	// pick out a valid looking grib

	// we don't need to pass in a template< enum> id, since we do the capture externally
	static int dec3( unsigned char * p)
	{ 
		return p[2] + 256 * ( p[1] + 256 * p[0] ); 
	}

	static const char * parse( const char *start, const char *finish, std::vector< event> &events )
	{
		// alternatives,
		// 1) glob until '7777'					(will not work because 7777 could appear in body as well as end)
		// 2) glob until end_of_gts_bulletin    (will not work because may have more than one grib in bulletin)
		// 3) rely on the encoded grib size instead (use this approach). 

		typedef chseq_< 'G', 'R', 'I', 'B'>		grib_header;
		start = grib_header::parse( start, finish, events ); 
		if( !start ) return 0;

		// may not be correct for grib2 - need to check edition as well.
		// we need at least 3 chars, to pick out grib len, note, finish is one past end.
		if( !( finish - start > 5))
			return 0;
		int len = dec3( (unsigned char *)start ); 
		start += len - 8; 

		// we have to do this, because many parsing constructs only check 
		// parse == finish for termination
		if( start >= finish )
			return 0;

		typedef chseq_< '7', '7', '7', '7'>		grib_footer;
		start = grib_footer::parse( start, finish, events ); 
		return start; 	
	}
};



struct parse_gts_bulletin
{
	// we might want to differentiate this for reports AN and binary BI rather than combining gribs here.
	
	// try to parse a gts bulletin, does not handle frame
	// we express it as a struct, so that it can be used in other contexts, although this
	// means that capture id's are hard.

	static const char * parse( const char *start, const char *finish, std::vector< event> &events )
	{
		/*
			we can handle error at different points, by just capturing to the eos	
			VERY IMPORTANT - WE SHOULD INCLUDE THE frame_type
			and only do reports if text.
		*/
		typedef char_< 0x01 >					soh; 
		typedef chseq_< 0x0d, 0x0d, 0x0a >		crcrlf;
		typedef char_< 0x03 >					etx; 
		typedef range_< '0', '9'>				digit; 
		typedef range_< 'A', 'Z'>				upper; 
		typedef range_< 'a', 'z'>				lower; 
		typedef or_< digit, upper, lower>		alphadigit;             
		typedef char_< ' '>						sp;	
		typedef or_< upper, lower>				alpha;		

		// for reports
		typedef chset_< ' ',   '\r', '\n' >		white; 
		typedef star_< white>					maybe_white; 								
		typedef plus_< white>					plus_white; 								
		typedef or_< digit, char_< '/'> >		digit_or_missing;	

		#if 0
			// the frame
			typedef capture_< 1, repeat_< digit, 8>	>	indicated_len; 
			typedef capture_< 2, repeat_< alphadigit, 2 > >		frame_type;  // can really be digit ? 

			// grabbing leading zeros, we have to relax the indicated_len to not be 8
			// typedef seq_< star_< char_< '0'> >,  capture_< 1, indicated_len>, capture_< 2, frame_type>, soh, crcrlf >       header; 
			typedef seq_< indicated_len,  frame_type /*, soh, crcrlf */ >    frame_header; 
		#endif


		// gts header	

		// i don't think we really want to make the csn optional ?
		typedef seq_< capture_< 4, repeatnn_< digit, 2, 6> > , crcrlf >	csn_present; 
		typedef capture_< 4, none_ >					csn_missing;
		typedef or_< csn_present, csn_missing  > 		csn;			// seems to use leading 0, eg 003, should be repeatn 0, 6
		//typedef capture_< 0, repeatnn_< digit, 2, 6> > 		csn;			// seems to use leading 0, eg 003, should be repeatn 0, 6


		typedef capture_< 1, seq_< repeat_< alpha, 4>, repeat_< digit, 2> > > 	ttaaii;	
		typedef capture_< 2, repeat_< alpha, 4> > 		cccc;
		typedef capture_< 3, repeat_< digit, 6> > 		yygggg;

		// we always want to capture this, which is a problem  	
		// therefore we want the plus inside the capture
		// alternatively we could have two captures inside an or - using the same id.
		typedef seq_< sp, capture_< 4, repeat_< alpha, 3> > >	bbb_present;
		typedef capture_< 4, none_ >					bbb_missing;
		typedef or_< bbb_present, bbb_missing  >		bbb;
		
		typedef seq_<	
			soh, crcrlf , 
			csn, 
			ttaaii, sp, cccc, sp, yygggg, bbb, 
			crcrlf  
		> gts_header;  


		typedef	seq_< crcrlf, etx, eos_ >				   gts_footer;	


		// this is not actually correct for binary, siince the message end sequence could be in binary message
		//typedef capture_< 5, advance_if_not< seq_ < crcrlf, etx > > > message; 

		// VERY IMPORTANT WE CAN ACTUALLY CAPTURE GRIBS EXPLICTLY advance to eos .
		// but if multiple gribs in a single message then it's no good.
		typedef	capture_< 6, parse_grib1>				grib;
		typedef seq_< maybe_white, plus_< grib > >		gribs;


		typedef	gribs  reports; 

		// unknown should pick up message errors as well.
		typedef capture_< 5,  advance_if_not_< gts_footer  > >		unknown_message;	

		typedef or_< reports, unknown_message >				message; 

		//typedef seq_< frame_header, gts_header, message, gts_footer >	bulletin_grammar;
		typedef seq_<  gts_header, message, gts_footer >	bulletin_grammar;


		return bulletin_grammar::parse( start, finish, events ); 
	}
};



template< class A1 /* int len_id, int type_id */ > 
struct parse_frame
{
	/*
		parses frames - this is like grib_parser and uses the indicated len to perform the parse 
		it then calls the argument based on the size, eg segmenting.

		So, 
			(1) we can use this to parse_frames, by passing in the gts bulletin parser, 
			(2) or just capture the frames and use separately to parse the message once split, 
			(3) or as part of a buffer handling strategy, to process on the fly.

		important,
			(1) we must pass in an argument for one of the above 
			(2) we don't have to do anything complicated, such as handling multiple frames, or errors 
				instead we only pick out valid frames (can then use advance_if_not externally etc ).
				handle frame dropping etc in another function.
		note,
			- we could actually pass the complete frame to the subparser if we wanted, to
			perform capture of frame len, and type. might be useful.

		the key point of this, as that to correctly parse a bulletin without error, we don't want
			to rely on correctly coded gts, when incorrect could extend past the frame size 
			so we use the frame len, to parse a created eos to the argument parser. 
	*/

	static const char * parse( const char *start, const char *finish, std::vector< event> &events )
	{
		typedef range_< '0', '9'>				digit; 
		typedef range_< 'A', 'Z'>				upper; 
		typedef range_< 'a', 'z'>				lower; 
		typedef or_< digit, upper, lower>		alphadigit;             

		// i don't know if we really want to capture these or let the subframe parser handle

		typedef capture_< -1, repeat_< digit, 8> >	indicated_len; 
		typedef capture_< -2, repeat_< alphadigit, 2 > > frame_type;

		const char *indicated_len_start = start; 
		const char *indicated_len_finish = indicated_len::parse( start, finish, events ); 
		start = indicated_len_finish; 
		if( !start ) return 0;

		start = frame_type::parse( start, finish, events ); 
		if( !start ) return 0;

		int len = -1; 
		std::istringstream	ss( std::string( indicated_len_start, indicated_len_finish) );	// avoid boost depend.
		ss >> len ;

		const char *msg_finish = start + len; 
		if( msg_finish > finish ) // == is ok.
			return 0; 

		const char *ret = A1::parse( start, msg_finish, events); 
		assert( ret);	// eg the argument should not fail (should capture empty)
		if( ret) 
		{	
			assert( ret == msg_finish ); 
			start = ret;
		}	
		else return 0;

		// optional 8 zeros
		typedef capture_< -3, repeat_< char_< '0'>, 8 > >	zeros; 
		ret = zeros::parse( start, finish, events ); 
		if( ret) start = ret;

		return start; 	
	}
};


static std::string format_event_text( const event & e, int n = 60 )
{
	//int n = 60; 

	std::string text = 
		( e.finish - e.start > n ) 
		? std::string( e.start, e.start + n / 2 ) + " ... " + std::string( e.finish - n / 2, e.finish )
		: std::string( e.start, e.finish );  

	for( unsigned i = 0; i < text.size(); ++i)
	{
		if( std::isspace( text[ i]) ) 
			text[ i] = ' '; 
		else if( !isprint( text[ i]))
			text[ i] = '*';
	}

	// std::cout << "pos " << (e.start - start) << " event " << e.id << " " << text << std::endl;
	return text; 
}


} ; // anon namespace 




void decode_gribs_from_gts(  const std::vector< unsigned char> & buf, IDecodeGribsFromGtsCallback & callback )
//int main1( int argc, char **argv)
{
#if 0
	std::vector< std::string >	args( argv, argv + argc ); 

	if( args.size() != 2)
	{
		//std::cout << "Usage eg. file.gts '////// //// ////// //////' " << std::endl; 
		std::cout << "Usage eg. file.gts" << std::endl; 
		return 0; 
	}	

	std::string				filename = args.at( 1); 
	std::ifstream			is( filename.c_str() , std::ios::binary ); 
	assert( is.is_open());

#if 0
	stream_reader	reader( is ); 
	reader.run();
	return 0;
#endif
	
	// read the file into memory	
	std::vector< char>		buf; 

	is.seekg( 0, std::ios::end);
	buf.resize(  is.tellg());
	is.seekg( 0, std::ios::beg);
	is.read( & buf[ 0], buf.size());
	is.close();

#endif
	const char *start = (const char * ) & buf[ 0]; 
	const char *finish = (const char * ) start + buf.size(); 


	std::vector< event>			events; 

	// ok, they are all ending up as bad bulletins ????
	// which is weird because there is not eve

	// bulletin or error
	typedef capture_< 7, advance_if_not_< eos_> > bulletin_bad;  
	typedef or_< parse_gts_bulletin, bulletin_bad> bulletin;

	// frame or frame bad
	// note that having the frame bad, means it will advance even 
	typedef parse_frame< bulletin >				frame; 
	typedef capture_< 8, advance_if_not_< frame> > frame_bad;  

	typedef star_< or_< frame, frame_bad> >	frames; 

	const char *ret = frames::parse( start, finish, events );  

	//std::vector< ptr< Surface> >	surfaces;
	//ptr< GtsHeader> gts_header = new GtsHeader ; 

	std::string ttaaii; 
	std::string cccc; 
	std::string yygggg; 
	std::string bbb; 

	//ptr< Surface> surface ;

	for( int i = 0; i < events.size(); ++i)
	{
		const event & e = events[ i];

		switch( e.id )
		{
			case 1: ttaaii =  std::string( e.start, e.finish ); break;
			case 2: cccc =  std::string( e.start, e.finish ); break;
			case 3: yygggg =  std::string( e.start, e.finish ); break;
			case 4: bbb =  std::string( e.start, e.finish ); break;
			case 6: 
			{
/*
				std::cout << "------------------------------" << std::endl;
				std::cout << ttaaii << " " << cccc << " " << yygggg << " " << bbb << std::endl;
				std::cout <<  format_event_text(  e, 60 ) << std::endl;
*/

				ptr< GtsHeader> header = new GtsHeader( ttaaii, cccc, yygggg, bbb ); 
				
				struct DecodeGribCallback : IDecodeGribCallback 
				{	
					DecodeGribCallback( 
						IDecodeGribsFromGtsCallback & callback, 
						const ptr< GtsHeader> & header )
						: callback( callback),
						header( header)
					{ } 

					void push_back( 
						const ptr< Grib1_sections> & grib_sections, 
						const ptr< Grid > & grid, 
						const ptr< SurfaceCriteria> & criteria ) 
					{
						/*
							note that we could append the gts header into the criteria tags ...	
						*/	
						callback.add( grib_sections, grid, criteria, header ); 
					}     

					IDecodeGribsFromGtsCallback & callback; 
					ptr< GtsHeader> header;
				}; 

				DecodeGribCallback 	callback__( callback, header);
				
				decode_grib_to_surface( std::vector< unsigned char>( e.start, e.finish), callback__  ); 

				break;
			}
			case 8: 
			{
				std::cout << "bad frame" << std::endl;
				break;
			}

		};
	}	
	
	//std::cout << "surfaces.size() " << surfaces.size() << std::endl;;

	return ;
}



