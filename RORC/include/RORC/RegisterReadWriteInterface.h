/// \file RegisterReadWriteInterface.h
/// \brief Definition of the RegisterReadWriteInterface class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_REGISTERREADWRITEINTERFACE_H_
#define ALICEO2_INCLUDE_RORC_REGISTERREADWRITEINTERFACE_H_

#include <cstdint>

namespace AliceO2 {
namespace Rorc {

/// A simple interface for reading and writing 32-bit registers.
class RegisterReadWriteInterface
{
  public:
    virtual ~RegisterReadWriteInterface()
    {
    }

    /// Reads a BAR register. The registers are indexed per 32 bits
    /// \param index The index of the register
    virtual uint32_t readRegister(int index) = 0;

    /// Writes a BAR register
    /// \param index The index of the register
    /// \param value The value to be written into the register
    virtual void writeRegister(int index, uint32_t value) = 0;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_REGISTERREADWRITEINTERFACE_H_
