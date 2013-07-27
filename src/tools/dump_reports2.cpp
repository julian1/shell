/*
	This should replace dump_reports (and

	except it's not going 
*/

#include <common/parser.h>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>



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


		typedef	advance_until_< char_< '=' > >				body; 


		// capture the synop header, as well as the messages separately 
		// and we will reconstruct	
		typedef chseq_< 'A', 'A', 'X', 'X' >				aaxx;
		typedef repeat_< digit_or_missing, 5 >				yyggi; 
		typedef capture_< 0, seq_< aaxx, plus_white, yyggi> >	synop_header; 
		typedef capture_< 1, body >							synop; 
		typedef seq_< synop_header, plus_< seq_< plus_white, synop > > > synops; 


		// bbxx will also probably want the header attatched
		typedef chseq_< 'B', 'B', 'X', 'X' >				bbxx;
		typedef capture_< 0, bbxx >							ship_header; 
		typedef capture_< 1, body >							ship; 
		typedef seq_< ship_header, plus_< seq_< plus_white, ship > > > ships; 


		// airep and amdar
		// sigmet only have one, per bulletin
		// we should not advance to '=' we should advance to the end.
		typedef repeat_< upper, 4 >							cccc_sigmet; 	
		typedef chseq_< 'S', 'I', 'G', 'M', 'E', 'T' >		sigmet_code_form;
		// advance until, the end, otherwise a lose = in body, will mean losing the subsequent text 
		// THIS MAY BE WRONG NOW. WE REALLY HAVE TO HAVE THE =
		// we can postentially advance while not the gts_footer
		typedef	capture_< 2, seq_< cccc_sigmet, plus_white, sigmet_code_form, body> > sigmet; 


		// upper	
		typedef chseq_< 'T', 'T', 'A', 'A' >				ttaa;
		typedef chseq_< 'T', 'T', 'B', 'B' >				ttbb;
		typedef chseq_< 'T', 'T', 'C', 'C' >				ttcc;
		typedef chseq_< 'T', 'T', 'D', 'D' >				ttdd;
		typedef chseq_< 'U', 'U', 'A', 'A' >				uuaa;
		typedef chseq_< 'U', 'U', 'B', 'B' >				uubb;
		typedef or_< ttaa, ttbb, ttcc, ttdd,  uuaa, uubb  >			temp_code_form;
		typedef capture_< 2, seq_< temp_code_form, plus_white, body > > temp; 
		typedef plus_< seq_< maybe_white, temp> > temps;


		// metars and speci have sometimes have a header, and sometimes all reports have 'metar' in text
		// at the moment, we are only parsing when we assume all reports have the meter header
		// actually metars follow a pretty standard form 'metar cccc nnnnnnZ', so 
		// we should pick them up, whether however they are 
		// we don't have to have a header, because there is nothing actually important, 
		typedef chseq_< 'M', 'E', 'T', 'A', 'R' >			metar_;
		typedef chseq_< 'S', 'P', 'E', 'C', 'I' >			speci;
		typedef or_< metar_, speci >						metar_code_form;
		typedef repeat_< or_< upper, digit>,  4 >			metar_cccc; 	
		typedef capture_< 2, body >							metar; 
		//typedef seq_< metar_code_form, star_< seq_< maybe_white, metar> >, maybe_white /*, eos_*/ > metars; 
		typedef seq_< metar_code_form, star_< seq_< maybe_white, metar> > > metars; 
		// should be plus_white ??


		// TAF's have their own header, either 'TAF' or 'TAF AMD', that we will need to get 
		// they are like synop, and don't have repeating elements
		typedef chseq_< 'T', 'A', 'F' >						taf1;
		typedef chseq_< 'A', 'M', 'D' >						amd;
		typedef seq_< taf1, plus_white, amd>				taf_amd; 
		typedef capture_< 0, or_< taf_amd, taf1> >			taf_header; 
		typedef capture_< 1, body >							taf; 
	//	typedef seq_< taf_header, plus_< seq_< plus_white, taf > > > tafs_; 
	//	typedef seq_< tafs_, maybe_white/*, eos_*/ >			tafs;
		typedef seq_< taf_header, plus_< seq_< plus_white, taf > > > tafs; 
		// we need to be checking eos everywhere

	#if 0
		// for nil, it means there are no reports, this is explicit and valid 
		// and should be handled here
		typedef chseq_< 'N', 'I', 'L', '=' >				nil0;
		typedef chseq_< 'N', 'I', 'L' >						nil1;
		typedef seq_< maybe_white, or_< nil0, nil1>, maybe_white, eos_ >		nil;   

		// ok, - should we treat as failure,  
		// or should we attempt to handle ?
		//typedef  capture_< 3,  advance_until< char_< '=' > > >  unrecognized; 
		typedef  capture_< 3,  advance_until< eos_ > >		error; 
	#endif


		typedef	capture_< 6, parse_grib1>				grib;
		typedef seq_< maybe_white, plus_< grib > >		gribs;



		//typedef	or_< synops, ships, sigmet, temps, metars, tafs /*, nil, error */ > reports; 
		typedef	or_< synops, ships, sigmet, temps, metars, tafs, gribs /*, nil, error */ > reports; 

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

#if 0
struct stream_reader
{
	// here, we try to use the parser to parse frames on fly
	std::istream		& is;
	std::vector< char>	buf; 

	stream_reader( std::istream & is )
		: is( is)
	{ 
		// assert( is.is_open() ); 
	} 


	void parse_bulletin( const char *start, const char *finish )
	{


	}

	void run()
	{
		// we try and read enough into the buffer. 
		// ok, but if the parse fails - then 

		while( is.good() )
		{
			const int max_frame_sz = 14 * 1000000; 
			if( buf.size() < max_frame_sz )
			{
				// read 1000 bytes into buffer.
				int n = 1000; 
				int cur_size = buf.size(); 
				buf.resize( cur_size + n ); 
				is.read( & buf[ 0] + cur_size, n);
				size_t x = is.gcount(); 
				buf.resize( cur_size + x );  
				//if( x <= 0) break;	// hmmmmm ???? 
	
				if( buf.empty())
					break;

				assert( ! buf.empty());
				const char *start = & buf[ 0]; 
				const char *finish = start + buf.size(); 

				// std::cout << "buf.size() " << buf.size() << std::endl;

				// bulletin or error
				typedef capture_< 7, advance_if_not< eos_> > bulletin;  
				typedef parse_frame< bulletin >				frame; 
				typedef star_< frame>						grammar;	// star to allow no frames 

				std::vector< event>			events; 
				const char *ret = grammar::parse( start, finish, events );  
				assert( ret);
		
				for( unsigned i = 0; i < events.size(); ++i )
				{
					const event & e = events[ i ] ; 
					if( e.id == 7 )
					{	
						std::cout << "frame " << (e.finish - e.start) << std::endl;
					}
				}

				buf.erase( buf.begin(), buf.begin() + (ret - start)); 
			}

			// NO hang on - why not just call advance_if_not the frame ?
			// because it has to be a certain size ...

			else
			{
				assert( 0);

				// ok, this can lock up. because it's not a frame 
	
				const char *start = & buf[ 0] ;
				const char *finish = start + buf.size()  ;
				const char *ret = start; 
				while( ret != finish && *ret != '0' )				
					++ret; 

				if( ret - start == 0)
					++ret;			
	
		//		std::cout << "removing " << (ret - start) << std::endl;
	
				buf.erase( buf.begin(), buf.begin() + (ret - start) ); 

#if 0
				std::cout << "out of sync " << buf.size() << std::endl;
				typedef parse_frame< advance_if_not< eos_>  >	frame; 
				typedef advance_if_not< frame>					grammar;	// star to allow no frames 

				assert( ! buf.empty());
				const char *start = & buf[ 0]; 
				const char *finish = start + buf.size(); 
				std::vector< event>			events; 
				const char *ret = grammar::parse( start, finish, events );  
		
				// we give up, and assume we are out of sync
				//buf.erase( buf.begin(), buf.begin() + 1 ); 
				if( ret)
					buf.erase( buf.begin(), buf.begin() + (ret - start)); 
#endif
			}
		}

		std::cout << "buf.size() at end " << buf.size() << std::endl;
	}
};
#endif

int main( int argc, char **argv)
{
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

	const char *start = & buf[ 0]; 
	const char *finish = start + buf.size(); 


	std::vector< event>			events; 


	// bulletin or error
	typedef capture_< 7, advance_if_not_< eos_> > bulletin_bad;  
	typedef or_< parse_gts_bulletin, bulletin_bad> bulletin;

	// frame or frame bad
	// note that having the frame bad, means it will advance even 
	typedef parse_frame< bulletin >				frame; 
	typedef capture_< 8, advance_if_not_< frame> > frame_bad;  

	typedef star_< or_< frame, frame_bad> >	frames; 

	const char *ret = frames::parse( start, finish, events );  

	for( int i = 0; i < events.size(); ++i)
	{
		const event & e = events[ i];
		if( e.id == -1)
			std::cout << "----------------" << std::endl;
		
		// just try to clean up the text a bit.	
		int n = 60; 
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


		std::cout << "pos " << (e.start - start) << " event " << e.id << " " << text << std::endl;
	}	

	return 0;
}

// No the 000000000 are at the end.
// we can see this if we do 
// head -c 12  20090510/*.f


#if 0
static void output_bulletin( const char *start, const char *finish )
{

	std::vector< event>			events; 
	const char * ret = parse_bulletin::parse( start, finish , events);
	if( ret) 
	{
		//std::cout << "parse finished at '" << std::string( ret, std::min( ret + 30, finish )) << "'" << std::endl;
		std::cout << "parse finished at '" << std::string( ret, finish ) << "'" << std::endl;
		for( int i = 0; i < events.size(); ++i)
		{
			const event & e = events[ i];
			std::cout << "event " << e.id << " " << std::string( e.start, std::min( e.start + 30, e.finish ) ) << std::endl;
		}	
	}
	else
	{
		std::cout << "parse error" << std::endl;
	}
}
#endif


#if 0

	std::vector< std::pair< const char *, const char *> >	frames ;  
	parse_frames( start, finish, frames ); 

	std::cout << "frames.size() " << frames.size() << std::endl;

	for( unsigned i = 0; i < frames.size(); ++i )
	{
		std::cout << "-----------------------" << std::endl;
		const std::pair< const char *, const char *> & frame = frames[ i];

		output_bulletin( frame.first, frame.second ); 

	/*
		ok we have to split into frames - otherwise, the report parser, just keeps looking for '=', 
		rather than looking for end_of_message, eos.
		eg we have to use eos to indicate the end of the message, unless we use the length of the message 
	*/
	
	//foreach( event e, events)

		//parse( frame.first, frame.second );  
		// break;
	}

#endif

//	std::cout << "buf size " << buf.size() << std::endl;
//	parse( & buf[ 0], & buf[ 0] + buf.size() );  
	// synops are on tt = si, sn, sm  , maybe SA	for 10_2_2011
	// synops are on tt = sn, for out.gts 



#if 0
/*
	note that combining frame + gts + report actually tightens parsing, because we can handle combinations
	eg. frame always has a gts header.

	but if raw reports or aftn, then only have gts
*/

static const char * parse_frames( const char *start, const char *finish, std::vector< std::pair< const char *, const char *> >	& frames  )
{
	/*
		this is not very good.

		- I think our grammar, ought to be able to handle multiple bulletins. 
		- then we can very simply manipulate the result from a sql query db, or file without having
			to . 

		ok, our frame parser, ought to be able to pull off complete frames - or errors. 
	*/

	/*
		we can handle error at different points, by just capturing to the eos	
		ok, the splitting into frames, actually has to use the frame header length. 
	*/
	typedef range_< '0', '9'>				digit; 
	typedef range_< 'A', 'Z'>				upper; 
	typedef range_< 'a', 'z'>				lower; 
	typedef or_< digit, upper, lower>		alphadigit;             

	// the frame
	typedef capture_< 1, repeat_< digit, 8>	>	indicated_len; 
	typedef capture_< 2, repeat_< alphadigit, 2 > >		frame_type;  // can really be digit ? 
	typedef seq_< indicated_len, frame_type >    frame_header; 

	typedef or_< frame_header, eos_>		frame_header_or_eos;             

	const char *start0 = start; 
	while( true)
	{ 
		// std::cout << "here '" << std::string( start, std::min( start + 10, finish )) << "'" << std::endl;
		std::vector< event>			events; 
		const char * ret = frame_header_or_eos::parse( start, finish, events);
		if( ret == finish )
		{
			// parsed eos
			break;
		}
		else if( ret) 
		{
			// parsed frame
			/*
			std::cout << "'" 
				<< std::string( start, std::min( start + 15, finish )) 
				<< "..." 
				<< std::string( std::max( finish - 15, start), finish )
			<< std::endl;
			*/
			assert( events.size() == 2);
			assert( events.at( 0).id == 1); 
			assert( events.at( 1).id == 2); 
			int len = -1; 
			std::istringstream	ss( std::string( events.at( 0).start, events.at( 0).finish ));	// avoid boost dep
			ss >> len ;
			//std::cout << "len " << len << std::endl; 
			// IMPORTANT - we add the size of the frame indicator, permitting variable sized indicators
			const char *msg_finish = start + len + (ret - start);
			assert( msg_finish <= finish);	// this is possible if frame incorrectly codes length
			frames.push_back( std::make_pair( start, msg_finish) ); 
			start = msg_finish;
		}
		else
		{
			// failed to parse
			std::cout << "parse error at pos " << (start - start0) << " bytes " << std::endl;
			std::cout << "file len " << (finish - start0) << " bytes " << std::endl;
			std::cout << "'" << std::string( start, std::min( start + 15, finish )) << std::endl;
			assert( 0); 
		}
	}
}
#endif


