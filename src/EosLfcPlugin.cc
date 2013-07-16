//------------------------------------------------------------------------------
// File: EosLfcPlugin.cc
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
#include <cstdio>
#include <sstream>
/*----------------------------------------------------------------------------*/
#include <glob.h>
#include <unistd.h>
#include <dirent.h>
/*----------------------------------------------------------------------------*/
#include "EosLfcPlugin.hh"
#include "LfcCache.hh"
/*----------------------------------------------------------------------------*/
#include "XrdSfs/XrdSfsInterface.hh"
#include "XrdSys/XrdSysError.hh"
#include "XrdSys/XrdSysLogger.hh"
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOuc/XrdOucEnv.hh"
#include "XrdSec/XrdSecEntity.hh"
/*----------------------------------------------------------------------------*/

// Singleton variable
static XrdCmsClient* instance = NULL;

using namespace XrdCms;

namespace XrdCms {
  XrdSysError  LfcError( 0, "EosLfc_" );
  XrdOucTrace  Trace( &LfcError );
};

//------------------------------------------------------------------------------
// CMS Client Instantiator
//------------------------------------------------------------------------------
extern "C"
{
  XrdCmsClient* XrdCmsGetClient( XrdSysLogger* logger,
                                 int           opMode,
                                 int           myPort,
                                 XrdOss*       theSS )
  {
    if ( instance ) {
      return static_cast<XrdCmsClient*>( instance );
    }

    instance = new EosLfcPlugin( logger );
    return static_cast<XrdCmsClient*>( instance );
  }
}


//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
EosLfcPlugin::EosLfcPlugin( XrdSysLogger* logger ):
  XrdCmsClient( XrdCmsClient::amRemote ),
  mRedirPort( 1094 ),
  mMetaMgrPort( 1094 ),
  mSessionInitialised( false ),
  mCache( NULL )
{
  LfcError.logger( logger );
  mRoot.clear();
  mMetaMgrHost.clear();
  mRedirHost.clear();
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
EosLfcPlugin::~EosLfcPlugin()
{
  if ( mSessionInitialised ) {
    ( void ) lfc_endsess();
  }
}


//------------------------------------------------------------------------------
// Configure
//------------------------------------------------------------------------------
int
EosLfcPlugin::Configure( const char* cfn, char* params, XrdOucEnv* EnvInfo )
{
  LfcError.Emsg( "Configure", "Init LFC plugin with params:", params );
  LfcError.Emsg( "Configure", "Using LFC server:", getenv( "LFC_HOST" ) );
  LfcError.Emsg( "Configure", "To enable debug set the env variable LFCDEBUG "
                 "from 0 (no debug) to 2 (full debug)" );
  char* var =  getenv( "LFCDEBUG" );

  //............................................................................
  // Set the bebug level based on the env variable LFCDEBUG
  //............................................................................
  if ( var ) {
    LfcError.Emsg( "Configure", "LFCDEBUG=", var );
    LfcError.setMsgMask( atoi( var ) );
  } else {
    LfcError.Emsg( "Configure", "LFCDEBUG=", 0 );
    LfcError.setMsgMask( 0 );
  }


  //............................................................................
  // Get the uplink host to which we redirect when the file is not in EOS
  //............................................................................
  var = getenv( "N2N_UPLINK_HOST" );
  if ( var ) {
    mMetaMgrHost = var;
    LfcError.Emsg( "Configure", "N2N_UPLINK_HOST=", var );
  } else {
    LfcError.Emsg( "Configure", "No uplink host is configured, set N2N_UPLINK_HOST" );
    return 0;
  }

  var = getenv( "N2N_UPLINK_PORT" );
  if ( var ) {
    mMetaMgrPort = atoi( var );
    LfcError.Emsg( "Configure", "N2N_UPLINK_PORT=", var );
  } else {
    LfcError.Emsg( "Configure", "Use the default uplink port number: 1094" );
    mMetaMgrPort = 1094;
  }

  //............................................................................
  // Initialize Cthread library - should be called before any LFC-API function
  //............................................................................
  serrno = 0;

  if ( params && ParseParameters( params ) ) {
    LfcError.Emsg( "Configure", "Error while parsing parameters" );
    return 0;
  }

  if ( Cthread_init() ) {
    LfcError.Emsg( "Configure", serrno, " Cthread_init error" );
    return 0;
  }

  if ( StartLfcSession() ) {
    LfcError.Emsg( "Configure", "Error while starting LFC session" );
    return 0;
  }

  return 1;
}


//------------------------------------------------------------------------------
// Locate
//------------------------------------------------------------------------------
int
EosLfcPlugin::Locate( XrdOucErrInfo& Resp,
                      const char*    path,
                      int            flags,
                      XrdOucEnv*     Info )
{
  XrdSecEntity* sec_entity = NULL;

  if ( Info ) {
    sec_entity = const_cast<XrdSecEntity*>( Info->secEnv() );
  }

  if ( !sec_entity ) {
    sec_entity = new XrdSecEntity( "" );
    sec_entity->tident = new char[16];
    sec_entity->tident = strncpy( sec_entity->tident, "unknown", 7 );
    sec_entity->tident[7]='\0';
  }
  
  std::string retString ;
  LfcString pfn;

  if ( !Lfn2Pfn( path, pfn, sec_entity ) ) {
    const char* filePath;
    filePath = strstr( pfn.c_str(), mRoot.c_str() );
    retString = mRedirHost ;

    if ( filePath ) {
      retString += "?eos.lfn=";
      retString += filePath;
      retString += "&eos.app=lfc";
    }

    Resp.setErrCode( mRedirPort );
  } else {
    LfcError.Emsg( "Locate", sec_entity->tident,
                   "error=pfn not found, redirect to meta_mgr for lfn=", path );
    retString = mMetaMgrHost.c_str();
    Resp.setErrCode( mMetaMgrPort );
  }
  
  Resp.setErrData( retString.c_str() );
  return SFS_REDIRECT;
}


//------------------------------------------------------------------------------
// Space
//------------------------------------------------------------------------------
int
EosLfcPlugin::Space( XrdOucErrInfo& Resp,
                     const char*    path,
                     XrdOucEnv*     Info )
{
  //............................................................................
  // Can be left unimplemented as we don't provide this functionality
  //............................................................................
  return 0;
}


//------------------------------------------------------------------------------
// Start the LFC session
//------------------------------------------------------------------------------
int
EosLfcPlugin::StartLfcSession()
{
  int status;
  char* lfc_host;
  char comment[80];

  if ( !( lfc_host = getenv( "LFC_HOST" ) ) ) {
    mSessionInitialised = false;
    LfcError.Emsg( "StartLfcSession", "LFC_HOST not set." );
    return -EINVAL;
  }

  strncpy( comment, "EOS-LFC@", 8 );
  gethostname( comment + 8 , 72 );

  if ( ( status = lfc_startsess( lfc_host, comment ) ) ) {
    mSessionInitialised = false;
    LfcError.Emsg( "StartLfcSession", "Unable to open LFC session on: ", lfc_host );
    return status;
  }

  mSessionInitialised = true;
  return 0;
}


//------------------------------------------------------------------------------
// Parse the parameters for the LFC session
//------------------------------------------------------------------------------
int
EosLfcPlugin::ParseParameters( LfcString input )
{
  long int cacheTtl = LFC_CACHE_TTL;
  long int cacheMaxSize = LFC_CACHE_MAXSIZE;
  VectStrings::iterator it;
  VectStrings tokens = input.Split( " \t" );

  for ( it = tokens.begin(); it != tokens.end(); it++ ) {
    VectStrings keyval = it->Split( "=" );

    if ( keyval.size() != 2 ) {
      LfcError.Emsg( "ParseParameters", "EOS-LFC: Invalid parameter: ", *it );
      return EINVAL;
    }

    LfcString key = keyval[0];
    LfcString val = keyval[1];

    if ( key == "root" ) {
      mRoot = val;
    } else if ( key == "rdrhost" ) {
      mRedirHost = val;
    } else if ( key == "rdrport" ) {
      if ( !( std::stringstream( val ) >> mRedirPort ) ) {
        LfcError.Emsg( "ParseParameters", "EOS-LFC: Invalid numeric rdrport: ", val );
        return EINVAL;
      }
    } else if ( key == "match" ) {
      mMatch = val.Split( "," );
    } else if ( key == "nomatch" ) {
      mNotMatch = val.Split( "," );
    } else if ( key == "cache_ttl" ) {
      if ( !( std::stringstream( val ) >> cacheTtl ) ) {
        LfcError.Emsg( "ParseParameters", "EOS-LFC: Invalid numeric cache_ttl: ", val );
        return EINVAL;
      }
    } else if ( key == "cache_maxsize" ) {
      if ( !( stringstream( val ) >> cacheMaxSize ) ) {
        LfcError.Emsg( "ParseParameters", "EOS-LFC: Invalid numeric cache_maxsize: ", val );
        return EINVAL;
      }
    } else {
      LfcError.Emsg( "ParseParameters", "EOS-LFC: Invalid parameter: ", key );
      return EINVAL;
    }
  }

  if ( mRedirHost.empty() || mMetaMgrHost.empty() ) {
    LfcError.Emsg( "ParseParameters", "The rdrhost and meta_mgr_host parameters are mandatory!" );
    return ENODATA;
  }
  
  //............................................................................
  // Initialise the cache and the list of managers after getting all params
  //............................................................................
  mCache = new LfcCache( cacheTtl, cacheMaxSize );
  return 0;
}


//------------------------------------------------------------------------------
// Logical file name to physical file name translation
//------------------------------------------------------------------------------
int
EosLfcPlugin::Lfn2Pfn( LfcString           lfn,
                       LfcString&          pfn,
                       const XrdSecEntity* secEntity )
{
  char msg[4096];
  bool cache_miss = false;
  VectStrings::iterator it;
  pfn = "";

  //............................................................................
  // Check cache for lfn
  //............................................................................
  if ( ( mCache && !( mCache->GetEntry( lfn, pfn ) ) ) || ( !mCache ) ) {
    cache_miss = true;
    sprintf( msg, "%s Cache miss for lfn=%s.", secEntity->tident,
             static_cast<char*>( lfn ) );
    LfcError.Log( SYS_LOG_01, "Lfn2Pfn", msg );

    if ( ( pfn = LfnIsPfn( lfn ) ) ) {
      //........................................................................
      // No LFC lookup needed, input filename contains storage root
      //........................................................................
      sprintf( msg, "%s No LFC lookup needed, file contains storage root.",
               secEntity->tident );
      LfcError.Emsg( "Lfn2Pfn", msg );
    } else if ( !strcmp( lfn, "/atlas" ) ) {
      //........................................................................
      // We currently support translations only for /atlas files
      //........................................................................
      sprintf( msg, "%s Translations supported only for /atlas files.",
               secEntity->tident );
      LfcError.Emsg( "Lfn2Pfn", msg );
      pfn = mRoot;
    } else {
      VectStrings possibles = RewriteLfn( lfn );

      for ( it = possibles.begin(); it != possibles.end(); it++ ) {
        sprintf( msg, "%s LFC rewrite lfn=%s as new_lfn=%s. ", secEntity->tident,
                 static_cast<char*>( lfn ), static_cast<char*>( *it ) );
        LfcError.Log( SYS_LOG_01, "Lfn2Pfn", msg ) ;

        if ( ( pfn = QueryLfc( *it, secEntity ) ) ) {
          break;
        }
      }

      if ( !pfn ) {
        for ( it = possibles.begin(); it != possibles.end(); it++ ) {
          VectStrings possibles2 = FindMatchingLfcDirs( *it );
          VectStrings::iterator it2;

          for ( it2 = possibles2.begin(); it2 != possibles2.end(); it2++ ) {
            if ( pfn = QueryLfc( *it2, secEntity ) ) {
              break;
            }
          }
        }
      }
    }
  } else {
    sprintf( msg, "%s Cache hit for lfn=%s -> pfn=%s. ", secEntity->tident,
             static_cast<char*>( lfn ), static_cast<char*>( pfn ) );
    LfcError.Emsg( "Lfn2Pfn", msg ) ;
  }

  if ( !pfn ) {
    sprintf( msg, "%s No valid replica for lfn=%s. ", secEntity->tident,
             static_cast<char*>( lfn ) );
    LfcError.Emsg( "Lfn2Pfn", msg ) ;
    return -ENOENT;
  }

  if ( cache_miss ) {
    //..........................................................................
    // Insert the new entry in cache
    //..........................................................................
    mCache->Insert( lfn, pfn );
  }

  return SFS_OK;
}


//------------------------------------------------------------------------------
// Check if logical file name is already contains the storage root
//------------------------------------------------------------------------------
LfcString
EosLfcPlugin::LfnIsPfn( LfcString lfn )
{
  const char* pfn = NULL;

  if ( mRoot == "" ) {
    return NULL;
  }

  pfn = strstr( lfn.c_str(), mRoot.c_str() );
  return pfn;
}


//------------------------------------------------------------------------------
// Compensate for varying conventions for LFC path
//------------------------------------------------------------------------------
VectStrings
EosLfcPlugin::RewriteLfn( LfcString lfn )
{
  VectStrings ret;
  LfcString rebuild_path;
  ret.push_back( lfn );                         // 0) Unmodified LFC path

  if ( !lfn.StartsWith( "/grid" ) )
    ret.push_back( "/grid" + lfn );             // 1) Try adding /grid prefix

  VectStrings components = lfn.Split( "/" );

  if ( components.size() > 2 && components[0] == "atlas" ) {
    if ( components[1] != "dq2" ) {             // 2) /atlas/!dq2 -> /grid/atlas/dq2
      rebuild_path = LfcString::Join( VectStrings( components.begin() + 1, components.end() ), "/" );
      ret.push_back( "/grid/atlas/dq2" + rebuild_path );
    }

    //else if (components[1] != "pathena") {   // 3) /atlas/!pathena -> /grid/atlas/pathena
    // 4) etc..
  }

  return ret;
}


//------------------------------------------------------------------------------
// Query the LFC about an lfn
//------------------------------------------------------------------------------
LfcString
EosLfcPlugin::QueryLfc( LfcString lfn, const XrdSecEntity* secEntity )
{
  char msg[4096];
  struct lfc_filereplica* rep_entries = NULL;
  LfcString ret = NULL;
  int status;
  int n_entries;
  char* pfn = NULL;
  char* guid;
  VectStrings::iterator it;
  guid = strstr( ( char* )lfn, "!GUID=" );
  //............................................................................
  // Query LFC
  //............................................................................
  mMutexLfc.Lock();      // -->

  if ( guid == NULL ) {
    status = lfc_getreplica( lfn, NULL/*guid*/, NULL/*se*/, &n_entries, &rep_entries );
  } else {
    status = lfc_getreplica( NULL ,
                             const_cast<const char*>( guid + 6 ),
                             NULL/*se*/,
                             &n_entries,
                             &rep_entries );
  }

  mMutexLfc.UnLock();    // <--

  if ( status ) {
    //..........................................................................
    // Got error, recovery possible here, but most likely lfn not found
    //..........................................................................
    //LfcError.Emsg( "QueryLfc", "Error while doing the query for lfn=", lfn );
    return NULL;
  }

  bool replica_found = false;

  for ( int i = 0; i < n_entries; ++i ) {
    pfn = rep_entries[i].sfn;

    //..........................................................................
    // Reply may include empty names
    //..........................................................................
    if ( strlen( pfn ) == 0 ) {
      continue;
    }

    sprintf( msg, "%s Testing pfn=%s. ", secEntity->tident, static_cast<char*>( pfn ) );
    LfcError.Log( SYS_LOG_02, "Lfn2Pfn", msg ) ;
    
    //..........................................................................
    // Check for forbidden substrings
    //..........................................................................
    bool forbidden = false;

    for ( it = mNotMatch.begin(); it !=  mNotMatch.end(); it++ ) {
      if ( strstr( pfn, *it ) ) {
        forbidden = true;
        break;
      }
    }

    if ( forbidden ) {
      continue;
    }

    //..........................................................................
    // Check for required match string, if specified (match is boolean OR)
    //..........................................................................
    bool match_found = mMatch.empty();  // empty list is trivial match

    for ( it = mMatch.begin(); it != mMatch.end(); it++ ) {
      if ( strstr( pfn, *it ) ) {
        match_found = true;
        break;
      }
    }

    if ( !match_found ) {
      //LfcError.Emsg( "QueryLfc", "Warning match not found: ", pfn );
      continue;
    }

    //..........................................................................
    // Scan for local filesystem mount point, if specified
    //..........................................................................
    if ( ( mRoot != "" ) && !( pfn = strstr( pfn, mRoot.c_str() ) ) ) {
      //LfcError.Emsg( "QueryLfc", "Warning no pfn not found for lfn=%s",
      //               static_cast<char*>( lfn ) );
      continue;
    }

    sprintf( msg, "%s Found match for rewritten lfn=%s -> pfn=%s using matching=%s. ",
             secEntity->tident, static_cast<char*>( lfn ), static_cast<char*>( pfn ),
             static_cast<char*>( *it ) );
    LfcError.Emsg( "Lfn2Pfn", msg ) ;
    replica_found = true;
    break;
  }

  if ( replica_found ) {
    ret = LfcString( pfn );
  }

  if ( rep_entries ) {
    free( rep_entries );
  }

  return ret;
}


//------------------------------------------------------------------------------
// Find matching LFC directories - this can be very expensive
//------------------------------------------------------------------------------
VectStrings
EosLfcPlugin::FindMatchingLfcDirs( LfcString lfn )
{
  VectStrings ret;
  VectStrings components = lfn.Split( "/" );

  if ( components.size() < 3 ) {
    return ret;
  }

  VectStrings::iterator it = components.end();
  it--;
  LfcString filename = *it;
  it--;
  LfcString dirname = *it;
  LfcString parent_path = LfcString::Join( VectStrings( components.begin(), it ), "/" );
  
  //..........................................................................
  // Trying in case the name space is using the old format
  //
  // current: /atlas/dq2/data11_7TeV/NTUP_TOP/f369_m812_p530_p577/data11_7TeV.00180309.physics_Egamma.merge.NTUP_TOP.f369_m812_p530_p577_tid367204_00/
  // old: /atlas/dq2/data11_7TeV/NTUP_TOP/data11_7TeV.00180309.physics_Egamma.merge.NTUP_TOP.f369_m812_p530_p577_tid367204_00_sub021131151/
  //..........................................................................
  it--;
  LfcString old_path = LfcString::Join( VectStrings( components.begin(), it ), "/" );
  mMutexLfc.Lock();      // -->
  lfc_DIR* lfcdir = lfc_opendir( parent_path );
  struct dirent* dirent;
  
  if (lfcdir)
  {
    dirent = lfc_readdir( lfcdir );
    
    while ( dirent != 0 ) {
      LfcString cName = dirent->d_name;

      if ( cName.substr( 0, dirname.size() ) == dirname ) {
        ret.push_back( parent_path + "/" + cName + "/" + filename );
      }

      dirent = lfc_readdir( lfcdir );
    }

    lfc_closedir( lfcdir );
  }
  
  //............................................................................
  // Try the old path
  //............................................................................
  lfcdir = lfc_opendir( old_path );

  if (lfcdir)
  {
    dirent = lfc_readdir( lfcdir );

    while ( dirent != 0 ) {
      LfcString cName = dirent->d_name;
      
      if ( cName.substr( 0, dirname.size() ) == dirname ) {
        ret.push_back( old_path + "/" + cName + "/" + filename );
      }
      
      dirent = lfc_readdir( lfcdir );
    }
    
    lfc_closedir( lfcdir );
  }

  mMutexLfc.UnLock();    //  <--
  return ret;
}

