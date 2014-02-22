
#pragma once

#include <common/ptr.h>
#include <agg_path_storage.h>


struct IMapGridAggregateRoot
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual const agg::path_storage & get_path() const = 0;
	virtual void set_path( const agg::path_storage & ) = 0;
//	virtual void set_active( bool ) = 0;
//	virtual bool get_active() const = 0;
};

struct Services;
struct IProjection;
struct IProjectionAggregateRoot;

ptr< IMapGridAggregateRoot> create_test_mapgrid_aggregate_root();	// create T

void add_mapgrid_aggregate_root( Services & services, const ptr< IMapGridAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & proj) ; 
void remove_mapgrid_aggregate_root( Services & services, const ptr< IMapGridAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & proj)  ; 



