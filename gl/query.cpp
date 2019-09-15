#include "stdafx.h"
#include "query.h"

gl::query::query(targets target)
    : target(target)
{
	glGenQueries(1, *this);
}

gl::query::~query()
{
	end();
	glDeleteQueries(1, *this);
}

void gl::query::begin()
{
	if (active_queries[target])
		active_queries[target]->end();

	glBeginQuery(glenum_target(target), *this);
	active_queries[target] = this;
}

void gl::query::end()
{
	if (active_queries[target] != this)
		return;

	glEndQuery(glenum_target(target));
	active_queries[target] = nullptr;
}

std::optional<int64_t> gl::query::result()
{
	GLuint ready;
	glGetQueryObjectuiv(*this, GL_QUERY_RESULT_AVAILABLE, &ready);
	int64_t value;
	if (ready) {
		glGetQueryObjecti64v(*this, GL_QUERY_RESULT, &value);
		return std::optional<int64_t>(value);
	}
	return std::nullopt;
}

GLenum gl::query::glenum_target(targets target)
{
	static GLenum mapping[6] =
	{
	    GL_SAMPLES_PASSED,
	    GL_ANY_SAMPLES_PASSED,
	    GL_ANY_SAMPLES_PASSED_CONSERVATIVE,
	    GL_PRIMITIVES_GENERATED,
	    GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
	    GL_TIME_ELAPSED
	};
	return mapping[target];
}

thread_local gl::query* gl::query::active_queries[6];
