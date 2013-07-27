/*
	should be no gui specific code here.
*/
#pragma once

#include <common/ptr.h>

#include <common/desc.h>



struct IValidControllerJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual bool get_active() = 0;

	/*
		this covers everything we need for valid control for cube model, or analysis, or single grib from db etc. 
	*/
	virtual void set_valid( const ptr< Valid> & ) = 0; 
	virtual ptr< Valid> get_valid() const = 0; 
	virtual std::vector< ptr< Valid> > get_available_valids() const = 0; 

	// may not be required, unless need global movement 
	// actually still need to know if the projection is active.
	virtual void get_projection() = 0; 
};

struct IValidController
{
	// just used internally
	enum direction_t { prev, next } ; 


	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void add( const ptr< IValidControllerJob> & job ) = 0; 
	virtual void remove( const ptr< IValidControllerJob> & job ) = 0; 


	virtual void change_valid( direction_t direction ) = 0;

	/////////////////////////////////////
	/* these support slider operation */ 
	// think we need to expose the min and the max. and  
	// or else expose the active job ... whereby   
	// no. It should be wrapped.

	// IMPORTANT - FOR ANALYSIS - WE CAN AVOID SUPPORTING. PUT ON A DIFFERENT Validity control INTERFACE ETC. 

	// actually it may be better to return the min, max and increment. except. for analysis 
	
	// there will be no min and max. for historical report data.... But we could re-centre the model. 

/*
	//virtual void get_valids( ptr< Valid> &min, ptr< Valid> &max, int increment ) = 0 ;  
	virtual ptr< Valid> get_min_valid() = 0; 
	virtual ptr< Valid> get_max_valid() = 0; 
	virtual void set_valid( const ptr< Valid> & ) = 0; 
*/

	// why not just provide direct support for a slider control... 
	// rather than make the gui decode the valid types.
	// eg. the min, max and increment in degs...

	virtual void get_min_max( ptr< Valid> &, ptr< Valid > & ) = 0; 
	virtual void set_valid( const ptr< Valid> & ) = 0; 


};


ptr< IValidController> create_valid_controller_service() ;


