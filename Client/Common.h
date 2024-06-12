#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <queue>
#include <optional>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <boost/asio.hpp>

#include <boost/beast.hpp>
#include <boost/beast/version.hpp>
#include <google/protobuf/util/json_util.h>


const int HEADER_SIZE = 4;