#pragma once

#include <string>

class Guid
{
public:
	Guid():m_data0(0), m_data1(0), m_data2(0), m_data3(0) {}
	Guid(unsigned int _0, unsigned int _1, unsigned int _2, unsigned int _3) :m_data0(_0), m_data1(_1), m_data2(_2), m_data3(_3) {}
    Guid(const std::string& str) { fromString(str); }

	const static Guid zero;

public:
	unsigned int	m_data0;
	unsigned int	m_data1;
	unsigned int	m_data2;
	unsigned int	m_data3;

public:
	static Guid generate();

	Guid(const Guid&);
	Guid& operator = (const Guid&) = default;
	bool operator == (const Guid& other) const;
	bool operator != (const Guid& other) const;
	bool operator > (const Guid& other) const;
	bool operator < (const Guid& other) const;
	static const Guid& Invalid();
	std::string toString() const;
	void fromString(const std::string& str);
};