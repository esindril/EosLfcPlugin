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

/*----------------------------------------------------------------------------*/
#include <cstdio>
/*----------------------------------------------------------------------------*/
#include "EosLfcOfsPlugin.hh"
/*----------------------------------------------------------------------------*/
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdOss/XrdOssApi.hh"
/*----------------------------------------------------------------------------*/

// The global OFS handle
EosLfcOfsPlugin gOFS;

extern XrdSysError OfsEroute;
extern XrdOssSys  *XrdOfsOss;
extern XrdOfs     *XrdOfsFS;
extern XrdOucTrace OfsTrace;

extern XrdOss* XrdOssGetSS( XrdSysLogger*, 
                            const char*, 
                            const char* );


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
extern "C"
{
  XrdSfsFileSystem *XrdSfsGetFileSystem(XrdSfsFileSystem *native_fs, 
                                        XrdSysLogger     *lp,
                                        const char       *configfn)
  {
    // Do the herald thing
    //
    OfsEroute.SetPrefix("LfcOfs_");
    OfsEroute.logger(lp);
    XrdOucString version = "LfcOfs (Object Storage File System) ";
    version += VERSION;
    OfsEroute.Say("++++++ (c) 2012 CERN/IT-DSS ",
                  version.c_str());

    //static XrdVERSIONINFODEF( info, XrdOss, XrdVNUMBER, XrdVERSION);

    // Initialize the subsystems
    //
    gOFS.ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);

    if ( gOFS.Configure(OfsEroute) ) return 0;
    // Initialize the target storage system
    //

    if (!(XrdOfsOss = (XrdOssSys*) XrdOssGetSS(lp, configfn, 
                                               gOFS.OssLib ))) {
      return 0;
    } 

    XrdOfsFS = &gOFS;
    return &gOFS;
  }
}


//------------------------------------------------------------------------------
//! Rewrite the stat method so that, the redirection actually returns OK if the
//! LFC transaltion results in a redirection. This is because the old client can
//! not handle the redirection, therefore we fake and tell it we have the file,
//! and once it requests it we actually to the EOS instance with the name
//! translation already done.
//------------------------------------------------------------------------------
int
EosLfcOfsPlugin::stat( const char*             path,
                       struct stat*            buf,
                       XrdOucErrInfo&          out_error,
                       const XrdSecEntity*     client,
                       const char*             opaque )
{
  int retc = XrdOfs::stat( path, buf, out_error, client, opaque );

  if ( retc == SFS_REDIRECT ) {
    //..........................................................................
    // Here we just fake a stat info, so that the returning message is not empty,
    // anyway if the file does not exitst ( although it should, as we found it
    // in LFC ) then the client will get an error while trying to access it in EOS
    //..........................................................................
    lstat( "/etc/passwd", buf );
    OfsEroute.Emsg( "EosLfcOfsPlugin::stat", "got redirection for the stat but we "
                    "just return OK as the old client can not handle the redirection." );

    return SFS_OK;
  }

  return retc;
}


