/*
 * zone.h
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#ifndef ZONE_H_
#define ZONE_H_

#include <string>
#include "rr.h"


using namespace std;

namespace ndn {

class Zone {
public:
	Zone();
	virtual ~Zone();

	const RR hasName(string key);

};

} /* namespace ndn */

#endif /* ZONE_H_ */
