/*
	Avoiding the double hooking.
		rather than adding item to subsystem, the item adds itself through it's peer that
		hold refs to interested subsystems. 
		when the object is modified and needs to issue a notify event it does so through the 
		same mechanism as add and remove. 

	- using a peer is not any different for the add/remove
	- but it avoids having to do the reverse registration of listening by other subsystems. 

	- it supports two way databinding type operations - like silverlight stuff

	- the actual stateful object really does need

	- all the subsystems that need to know about an object can be notified
	- by the peer

	- the subsystems are getting the correct typesafe interface 

	- it's all tell- don't ask. 

	- 's very fast. there's no complicated type erasure or even virtual functions.

	- we can rely on the destructor to notify removal.

	- we can have three method add(), destroy(), notify( const char *).   

	- subsystems can maintain their ancillary data without cluttering the object

	- subsystem could propagate events to other subsystems.

	- Subsystems - database synchronization, animation update, property editing, collision detection. 
	 probably not projection.
*/



#include <vector>
#include <algorithm>
#include <iostream>

// another way to do it is we make the objects take a reference to the peer class
// other subsystems then watch the peer class

struct A;

struct IAPeer
{ 
	virtual void notify( A &, const char *msg ) = 0;
};

struct IRenderJob
{

};

struct IRenderer
{
	void notify( IRenderJob &, const char *msg )
	{
		std::cout << "IRenderer notify " << msg << std::endl;

		// the renderer is to		
		if( msg == "add" ) 
		{
			/*
				s.add( job )
			*/
		}
		if( msg == "notify" )
		{
			/*
				the renderer is told about changes to the object without
				having to hook the object directly.
			*/
		}
	} 
};

struct IEdit 
{

};


struct A : IRenderJob, IEdit
{
	A ( IAPeer & peer ) 
		: peer( peer)
	{ 
		peer.notify( *this, "created" );
	} 

	IAPeer & peer; 

	void change()
	{
		peer.notify( *this, "changed" );
	}
};


struct Peer : IAPeer
{
	Peer( IRenderer & renderer )
		: renderer( renderer )
	{ } 

	IRenderer & renderer ;
	std::vector< A *>	s;
	
	void create( /* factory args */ ) 
	{
		// when the object is created we push the peer into it ... 
		// the peer can then broadcast to other subsystems 
		s.push_back( new A( *this) );
	}

	void notify( A & o , const char *msg ) 
	{
		// other subsystems can then register their interest here.
		// and A can expose different interfaces...
		// eg. renderer, collision system, 

		// this would actually support the type
		// other subsystems can create their own wrappers and data as needed

		renderer.notify( o, msg ); 
	}
};

// ********************************8
// ok this is not really any clearer than pushing the item into the subsystem. eg. add() and remove() 
// except that change events are also routed. without the subsystem having to register itself back
// against the object 
// *****************************

int main()
{
	IRenderer	renderer;
	Peer		peer( renderer);
	peer.create();
}

