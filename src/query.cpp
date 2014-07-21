/*
 * query.cpp
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#include "query.h"

namespace ndn {

Query::Query()
: m_interestLifetime(time::milliseconds(4000))
{
	// TODO Auto-generated constructor stub

}

Query::~Query() {
	// TODO Auto-generated destructor stub
}


void
Query::fromWire(const Name &name, const Interest &interest)
{
    Name interestName;
    interestName = interest.getName();

    int qtflag = -1;
    size_t len = interestName.size();
    for (size_t i=0; i<len; i++)
    {
    	string comp = interestName.get(i).toEscapedString();
    	if (comp == ndn::toString(QueryType::DNS) || comp == ndn::toString(QueryType::DNS_R))
    	{
    		qtflag = i;
    		break;
    	}
    }//for

    if (qtflag == -1)
    {
    	cerr<<"There is no QueryType in the Interest Name: "<<interestName<<endl;
    	return;
    }
    this->m_queryType = ndn::toQueryType(interestName.get(qtflag).toEscapedString());
    this->m_rrType = ndn::toRRType(interestName.get(len-1).toEscapedString());
    this->m_authorityZone = interestName.getPrefix(qtflag); //the DNS/DNS-R is not included
    this->m_interestLifetime = interest.getInterestLifetime();

}

template<bool T>
size_t
Query::wireEncode(EncodingImpl<T>& block) const
{
	size_t totalLength = 0;
	totalLength += 0;


  size_t totalLength = prependByteArrayBlock(block, tlv::Bytes,
                                             m_key.get().buf(), m_key.get().size());
  totalLength += m_keyName.wireEncode(block);
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::PublicKey);

  return totalLength;
}

Interest
Query::toWire() const
{
	Name name = this->m_authorityZone;
	name.append(ndn::toString(this->m_queryType));
	name.append(this->m_rrLabel);
	name.append(ndn::toString(this->m_rrType));
	Selectors selector;
	//selector.setMustBeFresh(true);

	Interest interest = Interest(name, selector, -1, this->m_interestLifetime);
	return interest;
}

} /* namespace ndn */
