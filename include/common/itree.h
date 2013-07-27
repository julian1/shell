// Interval RB tree implementation -*- C++ -*-

/*
Generic Interval Tree Map
Copyright (C) 2006-2007 Keio University
(Kris Popendorf) <comp@bio.keio.ac.jp> (2007)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

This is an implementation of Interval Trees as described by Cormen in
"Introduction to Algorithms" implemented using the GCC Red/Black Tree implementation
for the ugly RB Tree insertion/deletion.
*/

#ifndef __ITREE_H_
#define __ITREE_H_

#include <utility>
#include <assert.h>
#include <iostream>

// ok we need to factorize our interval type out ...
	// somehow ...
	// so that the main tree takes the Interval as a structure
	// rather than taking the underlying type. 
	// except that the main tree is accessing start and stop
	// all the time ...

	// we allow access to the start and stop variables.  



/*
	ok we try and parametize all the iterators and the inodes by 
	key and value. 
*/

namespace Itree {
  using namespace std;



  enum _Rb_tree_color { _S_red = false, _S_black = true };


	template<typename KeyType>
	inline bool overlaps( const KeyType &y, const KeyType & x) 
	{ 
			//should be stated in terms of < only
		  return (!(x.stop<y.start) && !(y.stop<x.start));
	}




  template< typename KeyType, typename _Value>
  class itree_node {
  public:

	// possibly something accesses this from outside	
	typedef typename KeyType::Underlying_type	Underlying_type;	
   
	typedef KeyType		key_type;
    typedef _Value		value_type;


    typedef itree_node<KeyType,_Value> _Base_type;
    typedef itree_node<KeyType,_Value> node_type;

    typedef _Base_type* _Base_ptr;
    typedef pair< key_type,_Value> init_type;

    key_type key;
    value_type val; 

	// augmented type
    Underlying_type max; //contains the max
    
    _Rb_tree_color	_M_color;
    _Base_ptr		_M_parent;
    _Base_ptr		_M_left;
    _Base_ptr		_M_right;

/*
	static bool overlaps( const KeyType &y, const KeyType & x) 
	{ 
			//should be stated in terms of < only
		  return (!(x.stop<y.start) && !(y.stop<x.start));
	}
*/


    itree_node(const itree_node& a) :
      key(a.key),val(a.val),max(a.key.stop),
      _M_color(a._M_color),_M_parent(a._M_parent),_M_left(0),_M_right(0)
    { }

    itree_node(const init_type& a) :
      key(a.first),val(a.second),max(a.first.stop),
      _M_color(),_M_parent(0),_M_left(0),_M_right(0)
    { }

    itree_node() :
      key(),val(),max(),
      _M_color(),_M_parent(0),_M_left(0),_M_right(0)
    { }

    static node_type*
    itree_increment(node_type* __x)
    {
      if (__x->_M_right != 0) 
	{
	  __x = __x->_M_right;
	  while (__x->_M_left != 0)
	    __x = __x->_M_left;
	}
      else 
	{
	  node_type* __y = __x->_M_parent;
	  while (__x == __y->_M_right) 
	    {
	      __x = __y;
	      __y = __y->_M_parent;
	    }
	  if (__x->_M_right != __y)
	    __x = __y;
	}
      return __x;
    }

    static const node_type*
    itree_increment(const node_type* __x)
    {
      return itree_increment(const_cast<node_type*>(__x));
    }
    
    static node_type*
    itree_decrement(node_type* __x)
    {
      if (__x->_M_color == _S_red 
	  && __x->_M_parent->_M_parent == __x)
	__x = __x->_M_right;
      else if (__x->_M_left != 0) 
	{
	  node_type* __y = __x->_M_left;
	  while (__y->_M_right != 0)
	    __y = __y->_M_right;
	  __x = __y;
	}
      else 
	{
	  node_type* __y = __x->_M_parent;
	  while (__x == __y->_M_left) 
	    {
	      __x = __y;
	      __y = __y->_M_parent;
	    }
	  __x = __y;
	}
      return __x;
    }
    
    static const node_type*
    itree_decrement(const node_type* __x)
    {
      return itree_decrement(const_cast<node_type*>(__x));
    }

    static _Base_ptr
    _S_minimum(_Base_ptr __x)
    {
      while (__x->_M_left != 0) __x = __x->_M_left;
      return __x;
    }

    static _Base_ptr
    _S_maximum(_Base_ptr __x)
    {
      while (__x->_M_right != 0) __x = __x->_M_right;
      return __x;
    }
  };



  template<typename KeyType,typename _Value>
  struct itree_iterator
  {
    typedef _Value  value_type;
    typedef _Value& reference;
    typedef _Value* pointer;

   typedef KeyType key_type;
    typedef KeyType & key_reference;
    typedef KeyType  * key_pointer;



    typedef bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t                  difference_type;

    typedef itree_iterator<KeyType,_Value> _Self;
  
	 typedef typename itree_node<KeyType,_Value>::_Base_ptr           _Base_ptr;
    typedef itree_node<KeyType,_Value>*    _Link_type;
    typedef itree_node<KeyType,_Value>    node_type;


    itree_iterator()
      : _M_node() { }

    explicit
    itree_iterator(_Link_type __x)
      : _M_node(__x) { }

    key_reference
    key() const
    { return static_cast<_Link_type>(_M_node)->key; }

    reference
    operator*() const
    { return static_cast<_Link_type>(_M_node)->val; }

    pointer
    operator->() const
    { return &static_cast<_Link_type>(_M_node)->val; }

    _Self&
    operator++()
    {
      _M_node = node_type::itree_increment(_M_node);
      return *this;
    }

    _Self
    operator++(int)
    {
      _Self __tmp = *this;
      _M_node = node_type::itree_increment(_M_node);
      return __tmp;
    }

    _Self&
    operator--()
    {
      _M_node = node_type::itree_decrement(_M_node);
      return *this;
    }

    _Self
    operator--(int)
    {
      _Self __tmp = *this;
      _M_node = node_type::itree_decrement(_M_node);
      return __tmp;
    }

    bool
    operator==(const _Self& __x) const
    { return _M_node == __x._M_node; }

    bool
    operator!=(const _Self& __x) const
    { return _M_node != __x._M_node; }

    _Base_ptr _M_node;
  };




  template<typename KeyType,typename _Value>
  struct itree_range_iterator
  {
    typedef _Value  value_type;
    typedef _Value& reference;
    typedef _Value* pointer;

	typedef KeyType			key_type;
    typedef KeyType			& key_reference;
    typedef KeyType			* key_pointer;


    typedef bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t                  difference_type;

   typedef itree_range_iterator<KeyType,_Value> _Self;
    typedef itree_iterator<KeyType,_Value> iterator;
    typedef typename itree_node<KeyType,_Value>::_Base_ptr           _Base_ptr;
    typedef itree_node<KeyType,_Value>*    _Link_type;
    typedef itree_node<KeyType,_Value>    node_type;


    itree_range_iterator()
      : _M_node(),target() { }

    explicit
    itree_range_iterator(iterator __x,key_type key,_Base_ptr header)
      : _M_node(__x._M_node),target(key),_M_header(header) { }

    key_reference
    key() const
    { return static_cast<_Link_type>(_M_node)->key; }

    reference
    operator*() const
    { return static_cast<_Link_type>(_M_node)->val; }

    pointer
    operator->() const
    { return &static_cast<_Link_type>(_M_node)->val; }

    _Self&
    operator++()
    {
      _M_node = range_next_in(_M_node);
      return *this;
    }

    _Self
    operator++(int)
    {
      _Self __tmp = *this;
      _M_node = range_next_in(_M_node);
      return __tmp;
    }

    bool
    operator==(const _Self& __x) const
    { return (_M_node == __x._M_node && target==__x.target); }

    bool
    operator!=(const _Self& __x) const
    { return _M_node != __x._M_node || target!=__x.target; }

    bool
    operator==(const iterator& __x) const
    { return _M_node == __x._M_node; }

    bool
    operator!=(const iterator& __x) const
    { return _M_node != __x._M_node; }

    _Base_ptr range_next_in(_Base_ptr after)
    {
      _Base_ptr __x = node_type::itree_increment(after);
      _Base_ptr __z = _M_header;
      while(__x != _M_header && !((target.stop)<(__x->key.start))){ //mmm O(log n)

//	if(target.overlaps(__x->key)){
	if( overlaps( target, __x->key)){
	  __z=__x;
	  break;
	}

	//a sneaky modified increment. can skip left traversals if max is < target.start
	if (__x->_M_right != 0) 
	  {
	    __x = __x->_M_right;
	    while (__x->_M_left != 0 && !((__x->_M_left->max)<(target.start)))
	      __x = __x->_M_left;
	  }
	else 
	  {
	    node_type* __y = __x->_M_parent;
	    while (__x == __y->_M_right) 
	      {
		__x = __y;
		__y = __y->_M_parent;
	      }
	    if (__x->_M_right != __y)
	      __x = __y;
	  }
      }
      return __z;
    }

    _Base_ptr _M_node;
    key_type target;
    _Base_ptr _M_header;
  };
 



 
  template<typename KeyType,typename _Value,
		typename _Alloc = std::allocator<itree_node< KeyType,_Value> > >
  class itree {
  public:

	typedef typename KeyType::Underlying_type  Underlying_type;
    
	typedef itree< KeyType,_Value>  _Self;

    typedef itree_node< KeyType,_Value> node_type;

    typedef size_t size_type;
    typedef KeyType		 key_type;
    typedef _Value value_type;
    typedef pair<key_type,value_type> init_type;
    
    typedef typename _Alloc::template rebind<node_type >::other
    _Node_allocator;
    typedef _Alloc allocator_type;
 

    typedef itree_iterator<KeyType,_Value> iterator;
    typedef itree_range_iterator<KeyType,_Value> range_iterator;
    typedef itree_node<KeyType,_Value>* _Link_type;
    typedef const itree_node<KeyType,_Value>* _Const_Link_type;
 

  protected:
  //////////////////////
  // in normal gcc RB trees this is precompiled
  ////////////////
  inline void itree_reset_max(node_type* __x){
    __x->max=__x->key.stop;
    if(__x->_M_left && (__x->max) < (__x->_M_left->max))
      __x->max=__x->_M_left->max;
    if(__x->_M_right && (__x->max) < (__x->_M_right->max))
      __x->max=__x->_M_right->max;
  }

  inline void itree_reset_max(node_type* __x,node_type* __z){ //z is to be ignored
    __x->max=__x->key.stop;
    if(__x->_M_left && __x->_M_left!=__z && (__x->max) < (__x->_M_left->max))
      __x->max=__x->_M_left->max;
    if(__x->_M_right && __x->_M_right!=__z && (__x->max) < (__x->_M_right->max))
      __x->max=__x->_M_right->max;
  }


  void 
  itree_rotate_left(node_type* const __x, 
		    node_type*& __root)
  {
    node_type* const __y = __x->_M_right;

    __x->_M_right = __y->_M_left;
    if (__y->_M_left !=0)
      __y->_M_left->_M_parent = __x;
    __y->_M_parent = __x->_M_parent;
    
    if (__x == __root)
      __root = __y;
    else if (__x == __x->_M_parent->_M_left)
      __x->_M_parent->_M_left = __y;
    else
      __x->_M_parent->_M_right = __y;
    __y->_M_left = __x;
    __x->_M_parent = __y;

    itree_reset_max(__x);
    itree_reset_max(__y);
  }

  void 
  itree_rotate_right(node_type* const __x, 
		     node_type*& __root)
  {
    node_type* const __y = __x->_M_left;

    __x->_M_left = __y->_M_right;
    if (__y->_M_right != 0)
      __y->_M_right->_M_parent = __x;
    __y->_M_parent = __x->_M_parent;

    if (__x == __root)
      __root = __y;
    else if (__x == __x->_M_parent->_M_right)
      __x->_M_parent->_M_right = __y;
    else
      __x->_M_parent->_M_left = __y;
    __y->_M_right = __x;
    __x->_M_parent = __y;

    itree_reset_max(__x);
    itree_reset_max(__y);
  }

  void 
  itree_insert_and_rebalance(const bool          __insert_left,
			     node_type* __x,
			     node_type* __p,
			     node_type& __header)
  {
    node_type *& __root = __header._M_parent;

    // Initialize fields in new node to insert.
    __x->_M_parent = __p;
    __x->_M_left = 0;
    __x->_M_right = 0;
    __x->_M_color = _S_red;

    // Insert.
    // Make new node child of parent and maintain root, leftmost and
    // rightmost nodes.
    // N.B. First node is always inserted left.
    if (__insert_left)
      {
        __p->_M_left = __x; // also makes leftmost = __x when __p == &__header

        if (__p == &__header)
	  {
            __header._M_parent = __x;
            __header._M_right = __x;
	  }
        else if (__p == __header._M_left)
          __header._M_left = __x; // maintain leftmost pointing to min node
      }
    else
      {
        __p->_M_right = __x;

        if (__p == __header._M_right)
          __header._M_right = __x; // maintain rightmost pointing to max node
      }

    if(&__header!=__p)
      itree_reset_max(__p);

    // Rebalance.
    while (__x != __root 
	   && __x->_M_parent->_M_color == _S_red) 
      {
	node_type* const __xpp = __x->_M_parent->_M_parent;

	if (__x->_M_parent == __xpp->_M_left) 
	  {
	    node_type* const __y = __xpp->_M_right;
	    if (__y && __y->_M_color == _S_red) 
	      {
		__x->_M_parent->_M_color = _S_black;
		__y->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		__x = __xpp;
	      }
	    else 
	      {
		if (__x == __x->_M_parent->_M_right) 
		  {
		    __x = __x->_M_parent;
		    itree_rotate_left(__x, __root);
		  }
		__x->_M_parent->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		itree_rotate_right(__xpp, __root);
	      }
	  }
	else 
	  {
	    node_type* const __y = __xpp->_M_left;
	    if (__y && __y->_M_color == _S_red) 
	      {
		__x->_M_parent->_M_color = _S_black;
		__y->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		__x = __xpp;
	      }
	    else 
	      {
		if (__x == __x->_M_parent->_M_left) 
		  {
		    __x = __x->_M_parent;
		    itree_rotate_right(__x, __root);
		  }
		__x->_M_parent->_M_color = _S_black;
		__xpp->_M_color = _S_red;
		itree_rotate_left(__xpp, __root);
	      }
	  }
      }
    __root->_M_color = _S_black;
  }

  node_type*
  itree_rebalance_for_erase(node_type* const __z, 
			    node_type& __header)
  {
    node_type *& __root = __header._M_parent;
    node_type *& __leftmost = __header._M_left;
    node_type *& __rightmost = __header._M_right;
    node_type* __y = __z;
    node_type* __x = 0;
    node_type* __x_parent = 0;

    if (__y->_M_left == 0)     // __z has at most one non-null child. y == z.
      __x = __y->_M_right;     // __x might be null.
    else
      if (__y->_M_right == 0)  // __z has exactly one non-null child. y == z.
	__x = __y->_M_left;    // __x is not null.
      else 
	{
	  // __z has two non-null children.  Set __y to
	  __y = __y->_M_right;   //   __z's successor.  __x might be null.
	  while (__y->_M_left != 0)
	    __y = __y->_M_left;
	  __x = __y->_M_right;
	}
    if (__y != __z) 
      {
	// relink y in place of z.  y is z's successor
	
	__z->_M_left->_M_parent = __y; 
	__y->_M_left = __z->_M_left;
	if (__y != __z->_M_right) 
	  {
	    __x_parent = __y->_M_parent;
	    if (__x) __x->_M_parent = __y->_M_parent;
	    __y->_M_parent->_M_left = __x;   // __y must be a child of _M_left
	    __y->_M_right = __z->_M_right;
	    __z->_M_right->_M_parent = __y;
	  }
	else
	  __x_parent = __y;
	if (__root == __z)
	  __root = __y;
	else if (__z->_M_parent->_M_left == __z)
	  __z->_M_parent->_M_left = __y;
	else 
	  __z->_M_parent->_M_right = __y;
	_Link_type __old_y_parent=__y->_M_parent;
	__y->_M_parent = __z->_M_parent;
	std::swap(__y->_M_color, __z->_M_color);

	itree_reset_max(__x_parent);
	itree_reset_max(__old_y_parent);
	_M_update_parents(__old_y_parent);

	__y = __z;
	// __y now points to node to be actually deleted
      }
    else 
      {                        // __y == __z
	__x_parent = __y->_M_parent;
	if (__x) 
	  __x->_M_parent = __y->_M_parent;   
	if (__root == __z)
	  __root = __x;
	else 
	  if (__z->_M_parent->_M_left == __z)
	    __z->_M_parent->_M_left = __x;
	  else
	    __z->_M_parent->_M_right = __x;
	if (__leftmost == __z) {
	  if (__z->_M_right == 0)        // __z->_M_left must be null also
	    __leftmost = __z->_M_parent;
	// makes __leftmost == _M_header if __z == __root
	  else
	    __leftmost = node_type::_S_minimum(__x);
	}
	if (__rightmost == __z) {
	  if (__z->_M_left == 0)         // __z->_M_right must be null also
	    __rightmost = __z->_M_parent;  
	// makes __rightmost == _M_header if __z == __root
	  else                      // __x == __z->_M_left
	    __rightmost = node_type::_S_maximum(__x);
	}
	if(__x){
	  itree_reset_max(__x);
	  _M_update_parents(__x);
	}else{
	  itree_reset_max(__x_parent);
	  _M_update_parents(__x_parent);
	}
      }
    if (__y->_M_color != _S_red) 
      { 
	while (__x != __root && (__x == 0 || __x->_M_color == _S_black))
	  if (__x == __x_parent->_M_left) 
	    {
	      node_type* __w = __x_parent->_M_right;
	      if (__w->_M_color == _S_red) 
		{
		  __w->_M_color = _S_black;
		  __x_parent->_M_color = _S_red;
		  itree_rotate_left(__x_parent, __root);
		  __w = __x_parent->_M_right;
		}
	      if ((__w->_M_left == 0 || 
		   __w->_M_left->_M_color == _S_black) &&
		  (__w->_M_right == 0 || 
		   __w->_M_right->_M_color == _S_black)) 
		{
		  __w->_M_color = _S_red;
		  __x = __x_parent;
		  __x_parent = __x_parent->_M_parent;
		} 
	      else 
		{
		  if (__w->_M_right == 0 
		      || __w->_M_right->_M_color == _S_black) 
		    {
		      __w->_M_left->_M_color = _S_black;
		      __w->_M_color = _S_red;
		      itree_rotate_right(__w, __root);
		      __w = __x_parent->_M_right;
		    }
		  __w->_M_color = __x_parent->_M_color;
		  __x_parent->_M_color = _S_black;
		  if (__w->_M_right) 
		    __w->_M_right->_M_color = _S_black;
		  itree_rotate_left(__x_parent, __root);
		  break;
		}
	    } 
	  else 
	    {   
	      // same as above, with _M_right <-> _M_left.
	      node_type* __w = __x_parent->_M_left;
	      if (__w->_M_color == _S_red) 
		{
		  __w->_M_color = _S_black;
		  __x_parent->_M_color = _S_red;
		  itree_rotate_right(__x_parent, __root);
		  __w = __x_parent->_M_left;
		}
	      if ((__w->_M_right == 0 || 
		   __w->_M_right->_M_color == _S_black) &&
		  (__w->_M_left == 0 || 
		   __w->_M_left->_M_color == _S_black)) 
		{
		  __w->_M_color = _S_red;
		  __x = __x_parent;
		  __x_parent = __x_parent->_M_parent;
		} 
	      else 
		{
		  if (__w->_M_left == 0 || __w->_M_left->_M_color == _S_black) 
		    {
		      __w->_M_right->_M_color = _S_black;
		      __w->_M_color = _S_red;
		      itree_rotate_left(__w, __root);
		      __w = __x_parent->_M_left;
		    }
		  __w->_M_color = __x_parent->_M_color;
		  __x_parent->_M_color = _S_black;
		  if (__w->_M_left) 
		    __w->_M_left->_M_color = _S_black;
		  itree_rotate_right(__x_parent, __root);
		  break;
		}
	    }
	if (__x) __x->_M_color = _S_black;
      }

    return __y;
  }

  unsigned int
  itree_black_count(const node_type* __node,
		    const node_type* __root)
  {
    if (__node == 0)
      return 0;
    unsigned int __sum = 0;
    do 
      {
	if (__node->_M_color == _S_black) 
	  ++__sum;
	if (__node == __root) 
	  break;
	__node = __node->_M_parent;
      } 
    while (1);
    return __sum;
  }
    //////////////////////////////////////////////
    /////////////////////end of precompiled stuff
    ////////////////////////////////////////////

    node_type _M_header;
    size_type _M_node_count;
    allocator_type alloc;
    
    allocator_type get_allocator() const
    { return alloc; }
  
    node_type*
    _M_get_node()
    { return alloc.allocate(1); }
  
    void
    _M_put_node(node_type* __p)
    { alloc.deallocate(__p, 1); }
  
    _Link_type
    _M_create_node(const init_type& __x)
    {
      _Link_type __tmp = _M_get_node();
      try
	{ __tmp->key=__x.first;
	  __tmp->val=__x.second;
	  __tmp->max=__x.first.stop;
	}
      catch(...)
	{
	  _M_put_node(__tmp);
	  __throw_exception_again;
	}
      return __tmp;
    }
  
    _Link_type
    _M_clone_node(_Const_Link_type __x)
    {
      _Link_type __tmp = _M_create_node(*__x);
      return __tmp;
    }
  
    static const key_type&
    _S_key(_Const_Link_type __x)
    { return __x->key; }

	// note this is the only underlying type accessor 
    static const Underlying_type &
    _S_max(_Const_Link_type __x)
    { return __x->max; }
  
    void
    destroy_node(_Link_type __p)
    {
      alloc.destroy(__p);
      _M_put_node(__p);
    }
  
    _Link_type
    _M_begin()
    { return static_cast<_Link_type>(this->_M_header._M_parent); }
  
    _Link_type
    _M_end()
    { return static_cast<_Link_type>(&this->_M_header); }
    
    _Link_type&
    _M_root()
    { return this->_M_header._M_parent; }

    _Link_type&
    _M_leftmost()
    { return this->_M_header._M_left; }

    static _Link_type
    _S_left(_Link_type __x)
    { return static_cast<_Link_type>(__x->_M_left); }
  
    static _Link_type
    _S_right(_Link_type __x)
    { return static_cast<_Link_type>(__x->_M_right); }
  
    _Link_type&
    _M_rightmost()
    { return this->_M_header._M_right; }

    static _Link_type
    _S_minimum(_Link_type __x)
    {
      while (__x->_M_left != 0) __x = __x->_M_left;
      return __x;
    }
  
    static _Link_type
    _S_maximum(_Link_type __x)
    {
      while (__x->_M_right != 0) __x = __x->_M_right;
      return __x;
    }
  
    void _M_erase(_Link_type __x)
    {
      // Erase without rebalancing. - only used in destructor/clear .
      while (__x != 0)
	{
	  _M_erase(_S_right(__x));
	  _Link_type __y = _S_left(__x);
	  destroy_node(__x);
	  __x = __y;
	}
    }
    
    bool _M_key_compare(const key_type& a,const key_type& b){
      return a.start < b.start;
    }

    bool _M_stop_start_compare(const key_type& a,const key_type& b){
      return (a.stop<b.start);
    }

    bool _M_max_compare(const key_type& key,_Link_type __x){
      return (key.stop<_S_max(__x));
    }

    bool _M_edge_max_compare(const key_type& key,_Link_type __x){
      return (_S_max(__x)<key.start);
    }

    iterator
    _M_insert(_Link_type __x, _Link_type __p, const key_type& __k, const value_type& __v)
    {
      bool __insert_left = (__x != 0 || __p == _M_end()
			    || _M_key_compare(__k, 
					      _S_key(__p)));
      
      _Link_type __z = _M_create_node(init_type(__k,__v));
      
      itree_insert_and_rebalance(__insert_left, __z, __p,  
				  this->_M_header);
      _M_update_parents(__z);
      ++_M_node_count;
#ifdef DEBUG_ITREE
      assert(verify());
#endif
      return iterator(__z);
    }

    void _M_update_parents(_Link_type __x,_Link_type __ignore){
#ifdef DEBUG_ITREE
      assert(__x);
#endif
      __x=__x->_M_parent;
      itree_reset_max(__x,__ignore);

      _Link_type prev=__x;
      for(__x=__x->_M_parent;__x && __x!=&_M_header;__x=__x->_M_parent){
	itree_reset_max(__x);
      }
    }

    void _M_update_parents(_Link_type __x){
#ifdef DEBUG_ITREE
      assert(__x);
#endif
      for(__x=__x->_M_parent;__x && __x!=&_M_header;__x=__x->_M_parent){
	itree_reset_max(__x);
      }
    }

  public:
    iterator
    begin()
    { return iterator(this->_M_header._M_left); }

    iterator
    end()
    { return iterator(&this->_M_header); }

    itree() :
      _M_header(),_M_node_count(0),alloc()
    {
      this->_M_header._M_color = _S_red;
      this->_M_header._M_parent = 0;
      this->_M_header._M_left = &this->_M_header;
      this->_M_header._M_right = &this->_M_header;
    }
  
    ~itree()
    { _M_erase(_M_begin()); }
  
  
    size_t size() const{
      return _M_node_count;
    }

    bool empty() const{
      return !_M_node_count;
    }
  
    iterator insert(const key_type& key,const value_type& val){
      //add stuff
      _Link_type __x = _M_begin();
      _Link_type __y = _M_end();
      while (__x != 0)
	{
	  __y = __x;
	  __x = _M_key_compare(key, _S_key(__x)) ?
	    _S_left(__x) : _S_right(__x);
	}
      return _M_insert(__x, __y, key, val);
    }
  
    void
    clear()
    {
      _M_erase(_M_begin());
      _M_leftmost() = _M_end();
      _M_root() = 0;
      _M_rightmost() = _M_end();
      _M_node_count = 0;
    }
  
    inline void erase(iterator& __position){
      //kill it
      _Link_type __y =
	static_cast<_Link_type>(itree_rebalance_for_erase
				(__position._M_node,
				 this->_M_header));
      destroy_node(__y);
      --_M_node_count;
#ifdef DEBUG_ITREE
      assert(verify());
#endif
    }

    inline void erase( const range_iterator& __position){
		// JA added
//		__position._M_node; 
//		__position._M_header;

      //kill it
      _Link_type __y =
	static_cast<_Link_type>(itree_rebalance_for_erase
				(__position._M_node,
				 //this->_M_header));		// JA sure we shouldnt be using __position._M_header ?
				 this->_M_header));
      destroy_node(__y);
      --_M_node_count;

#ifdef DEBUG_ITREE
      assert(verify());
#endif
    }




    iterator lower_bound(const key_type& key){
      //find first value not less than key
      _Link_type __x = _M_begin(); // Current node.
      _Link_type __y = _M_end(); // Last node which is not less than __k.
    
      while (__x != 0)
	if (_M_edge_max_compare(key,__x))
	  __x = _S_right(__x);
	else
	  __y = __x, __x = _S_left(__x);
    
      if(_M_end() == __y)
	return iterator(__y);
      //if(key.overlaps(__y->key)){
      if( overlaps( key, __y->key)){
	return iterator(__y);
      }
      return next_in(key,__y);
    }

    inline iterator next_in(const key_type& key,iterator after){
      _Link_type tmp=after._M_node;
      return next_in(key,tmp);
    }
    
    iterator next_in(const key_type& key,_Link_type after){
      _Link_type __x = node_type::itree_increment(after);
      _Link_type __z = _M_end();
      while(__x != _M_end() && !_M_stop_start_compare(key,_S_key(__x))){ //mmm O(log n)
//	if(key.overlaps(__x->key)){
	if( overlaps( key, __x->key)){
	  __z=__x;
	  break;
	}

	//a sneaky modified increment. can skip left traversals if max is < key.start
	if (__x->_M_right != 0) 
	  {
	    __x = __x->_M_right;
	    while (__x->_M_left != 0 && !((__x->_M_left->max)<(key.start)))
	      __x = __x->_M_left;
	  }
	else 
	  {
	    node_type* __y = __x->_M_parent;
	    while (__x == __y->_M_right) 
	      {
		__x = __y;
		__y = __y->_M_parent;
	      }
	    if (__x->_M_right != __y)
	      __x = __y;
	  }
      }
      return iterator(__z);
    }
  
    iterator upper_bound(const key_type& key){
      //find first value a where key<a
      _Link_type __x = _M_begin(); // Current node.
      _Link_type __y = _M_end(); // Last node which is greater than __k.
    
      while (__x != 0)
	if (_M_stop_start_compare(key, _S_key(__x)))
	  __y = __x, __x = _S_left(__x);
	else
	  __x = _S_right(__x);
    
      return iterator(__y);
    }
  
    pair<iterator,iterator> equal_range(const key_type& key){
      return pair<iterator,iterator>(lower_bound(key),upper_bound(key));
    }

    range_iterator in_range(const key_type& key){
      return range_iterator(lower_bound(key),key,&_M_header);
    }
  
    std::ostream& drawTree(std::ostream& os){
      return drawSubTree(os,_M_root());
    }

	// the question is - whether deleting exact nodes is now
	// not o( log n). 



	iterator find( const key_type &key)
	{
		return find( key, _M_root());
	}

	iterator find( const key_type& key, _Link_type __x)
	{
/*
	rare instances when both not get the left most instance ???
*/
		// hmmm - this isnt looking very tail recursive. 
		// transform some most of this into a loop - and perhaps only embed 
		// the iterator as the last step in a different function 
		if( __x) { 


			if( key.start < __x->key.start) 
				return find( key, __x->_M_left);
			
			else if( __x->key.start < key.start ) 
				return find( key, __x->_M_right);

			// we have a match for start

			// if havent got the stop - must try both left and right nodes
			// might be able to do a further limit using max ?
			else if( key.stop < __x->key.stop
				|| __x->key.stop < key.stop)  
			{
				// no this part is ok ...
//				std::cout << "going again" << std::endl;
				iterator x = find( key, __x->_M_left);
				if( x != end()) return x;   // may not be the left most ??	
				return find( key, __x->_M_right);
			}	

			// if we have identical elements ---- then returning here will return 
			// the first instance at a higher node - BUT we want to make sure we return the
			// left most instance   
			iterator x = find( key, __x->_M_left);
			if( x != end()) return x;   // may not be the left most ??	
	
			// should be expressed in terms 
			return iterator( __x);				
		}
		
		else {	
			return  end() ;
		}
	}


#if 0
	iterator find( const key_type &key)
	{
		_Link_type __x = find( key, _M_root());
		if( __x) return iterator( __x);
		return end();

//		if( __x);
//		__x = NULL;
	}

	_Link_type find( const key_type& key, _Link_type __x)
	{
/*
	rare instances when both not get the left most instance ???
	non recursive is better - for large sets of data
*/
		// hmmm - this isnt looking very tail recursive. 
		// transform some most of this into a loop - and perhaps only embed 
		// the iterator as the last step in a different function 
		while( __x) { 

			if( key.start < __x->key.start)  {
				__x = find( key, __x->_M_left);
				continue;	
			}
	
			else if( __x->key.start < key.start ) {
				__x = find( key, __x->_M_right);
				continue;	
			}
			// we have a match for start

			// if havent got the stop - must try both left and right nodes
			// might be able to do a further limit using max ?
			else if( key.stop < __x->key.stop
				|| __x->key.stop < key.stop)  
			{
				// no this part is ok ...
				_Link_type	x = find( key, __x->_M_left);
				if( x) return x;   // may not be the left most ??	
				__x = find( key, __x->_M_right);
				continue;
			}	

			// if we have identical elements ---- then returning here will return 
			// the first instance at a higher node - BUT we want to make sure we return the
			// left most instance   
			_Link_type x = find( key, __x->_M_left);
			if( x) return x;   // may not be the left most ??	
	
			// should be expressed in terms 
			return  __x;				
		}
	
		return __x ;
	}
#endif
    std::ostream& drawSubTree( std::ostream& os,_Link_type __x, size_t level = 0){
		// JA
		os << "\n";
		for( size_t i = 0; i < level; ++i)
			os << "   " ;

		// all right it should be fairly easy to recurse on the key ???
	
      if(__x){

//		os << "(" << __x->key.start << ")"; 
		//os << "(";
		// where is the + comming from - the link type ???

		// how do we get access to the link type data ... or do we even have to ??

		os <<  *__x ;
		drawSubTree(os,__x->_M_left, level + 1);
		drawSubTree(os,__x->_M_right, level + 1);
		//os << ")";
//		os << "\n";
      }else{
			os << "#";
      }
      return os;
    }

    bool verify(){
      //      std::cerr << "Verifying: " << *this<<endl;
      if (_M_node_count == 0 || begin() == end())
	return _M_node_count == 0 && begin() == end()
	       && this->_M_header._M_left == _M_end()
	       && this->_M_header._M_right == _M_end();

      unsigned int __len = _itree_black_count(_M_leftmost(), _M_root());
      for (iterator __it = begin(); __it != end(); ++__it)
	{
	  _Link_type __x = __it._M_node;
	  _Link_type __L = _S_left(__x);
	  _Link_type __R = _S_right(__x);

	  if (__x->_M_color == _S_red){
	    if ((__L && __L->_M_color == _S_red)
		|| (__R && __R->_M_color == _S_red)){
	      std::cerr << "** red node children both black failed"<<endl;
	      return false;
	    }
	  }
	  
	  if(__L && _S_max(__x)<_S_max(__L)){
	    std::cerr << "** max wrong from left"<<endl;
	    return false;
	  }
	  if(__R && _S_max(__x)<_S_max(__R)){
	    std::cerr<< "** max wrong from right"<<endl;
	    return false;
	  }

	  if (__L && _M_key_compare(_S_key(__x), _S_key(__L))){
	    std::cerr << "** Children out of order (left)"<<endl;
	    return false;
	  }
	  if (__R && _M_key_compare(_S_key(__R), _S_key(__x))){
	    std::cerr << "** Children out of order (right)"<<endl;
	    return false;
	  }

	  if (!__L && !__R && itree_black_count(__x, _M_root()) != __len){
	    std::cerr << "** black count != len"<<endl;
	    return false;
	  }
	}

      if (_M_leftmost() != _S_minimum(_M_root())){
	std::cerr << "** leftmost wrong."<<endl;
	return false;
      }
      if (_M_rightmost() != _S_maximum(_M_root())){
	std::cerr << "** rightmost wrong."<<endl;
	return false;
      }
      return true;
    }

    unsigned int
    _itree_black_count(const node_type* __node,
		       const node_type* __root)
    {
      if (__node == 0)
	return 0;
      unsigned int __sum = 0;
      do 
	{
	  if (__node->_M_color == _S_black) 
	    ++__sum;
	  if (__node == __root) 
	    break;
	  __node = __node->_M_parent;
	} 
      while (1);
      return __sum;
    }

  };

  template<typename KeyType,typename _Value,typename _Alloc>
  std::ostream& operator<<(std::ostream& os,itree<KeyType,_Value,_Alloc>& t){
    return t.drawTree(os);
  }
  
  template<typename KeyType,typename _Value>
  std::ostream& operator<<(std::ostream& os,const itree_node<KeyType,_Value>& t){
    return os << ((t._M_color == _S_red) ? "+":"-") << (t.key) << "%" << (t.max);
  }

}

#endif
