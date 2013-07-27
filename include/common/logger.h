
#ifndef MY_LOGGER_H
#define MY_LOGGER_H



#include <sstream>
#include <boost/scoped_ptr.hpp>

struct ILogger
{   
    virtual void dispatch( const char *file, unsigned line, const std::stringstream & msg ) = 0;
};

#define LOG( logger, msg)  \
    do {  std::stringstream ss; ss << msg; logger.dispatch( __FILE__, __LINE__, ss ); } while( 0) 


struct Logger : ILogger
{
	Logger( std::ostream & os ) ; 
	~Logger() ; 

    void dispatch( const char *file, unsigned line, const std::stringstream & msg ) ;
private:
	boost::scoped_ptr< struct LoggerImpl>	self; 
};



/*
	we don't really want to add concrete implementation classes here, 
	because their dependencies will become explicit, and included in everything 

	how do we manage interface headers and other headers ?
	separate includes, 
	eg logger.h
	grid_edit_service.h   grid_edit_service_impl.h ?


	REMEMBER - that an interface file, enables us to avoid the boost::noncopyable, or disabling
	of copy and assignment operators. 

*/



/*
	very important - note how easy it is to filter on a file. just add a pattern match, - very useful for debugging 
*/

#if 0
#include <iostream>

struct Logger : ILogger
{   
    void event( const Event &e )     // non-const
    {   
        std::cout << "Logger " << e.file << ":" << e.line << " " << e.msg.str()  << std::endl;
    }
};
#endif

#if 0
// ISP dictates that it is better to use this than std::ostream
#endif


/*
struct Event
{   
	// get rid of this and pass this directly to the event interface

    const char  *file;
    unsigned    line;
    const std::stringstream &msg;

    Event( const char *file, unsigned line, const std::stringstream & msg)
        : file( file),
        line( line),
        msg( msg )
    { }
};
*/

    //virtual void event( const Event &e ) = 0;       // post() implies async 
#endif

