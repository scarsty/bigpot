#include "BigPotString.h"

namespace BigPotString
{
	bool fileExist(const string& filename)
	{
		if (filename.length() <= 0)
			return false;

		fstream file;
		bool ret = false;
		file.open(filename.c_str(), ios::in);
		if (file)
		{
			ret = true;
			file.close();
		}
		return ret;
	}

	string readStringFromFile(const string& filename)
	{
		FILE *fp = fopen(filename.c_str(), "rb");
		if (fp == nullptr)
		{
			printf("Can not open file %s\n", filename.c_str());
			return "";
		}
		fseek(fp, 0, SEEK_END);
		int length = ftell(fp);
		fseek(fp, 0, 0);
		char* s = new char[length + 1];
		for (int i = 0; i <= length; s[i++] = '\0');
		fread(s, length, 1, fp);
		string str(s);
		fclose(fp);
		delete[] s;
		return str;
	}

	string getFileExt(const string& filename)
	{
		int pos_p = filename.find_last_of(_path_);
		int pos_d = filename.find_last_of('.');
		if (pos_p < pos_d)
			return filename.substr(pos_d + 1);
		return "";
	}

	//һ���������һ���㣬Ϊ1���ҵ�һ����
	string getFileMainname(const string& filename, FindMode mode)
	{
		int pos_p = filename.find_last_of(_path_);
		int pos_d = filename.find_last_of('.');
		if (mode == FINDFIRST)
			pos_d = filename.find_first_of('.', pos_p + 1);
		if (pos_p < pos_d)
			return filename.substr(0, pos_d);
		return filename;
	}

	std::string changeFileExt(const string& filename, const string& ext)
	{
		auto e = ext;
		if (e != "" && e[0] != '.')
			e = "." + e;
		return getFileMainname(filename) + e;
	}

	std::string getFilePath(const string& filename)
	{
		int pos_p = filename.find_last_of(_path_);
		if (pos_p != string::npos)
			return filename.substr(0, pos_p);
		return "";
	}

	//���������ļ������ļ���ֻ������
	std::string fingFileWithMainName(const string& filename)
	{
#ifdef __APPLE__
		return "";
#else
		_finddata_t file;
		long fileHandle;
		string path = getFilePath(filename);
		string ext = getFileExt(filename);
		if (path != "") path = path + _path_;
		string findname = getFileMainname(filename) + ".*";
		string ret = "";
		fileHandle = _findfirst(findname.c_str(), &file);
		ret = path + file.name;
		if (getFileExt(ret) == ext)
		{
			_findnext(fileHandle, &file);
			ret = path + file.name;
			if (getFileExt(ret) == ext)
				ret = "";
		}
		_findclose(fileHandle);
		return ret;
#endif
	}

	string toLowerCase(const string& str)
	{
		auto str1 = str;
		//transform(str1.begin(), str1.end(), str1.begin(), tolower);
		for (auto &c : str1)
		{
			if (c >= 'A'&&c <= 'Z')
				c = c + 'a' - 'A';
		}
		return str1;
	}

	std::string formatString(const char *format, ...)
	{
		char s[4096];
		va_list arg_ptr;
		va_start(arg_ptr, format);
		vsnprintf(s, sizeof(s), format, arg_ptr);
		va_end(arg_ptr);
		return s;
	}

	std::string getFilenameWithoutPath(const string& filename)
	{
		int pos_p = filename.find_last_of(_path_);
		if (pos_p != string::npos)
			return filename.substr(pos_p + 1);
		return filename;
	}
}