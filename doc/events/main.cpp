
// boost bind etc is not comparable. and therefore can't be used for unsubscribing 
/*
	it would also be better to use a real allocator.
	except we can't because we don't know the size ahead of time.
*/
// won't work for binding have something comparable we have
// to write it ourselves. 
// - it requires polymorphism. 
// ptr to a method. Has to be class specific. 
// remember 
// void func( void *sender, char *msg );  
// it's type specific ...
// typedef void(Dog::*BarkFunction)(void);

#include <vector>
#include <algorithm>
#include <iostream>

// type erased invoker.
// that binds in the calling convention

struct IInvoker
{
	virtual ~IInvoker() { } ;
	virtual void operator () ( void * sender) = 0;
};

template< class C>
struct Invoker : IInvoker
{
	C *c;
	void (C::*m)( void * sender ); 

	Invoker( C *c, void (C::*m)( void * sender ) )  
		: c( c), 
		m( m)
	{ 
		std::cout << "constructor" << std::endl;
	}
	~Invoker()  
	{ 
		std::cout << "destructor" << std::endl;
	} 
	virtual void operator () ( void * sender)
	{
		(c->*m)( sender );
	} 
};


template< class C>
struct Compare
{
	C *c;
	void (C::*m)( void * sender ); 

	Compare( C *c, void (C::*m)( void * sender ) )
		: c( c),
		m( m)
	{ } 

	bool operator () ( const IInvoker * a) const
	{
		const Invoker< C> *x = dynamic_cast< const Invoker< C> *>( a); 
		return x && x->c == c && x->m == m;  		
	}
};

struct Deleter 
{  
	template< class C> 
	void operator()( C * p) {
		delete p;
	}
};

struct Events 
{
	typedef std::vector< IInvoker * > container_type ;
	container_type  subs;

	template< class C> 
	void subscribe( C *c, void (C::*m)( void * sender ) )  
	{ 
		subs.push_back( new Invoker< C> ( c, m ) ); 
	}   

	template< class C> 
	void unsubscribe( C *c, void (C::*m)( void * sender ) )  
	{
		container_type::iterator i 
			= std::remove_if( subs.begin(), subs.end(), Compare< C> ( c, m)); 

		std::for_each( i, subs.end(), Deleter() );

		subs.erase( i, subs.end() );
	}

	void fire( void * sender )
	{

		for( container_type::iterator i = subs.begin();
			i != subs.end(); ++i )
		{
			// if( subs.msg == msg ) // eg. a filter 
			(*i)->operator() ( sender );
		} 
	}
};

///////////////////////////////

/*
	- everything above here can be factored into a separate file. 

	- a standard message interface simplifies things.
		void msg( void *sender, char *msg )
	- keeping the events class separate is nice. 
	- text strings make it easy to filter or take everything etc.
*/

struct A
{
	virtual ~A() { } 

	Events events;
	
	void fire()
	{
		events.fire( this );
	}
};



struct X
{
	A	& a;

	X( A & a)
		: a( a)
	{
		a.events.subscribe( this, &X::on_a_event ); 
//		a.events.unsubscribe( this, &X::on_a_event ); 


		std::cout << "sub " << a.events.subs.size() << std::endl;
	}
	~X()
	{
		a.events.unsubscribe( this, &X::on_a_event ); 

		std::cout << "unsub " << a.events.subs.size() << std::endl;
	}

	void on_a_event( void * sender)
	{
		std::cout << "a event " << sender << std::endl;		
		A * a = dynamic_cast< A *>( sender);
	}
};


int main()
{
	A	a;
	X	x( a);
	
	a.fire();
}



