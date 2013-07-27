
#include <sstream>
#include <set>
#include <algorithm>

#include <service/valid_controller.h>

#include <gtkmm.h>	// GtkBox

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <boost/bind.hpp>


namespace {



typedef std::set<  ptr< IValidControllerJob> >	objects_t;


static bool valids_less_than( const ptr< Valid> & a , const ptr< Valid> & b )
{
	if( a->type < b->type ) return true;
	if( a->type > b->type ) return false;
	assert( a->type == b->type ); 

	switch( a->type ) 
	{
		case Desc::valid_simple: 
			return (( ValidSimple & ) *a). valid < (( ValidSimple & ) *b). valid ;
			break;

		default:
			assert( 0 );
			return a < b ; //false;
	}; 

	assert( 0 );
}




static bool valids_equal( const ptr< Valid> & a , const ptr< Valid> & b )
{
	// this is already overloaded for us
	return *a == *b;
}


struct ValidController 
	: IValidController 
{
	// Change name to valid controller. This is not editing state. 


	ValidController(  ) 
		: count( 0),
		jobs()
	{ 
		std::cout << "!!!! valid editor constructor" << std::endl;
	} 

	~ValidController(  ) 
	{
		std::cout << "!!!! valid editor destructor" << std::endl;
	}

	void add_ref() { ++count; }
	void release() { if( --count == 0 ) delete this; }

	void add( const ptr< IValidControllerJob> & job )
	{ 
		std::cout << "@@@@ valid edit job job added" << std::endl;
		assert( jobs.find( job) == jobs.end() );
		jobs.insert( job );
	} 
	
	void remove(  const ptr< IValidControllerJob> & job )
	{ 
		assert( jobs.find( job) != jobs.end() );
		jobs.erase( job );
	} 

private:
	ptr< IValidControllerJob> get_active_job()
	{

		// find an active job
		objects_t::iterator ii 
			= std::find_if( jobs.begin(), jobs.end(), boost::bind( & IValidControllerJob::get_active, _1 )  ); 

		// none, then return
		if( ii == jobs.end() )
			return ptr< IValidControllerJob>() ;	// empty
	
		return *ii; 	
	}

public:
	void get_min_max( ptr< Valid> &start, ptr< Valid > & finish ) 
	{
		// find an active job
		ptr< IValidControllerJob> active = get_active_job(); 
		// none, then return
		if( ! active )
			return ;
		
		std::vector< ptr< Valid> > valids = active->get_available_valids(); 
		std::sort( valids.begin(), valids.end(), valids_less_than ); 

		if( ! valids.empty()  )
		{
			start = valids.front(); 
			finish = valids.back(); 
		}
		else
		{
			// should set to an explict none ?
		}
	}

	void set_valid( const ptr< Valid> & valid ) 
	{
		// Maybe off the list of valids
		ptr< IValidControllerJob> active = get_active_job(); 
		// none, then return
		if( ! active )
			return ;
	
		active->set_valid( valid );
	}


	//virtual ptr< Valid> get_max_valid() = 0; 
	
	void change_valid( direction_t direction )
	{
		// find an active job
		ptr< IValidControllerJob> active = get_active_job(); 
		// none, then return
		if( ! active )
			return ;
	
		std::vector< ptr< Valid> > valids = active->get_available_valids(); 

/*
		std::cout << "---------------------"  << std::endl;
		std::cout << "valids to sort  " << valids.size() << std::endl;
		for( unsigned i = 0; i < valids.size(); ++ i )
		{
			std::cout << "address " << ((void *)& valids[ i] ) << "   " << ((void *)& *valids[ i] ) << " " << valids[ i]->count << std::endl;
		}
*/

		// sort the valids 
		std::stable_sort( valids.begin(), valids.end(), valids_less_than ); 
//		std::sort( valids.begin(), valids.end(), valids_less_than() ); 


		// get the active valid and available valids
		ptr< Valid>  valid = active->get_valid(); 

		// look prev the iterator for current valid **** this should value comparison rather than ptr ... ???
		//std::vector< ptr< Valid> >::const_iterator i = std::find( valids.begin(), valids.end(), valid ); 
		std::vector< ptr< Valid> >::const_iterator i 
			= std::find_if( valids.begin(), valids.end(), boost::bind( & valids_equal, valid, _1 ) ); 


		ptr< Valid> new_valid; 

		// set the new valid if possible
		if( direction == prev )
		{		
			if( i != valids.end() && i != valids.begin()  )
			{
				new_valid = *(i - 1 ); 
			}
		}
		else if ( direction == next )
		{
			if( i != valids.end() && i + 1 != valids.end() )
			{
				new_valid = *(i + 1 ); 
			}
		}
		else assert( 0 );

		if( new_valid) 
		{
			active->set_valid( new_valid );

#if 0
			// this can/should be factored to sprevport different gui
			std::stringstream	ss; 
			ss << *new_valid ;
			label.set_text( ss.str() ) ;  
#endif
		}
	}

/*
	if a new model is made active, then we need to reflect the correct valid !!. 
	how !!?!??
		using callback on the component. or else scan each time. ?
*/
/*
	void prevdate()
	{ } 
*/
private:
	unsigned count;
	objects_t	jobs;
};




};	// anon namespace

ptr< IValidController> create_valid_controller_service(   )
{

	return new ValidController  ;
/*
	ptr< ValidController> valid_controller( new ValidController  ) ;
	return new GUIValidController ( box, valid_controller ); 
*/
}

#if 0
ptr< GUIValidController> create_gui_valid_controller_service( Gtk::Box & box, ValidController & valid_controller   )
{
	return new GUIValidController ( box, valid_controller ); 
}
#endif



