#include "Common.h"
#include "Client.h"
#include "Util/HsLogger.hpp"

int main()
{
	Logger::instance().init(LogLevel::LOG_DEBUG, LogPeriod::DAY, true, false); // 파일에만 로그 출력

	Client client;
	client.Start();

	return 0;
}
