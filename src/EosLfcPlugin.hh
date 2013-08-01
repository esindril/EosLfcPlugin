//------------------------------------------------------------------------------
// File: EosLfcPlugin.hh
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

#ifndef __EOS_PLUGIN_CMSLFCPLUGIN_HH__
#define __EOS_PLUGIN_CMSLFCPLUGIN_HH__

/*----------------------------------------------------------------------------*/
#define NSTYPE_LFC
#include <sys/types.h>
#include <lfc_api.h>
#include <serrno.h>
/*----------------------------------------------------------------------------*/
#include "XrdCms/XrdCmsClient.hh"
#include "XrdOuc/XrdOucErrInfo.hh"
#include "XrdOuc/XrdOucTList.hh"
#include "XrdOss/XrdOss.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdSec/XrdSecEntity.hh"
/*----------------------------------------------------------------------------*/
#include "LfcString.hh"
/*----------------------------------------------------------------------------*/

#define LFC_CACHE_TTL 2*3600         // 2 hours
#define LFC_CACHE_MAXSIZE 500000

//! Forward declaration
class LfcCache;

//..............................................................................
//! Initialize Cthread library - should be called before any LFC-API function
//..............................................................................
extern "C"
{
  int Cthread_init();
}


//------------------------------------------------------------------------------
//! Plugin class for the XrdCmsClient to deal with LFC mappings
//------------------------------------------------------------------------------
class EosLfcPlugin: public XrdCmsClient
{
  public:

    //--------------------------------------------------------------------------
    //! Constructor
    //--------------------------------------------------------------------------
    EosLfcPlugin(XrdSysLogger* logger);


    //--------------------------------------------------------------------------
    //! Destructor
    //--------------------------------------------------------------------------
    virtual ~EosLfcPlugin();
 

    //--------------------------------------------------------------------------
    //! Configue() is called to configure the client. If the client is obtained
    //!            via a plug-in then Parms are the  parameters specified after
    //!            cmslib path. It is zero if no parameters exist.
    //! @return:   If successful, true must be returned; otherise, false
    //!
    //--------------------------------------------------------------------------
    virtual int Configure( const char* cfn, char* Parms, XrdOucEnv* EnvInfo );


    //--------------------------------------------------------------------------
    //! Locate() is called to retrieve file location information. It is only used
    //!        on a manager node. This can be the list of servers that have a
    //!        file or the single server that the client should be sent to. The
    //!        "flags" indicate what is required and how to process the request.
    //!
    //!        SFS_O_LOCATE  - return the list of servers that have the file.
    //!                        Otherwise, redirect to the best server for the file.
    //!        SFS_O_NOWAIT  - w/ SFS_O_LOCATE return readily available info.
    //!                        Otherwise, select online files only.
    //!        SFS_O_CREAT   - file will be created.
    //!        SFS_O_NOWAIT  - select server if file is online.
    //!        SFS_O_REPLICA - a replica of the file will be made.
    //!        SFS_O_STAT    - only stat() information wanted.
    //!        SFS_O_TRUNC   - file will be truncated.
    //!
    //!        For any the the above, additional flags are passed:
    //!        SFS_O_META    - data will not change (inode operation only)
    //!        SFS_O_RESET   - reset cached info and recaculate the location(s).
    //!        SFS_O_WRONLY  - file will be only written    (o/w RDWR   or RDONLY).
    //!        SFS_O_RDWR    - file may be read and written (o/w WRONLY or RDONLY).
    //!
    //! @return:  As explained above
    //!
    //--------------------------------------------------------------------------
    virtual int Locate( XrdOucErrInfo& Resp,
                        const char*    path,
                        int            flags,
                        XrdOucEnv*     Info = 0 );


    //--------------------------------------------------------------------------
    //! Space() is called to obtain the overall space usage of a cluster. It is
    //!         only called on manager nodes.
    //!
    //! @return: Space information as defined by the response to kYR_statfs. For a
    //!         typical implementation see XrdCmsNode::do_StatFS().
    //!
    //--------------------------------------------------------------------------
    virtual int Space( XrdOucErrInfo& Resp,
                       const char*    path,
                       XrdOucEnv*     Info = 0 );

  private:

    std::string mRoot;          ///< the root directory we are interested in
    std::string mRedirHost;     ///< host to where we redirect in EOS
    unsigned int mRedirPort;    ///< port to where we redirect in EOS, by default 1094
    std::string mMetaMgrHost;   ///< meta mgr to which we redirect when req is not in EOS
    unsigned int mMetaMgrPort;  ///< meta mgr port to where we redirect, by default 1094
    bool mSessionInitialised;   ///< mark if the LFC session has been initialised
    VectStrings mMatch;         ///< string to match in the path found
    VectStrings mNotMatch;      ///< string not to match in the path found
    XrdOucTList* mListMgr;      ///< list of managers up the tree

    int mLfcCacheTtl;           ///< time to live of the entries in cache
    int mLfcCacheMaxSize;       ///< max size of cache entries
    XrdSysSemaphore mSemLfc;    ///< semaphore for accessing LFC sessions
    LfcCache* mCache;           ///< cache for the LFC entries
    int mMaxLfcSessions;        ///< max number of concurrent LFC sessions


    //--------------------------------------------------------------------------
    //! Start the LFC session
    //!
    //! @return 0 if successful, otherwise -errno
    //!
    //--------------------------------------------------------------------------
    int StartLfcSession();


    //--------------------------------------------------------------------------
    //! Parse the parameters passed to the LFC plugin
    //!
    //! @param input string representing the prameters passed to the library
    //!
    //! @return 0 if successful, otherwise error code
    //!
    //--------------------------------------------------------------------------
    int ParseParameters( LfcString input );


    //--------------------------------------------------------------------------
    //! Logical file name to physical file name translation
    //!
    //! @param lfn logical file name to translate
    //! @param pfn physical file name
    //! @param secEntity security entity
    //!
    //! @return SFS_OK if successful, otherwise error code
    //!
    //--------------------------------------------------------------------------
    int Lfn2Pfn( LfcString lfn, LfcString& pfn, const XrdSecEntity* secEntity );


    //--------------------------------------------------------------------------
    //! Check if logical file name is already the  physical file name, in the sens
    //! that it begins with the same path as mRoot specified during configuration
    //!
    //! @param lfn logical file name
    //!
    //! @return NULL if storage root not found in the lfc, otherwise the pfn which
    //!              which begins with the storage root path
    //!
    //--------------------------------------------------------------------------
    LfcString LfnIsPfn( LfcString lfn );


    //--------------------------------------------------------------------------
    //! Compensate for various conventions for the LFC path
    //!
    //! @param lfn logical file name
    //!
    //! @return a vector of path posibilities
    //!
    //--------------------------------------------------------------------------
    VectStrings RewriteLfn( LfcString lfn );


    //--------------------------------------------------------------------------
    //! Query the LFC about an lfn
    //!
    //! @param lfn logical file name we query for
    //! @param secEntity security entity
    //!
    //! @return physical file name or NULL if none found
    //!
    //--------------------------------------------------------------------------
    LfcString QueryLfc( LfcString lfn, const XrdSecEntity* secEntity );


    //------------------------------------------------------------------------------
    //! Find matching LFC directories - this can be very expensive
    //!
    //! @param lfn logical file name
    //!
    //! @return vector of ??
    //!
    //------------------------------------------------------------------------------
    VectStrings FindMatchingLfcDirs( LfcString lfn );
};

#endif // __EOS_PLUGIN_CMSLFCPLUGIN_HH__  
