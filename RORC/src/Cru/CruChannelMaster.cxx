/// \file CruChannelMaster.cxx
/// \brief Implementation of the CruChannelMaster class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruChannelMaster.h"
#include <boost/dynamic_bitset.hpp>
#include <thread>
#include "ChannelPaths.h"
#include "ChannelUtilityImpl.h"
#include "ExceptionInternal.h"
#include "Utilities/SmartPointer.h"

/// Creates a CruException and attaches data using the given message string
#define CRU_EXCEPTION(_err_message) \
  CruException() \
      << errinfo_rorc_generic_message(_err_message)

using namespace std::literals;

namespace AliceO2 {
namespace Rorc {

namespace Register = CruRegisterIndex;

namespace
{

/// DMA page length in bytes
/// Note: the CRU has a firmware defined fixed page size
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int FIFO_FW_ENTRIES = 4; ///< The firmware works in blocks of 4 pages
constexpr int NUM_OF_FW_BUFFERS = 32; ///< ... And as such has 32 "buffers" in the FIFO
constexpr int NUM_PAGES = FIFO_FW_ENTRIES * NUM_OF_FW_BUFFERS; ///<... For a total number of 128 pages
static_assert(NUM_PAGES == CRU_DESCRIPTOR_ENTRIES, "");

/// DMA addresses must be 32-byte aligned
constexpr uint64_t DMA_ALIGNMENT = 32;

} // Anonymous namespace

CruChannelMaster::CruChannelMaster(const Parameters& parameters)
    : ChannelMasterPdaBase(CARD_TYPE, parameters, allowedChannels(), sizeof(CruFifoTable)), //
      mInitialResetLevel(ResetLevel::Rorc), // It's good to reset at least the card channel in general
      mLoopbackMode(parameters.getGeneratorLoopback().get_value_or(LoopbackMode::Rorc)), // Internal loopback by default
      mGeneratorEnabled(parameters.getGeneratorEnabled().get_value_or(true)), // Use data generator by default
      mGeneratorPattern(parameters.getGeneratorPattern().get_value_or(GeneratorPattern::Incremental)), //
      mGeneratorMaximumEvents(0), // Infinite events
      mGeneratorInitialValue(0), // Start from 0
      mGeneratorInitialWord(0), // First word
      mGeneratorSeed(0), // Presumably for random patterns, incremental doesn't really need it
      mGeneratorDataSize(parameters.getGeneratorDataSize().get_value_or(DMA_PAGE_SIZE)) // Can use page size
{
  /// TODO XXX TODO initialize configuration params

  if (auto pageSize = parameters.getDmaPageSize()) {
    if (pageSize.get() != DMA_PAGE_SIZE) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU only supports an 8kB page size")
          << ErrorInfo::DmaPageSize(pageSize.get()));
    }
  }

  if (auto enabled = parameters.getGeneratorEnabled()) {
    if (enabled.get() == false) {
      BOOST_THROW_EXCEPTION(CruException()
          << ErrorInfo::Message("CRU does not yet support non-datagenerator operation"));
    }
  }

  initFifo();
}

auto CruChannelMaster::allowedChannels() -> AllowedChannels {
  // Note: BAR 1 is not available because BAR 0 is 64-bit wide, so it 'consumes' two BARs.
  return {0, 2};
}

CruChannelMaster::~CruChannelMaster()
{
  setBufferNonReady();
}

void CruChannelMaster::deviceStartDma()
{
  resetCru();
  initCru();
  mSuperpageQueue.clear();
  // Push initial 128 pages
//  fillFifoNonLocking();
//  setBufferReadyGuard();
}

/// Set buffer to ready
void CruChannelMaster::setBufferReady()
{
  if (!mBufferReady) {
    mBufferReady = true;
    getBar().setDataEmulatorEnabled(true);
    std::this_thread::sleep_for(10ms);
  }
}

/// Set buffer to non-ready
void CruChannelMaster::setBufferNonReady()
{
  if (mBufferReady) {
    mBufferReady = false;
    getBar().setDataEmulatorEnabled(false);
  }
}

void CruChannelMaster::deviceStopDma()
{
  setBufferNonReady();
}

void CruChannelMaster::deviceResetChannel(ResetLevel::type resetLevel)
{
  if (resetLevel == ResetLevel::Nothing) {
    return;
  }

  resetCru();
}

CardType::type CruChannelMaster::getCardType()
{
  return CardType::Cru;
}

/// Initializes the FIFO and the page addresses for it
void CruChannelMaster::initFifo()
{
//  if (getPageAddresses().size() <= CRU_DESCRIPTOR_ENTRIES) {
//    BOOST_THROW_EXCEPTION(CruException()
//        << ErrorInfo::Message("Insufficient amount of pages fit in DMA buffer")
//        << ErrorInfo::Pages(getPageAddresses().size())
//        << ErrorInfo::DmaBufferSize(getChannelParameters().dma.bufferSize)
//        << ErrorInfo::DmaPageSize(getChannelParameters().dma.pageSize));
//  }

  getFifoUser()->resetStatusEntries();
}

void CruChannelMaster::resetCru()
{
  getBar().resetDataGeneratorCounter();
  std::this_thread::sleep_for(100ms);
  getBar().resetCard();
  std::this_thread::sleep_for(100ms);
}

void CruChannelMaster::initCru()
{
  // Set data generator pattern
  if (mGeneratorEnabled) {
    getBar().setDataGeneratorPattern(mGeneratorPattern);
  }

  // Status base address in the bus address space
  log((Utilities::getUpper32Bits(uint64_t(getFifoBus())) != 0)
        ? "Using 64-bit region for status bus address, may be unsupported by PCI/BIOS configuration"
        : "Using 32-bit region for status bus address");

  if (!Utilities::checkAlignment(getFifoBus(), DMA_ALIGNMENT)) {
    BOOST_THROW_EXCEPTION(CruException() << ErrorInfo::Message("FIFO bus address not 32 byte aligned"));
  }

  getBar().setFifoBusAddress(getFifoBus());

  // TODO Note: this stuff will be set by firmware in the future
  {
    // Status base address in the card's address space
    getBar().setFifoCardAddress();

    // Set descriptor table size (must be size - 1)
    getBar().setDescriptorTableSize();

    // Send command to the DMA engine to write to every status entry, not just the final one
    getBar().setDoneControl();
  }
}

std::vector<uint32_t> CruChannelMaster::utilityCopyFifo()
{
  std::vector<uint32_t> copy;
  auto* fifo = getFifoUser();
  size_t size = sizeof(std::decay<decltype(fifo)>::type);
  size_t elements = size / sizeof(decltype(copy)::value_type);
  copy.reserve(elements);

  auto* fifoData = reinterpret_cast<char*>(fifo);
  auto* copyData = reinterpret_cast<char*>(copy.data());
  std::copy(fifoData, fifoData + size, copyData);
  return copy;
}

void CruChannelMaster::utilityPrintFifo(std::ostream& os)
{
  ChannelUtility::printCruFifo(getFifoUser(), os);
}

void CruChannelMaster::utilitySetLedState(bool state)
{
  int on = 0x00; // Yes, a 0 represents the on state
  int off = 0xff;
  getBarUserspace()[CruRegisterIndex::LED_STATUS] = state ? on : off;
}

void CruChannelMaster::utilitySanityCheck(std::ostream& os)
{
  ChannelUtility::cruSanityCheck(os, this);
}

void CruChannelMaster::utilityCleanupState()
{
  ChannelUtility::cruCleanupState(ChannelPaths(CARD_TYPE, getSerialNumber(), getChannelNumber()));
}

int CruChannelMaster::utilityGetFirmwareVersion()
{
  return getBarUserspace()[CruRegisterIndex::FIRMWARE_COMPILE_INFO];
}

int CruChannelMaster::getSuperpageQueueCount()
{
  return mSuperpageQueue.getQueueCount();
}

int CruChannelMaster::getSuperpageQueueAvailable()
{
  return mSuperpageQueue.getQueueAvailable();
}

int CruChannelMaster::getSuperpageQueueCapacity()
{
  return mSuperpageQueue.getQueueCapacity();
}

auto CruChannelMaster::getSuperpageStatus() -> SuperpageStatus
{
  return mSuperpageQueue.getFrontSuperpageStatus();
}

void CruChannelMaster::pushSuperpage(size_t offset, size_t size)
{
  if (mSuperpageQueue.isFull()) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, queue at capacity"));
  }

  if (size == 0) {
    BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message("Could not enqueue superpage, size == 0"));
  }

  if ((size % 1024*1024) != 0) {
    BOOST_THROW_EXCEPTION(Exception()
        << ErrorInfo::Message("Could not enqueue superpage, size not a multiple of 1 MiB"));
  }

  // TODO check if offset is properly aligned
  // TODO check if size is multiple of ???

  SuperpageQueueEntry entry;
  entry.busAddress = getBusOffsetAddress(offset + getBufferProvider().getDmaOffset());
  entry.pushedPages = 0;
  entry.status.confirmedPages = 0;
  entry.status.maxPages = size / DMA_PAGE_SIZE;
  entry.status.offset = offset;

  mSuperpageQueue.addToQueue(entry);
}

auto CruChannelMaster::popSuperpage() -> SuperpageStatus
{
  return mSuperpageQueue.removeFromFilledQueue().status;
}

void CruChannelMaster::fillSuperpages()
{
  // Push new pages into superpage
  if (!mSuperpageQueue.getPushing().empty()) {
    auto offset = mSuperpageQueue.getPushing().front();
    SuperpageQueueEntry& entry = mSuperpageQueue.getEntry(offset);

    int freeDescriptors = FIFO_QUEUE_MAX - mFifoSize;
    int freePages = entry.status.maxPages - entry.pushedPages;
    int possibleToPush = std::min(freeDescriptors, freePages);

    for (int i = 0; i < possibleToPush; ++i) {
      pushIntoSuperpage(entry);
    }

    if (mFifoSize >= READYFIFO_ENTRIES) {
      // We should only enable the buffer when all the descriptors are filled, because the card may use them all as soon
      // as the ready signal is given
      setBufferReady();
    }

    if (entry.pushedPages == entry.status.maxPages) {
      // Remove superpage from pushing queue
      mSuperpageQueue.removeFromPushingQueue();
    }
  }

  // Check for arrivals & handle them
  if (!mSuperpageQueue.getArrivals().empty()) {
    auto isArrived = [&](int descriptorIndex) { return getFifoUser()->statusEntries[descriptorIndex].isPageArrived(); };
    auto resetDescriptor = [&](int descriptorIndex) { getFifoUser()->statusEntries[descriptorIndex].reset(); };

    while (mFifoSize > 0) {
      auto offset = mSuperpageQueue.getArrivals().front();
      SuperpageQueueEntry& entry = mSuperpageQueue.getEntry(offset);

      if (isArrived(mFifoBack)) {
//        printf("        is arrived\n");

        resetDescriptor(mFifoBack);
        mFifoSize--;
        mFifoBack = (mFifoBack + 1) % READYFIFO_ENTRIES;
        entry.status.confirmedPages++;

        if (entry.status.confirmedPages == entry.status.maxPages) {
          // Move superpage to filled queue
          mSuperpageQueue.moveFromArrivalsToFilledQueue();
        }
      } else {
        // If the back one hasn't arrived yet, the next ones will certainly not have arrived either...
        break;
      }
    }
  }
}

void CruChannelMaster::pushIntoSuperpage(SuperpageQueueEntry& superpage)
{
  assert(mFifoSize < FIFO_QUEUE_MAX);
  assert(superpage.pushedPages < superpage.status.maxPages);

  auto pageBusAddress = getNextSuperpageBusAddress(superpage);
  auto descriptorIndex = getFifoFront();
  auto sourceAddress = reinterpret_cast<volatile void*>((descriptorIndex % NUM_OF_FW_BUFFERS) * DMA_PAGE_SIZE);

  getFifoUser()->setDescriptor(descriptorIndex, DMA_PAGE_SIZE_32, sourceAddress, pageBusAddress);

  if (mBufferReady) {
//    printf("Sending ACK\n");
    getBar().sendAcknowledge();
  } else {
//    printf("Skipping ACK\n");
  }

//  if (mBufferReady) {
//    printf(" - Sent ACK!\n");
//    getBar().sendAcknowledge();
//  } else {
//    mPendingAcks++;
//  }

  mFifoSize++;
  superpage.pushedPages++;
}

volatile void* CruChannelMaster::getNextSuperpageBusAddress(const SuperpageQueueEntry& superpage)
{
  return reinterpret_cast<volatile void*>(reinterpret_cast<volatile char*>(superpage.busAddress)
      + DMA_PAGE_SIZE * superpage.pushedPages);
}

} // namespace Rorc
} // namespace AliceO2
