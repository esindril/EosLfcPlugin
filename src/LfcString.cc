//------------------------------------------------------------------------------
// File: LfcString.cc
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
#include "LfcString.hh"
/*----------------------------------------------------------------------------*/

//--------------------------------------------------------------------------
// Tokenize a string based on a set of delimiters
//--------------------------------------------------------------------------
const VectStrings
LfcString::Split( LfcString delim )
{
  VectStrings result;
  std::string::size_type tok_start, tok_end = 0;
  
  do {
    if ( ( tok_start = find_first_not_of( delim, tok_end ) ) == std::string::npos )
      break;

    tok_end = find_first_of( delim, tok_start );
    result.push_back( LfcString( substr( tok_start,
                                         tok_end == std::string::npos ?  tok_end : tok_end - tok_start ) ) );
  } while ( tok_end != std::string::npos );

  return result;
}
