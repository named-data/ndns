#ifndef NDNS_TESTS_BOOST_TEST_HPP
#define NDNS_TESTS_BOOST_TEST_HPP

// suppress warnings from Boost.Test
#pragma GCC system_header
#pragma clang system_header

#include "logger.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/concept_check.hpp>
#include <boost/test/output_test_stream.hpp>

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>

namespace ndn {
namespace ndns {
namespace tests {

} //tests
} //ndns
} //ndn

#endif // NDN_TESTS_BOOST_TEST_HPP
