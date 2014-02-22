
#pragma once


/*
	All items bounds should be inserted, with an ordering number. 
	and also whether it's dirty.	
		
	Then in one go we update, by building the tree  

	----
	eg. labels are permanently mantained. if something changes then we recalculate. 
	this avoids, issue of buliding up the graph. 
	--
	eg. if the projection is active, then the aggregate will be active, then the thing will need recalc ? 
*/

#include <common/ptr.h>

struct ILabelJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	// queries performed from subsystem side. these are all queries made by the service
	virtual void get_bounds( int *x1, int *y1, int *x2, int *y2 ) = 0;  
	virtual int get_z_order() const = 0; 
	virtual bool get_invalid() const = 0;	

	/*
		- note there is no persistance of this information here. 
		it is set, free_of_intersections is set the time that labelling is run
		- if it's cached then there will be no change.
	*/	
	// commands now how does the client know ??
	virtual void free_of_intersections( bool ) = 0;		// we will have to set a recorded value
};

struct ILabels
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void update() = 0; 

//	virtual void add( const ptr< IKey> & key, const ptr< ILabelJob> & job) = 0; 
//	virtual void remove( const ptr< IKey> & key ) = 0; 

	virtual void add( const ptr< ILabelJob> & job) = 0; 
	virtual void remove( const ptr< ILabelJob> & job ) = 0; 
};

ptr< ILabels>	create_labels_service();		

