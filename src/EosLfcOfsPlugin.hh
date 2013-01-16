// ----------------------------------------------------------------------
// File: XrdFstOfs.hh
// Author: Elvin-Alin Sindrilaru - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

#ifndef __EOS_EOSLFCOFSPLUGIN_HH__
#define __EOS_EOSLFCOFSPLUGIN_HH__

/*----------------------------------------------------------------------------*/
#include "XrdOfs/XrdOfs.hh"
#include <string>
/*----------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//! Class EosLfcOfsPlugin built on top of XrdOfs
//------------------------------------------------------------------------------
class EosLfcOfsPlugin: public XrdOfs
{
 public:

  //----------------------------------------------------------------------------
  //! Constuctor
  //----------------------------------------------------------------------------
  EosLfcOfsPlugin();


  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  virtual ~EosLfcOfsPlugin();

 
  //----------------------------------------------------------------------------
  //! Stat function
  //----------------------------------------------------------------------------
  int stat( const char*             path,
            struct stat*            buf,
            XrdOucErrInfo&          out_error,
            const XrdSecEntity*     client,
            const char*             opaque = 0 );

 private:

  std::string mMetaMgrHost; ///< manager address to which requests are redirected if
                            ///< they can not be found in EOS
  
};

//------------------------------------------------------------------------------
//! Global OFS object
//------------------------------------------------------------------------------
extern EosLfcOfsPlugin* gOFS;


#endif //__EOS_EOSLFCOFSPLUGIN_HH__







