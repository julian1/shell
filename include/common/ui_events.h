
#pragma once

/*
	- If we really want to make events objects that we switch of and async etc, then we can do so easily.  
	However do it behind a sync method interface.

	- For recording application etc. could be useful. but there is no reason to drop the type safety.

	- we can do anything with this simple interface however. including multiplexing, selective delegation, rerouting, macro recording etc.
*/


struct IMyEvents
{
	virtual void mouse_move( unsigned x, unsigned y )  = 0;
	virtual void button_press( unsigned x, unsigned y ) = 0;  
	virtual void button_release( unsigned x, unsigned y ) = 0;  

	enum { shift_key = 65505, ctrl_key = 65507 } ;  

	virtual void key_press( int ) = 0;  
	virtual void key_release( int ) = 0;  

	/*	UISizeAllocate 
		UIPaint
		UITimeout	// Timer */
};



/*
	- VERY IMPORTANT - why have the event broadcaster. 
	- Why not from top level just call dispatch() on each subsystem that can process events. 
	- just like we call update() when we expose ?
	- eg. top level should determine - order. (and this is done already by calling explicitly )
	- the event broker, is like a factory and is an antipattern (compared with dependency injection ).
	---
	the only thing is if we want other items to handle events (eg map), or layer 
	but i don't think so. I thnk subsystems are the correct focus point for events. 
*/


