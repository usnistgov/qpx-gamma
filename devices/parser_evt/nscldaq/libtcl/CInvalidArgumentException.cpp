/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2005.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Author:
             Ron Fox
             NSCL
             Michigan State University
             East Lansing, MI 48824-1321
*/
#include <config.h>
#include "CInvalidArgumentException.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// Implementation of constructors and other canonical operations.
//

/*!
 *  \param argument - The texulized version of the bad argument.
 *  \param whybad   - The reason the argument is bad.
 *  \param wasdoing - Context information about what was happening when the
 *                    exception was thrown.
 */
CInvalidArgumentException::CInvalidArgumentException(std::string argument, 
                                                     std::string whybad,
                                                     std::string wasDoing) :
        CException(wasDoing),
        m_valuePassed(argument),
        m_whyBad(whybad)
{}
/*!
 *  Copy construction
 * \param rhs Object from which a new copy is constructed.
 */
CInvalidArgumentException::CInvalidArgumentException(const CInvalidArgumentException& rhs) :
        CException(rhs),
        m_valuePassed(rhs.m_valuePassed),
        m_whyBad(rhs.m_whyBad)
{}
/*!
 *   Destructor just needs to chain to the base class.
 */
CInvalidArgumentException::~CInvalidArgumentException()
{}

/*!
 *   Assignement from rhs.
 *  \param rhs - The object that will be assigned into *this.
 *  \return CInvalidArgumentException&
 *  \retval *this
 */
CInvalidArgumentException&
CInvalidArgumentException::operator=(const CInvalidArgumentException& rhs)
{
    if (this != &rhs) {
        CException::operator=(rhs);
        m_valuePassed = rhs.m_valuePassed;
        m_whyBad      = rhs.m_whyBad;
    }
    return *this;
}
/*!
 *   \param rhs - Object compared for equality with *this.
 *   \return int
 *   \retval 1   - Equal.
 *   \retval 0   - Not equal.
 */
int
CInvalidArgumentException::operator==(const CInvalidArgumentException& rhs) const
{
    return  ((CException::operator==(rhs))                &&
             (m_valuePassed   == rhs.m_valuePassed)       &&
             (m_whyBad        == rhs.m_whyBad));
}
/*!
 *   Inequality is the logical inverse of equality.
 *  \param rhs - Object being compared to *this.
 *  \return int
 *  \retval !operator==(rhs)
 */
int
CInvalidArgumentException::operator!=(const CInvalidArgumentException& rhs) const
{
    return !(*this == rhs);
}
////////////////////////////////////////////////////////////////////////////////
//
//  Operations on the object.
//
/*!
 * \return const char*
 * \retval A human readable string that describes the error that caused the
 * exception to be thrown.
 */
const char*
CInvalidArgumentException::ReasonText() const
{
    // We will build up the string in m_reason which is mutable.
    // for exactly this purpose.
    // There is a presumption that only one thread will be catching a specific
    // object at a time which is not unreasonable.
    
    m_reason  = "Invalid Argument: ";
    m_reason += m_valuePassed;
    m_reason + " ";
    m_reason += m_whyBad;
    m_reason += " while ";
    m_reason += WasDoing();
    
    return m_reason.c_str();
            
}
