//------------------------------------------------------------------------------
// File: LfcString.hh
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

#ifndef __EOS_PLUGIN_LFCSTRING_HH__
#define __EOS_PLUGIN_LFCSTRING_HH__

/*----------------------------------------------------------------------------*/
#include <string>
#include <vector>
/*----------------------------------------------------------------------------*/

class LfcString;
typedef std::vector<LfcString> VectStrings;


//------------------------------------------------------------------------------
//! String class extended with some useful tokenizing functions
//------------------------------------------------------------------------------
class LfcString : public std::string
{
  public:

    //--------------------------------------------------------------------------
    //! Constructor
    //--------------------------------------------------------------------------
    LfcString() : std::string() {}


    //--------------------------------------------------------------------------
    //! Constructor with parameter
    //--------------------------------------------------------------------------
    LfcString( std::string s ):  std::string( s ) {}


    //--------------------------------------------------------------------------
    //! Constructor with parameter
    //--------------------------------------------------------------------------
    LfcString( const char* p ):  std::string( p ? p : "" ) {}


    //--------------------------------------------------------------------------
    //! Destructor
    //--------------------------------------------------------------------------
    virtual ~LfcString()  {}


    //--------------------------------------------------------------------------
    //
    //--------------------------------------------------------------------------
    operator bool() {
      return !empty();
    }

    //--------------------------------------------------------------------------
    //! Get pointer to the const C string
    //--------------------------------------------------------------------------
    operator const char* () {
      return c_str();
    }


    //--------------------------------------------------------------------------
    //! Get pointer to the C string
    //--------------------------------------------------------------------------
    operator char* () {
      return const_cast<char*>( c_str() );
    }


    //--------------------------------------------------------------------------
    //! Test if string starts with a certain sequence
    //!
    //! @param s expected begining sequence
    //!
    //! @return true if string starts with the specified sequence, false otherwise
    //--------------------------------------------------------------------------
    bool StartsWith( const LfcString s ) {
      return substr( 0, s.size() ) == s ;
    }


    //--------------------------------------------------------------------------
    //! Tokenize a string based on a set of delimiters
    //!
    //! @param delim delimitator
    //!
    //! @return the vector of string obtained by splitting the inital string
    //!
    //--------------------------------------------------------------------------
    const VectStrings Split( LfcString delim );


    //------------------------------------------------------------------------------
    //! Inverse to split
    //!
    //! @param v vector containing strings
    //! @param delim delimitator
    //!
    //! @return the string obtained by contatenating the delim after each element
    //!         from the vector
    //------------------------------------------------------------------------------
    static LfcString Join( VectStrings v, LfcString delim ) {
      LfcString result;

      for ( VectStrings::iterator it = v.begin(); it != v.end(); ++it )
        result += delim + *it;

      return result;
    }
};

#endif // __EOS_PLUGIN_LFCSTRING_HH__

