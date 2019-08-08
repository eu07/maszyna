#pragma once

#include "object.h"
#include "bindable.h"
#include "buffer.h"

namespace gl
{
    class vao : object, public bindable<vao>
    {
		struct attrib_params {
			// TBD: should be shared_ptr? (when buffer is destroyed by owner VAO could still potentially exist)
			gl::buffer &buffer;

			int attrib;
			int size;
			int type;
			int stride;
			int offset;
		};
		buffer *ebo = nullptr;

		std::vector<attrib_params> params;

    public:
		vao();
		~vao();

		void setup_attrib(buffer &buffer, int attrib, int size, int type, int stride, int offset);
		void setup_ebo(buffer &ebo);

		void bind();
		static void unbind();

		static bool use_vao;
    };
}
