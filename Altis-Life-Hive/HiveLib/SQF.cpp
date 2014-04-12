#include "stdafx.h"
#include "SQF.hpp"

SQF::SQF() {

}

SQF::~SQF() {

}

void SQF::push_str(char *_string) {
	if (_string != NULL) {
		std::stringstream string;
		string << "\"" << _string << "\"";
		this->arrayStack.push_back(string.str().c_str());
	}
	else {
		this->arrayStack.push_back("nil");
	}
}
void SQF::push_array(char *_string) {
	if (_string != NULL) {
		this->arrayStack.push_back(_string);
	}
	else {
		this->arrayStack.push_back("nil");
	}
}

std::string SQF::toArray() {
	std::stringstream arrayStream;
	arrayStream << "[";

	for (std::vector<std::string>::iterator
		it = this->arrayStack.begin();
		it != this->arrayStack.end();
	) {
		arrayStream << (this->arrayStack.begin() != it ? "," : "") << *it;
		it++;
	}

	arrayStream << "]";

	return arrayStream.str();
}