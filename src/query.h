/*
 * query.h
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#ifndef QUERY_H_
#define QUERY_H_

#include <ndn-cxx/name.hpp>
#include <rr.h>

using namespace std;

namespace ndn {


enum class QueryType
{
	DNS,
	DNS_R,
};

static const string
toString(QueryType qType) const
{
	string label;
	switch (qType)
	{
		case QueryType::DNS:
			label = "DNS";
			break;
		case QueryType::DNS_R:
			label = "DNS-R";
			break;
		default:
			label = "Default";
			break;
	}
	return label;

}

static const QueryType
toQueryType(string str)
{
	QueryType atype;
	switch (str)
	{
		case "DNS":
			atype = QueryType::DNS;
			break;
		case "DNS-R":
			atype = QueryType::DNS_R;
			break;
		defalut:
			atype = QueryType::DNS;
			break;
	}
	return atype;
}




class Query {
public:
	Query();

	virtual ~Query();



	const Name& getAuthorityZone() const {
		return m_authorityZone;
	}

	void setAuthorityZone(const Name& authorityZone) {
		m_authorityZone = authorityZone;
	}

	time::milliseconds getInterestLifetime() const {
		return m_interestLifetime;
	}

	void setInterestLifetime(time::milliseconds interestLifetime) {
		m_interestLifetime = interestLifetime;
	}

	enum QueryType getQueryType() const {
		return m_queryType;
	}

	void setQueryType(enum QueryType queryType) {
		m_queryType = queryType;
	}

	const Name& getRrLabel() const {
		return m_rrLabel;
	}

	void setRrLabel(const Name& rrLabel) {
		m_rrLabel = rrLabel;
	}

	const RRType& getRrType() const {
		return m_rrType;
	}

	void setRrType(const RRType& rrType) {
		m_rrType = rrType;
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

void
fromWire(const Name &name, const Interest& interest);





public:
	Name m_authorityZone;
	enum QueryType m_queryType;
	time::milliseconds m_interestLifetime;
	Name m_rrLabel;
	enum RRType m_rrType;

	mutable Block m_wire;
	//bool hasWire;
};

} /* namespace ndn */

#endif /* QUERY_H_ */
