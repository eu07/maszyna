#pragma once

#include "object.h"

namespace gl
{
class query : public object
{
public:
	enum targets
	{
		SAMPLES_PASSED = 0,
		ANY_SAMPLES_PASSED,
		ANY_SAMPLES_PASSED_CONSERVATIVE,
		PRIMITIVES_GENERATED,
		TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
		TIME_ELAPSED
	};

private:
	targets target;
	thread_local static query* active_queries[6];

protected:
	GLenum glenum_target(targets target);

public:
	query(targets target);
	~query();

	void begin();
	void end();

	std::optional<int64_t> result();
};
}
