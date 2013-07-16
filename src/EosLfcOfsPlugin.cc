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
EosLfcOfsPlugin* gOFS;

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
    gOFS = new EosLfcOfsPlugin();
    
    gOFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);

    if ( gOFS->Configure(OfsEroute) ) return 0;
    
    XrdOfsFS = gOFS;
    return gOFS;
  }
}

//------------------------------------------------------------------------------
// Constructor 
//------------------------------------------------------------------------------
EosLfcOfsPlugin::EosLfcOfsPlugin(): XrdOfs()
{
  //............................................................................
  // If this parameter is not present, then also the configuration which is
  // called afterwards will fail.
  //............................................................................
  char* var = getenv( "N2N_UPLINK_HOST" );

  if (!var)
  {
    OfsEroute.Emsg("EosLfcOfsPlugin", "No N2N_UPLINK_HOST env. variable defined");
    mMetaMgrHost = "";
  }
  else
  {
    mMetaMgrHost = var;
  }
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
EosLfcOfsPlugin::~EosLfcOfsPlugin()
{
  //empty
}


//------------------------------------------------------------------------------
//! Rewrite the stat method so that it actually returns OK if the LFC transaltion
//! results in a redirection. This is because the old client can not handle the
//! redirection on stat nad therefore we it fake and tell it we have the file.
//! Once once it requests it, we actually go to the EOS instance with the name
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
    std::string err_data = out_error.getErrData();
   
    if ( err_data == mMetaMgrHost ) {
      //........................................................................
      // If the file is not in EOS we redirect up to a meta manager and we
      // signal that we don't have the file by replying SFS_ERROR
      //........................................................................
      OfsEroute.Emsg( "stat", "got redirection because the file is not in EOS "
                      "according to LFC" );
      return SFS_ERROR;
    }
    else {
      //..........................................................................
      // Here we just fake a stat info, so that the returning message is not empty,
      // anyway if the file does not exitst ( although it should, as we found it
      // in LFC ) then the client will get an error while trying to access it in EOS
      //..........................................................................
      lstat( "/etc/passwd", buf );
      OfsEroute.Emsg( "stat ", "return SFS_OK as the file is in EOS");
      return SFS_OK;
    }
  }

  return retc;
}



