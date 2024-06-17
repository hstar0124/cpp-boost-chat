#include "ApiCaller.h"

#include <iostream>
#include <fstream>
#include <json/json.h> // JSON 라이브러리 사용 예시

struct ApiCaller::Impl {
    void CallApi(const std::string& apiUrl) const {
        std::cout << "Calling API: " << apiUrl << std::endl;
        // API 호출 처리...
    }
};

ApiCaller::ApiCaller() : m_Impl(std::make_unique<Impl>()) {
    LoadApiUrls();
}

ApiCaller::~ApiCaller() = default;

void ApiCaller::CallApi(const std::string& apiName) const {
    auto it = m_ApiUrls.find(apiName);
    if (it != m_ApiUrls.end()) {
        m_Impl->CallApi(it->second);
    }
    else {
        std::cout << "Error: API not found" << std::endl;
    }
}

void ApiCaller::LoadApiUrls() {
    // 설정 파일에서 API URL을 읽어옴 (JSON 파일을 예시로 사용)
    std::ifstream configFile("config.json");
    if (configFile.is_open()) {
        Json::Value root;
        configFile >> root;

        const Json::Value& apiUrls = root["api_urls"];
        for (const auto& entry : apiUrls) {
            std::string apiName = entry["name"].asString();
            std::string apiUrl = entry["url"].asString();
            m_ApiUrls[apiName] = apiUrl;
        }

        configFile.close();
    }
    else {
        std::cout << "Error: Failed to open config file" << std::endl;
    }
}