#pragma once
#ifdef MYDLL_EXPORTS
#define MATH_API __declspec(dllexport)
#else
#define MATH_API __declspec(dllimport)
#endif
 
#include <vector>
#include <json/json.h>


MATH_API void invertBits(std::string& data);
MATH_API bool readFileData_dec(const std::string filePath, Json::Value& jsonData);
MATH_API bool writeFileData_enc(const std::string filePath, const Json::Value& jsonData);
