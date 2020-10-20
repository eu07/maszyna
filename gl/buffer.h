#pragma once

#include "object.h"
#include "bindable.h"

namespace gl
{
    class buffer : public object
    {
		thread_local static GLuint binding_points[13];

    public:
        enum targets
        {
            ARRAY_BUFFER = 0,
            ATOMIC_COUNTER_BUFFER,
            COPY_READ_BUFFER,
            COPY_WRITE_BUFFER,
            DISPATCH_INDIRECT_BUFFER,
            DRAW_INDIRECT_BUFFER,
            ELEMENT_ARRAY_BUFFER, // ELEMENT_ARRAY_BUFFER is part of VAO and therefore shouldn't be managed by global tracker!
            PIXEL_PACK_BUFFER,
            PIXEL_UNPACK_BUFFER,
            SHADER_STORAGE_BUFFER,
            TEXTURE_BUFFER,
            TRANSFORM_FEEDBACK_BUFFER,
            UNIFORM_BUFFER
        };

	protected:
        static GLenum glenum_target(targets target);

    public:
        buffer();
        ~buffer();

        void bind(targets target);
        void bind_base(targets target, GLuint index);
        static void unbind(targets target);
		static void unbind();

        void allocate(targets target, int size, GLenum hint);
        void upload(targets target, const void *data, int offset, int size);
        void download(targets target, void *data, int offset, int size);
    };
}
