///
/// \file RorcUtilsReadRegister.cxx
/// \author Pascal Boeschoten
///
/// Utility that reads a register from a RORC
///

#include <iostream>
#include <stdexcept>
#include <boost/exception/diagnostic_information.hpp>
#include "RORC/ChannelFactory.h"
#include "RorcUtilsOptions.h"
#include "RorcUtilsCommon.h"
#include "RorcUtilsDescription.h"
#include "ChannelPaths.h"
#include "TypedMemoryMappedFile.h"
#include "ReadyFifo.h"
#include <boost/format.hpp>

using namespace AliceO2::Rorc::Util;
using std::cout;
using std::endl;

static const UtilsDescription DESCRIPTION(
    "Print FIFO",
    "Prints the FIFO of a RORC",
    "./rorc-print-fifo --serial=12345 --channel=0"
    );

int main(int argc, char** argv)
{
  auto optionsDescription = Options::createOptionsDescription();
  Options::addOptionChannel(optionsDescription);
  Options::addOptionSerialNumber(optionsDescription);

  try {
    auto variablesMap = Options::getVariablesMap(argc, argv, optionsDescription);
    RORC_UTILS_HANDLE_HELP(variablesMap, DESCRIPTION, optionsDescription);
    int serialNumber = Options::getOptionSerialNumber(variablesMap);
    int channelNumber = Options::getOptionChannel(variablesMap);
    auto channel = AliceO2::Rorc::ChannelFactory().getSlave(serialNumber, channelNumber);

    using namespace AliceO2::Rorc;
    auto fileName = ChannelPaths::fifo(serialNumber, channelNumber);

    cout << "Printing FIFO at '" << fileName << "'" << endl;
    auto str = boost::str(boost::format(" %-3s %-14s %-14s %-14s %-14s\n") % "#" % "Length (hex)" % "Status (hex)"
        % "Length (dec)" % "Status (dec)");
    auto line1 = std::string(str.length(), '=') + '\n';
    auto line2 = std::string(str.length(), '-') + '\n';

    TypedMemoryMappedFile<ReadyFifo> mappedFileFifo(fileName.c_str());
    auto fifo = mappedFileFifo.get();

    cout << line1 << str << line2;
    for (int i = 0 ; i < fifo->entries.size(); ++i) {
      int32_t length = fifo->entries[i].length;
      int32_t status = fifo->entries[i].status;
      cout << boost::str(boost::format(" %-3d %14x %14x %14d %14d\n") % i % length % status % length % status);
      //cout << " " << i << " -> " << fifo->entries[i].length << " " << fifo->entries[i].status << endl;
    }

    cout << line1;

  } catch (std::exception& e) {
    RORC_UTILS_HANDLE_EXCEPTION(e, DESCRIPTION, optionsDescription);
  }

  return 0;
}