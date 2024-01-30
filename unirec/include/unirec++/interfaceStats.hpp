/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief This file contains the declarations of the interface stats structure.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdint>

#pragma once

namespace Nemea {

/**
 * @brief Structure to store statistics related to an input interface.
 */
struct InputInteraceStats {
	uint64_t receivedBytes; /**< Total number of bytes received. */
	uint64_t receivedRecords; /**< Total number of records received. */
	uint64_t missedRecords; /**< Total number of missed records. */
};

} // namespace Nemea
