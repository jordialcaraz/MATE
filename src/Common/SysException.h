
/**************************************************************************
*                    Universitat Autonoma de Barcelona,					  *
*              HPC4SE: http://grupsderecerca.uab.cat/hpca4se/             *
*                        Analysis and Tuning Group, 					  *
*					            2002 - 2018                  			  */
/**************************************************************************
*	  See the LICENSE.md file in the base directory for more details      *
*									-- 									  *
*	This file is part of MATE.											  *	
*																		  *
*	MATE is free software: you can redistribute it and/or modify		  *
*	it under the terms of the GNU General Public License as published by  *
*	the Free Software Foundation, either version 3 of the License, or     *
*	(at your option) any later version.									  *
*																		  *
*	MATE is distributed in the hope that it will be useful,				  *
*	but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 		  *
*	GNU General Public License for more details.						  *
*																		  *
*	You should have received a copy of the GNU General Public License     *
*	along with MATE.  If not, see <http://www.gnu.org/licenses/>.         *
*																		  *
***************************************************************************/

// MATE
// Copyright (C) 2002-2008 Ania Sikora, UAB.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef __SYSEXCEPTION_H__
#define __SYSEXCEPTION_H__

// Local includes
#include "Exception.h"

// C++ includes
#include <iostream>
#include <errno.h>

//extern int errno;

namespace Common {
	/**
	 * @class SysException
	 * @brief System exception.
	 *
	 * @extends Exception
	 *
	 * @version 1.0b
	 * @since 1.0b
	 * @author Ania Sikora, 2002
	 */

	// ------------------------------------------------------------
	class SysException: public Exception
	{
	public:

		/**
		 * @brief Constructor.
		 *
		 * @param msg Exception message.
		 * @param objName Name of the object causing the exception,
		 * "" by default.
		 */
		SysException (std::string const & msg,
					  std::string const & objName = std::string ())
			: Exception (msg, objName, errno)
		{}

		/**
		 * @brief Constructor.
		 *
		 * @param msg Exception message.
		 * @param errorCode Exception error code.
		 */
		SysException (std::string const & msg, long errorCode)
			: Exception (msg, std::string (), errorCode)
		{}

		/**
		 * @brief Displays exception message on the given output stream.
		 * @param os Output stream to display the message.
		 */
		void Display (std::ostream & os) const;

		/**
		 * @brief Displays exception message on the standard error output.
		 */
		void Display () const;

		/**
		 * @brief Returns a string containing the error message.
		 */
		std::string GetReason () const;
	};
}

#endif
