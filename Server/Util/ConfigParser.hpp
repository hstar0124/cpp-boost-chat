#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>

class ConfigParser
{
private:
    std::string filename;               // ���� ������ �̸��� �����ϴ� ����
    std::map<std::string, std::string> configMap;  // ���� �׸��� �����ϴ� ��

public:
    // ������: ���� ���� �̸��� �޾� �ʱ�ȭ�մϴ�.
    ConfigParser(const std::string& filename) : filename(filename)
    {}

    // ���� ������ �о �ʿ� �����ϴ� �Լ�
    bool readConfig()
    {
        std::ifstream inputFile(filename);  // ���� ��Ʈ�� ����

        if (!inputFile.is_open())
        {
            // ���� ���� ���� �� ���� �޽��� ��� �� false ��ȯ
            std::cerr << "Error: Cannot open file: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(inputFile, line))
        {
            // '=' ��ȣ�� �������� Ű�� ���� �����Ͽ� �ʿ� ����
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos)
            {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                configMap[key] = value;
            }
        }

        inputFile.close();  // ���� �ݱ�
        return true;
    }

    // ���� ���� ��ȯ�ϴ� �Լ�
    const std::map<std::string, std::string>& getConfig() const
    {
        return configMap;  // ���� ���� ��ȯ
    }
};