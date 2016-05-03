#include <iostream>
#include <sstream>
#include "Monitoring/ApMonBackend.h"

namespace AliceO2 {
namespace Monitoring {
namespace Core {

ApMonBackend::ApMonBackend(const std::string configurationFile)
{
	try {
		mApMon = new ApMon(const_cast<char *>(configurationFile.c_str()));
	} catch (std::runtime_error& e) {
		//throw FileNotFoundException(configurationFile);
	}
}

ApMonBackend::~ApMonBackend()
{
	delete mApMon;
}

inline int ApMonBackend::convertTimestamp(const std::chrono::time_point<std::chrono::system_clock> timestamp)
{
	return std::chrono::duration_cast <std::chrono::milliseconds>(
                timestamp.time_since_epoch()
        ).count();
}

void ApMonBackend::send(const int value, const std::string name, const std::string entity, const std::chrono::time_point<std::chrono::system_clock> timestamp)
{
	mApMon->sendTimedParameter(const_cast<char *>(entity.c_str()), const_cast<char *>(entity.c_str()),
		const_cast<char *>(name.c_str()), XDR_INT32, (char *) &value, convertTimestamp(timestamp));
}
void ApMonBackend::send(const double value, const std::string name, const std::string entity, std::chrono::time_point<std::chrono::system_clock> timestamp)
{
	mApMon->sendTimedParameter(const_cast<char *>(entity.c_str()), const_cast<char *>(entity.c_str()), 
		const_cast<char *>(name.c_str()), XDR_REAL64, (char *) &value, convertTimestamp(timestamp));
}
void ApMonBackend::send(const std::string value, const std::string name, const std::string entity, std::chrono::time_point<std::chrono::system_clock> timestamp)
{
	mApMon->sendTimedParameter(const_cast<char *>(entity.c_str()), const_cast<char *>(entity.c_str()),
		const_cast<char *>(name.c_str()), XDR_STRING, const_cast<char *>(value.c_str()), convertTimestamp(timestamp));
}
void ApMonBackend::send(const uint32_t value, const std::string name, const std::string entity, const std::chrono::time_point<std::chrono::system_clock> timestamp)
{
        mApMon->sendTimedParameter(const_cast<char *>(entity.c_str()), const_cast<char *>(entity.c_str()),
                const_cast<char *>(name.c_str()), XDR_INT32, (char *) &value, convertTimestamp(timestamp));
}
} // namespace Core
} // namespace Monitoring
} // namespace AliceO2