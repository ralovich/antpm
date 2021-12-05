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

# include <memory>


# ifdef _MSC_VER
#  if _MSC_VER == 1600 || _MSC_VER == 1700 // VS2010, VS2012

// smart ptr stuff is already in std::

#  elif _MSC_VER == 1500 && _MSC_FULL_VER >= 150030729 // VS2008 SP1
#   include <boost/static_assert.hpp>

namespace std
{
  using std::tr1::shared_ptr;
  using std::tr1::weak_ptr;

  using std::tr1::static_pointer_cast;
  using std::tr1::const_pointer_cast;
  using std::tr1::dynamic_pointer_cast;
}

#   define static_assert(x) BOOST_STATIC_ASSERT(x)

#  else // lower than VS2008 SP1
#   include <boost/shared_ptr.hpp>
#   include <boost/weak_ptr.hpp>
#   include <boost/static_assert.hpp>

namespace std
{
  using boost::shared_ptr;
  using boost::weak_ptr;

  using boost::static_pointer_cast;
  using boost::const_pointer_cast;
  using boost::dynamic_pointer_cast;
}

#   define static_assert(x) BOOST_STATIC_ASSERT(x)


#  endif
# endif // _MSC_VER

