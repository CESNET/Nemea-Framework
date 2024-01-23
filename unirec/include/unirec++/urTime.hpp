#pragma once

#include <sys/time.h>
#include <unirec/unirec.h>

namespace Nemea {

struct UrTime {
	ur_time_t time;

	static UrTime now()
	{
		struct timeval t;
		gettimeofday(&t, nullptr);
		return {ur_time_from_sec_msec(t.tv_sec, t.tv_usec / 1000)};
	}
};

static_assert(sizeof(UrTime) == sizeof(ur_time_t), "Invalid header definition");

} // namespace Nemea
