//����������
//˽�д�ǰ�»���
//���������»���
//ͨ����ʹ�ù���������ͨ����������д

#pragma once

#include "BigPotEngine.h"
#include "BigPotString.h"
#include "BigPotConfig.h"


using namespace std;


//���ܲ���ȫ

class BigPotBase
{
protected:
	BigPotEngine* engine_;
	BigPotConfig* config_;
public:
	BigPotBase();
	~BigPotBase() {};

	//void setFilePath(char *s) { BigPotString::setFilePath(s); }
	//static bool fileExist(const string& filename);
	//void safedelete(void* p){ if (p) delete p; p = nullptr; };

};

