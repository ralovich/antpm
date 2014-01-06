// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                            //
//                                                                      //
// This program is free software; you can redistribute it and/or modify //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation; either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// This program is distributed in the hope that it will be useful,      //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****

#pragma once

#include <memory>

//#define PSO_LAZYSINGLETON_DEBUG 1
#ifdef PSO_LAZYSINGLETON_DEBUG
# include <cstdio>
# define lazySingletonTrace()                                           \
  do                                                                    \
  {                                                                     \
    ::fprintf(stdout, "%s: %s:%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    ::fflush(stdout);                                                   \
  } while(false)
#else // !PSO_LAZYSINGLETON_DEBUG
# define lazySingletonTrace() do { } while(false)
#endif // !PSO_LAZYSINGLETON_DEBUG


namespace antpm
{

  template <class T>
  class ClassInstantiator
  {
    public:
      virtual    ~ClassInstantiator() {}
    protected:
      static /*inline*/ T* instantiate();

      // template < class P1 >
      // static inline T* instantiate(P1 p1);

      template <class T1, class I1> friend class LazySingleton;
  };

  template <class T, class I = ClassInstantiator<T> >
  class LazySingleton
  {
    public:
      static inline T& reference();
      static inline T* pointer();
      static inline T* instance();
    protected:
      inline         LazySingleton();
      virtual inline ~LazySingleton();
    private:
      LazySingleton(const LazySingleton<T>&); // no copy ctor
      const LazySingleton<T>& operator= (const LazySingleton<T>&); // no copy assignment
    private:
      static std::auto_ptr<T> theObject;
  };

  template<class T, class I>
  std::auto_ptr<T> LazySingleton<T, I>::theObject(0);

  template<class T, class I>
  inline T&
  LazySingleton<T, I>::reference()
  {
    lazySingletonTrace();
    return *instance();
  }

  template<class T, class I>
  inline T*
  LazySingleton<T, I>::pointer()
  {
    lazySingletonTrace();
    return theObject.get();
  }

  template<class T, class I>
  inline T*
  LazySingleton<T, I>::instance()
  {
    lazySingletonTrace();
  
    if(!theObject.get())
    {
      //theObject = std::auto_ptr<T>(create<T>());
      theObject = std::auto_ptr<T>(I::instantiate());
    }

    return theObject.get();
  }

  template <class T, class I>
  inline
  LazySingleton<T, I>::LazySingleton()
  {
    lazySingletonTrace();
  }

  template <class T, class I>
  inline
  LazySingleton<T, I>::~LazySingleton()
  {
    lazySingletonTrace();
  }

}



