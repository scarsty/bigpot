#pragma once

#include "BigPotControl.h"
#include <string>
#include <iostream>

using namespace std;

//������
//���ܲ���ȫ

class BigPotBase
{
protected:
	BigPotControl* control;
public:
	BigPotBase() { control = BigPotControl::getInstance(); };
	~BigPotBase() {};
};

