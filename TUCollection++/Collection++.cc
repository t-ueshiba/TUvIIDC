/*
 *  $Id: Collection++.cc,v 1.2 2002-07-25 02:38:01 ueshiba Exp $
 */
#include "TU/Collection++.h"

namespace TU
{
/************************************************************************
*  class ObjList							*
************************************************************************/
template <class T> ObjList<T>*
ObjList<T>::findnode(T* p) const
{
    ObjList<T>* const	head = (ObjList<T>*)this;
    T* const		tmp  = head->_p;	
    head->_p = p;
    for (ObjList<T>* node = head; node->_next->_p != p; node = node->_next);
    head->_p = tmp;
    return (node->_next == head ? 0 : node);
}

/************************************************************************
*  class ObjDList							*
************************************************************************/
template <class T> ObjDList<T>*
ObjDList<T>::findnode(T* p) const
{
    ObjDList<T>* const	head = (ObjDList<T>*)this;
    T* const		tmp  = head->_p;	
    head->_p = p;
    for (ObjDList<T>* node = head; (node = node->_next)->_p != p; );
    head->_p = tmp;
    return (node->_next == head ? 0 : node);
}
 
}
