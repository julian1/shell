


// two classes each holds a reference to the other through normal constructor mechanism
// the way to do it, is to allocate mem first, before constructing the entire object
// note that we can't use the object until everything has taken place. 

#include <iostream>

struct B; 

struct A 
{
	A( B & b )
		: b( b)
	{ 
		std::cout << "address of b " << &b << std::endl;
	} 
	B & b; 
	void func() { } 
};

struct B 
{
	B( A & a)
		: a( a)
	{ 
		std::cout << "address of a " << & a << std::endl;
	} 
	A & a;
	void func() { } 
};

int main()
{
	// use placement new ...

	// it's not possible to instantiate so that they
	// each hold references to the other.
	char a[ sizeof( A) ]; 
	B	b( *(A *)(void *)a ); 

	// now we just have to construct A over X 
	new (a) A( b) ; 

	std::cout << " address of a " << & a << std::endl;
	std::cout << " address of b " << & b << std::endl;

	// explicitly call A destructor	
	( *(A *)(void *)a ).~A(); 
}

