
#ifndef MY_PARSER_H
#define MY_PARSER_H

/*
	important - we could template the events/context, to be able to carry around context
	for context sensitive parsing (eg symbols and types in c/c++, or nesting levels etc ). 

	we then write special rules for when we need this (eg. same as grib and frame parsing). 

	Also, Why do we pass std::vector<> events, when we should just pass the mark - because
	it records for the currnt context.

	- also the marker is not really doing anything useful. it's the same number of lines
	of code.
*/

/*
	g++ -Wall main.cpp -I /opt/boost/include/ /opt/boost/lib/libboost_test_exec_monitor.a
*/
/*
	- the really nice thing, is that we can just have enum defined captures to extract data, if we don't need a tree.
	but it is still fully recursive, unlike regex. Alternatively we can have a full parse tree.  
*/

/*
	g++ main.cpp

	Can we shield the tree building behind an interface. so we just pass that 
	interface around, and not the tree. like SAX  
	

	useful, 
	http://lambda-the-ultimate.org/node/2341
	http://aurochs.fr/
	http://fossil.wanderinghorse.net/repos/pegc/index.cgi/index
	http://code.google.com/p/pegtl/
	http://fossil.wanderinghorse.net/repos/parsepp/index.cgi/wiki/parsepp
	http://p-stade.sourceforge.net/biscuit/index.html

	general (peg), 
	http://www.codeproject.com/KB/recipes/crafting_interpreter_p1.aspx
-----
	Aurochs, 
	Actually, tags are emitted as CREATE_NODE, ADD_CHILDREN and ADD_ATTRIBUTE instructions in the automaton code. 
	The interpreter calls the appropriate hooks upon encountering these instructions. 

	- can we make it hook driven ? - rather than creating a concrete tree ? 
	- or hide the tree creation behind the interface.
-----------------------------
	we avoid all the complexity of boost::spirit, qi, phoenexo
	
	c or c++ locales
	unicode
	different types of iterators and parsers	
	various types of hard to bind actions

	and replace boost::regex, 
	boost::xpressive, 

	see yard (yet another recusive descent), and biscuit for other examples of template parsers
	we probably do want a dollar for eof, and question,  

----------	
	aurochs uses constructors , and captures.     we need to work out the logic of this. 
	
	Rembmer we can pass in an enum, but probably not a string.
-----
	http://p-stade.sourceforge.net/biscuit/index.html 
---
	biscuit uses captures, and template int/enum with it, 

	BOOST_CHECK((
		biscuit::results_match<
			seq<
				capture< 1, star_before< any, char_<'x'> > >,
				char_<'x'>,
				capture< 2, backref<1> >,
				char_<'x'>
			>
		>(text, caps)
	));
----

	it is possible to combine expressions - eg from Yard (from google)

	template<typename T>
	struct advance_if_not :
	  bnf_and<
		bnf_not<
		  T
		>,
		AnyChar
	  >
	{ }

----------
	this template combining is a lot like functional programming. 
*/

/*
So far, the grammars only describe valid input.
To get a parse tree, you use constructors.

word ::= <Word>{[a-z]+}</Word>;
wordlist ::= <WordList> wordlist_inner </WordList>;
wordlist_inner ::= word ' ' wordlist | word;

On input “foo bar baz” this gives:

<WordList>
	<Word>foo</Word> <Word>bar</Word> <Word>baz</Word>
</WordList>
*/
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

/*
	ok the thing that we are missing 
	can we clean the template specification ? 
*/
struct event
{
	// change name, construct, destruct ? 
	typedef enum { NONE, PUSH_NODE, POP_NODE, CAPTURE  } event_type; 

	event_type	type;

	int			id; 
	const char *start; 
	const char *finish; 

	event( event_type type)
		: type( type)
	{ } 	

	event( event_type type, int id, const char *start, const char *finish)
		: type( type),
		id( id),
		start( start),
		finish( finish)
	{ }
};


struct mark
{
private:
	// tracker for events
	// doesn't work - because the  mark state wants to be on the stack ...
	std::vector< event>		&buf; 
	std::size_t				pos; 

public:
	mark( std::vector< event> &buf)  
		: buf( buf), 
		pos( buf.size() )
	{ }

	void rewind()
	{
		// if pos != buf.end() 
		buf.erase( buf.begin() + pos, buf.end() ); 
	}

	void update()
	{
		// needs to be fase
		pos = buf.size();
	}
};

// note that we cannot actually handle testing the start of sequence (regex ^), unless we passed the start of the
// buffer pointer through the entire parse.

struct eos_
{
	// end of sequence - or end of file etc, $ in regex 

	static const char * parse( const char *start, const char *finish, std::vector< event> &events ) 
	{
		assert( start && start <= finish); 

		// should be, if( start == finish ) ?  
		if( start == finish ) 
			return start; 
		return 0; 
	}
};




struct none_
{
	// none, guaranteed succees (eg even if on eof) and doesn't advance - used for alternate captures 

	static const char * parse( const char *start, const char *finish, std::vector< event> &events ) 
	{
		assert( start); 
		return start; 
	}
};



/*
	return 0 if parse failed
	return current pos if succeeded
	only terminals will actually advance character
*/



struct any_
{
	// should this be called 'any' ?

	static const char * parse( const char *start, const char *finish, std::vector< event> &events ) 
	{
		assert( start && start <= finish); 
		if( start == finish ) 
			return 0; 
		return start + 1; 
	}
};

template< int ch> 
struct char_
{
	// parse a specific character
	static const char * parse ( const char *start, const char *finish, std::vector< event> &events ) 
	{
		assert( start && start <= finish); 
		if( start == finish ) 
			return 0; 
		if( *start == ch) 
			return start + 1; 
		return 0; 	
	}
};

template< int ch1, int ch2> 
struct range_
{
	// parse a character range
	static const char * parse ( const char *start, const char *finish, std::vector< event> &events ) 
	{
		assert( start && start <= finish); 
		if( start == finish ) 
			return 0; 
		if( *start >= ch1 && *start <= ch2)
			return start + 1; 
		return 0;
	}
};


struct empty
{ };  

 
/*
	it might be better for the compiler (eg simpler) to make _or2 explicit, and then 
	just do partial specializations for the or_. Eg so the basic construct is
	not a specialization
*/

template< class A1, class A2> 
struct or2_ 
{
	static const char * parse ( const char *start, const char *finish,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start  ); 
		const char *ret = 0; 
		ret = A1::parse( start, finish, events); 
		if( ret) return ret; 
		m.rewind(); 
		ret = A2::parse( start, finish, events); 
		if( ret) return ret; 
		m.rewind(); 
		return 0;
	}
};

template< class A1, class A2 = empty, class A3 = empty, class A4 = empty, class A5 = empty, class A6 = empty, class A7 = empty, class A8 = empty, class A9 = empty> 
struct or_
{ }; 

// an or with one argument, is useful for debugging, when we don't to avoid rewriting the grammar to remove the term  
template< class A1> 
struct or_< A1> : A1 
{ };


template< class A1, class A2> 
struct or_< A1, A2> : or2_< A1, A2> 
{ };

template< class A1, class A2, class A3> 
//struct or_< A1, A2, A3> : or2_< or2_< A1, A2>, A3> 
struct or_< A1, A2, A3> : or2_< A1, or2_< A2, A3> > 
{ };

template< class A1, class A2, class A3, class A4>  
//struct or_< A1, A2, A3, A4> : or2_< or2_< or2_< A1, A2>, A3>, A4 > 
struct or_< A1, A2, A3, A4> : or2_< A1, or2_< A2, or2_< A3, A4> > > 
{ };

template< class A1, class A2, class A3, class A4, class A5>  
//struct or_< A1, A2, A3, A4, A5> : or2_< or2_< or2_< or2_< A1, A2>, A3>, A4>, A5> 
struct or_< A1, A2, A3, A4, A5> : or2_< A1, or2_< A2, or2_< A3, or2_< A4, A5> > > >  
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6>  
struct or_< A1, A2, A3, A4, A5, A6> : or2_< A1, or2_< A2, or2_< A3, or2_< A4, or2_< A5, A6> > > > >  
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7>  
struct or_< A1, A2, A3, A4, A5, A6, A7> : or2_< A1, or2_< A2, or2_< A3, or2_< A4, or2_< A5, or2_< A6, A7> > > > > >  
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>  
struct or_< A1, A2, A3, A4, A5, A6, A7, A8> : or2_< A1, or2_< A2, or2_< A3, or2_< A4, or2_< A5, or2_< A6, or2_< A7, A8> > > > > > >  
{ };





template< class A1, class A2> 
struct and2_
{
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start  ); 
		const char *ret = start;
		ret = A1::parse( ret, finish, events); 
		if( !ret)  { m.rewind(); return 0; } 	
		ret = A2::parse( ret, finish, events); 
		if( !ret ) { m.rewind(); return 0; }   	
		return ret;	
	}
};


template< class A1, class A2 = empty, class A3 = empty, class A4 = empty, class A5 = empty, class A6 = empty, class A7 = empty, 
	class A8 = empty, class A9 = empty, class A10 = empty, class A11 = empty, class A12 = empty> 
struct seq_
{ };  

// a sequence of one, is useful for debugging, when we don't to avoid rewriting the grammar to remove the seq_ term  
template< class A1> 
struct seq_< A1> : A1 
{ };


template< class A1, class A2> 
struct seq_< A1, A2> : and2_< A1, A2>
{ };

template< class A1, class A2, class A3> 
//struct seq_< A1, A2, A3> : and2_< and2_< A1, A2>, A3> 
struct seq_< A1, A2, A3> : and2_< A1, and2_< A2, A3> > 
{ };

template< class A1, class A2, class A3, class A4>  
//struct seq_< A1, A2, A3, A4> : and2_< and2_< and2_< A1, A2>, A3>, A4 > 
struct seq_< A1, A2, A3, A4> : and2_< A1, and2_< A2, and2_< A3, A4> > > 
{ };

template< class A1, class A2, class A3, class A4, class A5>  
struct seq_< A1, A2, A3, A4, A5> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, A5> > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6>  
struct seq_< A1, A2, A3, A4, A5, A6> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, A6> > > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7>  
struct seq_< A1, A2, A3, A4, A5, A6, A7> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, and2_< A6, A7> > > > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>  
struct seq_< A1, A2, A3, A4, A5, A6, A7, A8> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, and2_< A6, and2_< A7, A8> > > > > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>  
struct seq_< A1, A2, A3, A4, A5, A6, A7, A8, A9> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, and2_< A6, and2_< A7, and2_< A8, A9> > > > > > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>  
struct seq_< A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, and2_< A6, and2_< A7, and2_< A8, and2_< A9, A10> > > > > > > > > 
{ };

template< class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11>  
struct seq_< A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> : and2_< A1, and2_< A2, and2_< A3, and2_< A4, and2_< A5, and2_< A6, and2_< A7, and2_< A8, and2_< A9, and2_< A10, A11> > > > > > > > > > 
{ };





template< class A1> 
struct plus_
{
/*
	// one or more
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start  ); 
		const char *ret = 0;
		// take first one, 
		ret = A1::parse( start, finish, events); 	
		if( !ret) { m.rewind(); return 0; }
		// take subsequent - we are now committed
		do {
			m.update();	// update mark point to current 
			const char *last_ret = ret; 
			ret = A1::parse( ret, finish, events); 	
			if( !ret) { m.rewind(); return last_ret;  }	 // rewind to last good, and return
		} while( true); 

		assert( 0); 
	}
*/
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start  ); 

		const char *last_ret = 0; 
		while( true) {
			m.update();	// update mark point to current 
			start = A1::parse( start, finish, events); 	
			if( !start) { m.rewind(); return last_ret;  }	 // rewind to last good (0 if none parsed) and return
			last_ret = start; 
		} ; 

		assert( 0); 
	}
};


template< class A1> 
struct star_
{
	// zero or more
/*
	static const char * parse( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start ); 
		const char *ret = start;
		do {
			m.update();  
			const char *last_ret = ret; 
			ret = A1::parse( ret, finish, events); 	
			if( !ret) { m.rewind();  return last_ret; } 	// i think this is correct 
		} while( true); 
		assert( 0);
	}
*/

	// we should be able to write the above like this - can't this be rewritten in terms of one_or_more ?
	static const char * parse( const char *start, const char *finish,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start ); 
		const char *ret = plus_< A1>::parse( start, finish, events );  
		if( ret) return ret;
		m.rewind();
		return start; 
	}
};

template< class A1> 
struct opt_
{
	// zero or one - eg always succeeds and non-greedy 
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start  ); 
		const char *ret = A1::parse( start, finish, events); 	
		if( !ret) { m.rewind(); return start; }
		else return ret;
		assert( 0); 
	}
};



template< class A1, int n> 
struct repeat_
{
	// change name to repeatn_ and the two argument version to repeatnn_

	// we need a range repeat as well eg 5-10
	// note that this could be synthesized. - but easier to hardcode, if don't have memoized backtracking
	// n number eg. { } 
/*
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		assert( n >= 0);
		mark	m( events); 
		assert( start ); 
		int i = 0; 
		const char *ret = start;
		do {
			ret = A1::parse( ret, finish, events); 	
			if( !ret) { m.rewind(); return 0; } 
			if( ++i == n) return ret; 
		} while( true); 
		assert( 0);
	}
*/
	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		assert( n >= 0);
		mark	m( events); 
		assert( start ); 
		for( int i = 0; i < n; ++i ) 
		{
			start = A1::parse( start, finish, events); 	
			if( !start) { m.rewind(); return 0; } 
		} 
		return start; 
	}
};

template< class A1, int n1, int n2> 
struct repeatnn_
{
/*
	this really needs more testing.
*/
	// it is a bit difficult to interpret meaning. if we have more A1 then n2 then we fail. or should we just avoid
	// being greedy. I think should avoid being greedy.
	// we need a range repeat as well eg 5-10


	static const char * parse ( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		assert( n1 >= 0 && n2 >= 0);
		mark	m( events);				// this is updated as we go. change name to last_mark ? 
		mark	start_mark( events);			// this records the beginning if couldn't match 
		assert( start ); 
		int i = 0; 
		const char *ret = start;
		do {
			m.update();  
			const char *last_ret = ret; 
			ret = A1::parse( ret, finish, events); 	
			if( !ret ) 
			{ 
				if( i >= n1 )
				{
					m.rewind(); 
					return last_ret; 
				}
				else						
				{
					start_mark.rewind(); 
					return 0; 
				}
			} 
			++i; 
			if( i == n2)
			{
				// don't be greedier than what was specified.
				return ret; 
			}

		} while( true); 
		assert( 0);
	}
};




template< class A1> 
struct require_
{
	// tests the predicate but does not advance (opposite of not ).  
	static const char * parse( const char *start, const char *finish, std::vector< event> &events)
	{
		mark	m( events); 
		assert( start); 
		// always remove events
		if( ! A1::parse( start, finish, events)) { m.rewind(); return 0; } 
		else { /*m.rewind();*/ return start; }  // we'll leave the events, although there really shouldn't be any 
	}
};


// rather than always removing events, in the event of failure. Wouldn't it be easier to append them if succeed ?
// eg just instantiate a vector, and pass it down, and then do append if it succeeded ?

// can't require be written in terms of not ? 

template< class A1> 
struct not_
{
	// not - unary operator that never advances
	static const char * parse( const char *start, const char *finish ,  std::vector< event> &events)
	{
		mark	m( events); 
		assert( start); 
		// if first succeed return fail - don't record events ?????
		// always remove events
		if( A1::parse( start, finish, events)) { m.rewind(); return 0; } 
		else { /*m.rewind();*/ return start; }  // if succeed we, shouldn't wipe the events, eg  seq< a, not< b> c > we want to keep events on a
												// no, i think the above is wrong. and we should assert nothing generates events
												// in the subexpression. - except that if we pick up a tree, it doesn't matter ?
	}
};


template< class A1> 
struct constructor_
{
	// construct a parse tree node
	// we just insert push and pop into the event stream

};


/*
	we could actually do partial decoding in the capture - eg grib values, here
	and store. 

	remember the capture is powerful, when pass in a named enum, then we can search the events.
	eg possible transfer into a std::map<> or multimap, for indexing
	
	decode44 = dec( m.find( DECODE_44, )) ; 
*/

template< int id, class A1> 
struct capture_
{
	// capture, the contents that were parsed

	static const char * parse( const char *start, const char *finish ,  std::vector< event> &events) 
	{
		mark	m( events); 
		assert( start);
		const char *ret = A1::parse( start, finish, events ); 
		if( !ret) { m.rewind(); return 0; } 
		else {		
			events.push_back( event( event::CAPTURE, id, start, ret)); 
			return ret; 
		}	
	}
};



// synthesized  rules


// take 'any' character, if not arg. advance upto the terminating term - change name star_if_not ?
template< class A1>
struct advance_if_not_ : plus_< seq_ < not_< A1 > , any_ > >  { }; 

// advance and glob the terminating term	change_name to star_until ? or take_until ? 
template< class A1>
struct advance_until_ : seq_< advance_if_not_< A1 > , A1 > { } ;  




// character or - eg  struct whitespace : chor< ' ','\t', '\r', '\n' > { } ;
template< int A1, int A2, int A3 = 0, int A4 = 0, int A5 = 0, int A6 = 0> 
struct chset_ { };  

template< int A1, int A2>  
struct chset_< A1, A2> : or_< char_< A1>, char_< A2> > { };

template< int A1, int A2, int A3>  
struct chset_< A1, A2, A3> : or_< char_< A1>, char_< A2>, char_< A3> > { };

template< int A1, int A2, int A3, int A4>  
struct chset_< A1, A2, A3, A4> : or_< char_< A1>, char_< A2>, char_< A3>, char_< A4> > { };

template< int A1, int A2, int A3, int A4, int A5>  
struct chset_< A1, A2, A3, A4, A5> : or_< char_< A1>, char_< A2>, char_< A3>, char_< A4>, char_< A5> > { };



// character sequence 'b', 'l', 'u', 'e'
template< int A1, int A2, int A3 = 0, int A4 = 0, int A5 = 0, int A6 = 0, int A7 = 0> 
struct chseq_ { };  

template< int A1, int A2>  
struct chseq_< A1, A2> : seq_< char_< A1>, char_< A2> > { };

template< int A1, int A2, int A3>  
struct chseq_< A1, A2, A3> : seq_< char_< A1>, char_< A2>, char_< A3> > { };

template< int A1, int A2, int A3, int A4>  
struct chseq_< A1, A2, A3, A4> : seq_< char_< A1>, char_< A2>, char_< A3>, char_< A4> > { };

template< int A1, int A2, int A3, int A4, int A5>  
struct chseq_< A1, A2, A3, A4, A5> : seq_< char_< A1>, char_< A2>, char_< A3>, char_< A4>, char_< A5> > { };

template< int A1, int A2, int A3, int A4, int A5, int A6>  
struct chseq_< A1, A2, A3, A4, A5, A6> : seq_< char_< A1>, char_< A2>, char_< A3>, char_< A4>, char_< A5>, char_< A6> > { };






#endif



#if 0
static void dump_events( std::vector< event> &events, std::ostream &os) 
{
	typedef std::vector< event> events_type;

	for( events_type::const_iterator i = events.begin(); 
		i != events.end(); ++i)
	{ 
		const event &e = *i; 
		// switch( e.id) // case: 
		os << " capture -> " << e.id << " " << std::string( e.start, e.finish ) << std::endl;	
	} 
}
#endif



