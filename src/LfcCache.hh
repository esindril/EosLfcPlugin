//------------------------------------------------------------------------------
// File: LfcCache.hh
// Author: Elvin-Alin Sindrilaru - CERN
//------------------------------------------------------------------------------

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

#ifndef __EOS_PLUGIN_LFCCACHE_HH__
#define __EOS_PLUGIN_LFCCACHE_HH__

/*----------------------------------------------------------------------------*/
#include <XrdSys/XrdSysPthread.hh>
#include <string>
#include <map>
#include <list>
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/


//------------------------------------------------------------------------------
//! Simple cache for the LFC entries
//------------------------------------------------------------------------------
class LfcCache
{
  public:

    typedef std::list< std::pair<std::string, time_t> > ListType;
    typedef std::map<std::string, std::pair< std::string, ListType::iterator > > MapType;

    //----------------------------------------------------------------------------
    //! Constructor
    //!
    //! @param cacheTtl time a record is valid in cache after insertion
    //! @param cacheMaxSize the maximum value to which the cache can grow
    //!
    //----------------------------------------------------------------------------
    LfcCache( uint64_t cacheTtl, uint64_t cacheMaxSize );


    //----------------------------------------------------------------------------
    //! Destructor
    //----------------------------------------------------------------------------
    virtual ~LfcCache();


    //----------------------------------------------------------------------------
    //! Insert a new entry in cache
    //!
    //! @param lfn logical file name
    //! @param pfn physiscal file name
    //
    //----------------------------------------------------------------------------
    virtual void Insert( std::string lfn, std::string pfn );


    //----------------------------------------------------------------------------
    //! Try to get an entry from cache
    //!
    //! @param lfn logical file name we are looking for
    //! @param pfn the pfn retrieved from cache
    //!
    //! @return true if entry found in cache, false otherwise
    //!
    //----------------------------------------------------------------------------
    virtual bool GetEntry( std::string lfn, std::string& pfn );

  private:

    uint64_t mCacheTtl;     ///< time a valid record can stay in cache
    uint64_t mCacheMaxSize; ///< maximum cache size to which it can grow
    XrdSysRWLock mRwLock;   ///< rw mutex for sync access to the cache

    MapType  mLfn2Pfn;    ///< map containing the lfn, pfn and iterator to the queue
    ListType mAgingQueue; ///< list that holds the lfn and the timestap when it was
                          ///< inserted in the cache ( it is used as a queue )

};

#endif // __EOS_PLUGIN_LFCCACHE_HH__

