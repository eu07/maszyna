#pragma once

namespace gl
{
    template <typename T>
    class bindable
    {
    protected:
		thread_local static bindable<T>* active;

    public:
        void bind()
        {
            if (active == this)
                return;
            active = this;
            T::bind(*static_cast<T*>(active));
        }
        static void unbind()
        {
            active = nullptr;
            T::bind(0);
        }
    };

	template <typename T> thread_local bindable<T>* bindable<T>::active;
}
