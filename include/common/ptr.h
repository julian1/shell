#ifndef BOOST_SMART_PTR_PTR_HPP_INCLUDED
#define BOOST_SMART_PTR_PTR_HPP_INCLUDED

//
//  ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ptr.html for documentation.
//

#include <boost/config.hpp>

#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/smart_ptr/detail/sp_convertible.hpp>

#include <boost/config/no_tr1/functional.hpp>           // for std::less

#if !defined(BOOST_NO_IOSTREAM)
#if !defined(BOOST_NO_IOSFWD)
#include <iosfwd>               // for std::basic_ostream
#else
#include <ostream>
#endif
#endif


//namespace boost
//{

//
//  ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//  
//      void ptr_add_ref(T * p);
//      void ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//

template<class T> class ptr
{
private:

    typedef ptr this_type;

public:

    typedef T element_type;

    ptr(): px( 0 )
    {
    }

    ptr( T * p, bool add_ref = true ): px( p )
    {
        if( px != 0 && add_ref ) intrusive_ptr_add_ref( px );
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U>
#if !defined( BOOST_SP_NO_SP_CONVERTIBLE )

    ptr( ptr<U> const & rhs, typename boost::detail::sp_enable_if_convertible<U,T>::type = boost::detail::sp_empty() )

#else

    ptr( ptr<U> const & rhs )

#endif
    : px( rhs.get() )
    {
        if( px != 0 ) intrusive_ptr_add_ref( px );
    }

#endif

    ptr(ptr const & rhs): px( rhs.px )
    {
        if( px != 0 ) intrusive_ptr_add_ref( px );
    }

    ~ptr()
    {
        if( px != 0 ) intrusive_ptr_release( px );
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U> ptr & operator=(ptr<U> const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

#endif

// Move support

#if defined( BOOST_HAS_RVALUE_REFS )

    ptr(ptr && rhs): px( rhs.px )
    {
        rhs.px = 0;
    }

    ptr & operator=(ptr && rhs)
    {
        this_type( static_cast< ptr && >( rhs ) ).swap(*this);
        return *this;
    }

#endif

    ptr & operator=(ptr const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    ptr & operator=(T * rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    void reset()
    {
        this_type().swap( *this );
    }

    void reset( T * rhs )
    {
        this_type( rhs ).swap( *this );
    }

    T * get() const
    {
        return px;
    }

    T & operator*() const
    {
        BOOST_ASSERT( px != 0 );
        return *px;
    }

    T * operator->() const
    {
        BOOST_ASSERT( px != 0 );
        return px;
    }

// implicit conversion to "bool"
#include <boost/smart_ptr/detail/operator_bool.hpp>

    void swap(ptr & rhs)
    {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

private:

    T * px;
};

template<class T, class U> inline bool operator==(ptr<T> const & a, ptr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(ptr<T> const & a, ptr<U> const & b)
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator==(ptr<T> const & a, U * b)
{
    return a.get() == b;
}

template<class T, class U> inline bool operator!=(ptr<T> const & a, U * b)
{
    return a.get() != b;
}

template<class T, class U> inline bool operator==(T * a, ptr<U> const & b)
{
    return a == b.get();
}

template<class T, class U> inline bool operator!=(T * a, ptr<U> const & b)
{
    return a != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template<class T> inline bool operator!=(ptr<T> const & a, ptr<T> const & b)
{
    return a.get() != b.get();
}

#endif

template<class T> inline bool operator<(ptr<T> const & a, ptr<T> const & b)
{
    return std::less<T *>()(a.get(), b.get());
}

template<class T> void swap(ptr<T> & lhs, ptr<T> & rhs)
{
    lhs.swap(rhs);
}

// mem_fn support

template<class T> T * get_pointer(ptr<T> const & p)
{
    return p.get();
}

template<class T, class U> ptr<T> static_pointer_cast(ptr<U> const & p)
{
    return static_cast<T *>(p.get());
}

template<class T, class U> ptr<T> const_pointer_cast(ptr<U> const & p)
{
    return const_cast<T *>(p.get());
}

template<class T, class U> ptr<T> dynamic_pointer_cast(ptr<U> const & p)
{
    return dynamic_cast<T *>(p.get());
}

// operator<<

#if !defined(BOOST_NO_IOSTREAM)

#if defined(BOOST_NO_TEMPLATED_IOSTREAMS) || ( defined(__GNUC__) &&  (__GNUC__ < 3) )

template<class Y> std::ostream & operator<< (std::ostream & os, ptr<Y> const & p)
{
    os << p.get();
    return os;
}

#else

// in STLport's no-iostreams mode no iostream symbols can be used
#ifndef _STLP_NO_IOSTREAMS

# if defined(BOOST_MSVC) && BOOST_WORKAROUND(BOOST_MSVC, < 1300 && __SGI_STL_PORT)
// MSVC6 has problems finding std::basic_ostream through the using declaration in namespace _STL
using std::basic_ostream;
template<class E, class T, class Y> basic_ostream<E, T> & operator<< (basic_ostream<E, T> & os, ptr<Y> const & p)
# else
template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, ptr<Y> const & p)
# endif 
{
    os << p.get();
    return os;
}

#endif // _STLP_NO_IOSTREAMS

#endif // __GNUC__ < 3

#endif // !defined(BOOST_NO_IOSTREAM)

//} // namespace boost




template< class T> 
inline void intrusive_ptr_add_ref( T *p)
{
	p->add_ref(); 
}

template< class T> 
inline void intrusive_ptr_release( T *p)
{
	p->release(); 
}

// no fucking template typedef .... ugghhh....
//template< class T> typedef  ptr< T> ptr< T >;  

/*
template< class T> 
struct ptr : ptr< T> 
{

};
*/

/*
	An object should not manage the lifetime of its dependencies. Instead it should be oblivious
	whether 
		(1) it holds the only instance, or a shared instance. 
		(2) Also whether the depencies lifetime is co-incident or extends beyond with it's own lifetime.  

	In practice it means, that an object should not delete it's deps. Note how this is normal practice
	in c++, with using references & for deps. Where the object just uses the reference without assumptions.	

	Where a ref won't work because there is no fundamental source, then ref counting is the only option.
*/

struct RefCounted
{
	RefCounted()
		: count( 0)
	{ } 

	virtual ~RefCounted() // must be virtual, because delete appears here
	{ }		

	void add_ref() { ++count; } 
	void release() { if( --count == 0) delete this; } 
private:
	unsigned count;
};

// we can do all this just with a simple composition macro. 
// it would a wrapper around the count to initialize it in the constructor


#if 0

/*
	FOR SOME REASON THIS IS NOT WOKRING CORRECTLY IT INTRODUCES ERRORS ???
*/
#define IMPLEMENT_REFCOUNT \
	struct XX { XX() : count( 0) { } unsigned count; } xx;	\
	void add_ref() { ++xx.count; } \
	void release() { if( --xx.count == 0) delete this; }

#endif	


#endif  // #ifndef BOOST_SMART_PTR_PTR_HPP_INCLUDED


