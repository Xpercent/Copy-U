# Copy-U

`Copy-U` ��һ�����ڼ��� USB �豸��U�̣����벢�Զ����������ļ��Ĺ��ߡ�

## �����ص�

- **�Զ����**������⵽ U �̲���ʱ�����߻��Զ��������Ʋ�����
- **�ļ�����**��U ���е��ļ����Զ����Ƶ�data�ļ����С�
- **��־�ļ�**�����Ƴɹ����ļ���Ϣ�ᱻ�洢��log�ļ��������̷����кŶ�Ӧ��json�С�
- **���ܲ���**�������ļ�(��λȡ��),��ʽ�洢�ļ�����¼��
- **������**������Ŀ������ `cupfunc.dll`�����ڴ����� U �̵Ľ�����


**dataĿ¼�ṹ**��
`u�����к�/�ļ��ĸ�·��(��ϣ)/�ļ���(��ϣ)`
**��־�ļ��ṹ**��
```
{
	"17411342879911263551(�ļ��ĸ�·��hash)": 
	{
		"files": 
		{
			"5996570075586381162(�ļ���hash)": 
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

## ��װ������

1. **���� `cupfunc.dll`**��
   ����Ŀ������ [cupfunc.dll](https://github.com/Xpercent/cupfunc)������Ҫ�����ز����� `cupfunc.dll`��ȷ���������������Ŀ����ȷ���á�
2. **��������`json.cpp`**��
   ʹ��vcpkg��װjsoncpp `vcpkg install jsoncpp`

2. **���л���**��
   C++17

## ʹ��˵��

#### �����ļ�: `config.json`
��`config.json`ʹ�� [invertBits](https://github.com/Xpercent/invertBits) ���߽��а�λȡ��
```
{
	"AllowedExtensions": [".docx",".xlsx",".doc",".xls",".xlsm"],
	"Keywords": ["��","��","֪ͨ","����","��"],
	"MaxFileSize": 1048576000
}
```