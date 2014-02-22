
#include <sstream>
#include <set>
#include <algorithm>

#include <controller/level_controller.h>

#include <gtkmm.h>	// GtkBox

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <boost/bind.hpp>


namespace {

// it's hard to get the button organization correct,
// this will have to be instantiated after the modal controller/delegator 


// it might make sense to put all dims controllers in here

// we just about ought to be able to work with the existing aggregate

// so is this going to be


typedef std::set<  ptr< ILevelControllerJob> >	objects_t;



static bool levels_less_than( const ptr< Level> & a , const ptr< Level> & b )
{

	if( a->type < b->type ) return true;
	if( a->type > b->type ) return false;
	assert( a->type == b->type ); 

	/*
		OK. Comparing things is stupidly difficult. 
		Because we must dereference first. 
	*/

	switch( a->type ) 
	{
		case Desc::level_isobaric: 
			return (( LevelIsobaric & ) *a). hpa < (( LevelIsobaric & ) *b). hpa  ;
			break;

		case Desc::level_isobaric_range: 
			return (( LevelIsobaricRange & )*a). hpa1 < (( LevelIsobaricRange & )*b). hpa1  ;
			break;

		default:
			// keep stable
			return a < b ; //false;
	}; 

	assert( 0 );
}




static bool levels_equal( const ptr< Level> & a , const ptr< Level> & b )
{
	// this is already overloaded for us
	return *a == *b;
}


struct LevelController 
	: ILevelController 
{
	// Change name to level controller. This is not editing state. 


	LevelController(  ) 
		: count( 0),
		jobs()
	{ 
		std::cout << "!!!! level editor constructor" << std::endl;
	} 

	~LevelController(  ) 
	{
		std::cout << "!!!! level editor destructor" << std::endl;
	}

	void add_ref() { ++count; }
	void release() { if( --count == 0 ) delete this; }

	void add( const ptr< ILevelControllerJob> & job )
	{ 
		std::cout << "@@@@ level edit job job added" << std::endl;
		assert( jobs.find( job) == jobs.end() );
		jobs.insert( job );
	} 
	
	void remove(  const ptr< ILevelControllerJob> & job )
	{ 
		assert( jobs.find( job) != jobs.end() );
		jobs.erase( job );
	} 

		
	void change_level( direction_t direction )
	{
		// find an active job
		objects_t::iterator ii 
			= std::find_if( jobs.begin(), jobs.end(), boost::bind( & ILevelControllerJob::get_active, _1 )  ); 

		// none, then return
		if( ii == jobs.end() )
			return ;
		
		ptr< ILevelControllerJob> active = *ii; 


		std::vector< ptr< Level> > levels = active->get_available_levels(); 

/*
		std::cout << "---------------------"  << std::endl;
		std::cout << "levels to sort  " << levels.size() << std::endl;
		for( unsigned i = 0; i < levels.size(); ++ i )
		{
			std::cout << "address " << ((void *)& levels[ i] ) << "   " << ((void *)& *levels[ i] ) << " " << levels[ i]->count << std::endl;
		}
*/

		// sort the levels 
		std::stable_sort( levels.begin(), levels.end(), levels_less_than ); 
//		std::sort( levels.begin(), levels.end(), levels_less_than() ); 


		// get the active level and available levels
		ptr< Level>  level = active->get_level(); 

		// look up the iterator for current level **** this should value comparison rather than ptr ... ???
		//std::vector< ptr< Level> >::const_iterator i = std::find( levels.begin(), levels.end(), level ); 
		std::vector< ptr< Level> >::const_iterator i 
			= std::find_if( levels.begin(), levels.end(), boost::bind( & levels_equal, level, _1 ) ); 


		ptr< Level> new_level; 

		// set the new level if possible
		if( direction == up )
		{		
			if( i != levels.end() && i != levels.begin()  )
			{
				new_level = *(i - 1 ); 
			}
		}
		else if ( direction == down )
		{
			if( i != levels.end() && i + 1 != levels.end() )
			{
				new_level = *(i + 1 ); 
			}
		}
		else assert( 0 );

		if( new_level) 
		{
			active->set_level( new_level );

#if 0
			// this can/should be factored to support different gui
			std::stringstream	ss; 
			ss << *new_level ;
			label.set_text( ss.str() ) ;  
#endif
		}
	}

/*
	if a new model is made active, then we need to reflect the correct level !!. 
	how !!?!??
		using callback on the component. or else scan each time. ?
*/
/*
	void update()
	{ } 
*/
private:
	unsigned count;
	objects_t	jobs;
};




};	// anon namespace




ptr< ILevelController> create_level_controller_service( )
{
	return new LevelController  ;
}


#if 0
ptr< ILevelController> create_level_controller_service( Gtk::Box & box  )
{
	ptr< LevelController> level_controller( new LevelController  ) ;

	return new GUILevelController ( box, level_controller ); 
}
#endif

#if 0
ptr< GUILevelController> create_gui_level_controller_service( Gtk::Box & box, LevelController & level_controller   )
{
	return new GUILevelController ( box, level_controller ); 
}
#endif



