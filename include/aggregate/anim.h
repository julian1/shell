
#ifndef MY_ANIM_H
#define MY_ANIM_H

#include <common/ptr.h>

#include <agg_path_storage.h>

struct IAnimAggregateRoot
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual const agg::path_storage & get_path() const = 0;
	virtual void set_path( const agg::path_storage & ) = 0;

	virtual void set_active( bool ) = 0;
	virtual bool get_active() const = 0;
};

struct Services;
struct IProjection;

//ptr< IAnimAggregateRoot> create_test_anim_aggregate_root();	// create T



void add_test_anim_layer( Services & services ); 

//void add_anim_aggregate_root( Services & services, const ptr< IAnimAggregateRoot> & root ); 
//void remove_anim_aggregate_root( Services & services, const ptr< IAnimAggregateRoot> & root );


#endif


