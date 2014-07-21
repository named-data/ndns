/*
 * RR.h
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#ifndef RR_H_
#define RR_H_

#include <string>


#include "ndns-tlv.h"
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/interest.hpp>


using namespace std;

namespace ndn {

enum RRType
{
	NS,
	TXT,
	UNKNOWN
};




class RR {
public:
	RR();
	virtual ~RR();

	const string& getRrdata() const {
		return m_rrData;
	}

	void setRrdata(const string& rrdata) {
		this->m_rrData = rrdata;
	}



private:
template<bool T>
size_t
wireEncode(EncodingImpl<T> & block) const;

public:

const Block&
wireEncode() const;

void
wireDecode(const Block& wire);


Interest
toWire() const;


private:
	unsigned long id;
	string m_rrData;

	mutable Block m_wire;
};

} /* namespace ndn */

#endif /* RR_H_ */
