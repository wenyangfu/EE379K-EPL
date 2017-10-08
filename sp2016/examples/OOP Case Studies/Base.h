#pragma once
#include <iostream>
#include <string>
#include <map>
#include <functional>


class Base {
public:
	virtual void doit(void) {
		std::cout << "Base\n";
	}
	virtual Base* clone(void) {
		return new Base{ *this };
	}
};


using CreateFun = std::function<Base*(void)>;
std::map<std::string, CreateFun>& getMap();

