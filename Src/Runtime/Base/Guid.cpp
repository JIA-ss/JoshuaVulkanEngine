#include "Guid.h"
#include <combaseapi.h>
#include <array>
#include <guid/guid.hpp>


constexpr std::array<short, 16> CharTable = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

template<short ...I>
constexpr std::array<short, 256> generateHexTable(std::integer_sequence<short, I...>)
{
    // assume that Windows is always little-endian
    std::array<short, 256> output {((CharTable[I & 0xF] << 8) | (CharTable[(I >> 4) & 0xF]))...};
    return output;
}

constexpr auto HexTable = generateHexTable(std::make_integer_sequence<short, 256>{});

void toHex(char output[37], std::array<unsigned int, 16> hexs)
{
    auto cursor = output;
    auto hexIndex = 0;
    for (auto i = 0; i < 4; i++)
    {
        *(reinterpret_cast<short*>(cursor)) = HexTable[hexs[hexIndex++]];
        cursor += 2;
    }

    *(cursor++) = '-';

    for (auto i = 0; i < 3; ++i)
    {
        *(reinterpret_cast<short*>(cursor)) = HexTable[hexs[hexIndex++]];
        cursor += 2;
        *(reinterpret_cast<short*>(cursor)) = HexTable[hexs[hexIndex++]];
        cursor += 2;
        *(cursor++) = '-';
    }

    for (auto i = 0; i < 6; ++i)
    {
        *(reinterpret_cast<short*>(cursor)) = HexTable[hexs[hexIndex++]];
        cursor += 2;
    }

    output[36] = '\0';
}


const Guid Guid::zero;

const Guid& Guid::Invalid()
{
	static Guid guid_(-1, -1, -1, -1);
	return guid_;
}


Guid Guid::generate()
{
	while (1)
	{
		Guid hr;
        xg::Guid t;
		xg::Guid xg_guid = xg::newGuid();
		hr.m_data0 = xg_guid.bytes()[0] << 24 | xg_guid.bytes()[1] << 16 | xg_guid.bytes()[2] << 8 | xg_guid.bytes()[3];
		hr.m_data1 = xg_guid.bytes()[4] << 24 | xg_guid.bytes()[5] << 16 | xg_guid.bytes()[6] << 8 | xg_guid.bytes()[7];
		hr.m_data2 = xg_guid.bytes()[8] << 24 | xg_guid.bytes()[9] << 16 | xg_guid.bytes()[10] << 8 | xg_guid.bytes()[11];
		hr.m_data3 = xg_guid.bytes()[12] << 24 | xg_guid.bytes()[13] << 16 | xg_guid.bytes()[14] << 8 | xg_guid.bytes()[15];

		if(hr.m_data0 != 0) return hr;
	}
}

std::string Guid::toString() const
{
	char out[37];

	snprintf(out, sizeof(out), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		(m_data0 >> 24) & 0x000000ff, (m_data0 >> 16) & 0x000000ff, (m_data0 >> 8) & 0x000000ff, m_data0 & 0x000000ff,
		(m_data1 >> 24) & 0x000000ff, (m_data1 >> 16) & 0x000000ff, (m_data1 >> 8) & 0x000000ff, m_data1 & 0x000000ff,
		(m_data2 >> 24) & 0x000000ff, (m_data2 >> 16) & 0x000000ff, (m_data2 >> 8) & 0x000000ff, m_data2 & 0x000000ff,
		(m_data3 >> 24) & 0x000000ff, (m_data3 >> 16) & 0x000000ff, (m_data3 >> 8) & 0x000000ff, m_data3 & 0x000000ff);

	return out;
}

void Guid::fromString(const std::string& str)
{
	unsigned int v[16];
	int hr = sscanf(str.c_str(), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		&v[0], &v[1], &v[2], &v[3],
		&v[4], &v[5], &v[6], &v[7],
		&v[8], &v[9], &v[10], &v[11],
		&v[12], &v[13], &v[14], &v[15]
	);

	if (hr == 16)
	{
		m_data0 = v[0] << 24 | v[1] << 16 | v[2] << 8 | v[3];
		m_data1 = v[4] << 24 | v[5] << 16 | v[6] << 8 | v[7];
		m_data2 = v[8] << 24 | v[9] << 16 | v[10] << 8 | v[11];
		m_data3 = v[12] << 24 | v[13] << 16 | v[14] << 8 | v[15];
	}
	else
	{
		*this = zero;
	}
}

Guid::Guid(const Guid& o)
	: m_data0(o.m_data0)
	, m_data1(o.m_data1)
	, m_data2(o.m_data2)
	, m_data3(o.m_data3)
{
}

bool Guid::operator == (const Guid& o) const
{
	return !(*this != o);
}

bool Guid::operator != (const Guid& o) const
{
	return m_data0 != o.m_data0 || m_data1 != o.m_data1
		|| m_data2 != o.m_data2 || m_data3 != o.m_data3;
}

bool Guid::operator > (const Guid& o) const
{
	if (m_data3 != o.m_data3)
		return m_data3 > o.m_data3;
	if (m_data2 != o.m_data2)
		return m_data2 > o.m_data2;
	if (m_data1 != o.m_data1)
		return m_data1 > o.m_data1;
	if (m_data0 != o.m_data0)
		return m_data0 > o.m_data0;
	return false;
}

bool Guid::operator < (const Guid& o) const
{
	if (m_data3 != o.m_data3)
		return m_data3 < o.m_data3;
	if (m_data2 != o.m_data2)
		return m_data2 < o.m_data2;
	if (m_data1 != o.m_data1)
		return m_data1 < o.m_data1;
	if (m_data0 != o.m_data0)
		return m_data0 < o.m_data0;
	return false;
}