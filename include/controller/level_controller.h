
#pragma once

#include <common/ptr.h>

#include <common/desc.h>

#include <gtkmm.h>	// GtkBox

/*
	- This thing is almost a criteria control. except it's only one element of the criteria. 
	that's why it's effective.

	- In a big way. it and the validity control. kind of removes the need for complicated criteria.  

	- since it's   should call it an editor or service ???? 
				
*/


struct ILevelControllerJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual bool get_active() = 0;

	/*
		this covers everything we need for level control for cube model, or analysis, or single grib from db etc. 
	*/
	virtual void set_level( const ptr< Level> & ) = 0; 
	virtual ptr< Level> get_level() const = 0; 
	virtual std::vector< ptr< Level> >  get_available_levels() const = 0; 

	// may not be required, unless need global movement 
	// actually still need to know if the projection is active.
	virtual void get_projection() = 0; 
};

struct ILevelController
{

	// just used internally
	enum direction_t { up, down } ; 


	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void add( const ptr< ILevelControllerJob> & job ) = 0; 
	virtual void remove( const ptr< ILevelControllerJob> & job ) = 0; 


	virtual void change_level( direction_t direction ) = 0; 

	// it needs an update, in order to determine what is active of not.
	//virtual void update() = 0; 

//	virtual void dispatch( const UIEvent & e) = 0; 
//	virtual void set_active( bool ) = 0; 
};


ptr< ILevelController> create_level_controller_service( ) ; 

// ptr< ILevelController> create_level_controller_service( Gtk::Box &  ); 


/*
	*** VERY IMPORTANT ***

	OK. 
	
	RATHER THAN HAVE ALL OF THESE KEYS. 

	- We still have separate classes to do everything. BUT. We can create a single class that 
	has every interface and that delegates for all of these jobs. 

	- In fact it could even inherit from the concrete instances. 

	It may be easier just to use keys.

	The key represents identity.

	It would be created for each different projection. (but wouldn't have to be. ) 
*/


