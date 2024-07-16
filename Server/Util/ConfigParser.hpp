#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>

class ConfigParser
{
private:
    std::string filename;               // 설정 파일의 이름을 저장하는 변수
    std::map<std::string, std::string> configMap;  // 설정 항목을 저장하는 맵

public:
    // 생성자: 설정 파일 이름을 받아 초기화합니다.
    ConfigParser(const std::string& filename) : filename(filename)
    {}

    // 설정 파일을 읽어서 맵에 저장하는 함수
    bool readConfig()
    {
        std::ifstream inputFile(filename);  // 파일 스트림 생성

        if (!inputFile.is_open())
        {
            // 파일 열기 실패 시 에러 메시지 출력 후 false 반환
            std::cerr << "Error: Cannot open file: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(inputFile, line))
        {
            // '=' 기호를 기준으로 키와 값을 구분하여 맵에 저장
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos)
            {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                configMap[key] = value;
            }
        }

        inputFile.close();  // 파일 닫기
        return true;
    }

    // 설정 값을 반환하는 함수
    const std::map<std::string, std::string>& getConfig() const
    {
        return configMap;  // 설정 맵을 반환
    }
};