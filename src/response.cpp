/*
 * response.cpp
 *
 *  Created on: 19 Jul, 2014
 *      Author: shock
 */

#include "response.h"



namespace ndn {

Response::Response() {
	// TODO Auto-generated constructor stub

}

Response::~Response() {
	// TODO Auto-generated destructor stub
}



Data
Response::toWire() const
{
	Name name = this->m_queryName;
	name.append(this->m_serial);

	Data data(name);

	data.setFreshnessPeriod(this->m_freshness);

	string content = "";

	size_t totalLen = 0;
	Block block = Block();
	block.push_back
		(nonNegativeIntegerBlock
				(tlv::ndns::Type, static_cast<unsigned long>(this->m_responseType))
		);

	block.push_back
			(nonNegativeIntegerBlock
					(tlv::ndns::Fressness, this->m_freshness.count())
			);

	Block block2 = Block(tlv::ndns::ContentBlob);
	block2.push_back
			(nonNegativeIntegerBlock
					(tlv::ndns::NumberOfRRData, this->m_numberOfRR)
			);

	for (int i=0; i<this->m_numberOfRR; i++)
	{
		RR rr = m_rrs[i];
		block2.push_back(rr.toWire());
	}

	block.push_back(block2);

	return data;

}

void
Response::fromWire(const Interest &interest, const Data &data)
{
    Name dataName;
    dataName = data.getName();

    int qtflag = -1;
    size_t len = dataName.size();
    for (size_t i=0; i<len; i++)
    {
    	string comp = dataName.get(i).toEscapedString();
    	if (comp == ndn::toString(QueryType::DNS) || comp == ndn::toString(QueryType::DNS_R))
    	{
    		qtflag = i;
    		break;
    	}
    }//for

    if (qtflag == -1)
    {
    	cerr<<"There is no QueryType in the Interest Name: "<<dataName<<endl;
    	return;
    }

    this->m_queryName = dataName.getPrefix(-1);

    string last = dataName.get(len-1).toEscapedString();
    if (ndn::toRRType(last) == RRType::UNKNOWN)
    {
    	this->m_serial = "";
    } else
    {
    	this->m_serial = last;
    }

    Block block = data.getContent();
    this->m_numberOfRR = readNonNegativeInteger(block.get(tlv::ndns::NumberOfRRData));

    Block block2 = block.get(tlv::ndns::ContentBlob);
    for (int i=0; i<this->m_numberOfRR; i++)
    {
    	Block block3 = block2.get(tlv::ndns::RRData);
    	RR rr;
    	rr.fromWire(block3);
    	m_rrs.push_back(rr);
    }

}


} /* namespace ndn */




