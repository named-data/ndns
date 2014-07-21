/*
 * response.h
 *
 *  Created on: 19 Jul, 2014
 *      Author: shock
 */

#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <boost/asio.hpp> // /opt/local/include
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/noncopyable.hpp>

#include <ndn-cxx/face.hpp> // /usr/local/include
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/tlv-ndnd.hpp>

#include<vector>

#include "ndns-tlv.h"

using namespace std;

namespace ndn {



enum class ResponseType
{
	NDNS_Resp,
	NDNS_Nack,
	NDNS_Auth
};


static const string
toString(ResponseType responseType) const
{
	string label;
	switch (responseType)
	{
		case ResponseType::NDNS_Resp:
			label = "NDNS Resp";
			break;
		case ResponseType::NDNS_Nack:
			label = "NDNS Nack";
			break;
		case ResponseType::NDNS_Auth:
			label = "NDNS Auth";
			break;
		default:
			label = "Default";
			break;
	}
	return label;
}

class Response {
public:
	Response();
	virtual ~Response();

Data
toWire() const;

void
fromWire(const Interest &interest, const Data &data);

private:
	Name m_queryName;
	string m_serial;
	ResponseType m_responseType;

	time::milliseconds m_freshness;

	unsigned int m_numberOfRR;
	vector<RR>  m_rrs;

};

} /* namespace ndn */

#endif /* RESPONSE_H_ */
