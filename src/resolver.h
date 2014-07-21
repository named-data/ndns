/*
 * resolver.h
 *
 *  Created on: 18 Jul, 2014
 *      Author: shock
 */

#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <string>
#include "rr.h"


using namespace std;

namespace ndn {

class Resolver {
public:
	Resolver();
	virtual ~Resolver();


	const RR iterativelyResolve(const string domain, const string name);
	const RR recusivelyResolve(const string domain, const string name);

};

} /* namespace ndn */

#endif /* RESOLVER_H_ */
