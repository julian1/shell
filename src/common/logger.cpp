
// logger is not a service 

#include <common/logger.h>

#include <cstring>	// strlen

/*
	// would it be easier just to use unique_ptr< > or scoped_ptr and incur the simple dependency ? 
	// using a scoped_ptr for self ptr is quite nice - it alleviates having to write supressing copy and assign operators. 
	// however we loose the . operator in place of requiring ->
	// also the ref counting code only goes in the destructor
*/


template<class InputIterator, class T>
static InputIterator find_last ( InputIterator first, InputIterator last, const T& value )
{
	InputIterator ret = last;  
	for ( ;first!=last; first++)	
		if ( *first==value ) 
			ret = first; 
	return ret;
}



struct LoggerImpl
{
	/*
		if we have several of these are there possible link issues
		resolving the constructor ?
	*/
	LoggerImpl( std::ostream & os)
		: os ( os)
	{ } 
	
	std::ostream & os; 	
}; 

Logger:: Logger( std::ostream & os ) 
	: self( new LoggerImpl( os))
{ }

Logger::~Logger() 
{ }

void Logger::dispatch( const char *file, unsigned line, const std::stringstream & msg )
{   
//		return; 
	// want the basename of the file
//	boost::filesystem::path		path( e.file); 

	self->os << msg.str() << "   " ;

	std::string s1( file);  

	size_t n = std::strlen( file ); 

	std::string s( find_last( file, file + n, '/' ), file + n); 

//       os << "Logger " << 
	self->os << "[" << s << ":" << line << "] " ; 
	for( int i = s.size(); i < 20; ++i)
		self->os << " ";

	self->os << std::endl;
}




