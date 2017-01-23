/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#include "platform/print.h"
#include "util/util.h"

#ifdef PRINTING_ENABLED
#if _WIN64 || _WIN32
FIXME_MESSAGE("implement printing for windows")
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    throw PrintingException("printing not implemented for windows");
}
}
}
}
#elif __ANDROID__
FIXME_MESSAGE("implement printing for android")
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    throw PrintingException("printing not implemented for android");
}
}
}
}
#elif __APPLE__
FIXME_MESSAGE("implement printing for apple")
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    throw PrintingException("printing not implemented for apple");
}
}
}
}
#elif __linux || __unix
#include <cups/cups.h>
#include <cstring>
#include "util/string_cast.h"
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
namespace
{
std::vector<std::shared_ptr<cups_option_t>> getOptions(std::shared_ptr<cups_dest_t> dest)
{
    assert(dest != nullptr);
    std::vector<std::shared_ptr<cups_option_t>> retval;
    retval.reserve((std::size_t)dest->num_options);
    for(std::size_t i = 0; i < (std::size_t)dest->num_options; i++)
    {
        retval.emplace_back(dest, &dest->options[i]);
    }
    return std::move(retval);
}

class CUPSPrinter final : public Printer
{
private:
    explicit CUPSPrinter(std::wstring id,
                         std::wstring name,
                         std::wstring type,
                         std::wstring location,
                         bool isOnline,
                         std::shared_ptr<cups_dest_t> dest)
        : Printer(id, name, type, location, isOnline), dest(dest)
    {
    }

public:
    const std::shared_ptr<cups_dest_t> dest;
    static std::shared_ptr<CUPSPrinter> make(std::shared_ptr<cups_dest_t> dest)
    {
        assert(dest != nullptr);
        std::wstring id = string_cast<std::wstring>(dest->name);
        if(dest->instance != nullptr)
            id += L"/" + string_cast<std::wstring>(dest->instance);
        std::wstring name = id, type = L"", location = L"";
        bool isOnline = true;
        for(std::size_t optionIndex = 0; optionIndex < (std::size_t)dest->num_options;
            optionIndex++)
        {
            const cups_option_t &option = dest->options[optionIndex];
            if(std::strcmp(option.name, "printer-info") == 0)
            {
                name = string_cast<std::wstring>(option.value);
            }
            else if(std::strcmp(option.name, "printer-location") == 0)
            {
                location = string_cast<std::wstring>(option.value);
            }
            else if(std::strcmp(option.name, "printer-make-and-model") == 0)
            {
                type = string_cast<std::wstring>(option.value);
            }
            else if(std::strcmp(option.name, "printer-is-accepting-jobs") == 0)
            {
                if(std::strcmp(option.value, "true") != 0)
                    isOnline = false;
            }
        }
        return std::shared_ptr<CUPSPrinter>(
            new CUPSPrinter(id, name, type, location, isOnline, dest));
    }

    /** print text
     * @param text the text to print
     * @param the name to use for the job
     * @throw PrintFailedException if there was an error printing
     */
    virtual void printText(std::wstring text, std::wstring jobName) const override
    {
        cups_option_t *options = nullptr;
        int optionCount = 0;
        optionCount = cupsAddOption("attributes-charset", "utf-8", optionCount, &options);
        int jobId = cupsCreateJob(CUPS_HTTP_DEFAULT,
                                  dest->name,
                                  string_cast<std::string>(jobName).c_str(),
                                  optionCount,
                                  options);
        cupsFreeOptions(optionCount, options);
        if(jobId == 0)
            throw PrintFailedException(std::string("cupsCreateJob failed: ")
                                       + cupsLastErrorString());
        cupsStartDocument(CUPS_HTTP_DEFAULT,
                          dest->name,
                          jobId,
                          string_cast<std::string>(jobName).c_str(),
                          CUPS_FORMAT_TEXT,
                          1);
        std::string utf8Text = string_cast<std::string>(text);
        cupsWriteRequestData(CUPS_HTTP_DEFAULT, utf8Text.data(), utf8Text.size());
        cupsFinishDocument(CUPS_HTTP_DEFAULT, dest->name);
    }
};
}

std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    std::vector<std::shared_ptr<Printer>> retval;
    cups_dest_t *dests = nullptr;
    int destCount = cupsGetDests(&dests);
    std::shared_ptr<cups_dest_t> destsAsSharedPtr =
        std::shared_ptr<cups_dest_t>(dests,
                                     [destCount](cups_dest_t *dests)
                                     {
                                         cupsFreeDests(destCount, dests);
                                     });
    retval.reserve((std::size_t)destCount);
    retval.push_back(nullptr); // slot for default printer
    std::shared_ptr<Printer> defaultPrinter = nullptr;
    for(std::size_t destIndex = 0; destIndex < (std::size_t)destCount; destIndex++)
    {
        std::shared_ptr<cups_dest_t> dest =
            std::shared_ptr<cups_dest_t>(destsAsSharedPtr, &dests[destIndex]);
        std::shared_ptr<Printer> printer = CUPSPrinter::make(dest);
        if(dest->is_default && defaultPrinter == nullptr) // in case cups returns multiple defaults
        {
            defaultPrinter = std::move(printer);
            continue;
        }
        retval.push_back(printer);
    }
    if(defaultPrinter == nullptr)
        retval.erase(retval.begin()); // erase empty slot
    else
        retval.front() = std::move(defaultPrinter);
    return std::move(retval);
}
}
}
}
#elif __posix
FIXME_MESSAGE("implement printing for other posix")
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    throw PrintingException("printing not implemented for other posix");
}
}
}
}
#else
#error unknown platform in print.cpp
#endif
#else
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
std::vector<std::shared_ptr<Printer>> getPrinterList()
{
    throw PrintingException("printing not implemented for windows");
}
}
}
}
#endif

#if 0
#include <iostream>
#include <cstdlib>
#include "util/util.h"
namespace programmerjake
{
namespace voxels
{
namespace Printing
{
namespace
{
initializer init1([]()
{
    std::vector<std::shared_ptr<Printer>> printers = getPrinterList();
    std::shared_ptr<Printer> defaultPrinter = nullptr;
    for(std::shared_ptr<Printer> printer : printers)
    {
        if(defaultPrinter == nullptr)
            defaultPrinter = printer;
        std::cout << "Printer:\nid='" << string_cast<std::wstring>(printer->id) << "'\nname='" << string_cast<std::wstring>(printer->name);
        std::cout << "'\ntype='" << string_cast<std::wstring>(printer->type) << "'\nlocation='" << string_cast<std::wstring>(printer->location);
        std::cout << "'\nisOnline=" << (printer->isOnline ? "true" : "false") << "\n" << std::endl;
    }
    printers.clear();
    defaultPrinter->printText(L"This is a printer code test.\nLine 2.\fPage 2.", L"Printer Code Test");
    defaultPrinter = nullptr;
    std::exit(0);
});
}
}
}
}
#endif
