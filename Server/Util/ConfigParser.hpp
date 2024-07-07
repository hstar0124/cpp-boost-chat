#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>

class ConfigParser 
{
private:
    std::string filename;
    std::map<std::string, std::string> configMap;

public:
    // 생성자: 파일 이름을 인자로 받음
    ConfigParser(const std::string& filename) : filename(filename) 
    {}

    // 파일을 읽고 키-값 쌍을 맵에 저장
    bool readConfig() 
    {
        std::ifstream inputFile(filename);

        if (!inputFile.is_open()) 
        {
            std::cerr << "Error: Cannot open file: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(inputFile, line)) 
        {
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) 
            {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                configMap[key] = value;
            }
        }

        inputFile.close();
        return true;
    }

    // 맵 리턴
    const std::map<std::string, std::string>& getConfig() const 
    {
        return configMap;
    }


};