/*
 * RR.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#include "RR.h"

namespace ndn {

RR::RR()
: id (0Lu)
, m_rrData("ex.com")
{
	// TODO Auto-generated constructor stub

}

RR::~RR() {
	// TODO Auto-generated destructor stub
}

template<bool T>
size_t
RR::wireEncode(EncodingImpl<T> & block) const
{
	size_t totalLength = 0;
	string msg = this->getRrdata();
	totalLength += prependByteArrayBlock(block,
						tlv::ndns::RRData,
						reinterpret_cast<const uint8_t*>(msg.c_str()),
						msg.size()
						);

	return totalLength;
}

const Block&
RR::wireEncode() const
{
	if (m_wire.hasWire())
		return m_wire;
	EncodingEstimator estimator;
	size_t estimatedSize = wireEncode(estimator);
	EncodingBuffer buffer(estimatedSize, 0);
	wireEncode(buffer);
	m_wire = buffer.block();
	return m_wire;
}

void
RR::wireDecode(const Block& wire)
{
	if (!wire.hasWire()) {
	    throw Tlv::Error("The supplied block does not contain wire format");
	}

	if (wire.type() != tlv::ndns::RRData)
	    throw Tlv::Error("Unexpected TLV type when decoding Content");

	m_wire = wire;
	m_wire.parse();

	Block::element_const_iterator it = m_wire.elements_begin();

	if (it != m_wire.elements_end() && it->type() == tlv::ndns::RRData)
	{
		m_rrData = string(reinterpret_cast<const char*>(it->value()), it->value_size());
		it ++;
	} else {
		throw Tlv::Error("not the RRData Type");
	}

}



} /* namespace ndn */
