
#pragma once

/*
	performs two jobs. 
		1. A sequenced update and post_update for layers. 
		2. lifetime management for layers (eg. ref counted). 
		3. naming
*/


struct ILayerJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void layer_update( ) = 0;  
	virtual void post_layer_update( ) = 0;  


	// load() and unload() 
	virtual std::string get_name() = 0;  // to identify to the user
};



struct ILayers
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void layer_update() = 0; 
	virtual void post_layer_update() = 0; 

	virtual void add(  const ptr< ILayerJob> & job) = 0; 
	virtual void remove( const ptr< ILayerJob> & job ) = 0; 
};

ptr< ILayers>	create_layers_service();		


