#include "object.h"
#include "bindable.h"

#define UBS_PAD(x) uint64_t : x * 4; uint64_t : x * 4;
#define DEFINE_UBS(x, y) _Pragma("pack(push, 1)"); struct x y; _Pragma("pack(pop)")

namespace gl
{
    static GLuint next_binding_point;

    class ubo : public object, public bindable<ubo>
    {
    public:
        ubo(int size, int index);
        ~ubo();

        using bindable::bind;
        static void bind(GLuint i);

        void update(void *data, int offset, int size);
    };

    _Pragma("pack(push, 1)")

    struct scene_ubs
    {
        glm::mat4 projection;

        void set(const glm::mat4 &m)
        {
            projection = m;
        }
    };

    struct model_ubs
    {
        glm::mat4 modelview;
        glm::mat4 modelviewnormal;

        void set(const glm::mat4 &m)
        {
            modelview = m;
            modelviewnormal = glm::mat3(glm::transpose(glm::inverse(m)));
        }
    };

    _Pragma("pack(pop)")
}
