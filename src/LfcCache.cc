//------------------------------------------------------------------------------
// File: LfcCache.cc
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

/*----------------------------------------------------------------------------*/
#include <utility>
/*----------------------------------------------------------------------------*/
#include <time.h>
/*----------------------------------------------------------------------------*/
#include "LfcCache.hh"
/*----------------------------------------------------------------------------*/


//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------`
LfcCache::LfcCache( uint64_t cacheTtl, uint64_t cacheMaxSize ):
  mCacheTtl( cacheTtl ),
  mCacheMaxSize( cacheMaxSize )
{
  //empty
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
LfcCache::~LfcCache()
{
  // empty
}


//------------------------------------------------------------------------------
// Insert a new entry in cache
//------------------------------------------------------------------------------
void
LfcCache::Insert( std::string lfn, std::string pfn )
{
  time_t now = time( NULL );

  mRwLock.WriteLock();   // -->

  //............................................................................
  // Clear expired cache entries
  //............................................................................
  MapType::iterator iterMap;
  ListType::iterator iterQ;

  for ( iterQ = mAgingQueue.begin(); iterQ != mAgingQueue.end(); /*empty*/ ) {
    if ( static_cast<uint64_t>( difftime( now , iterQ->second ) ) >= mCacheTtl ) {
      iterMap = mLfn2Pfn.find( iterQ->first );

      if ( iterMap != mLfn2Pfn.end() ) {
        mLfn2Pfn.erase( iterMap );
      } else {
        fprintf( stderr, "Warning1: Entry found in queue but not in map." );
      }

      mAgingQueue.erase( iterQ++ );
    } else {
      ++iterQ;
    }
  }

  //............................................................................
  // If still too many entries in cache - delete some more
  //............................................................................
  if ( mAgingQueue.size() >= mCacheMaxSize ) {
    iterQ = mAgingQueue.begin();

    while ( ( mAgingQueue.size() > static_cast<size_t>( 0.9 * mCacheMaxSize ) ) &&
            ( iterQ != mAgingQueue.end() ) )
    {
      iterMap = mLfn2Pfn.find( iterQ->first );

      if ( iterMap != mLfn2Pfn.end() ) {
        mLfn2Pfn.erase( iterMap );
      } else {
        fprintf( stderr, "Warning2: Entry found in queue but not in map." );
      }

      mAgingQueue.erase( iterQ++ );
    }
  }

  //............................................................................
  // If entry is not in cach do the insert
  //............................................................................
  if ( !mLfn2Pfn.count( lfn ) ) {
    iterQ = mAgingQueue.insert( mAgingQueue.end(), std::make_pair( lfn, time( NULL ) ) );
    mLfn2Pfn.insert( std::make_pair( lfn, std::make_pair( pfn, iterQ ) ) );
  }

  mRwLock.UnLock();      // <--
}


//------------------------------------------------------------------------------
// Try to get an entry from cache
//------------------------------------------------------------------------------
bool
LfcCache::GetEntry( std::string lfn, std::string& pfn )
{
  MapType::iterator iterMap;
  bool found = false;
  time_t now;
  mRwLock.ReadLock();    // -->

  if ( mLfn2Pfn.count( lfn ) ) {
    iterMap = mLfn2Pfn.find( lfn );
    now = time( NULL );

    //..........................................................................
    // Test if not expired
    //..........................................................................
    if ( static_cast<uint64_t>( difftime( now, iterMap->second.second->second ) ) <
         mCacheTtl )
    {
      pfn = iterMap->second.first;
      found = true;
    }
  }

  mRwLock.UnLock();      // <--
  return found;
}


