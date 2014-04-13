#include "../stdafx.h"
#include <vector>
#include <string>
#include <sstream>

#ifndef SQF_H
#define SQF_H

class SQF {
private:
	std::vector<std::string> arrayStack;

public:
	SQF();
	~SQF();
	void push_str(char *String);
	void push_array(char *String);
	std::string toArray();
};

#endif
