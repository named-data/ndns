/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NDNS (Named Data Networking Domain Name Service).
 * See AUTHORS.md for complete list of NDNS authors and contributors.
 *
 * NDNS is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NDNS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NDNS, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NDNS_DAEMON_CONFIG_FILE_HPP
#define NDNS_DAEMON_CONFIG_FILE_HPP

#include "common.hpp"

#include <boost/property_tree/ptree.hpp>

namespace ndn {
namespace ndns {

/** \brief a config file section
 */
typedef boost::property_tree::ptree ConfigSection;

/** \brief an optional config file section
 */
typedef boost::optional<const ConfigSection&> OptionalConfigSection;

/** \brief callback to process a config file section
 */
typedef function<void(const ConfigSection& section,
                      bool isDryRun,
                      const std::string& filename)> ConfigSectionHandler;

/** \brief callback to process a config file section without a \p ConfigSectionHandler
 */
typedef function<void(const std::string& filename,
                      const std::string& sectionName,
                      const ConfigSection& section,
                      bool isDryRun)> UnknownConfigSectionHandler;

/** \brief configuration file parsing utility
 */
class ConfigFile : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  explicit
  ConfigFile(UnknownConfigSectionHandler unknownSectionCallback = throwErrorOnUnknownSection);

public: // unknown section handlers
  static void
  throwErrorOnUnknownSection(const std::string& filename,
                             const std::string& sectionName,
                             const ConfigSection& section,
                             bool isDryRun);

  static void
  ignoreUnknownSection(const std::string& filename,
                       const std::string& sectionName,
                       const ConfigSection& section,
                       bool isDryRun);

public: // parse helpers
  /** \brief parse a config option that can be either "yes" or "no"
   *  \retval true "yes"
   *  \retval false "no"
   *  \throw Error the value is neither "yes" nor "no"
   */
  static bool
  parseYesNo(const ConfigSection& node, const std::string& key, const std::string& sectionName);

  static bool
  parseYesNo(const ConfigSection::value_type& option, const std::string& sectionName)
  {
    return parseYesNo(option.second, option.first, sectionName);
  }

  /**
   * \brief parse a numeric (integral or floating point) config option
   * \tparam T an arithmetic type
   *
   * \return the numeric value of the parsed option
   * \throw Error the value cannot be converted to the specified type
   */
  template<typename T>
  static T
  parseNumber(const ConfigSection& node, const std::string& key, const std::string& sectionName)
  {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    boost::optional<T> value = node.get_value_optional<T>();
    if (value) {
      return *value;
    }
    NDN_THROW(Error("Invalid value \"" + node.get_value<std::string>() +
                    "\" for option \"" + key + "\" in \"" + sectionName + "\" section"));
  }

  template <typename T>
  static T
  parseNumber(const ConfigSection::value_type& option, const std::string& sectionName)
  {
    return parseNumber<T>(option.second, option.first, sectionName);
  }

public: // setup and parsing
  /// \brief setup notification of configuration file sections
  void
  addSectionHandler(const std::string& sectionName,
                    ConfigSectionHandler subscriber);

  /**
   * \param filename file to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& filename, bool isDryRun);

  /**
   * \param input configuration (as a string) to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if file not found
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const std::string& input, bool isDryRun, const std::string& filename);

  /**
   * \param input stream to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(std::istream& input, bool isDryRun, const std::string& filename);

  /**
   * \param config ConfigSection that needs to be processed
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename logical filename of the config file, can appear in error messages
   * \throws ConfigFile::Error if parse error
   */
  void
  parse(const ConfigSection& config, bool isDryRun, const std::string& filename);

private:
  void
  process(bool isDryRun, const std::string& filename) const;

private:
  UnknownConfigSectionHandler m_unknownSectionCallback;
  std::map<std::string, ConfigSectionHandler> m_subscriptions;
  ConfigSection m_global;
};

} // namespace ndns
} // namespace ndn

#endif // NDNS_DAEMON_CONFIG_FILE_HPP
