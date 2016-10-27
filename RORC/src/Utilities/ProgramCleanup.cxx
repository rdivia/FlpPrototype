/// \file ProgramCleanup.cxx
/// \brief Utility that cleans up channel state
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include <iostream>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "Factory/ChannelUtilityFactory.h"
#include "ChannelPaths.h"
#include "RORC/Exception.h"
#include "RorcDevice.h"
#include "Utilities/Common.h"
#include "Utilities/Options.h"
#include "Utilities/Program.h"

using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;
namespace po = boost::program_options;

namespace {

template <typename ErrInfo, typename Exception, typename Vector>
void pushIfPresent(const Exception& exception, Vector& vector)
{
  if (auto info = boost::get_error_info<ErrInfo>(exception)) {
    vector.push_back(*info);
  }
}

class ProgramCleanup: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return {"Cleanup", "Cleans up RORC state", "./rorc-cleanup --serial=12345 --channel=0"};
    }

    virtual void addOptions(po::options_description& options)
    {
      Options::addOptionSerialNumber(options);
      Options::addOptionChannel(options);
      options.add_options()("force",po::bool_switch(&mForceCleanup),
          "Force cleanup of shared state files if normal cleanup fails");
    }

    void cleanFiles(std::vector<boost::filesystem::path> files, std::vector<std::string> namedMutexes)
    {

      if (files.empty() && namedMutexes.empty()) {
        cout << "Failed to discover files to clean up\n";
        cout << "### Cleanup failed" << endl;
      }

      for (const auto& f : files) {
        cout << "Deleting file '" << f << "'\n";
        boost::filesystem::remove(f.c_str());
      }

      for (const auto& n : namedMutexes) {
        cout << "Deleting named mutex '" << n << "'\n";
        boost::interprocess::named_mutex::remove(n.c_str());
      }
    }

    virtual void run(const boost::program_options::variables_map& map)
    {
      auto serialNumber = Options::getOptionSerialNumber(map);
      auto channelNumber = Options::getOptionChannel(map);

      try {
        // This non-forced cleanup asks the ChannelMaster to clean up itself.
        // It will not succeed if the channel was not initialized properly before the running of this program.
        cout << "### Attempting cleanup...\n";
        auto channel = ChannelUtilityFactory().getUtility(serialNumber, channelNumber);
        channel->utilityCleanupState();
        cout << "### Done!\n";
      }

      catch (const std::exception& e) {
        cout << "### Channel self-cleanup failed\n";

        if (isVerbose()) {
          cout << "Error: \n" << boost::diagnostic_information(e);
        }

        std::vector<boost::filesystem::path> files;
        std::vector<std::string> namedMutexes;

        if (mForceCleanup) {
          cout << "### Attempting forced cleanup...\n";
          // Try to get paths of the shared state files the internal way
          RorcDevice rorcDevice(serialNumber);
          ChannelPaths channelPaths(rorcDevice.getCardType(), serialNumber, channelNumber);
          files.push_back(channelPaths.fifo().string());
          files.push_back(channelPaths.lock().string());
          files.push_back(channelPaths.pages().string());
          files.push_back(channelPaths.state().string());
          namedMutexes.push_back(channelPaths.namedMutex());
        } else {
          cout << "### Attempting to find files to delete cleanup...\n";
          // Try to get paths of the shared state files from the exception's error info
          pushIfPresent<errinfo_rorc_shared_lock_file>(e, files);
          pushIfPresent<errinfo_rorc_shared_buffer_file>(e, files);
          pushIfPresent<errinfo_rorc_shared_fifo_file>(e, files);
          pushIfPresent<errinfo_rorc_shared_state_file>(e, files);
          if (auto namedMutex = boost::get_error_info<errinfo_rorc_named_mutex_name>(e)) {
            namedMutexes.push_back(*namedMutex);
          }
        }

        if (files.empty() && namedMutexes.empty()) {
          cout << "Failed to discover files to clean up\n";
          cout << "### Cleanup failed";
          if (mForceCleanup) {
            cout << " (can rerun with --force)\n";
          }
          cout << '\n';
        } else {
          for (const auto& f : files) {
            cout << "Deleting file '" << f << "'\n";
            boost::filesystem::remove(f.c_str());
          }

          for (const auto& n : namedMutexes) {
            cout << "Deleting named mutex '" << n << "'\n";
            boost::interprocess::named_mutex::remove(n.c_str());
          }

          cout << "### Done!" << endl;
        }
      }
    }

  private:

    bool mForceCleanup;
};
} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCleanup().execute(argc, argv);
}
