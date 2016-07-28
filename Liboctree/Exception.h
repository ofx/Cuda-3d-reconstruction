#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class OctreeException : public std::runtime_error
{
private:
	std::string m_Message;

	const bool m_Graceful;
public:
	OctreeException(const std::string &arg, const char *file, int line, bool graceful = false) : std::runtime_error(arg), m_Graceful(graceful)
	{
		std::ostringstream o;
		o << file << ":" << line << ": " << arg;
		this->m_Message = o.str();
	}
	~OctreeException() throw() {}

	const char *what() const throw()
	{
		return this->m_Message.c_str();
	}

	bool isGraceful() { return this->m_Graceful; }
};

#define throw_line(arg) throw OctreeException(arg, __FILE__, __LINE__);
#define throw_line_graceful() throw OctreeException("", __FILE__, __LINE__, true);

class StopException : public std::runtime_error {};