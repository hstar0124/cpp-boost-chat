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
#include <algorithm>

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <boost/asio.hpp>

const int HEADER_SIZE = 4;