
#include <common/parser.h>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>


// template< enum junk_id, enum frame_id>  
struct tcpip_frame_parser
{
	/*
		- much cleaner frame parser, that operates as a stream and discards junk when not syncronised efficiently
		adv.
		- fast, parses 600Mb in 2 seconds
		- junk and frames are described alternatively, so should always be able to clear
		- properly described by grammar means easy to maintain
		- the junk is also described properly by grammar, rather than by manual parsing using pointers.
		- multiple frames and/or junk are matched and extracted on one call to add() and simply described using the grammar
		- junk is removed efficiently eg both erase() from buf because all junk is got in one go, and junk detecting because 
			only need 14 chars to advance
		- we know exactly how many bytes are in junk, using regular parser	
		- don't have horrible in sync/out of sync etc.
		- can be easily/cleanly used with a istream/ socket or stringstream

		For reference see 
			The use of TCP/IP on the GTS - Attachment II.15

		To avoid getting false positives on junk we try to match the start of the gts header as well	
		ok, we are getting false positives  eg. in gts_data/20090510.b.gts
		eg. 77770000000000000106 which is 77Mb, which stops all parsing.

		- it might have been better to make frame type explicit eg. AN, BI, FX, 01
	*/

private: 
	
	// grammar
	typedef range_< '0', '9'>				digit; 
	typedef range_< 'A', 'Z'>				upper; 
	typedef range_< 'a', 'z'>				lower; 
	typedef or_< digit, upper, lower>		alphadigit;             
	
	typedef repeat_< digit, 8>				frame_size; 


// ok, there is a problem in that the different	 parsers are
// picking up slightly different and junk and not junk ?things 

// if we get a junk we need to show it.

//	typedef repeat_< alphadigit, 2 >		frame_type;

	typedef or_<
		// tcpip
		chseq_< 'A', 'N' >,	
		chseq_< 'B', 'I' >,	
		chseq_< 'F', 'X' >,	
		// ftp
		chseq_< '0', '0' >,	
		chseq_< '0', '1' >	 >				frame_type;

	// note that there is no nnn in the ftp type=00, but there is with type=01

	// ok,

	typedef char_< 0x01 >					soh; 
	typedef chseq_< 0x0d, 0x0d, 0x0a >		crcrlf;

	typedef or_<   
		// tcpip
		seq_< soh, crcrlf>,
		// ftp
		crcrlf 
	>										gts_start; 

	typedef seq_ < 
		 frame_size, 
		 frame_type, 
		 gts_start
	>										frame_header; 

	typedef seq_ < 
		capture_< 1, frame_size>, 
		capture_< 2, frame_type >, 
		gts_start
		>									capture_frame_header; 

	typedef repeat_< char_< '0'>, 8  >		trailing_zeros; 

	typedef plus_< 
		seq_< 
			require_< repeat_< any_, 16 > >,
			not_< frame_header >,
			any_ >	
			>								junk;

	struct parse_frame
	{
		static const char * parse( const char *start, const char *finish, std::vector< event> &  _)
		{
			// the frame is parsed by using the length indicated in the header
			std::vector< event> events; 
			const char *ret = capture_frame_header::parse( start, finish, events ); 
			if( ! ret)	// not enough data
				return 0;
			assert( events.size() == 2);
			assert( events.at( 0).id == 1 ); 

			// avoid depend on boost lexical_cast<>
			int len = -1; 
			std::istringstream	ss( std::string( events.at( 0).start, events.at( 0).finish ));	
			ss >> len ;

			start = events.at( 1).finish + len;
			if( start > finish ) // == is ok.
				return 0; 
		
			// optional trailing 8 zeros
			ret = trailing_zeros::parse( start, finish, events ); 
			if( ret) start = ret;

			return start;
		}
	};

	typedef parse_frame						frame; 

	typedef or_< 
		capture_< 1, junk> , 
		capture_< 2, frame > 
		>									junk_or_frame; 

	typedef plus_< junk_or_frame>			junk_or_frames; 

public:

	static const char * parse( const char *start, const char *finish,  std::vector< event>	 & events ) 
	{
		return junk_or_frames::parse( start, finish, events );
	}
};



struct streaming
{
	streaming( std::istream	&is) 
		: is( is )
	{ } 

	std::istream			& is; 

	// should pass this in by reference or template
	tcpip_frame_parser		parser; 

	void run()
	{
		std::vector< char > buf; 
		int bulletin_count = 0;
		int junk_count = 0;
		int junk_bytes = 0;

		while( is.good())
		{
			// read some chars into the buffer
			const int n = 1000; 
			char b[ n ] ; 
			is.read( b, n);
			//add( buf, is.gcount()); 

			buf.insert( buf.end(), b, b + is.gcount() ); 

			if( buf.empty())
				continue;

			// do our parsing operations 
			const char *start = & buf[ 0]; 
			const char *finish = start + buf.size() ; 

			std::vector< event>	 events; 
			const char *ret = parser.parse( start, finish, events);

			for( unsigned i = 0; i < events.size(); ++i)
			{
				const event & e = events[ i];
				//std::cout << "event " << e.id << std::endl;
				assert( e.id == 1 || e.id == 2 );
				if( e.id == 1 ) 
				{
					// std::cout << "junk " << ( e.finish - e.start) << " bytes" << std::endl;
					++junk_count; 
					junk_bytes += e.finish - e.start ; 
				}
				else if ( e.id == 2 ) 
				{
					// push to another virtual interface, add( e.start, e.finish - e.start ) 
					// std::cout << "bulletin" << std::endl;
					++bulletin_count; 
				}
			}

			if( ret)
			{
				// pull parsed data from front 
				buf.erase( buf.begin(), buf.begin() + ( ret - start ));  
			}
		}

		std::cout 
			<< "bulletins " << bulletin_count 
			<< ", junk " << junk_count << " bytes " << junk_bytes 
			<< ", bytes left " << buf.size() << std::endl; 
	}
};



struct nonstreaming
{
	// process the file in one go

	nonstreaming( std::istream	&is) 
		: is( is )
	{ } 

	std::istream			& is; 

	// should pass this in by reference or template
	tcpip_frame_parser		parser; 

	void run()
	{
		// read the file into memory	
		std::vector< char>		buf; 

		is.seekg( 0, std::ios::end);
		buf.resize(  is.tellg());
		is.seekg( 0, std::ios::beg);
		is.read( & buf[ 0], buf.size());

		if( buf.empty())
			return;

		const char *start = & buf[ 0]; 
		const char *finish = start + buf.size(); 
		std::vector< event>			events; 
		const char *ret = parser.parse( start, finish, events);

		int bulletin_count = 0;
		int junk_count = 0;
		int junk_bytes = 0;
		for( unsigned i = 0; i < events.size(); ++i)
		{
			const event & e = events[ i];
			assert( e.id == 1 || e.id == 2 );
			if( e.id == 1 ) 
			{
				// std::cout << "junk " << ( e.finish - e.start) << " bytes" << std::endl;
				++junk_count; 
				junk_bytes += e.finish - e.start ; 
			}
			else if ( e.id == 2 ) 
			{
				// push to another virtual interface, add( e.start, e.finish - e.start ) 
				// std::cout << "bulletin" << std::endl;
				++bulletin_count; 
			}
		}

		std::cout 
			<< "bulletins " << bulletin_count 
			<< ", junk " << junk_count << " bytes " << junk_bytes 
			<< ", bytes left " << (finish - ret ) << std::endl; 
	}
};




// ok, this thing is still doing two things parsing and handling the buffer
// lets separate them.
// because should be able to handle as a file, or as a blob



int main( int argc, char **argv)
{
	std::vector< std::string >	args( argv, argv + argc ); 

	if( args.size() != 2)
	{
		std::cout << "Usage eg. file.gts" << std::endl; 
		return 0; 
	}	

	{
	std::string				filename = args.at( 1); 
	std::ifstream			is( filename.c_str() , std::ios::binary ); 
	assert( is.is_open());

	streaming			x( is); 
	x.run();
	}

	{
	std::string				filename = args.at( 1); 
	std::ifstream			is( filename.c_str() , std::ios::binary ); 
	assert( is.is_open());

	nonstreaming			y( is);
	y.run();
	}

}



