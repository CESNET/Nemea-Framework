/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines custom exception classes
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdexcept>

namespace Nemea {

/**
 * @brief An exception that is thrown when the end of the input stream is reached.
 */
class EoFException : public std::exception {};

/**
 * @brief An exception that is thrown when the record format changes.
 */
class FormatChangeException : public std::exception {};

/**
 * @brief This exception is thrown when the libtrap command-line argument contains help flag.
 */
class HelpException : public std::exception {};

} // namespace Nemea
