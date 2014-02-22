
#pragma once

#include <common/ptr.h>
#include <boost/scoped_ptr.hpp>

#include <agg_pixfmt_rgba.h>
#include <agg_path_storage.h>

struct Services;
struct Shape;	
struct IProjection;	


/*
	- the Shape is no longer a wrapper or composed of internal Shape and wrapper Shape.... 
	- the full state, is part of the aggregate collection of elts. 
	- if something wants to give the aggregate a shape. then it has to give the full shape.
	- if we really don't want this and want defaults on state (eg color/style), then we impose a wrapper, but this occurs 
		outside the aggregate.
	- we are currently using this wrapper for loading right now. 	

	---
	we have also removed the shape service and updating.
*/




struct ISimpleShape
{
	virtual void add_ref() = 0; 
	virtual void release() = 0; 

	virtual int get_id() const = 0;	
	virtual const agg::path_storage & get_path() const = 0;	

	virtual void set_text( const std::string & ) = 0;
	virtual const std::string & get_text() const = 0;

	virtual bool get_invalid() const = 0; 
	virtual void clear_invalid() = 0;  

	// set_path 
	// set_id
};

struct IShapesObserve
{
	// should be injected into individual shapes, so that we can fire the modified

	virtual void add_shape( const ptr< ISimpleShape > & shape ) = 0;
	virtual void modify_shape( const ptr< ISimpleShape > & shape ) = 0;
};




struct IShapesAggregateRoot
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void subscribe( IShapesObserve & c ) = 0;
	virtual void unsubscribe( IShapesObserve & c ) = 0;

	virtual ptr< ISimpleShape> get_shape( int id ) const = 0;

	// we could actually implement IShapesObserve if we wanted 
	virtual void add_shape( const ptr< ISimpleShape > & shape ) = 0; 
	virtual void remove_shape( const ptr< ISimpleShape > &  shape ) = 0; 

	// maybe better here than on the object, because we can just delegate 
	virtual void set_filename( const std::string & ) = 0;	

	// virtual int get_z_order() = 0; 
};


ptr< IShapesAggregateRoot > create_shapes_aggregate_root();

struct IProjectionAggregateRoot;


void load_shapes_layer( Services & services, const ptr< IShapesAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate); 

//void add_shapes_aggregate_root( Services & services,  const ptr< IShapesAggregateRoot> & root,  const ptr< IProjectionAggregateRoot> & projection_aggregate); 
//void remove_shapes_aggregate_root( Services & services, const ptr< IShapesAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate );

// load some test shapes into the root.
void load_test_shapes( const std::string & filename, const ptr< IShapesAggregateRoot> & root );  



