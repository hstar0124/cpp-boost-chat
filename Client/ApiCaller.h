#pragma once

#include "Common.h"

class ApiCaller {
public:
    ApiCaller();
    ~ApiCaller();

    // API 호출
    void CallApi(const std::string& apiName) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;

    std::map<std::string, std::string> m_ApiUrls;
};
