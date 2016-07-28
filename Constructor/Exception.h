#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class ConstructorException : public std::runtime_error 
{
	std::string msg;
public:
	ConstructorException(const std::string &arg, const char *file, int line) : std::runtime_error(arg) 
	{
			std::ostringstream o;
			o << file << ":" << line << ": " << arg;
			msg = o.str();
		}
	~ConstructorException() throw() {}

	const char *what() const throw() 
	{
		return msg.c_str();
	}
};

#define throw_line(arg) throw ConstructorException(arg, __FILE__, __LINE__);

class StopException : public std::runtime_error {};