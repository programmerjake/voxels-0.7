/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED

#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
namespace Printing
{
class PrintingException : public std::runtime_error
{
public:
    explicit PrintingException(std::string msg)
        : runtime_error(msg)
    {
    }
};

class PrintFailedException : public PrintingException
{
public:
    explicit PrintFailedException(std::string msg)
        : PrintingException(msg)
    {
    }
};

class Printer;

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
GCC_PRAGMA(diagnostic ignored "-Wnon-virtual-dtor")
class Printer : public std::enable_shared_from_this<Printer>
{
GCC_PRAGMA(diagnostic pop)
    Printer(const Printer &) = delete;
    Printer &operator =(const Printer &) = delete;
public:
    const std::wstring id;
    const std::wstring name; // human-readable name. eg: "My Printer"
    const std::wstring type; // printer type. eg: "HP LaserJet 4000 Series"
    const std::wstring location; // printer location. eg: "Room 201"
    const bool isOnline;
    explicit Printer(std::wstring id, std::wstring name, std::wstring type, std::wstring location, bool isOnline)
        : id(id), name(name), type(type), location(location), isOnline(isOnline)
    {
    }
    virtual ~Printer() = default;
    /** print text
     * @param text the text to print
     * @param the name to use for the job
     * @throw PrintFailedException if there was an error printing
     */
    virtual void printText(std::wstring text, std::wstring jobName) const = 0;
};

/** get the list of printers
 * @return the list of printers with the default printer listed first
 * @throw PrintingException when the printer system fails
 */
std::vector<std::shared_ptr<Printer>> getPrinterList();

}
}
}

#endif // PRINT_H_INCLUDED
