
#pragma once

#include <common/ptr.h>
#include <boost/scoped_ptr.hpp>

#include <agg_trans_affine.h>
#include <agg_path_storage.h>

struct Services; 
struct IProjection; 


// aggregates are allowed to trace each others references.  

struct IProjectionAggregateRoot //: IKey
{
	/*
		- shouldn't really have any deps
		- and it maintains its state
		- and can be deserialized, and events replayed etc.
		// OK - But maybe it has a surface as a property, which should be injected ...
		// I THINK THIS IS CORRECT AND THE SURFACE SHOULD BE ABLE TO BE PROPAGATED THROUGH
		// IProjection and TO THE RENDERER.
		// Except there are several cases where we wrap the projection (GridProjection) and there
		// is no surface (Doesn't matter) it could still be propagated ...
		// The renderer adaptor would ask the projection  for the surface.
		// EXCEPT - WE ARE NOT NECESSARILY GOING TO RENDER ON IT, but use active and passive buffers.
	*/
	//	ProjectionAggregateRoot();	
	//	~ProjectionAggregateRoot();

	virtual void add_ref()  = 0;
	virtual void release()  = 0;

	/*
		SHOULD RENAME the IProjection to ITransform ???
	*/
	virtual ptr< IProjection> get_projection() const = 0; 

	virtual const agg::trans_affine & get_affine () const = 0;				 
	virtual void set_affine( const agg::trans_affine & ) = 0; 

	// We need a better way to return values ...
	virtual const agg::path_storage & get_limb() const = 0;

	//int get_z_order() const;		// this is for control points ???? - they should always be 100 or top layer.
									// otherwise the projection doesn't have a z_order. it only has an implied
									// order in relation to order projections.
	// private:
	//	boost::scoped_ptr< struct ProjectionAggregateRootImpl> self;

  	//virtual std::size_t hash()	= 0 ; 	
	//virtual bool equal_to( const ptr< IKey> & key )  = 0;


	virtual void set_active( bool ) = 0;
	virtual bool get_active() const = 0;

	virtual bool get_invalid() const = 0;
	virtual void clear_invalid() = 0;
};

// ok, it shouldn't have linked

ptr< IProjectionAggregateRoot> create_projection_aggregate_root();


ptr< IProjectionAggregateRoot>	create_projection_aggregate_root_2(); 

void add_projection_aggregate_root( Services & services, const ptr< IProjectionAggregateRoot> & root ) ;
void remove_projection_aggregate_root( Services & services, const ptr< IProjectionAggregateRoot> & root ) ;




