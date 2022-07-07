/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef PYINT_H
#define PYINT_H

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 5033 )
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#endif

#ifdef WITH_PYTHON
#ifdef _DEBUG
#undef _DEBUG // bez tego macra Py_DECREF powoduja problemy przy linkowaniu
#include "Python.h"
#define _DEBUG
#else
#include "Python.h"
#endif
#else
#define PyObject void
#define PyThreadState void
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "Classes.h"
#include "utilities.h"

#define PyGetFloat(param) PyFloat_FromDouble(param)
#define PyGetInt(param) PyInt_FromLong(param)
#define PyGetBool(param) param ? Py_True : Py_False
#define PyGetString(param) PyString_FromString(param)

// python rendertarget
struct python_rt {
	std::mutex mutex;

	GLuint shared_tex;

	int format;
	int components;
	int width;
	int height;
	unsigned char *image = nullptr;

	std::chrono::high_resolution_clock::time_point timestamp;

	~python_rt() {
		if (image)
			delete[] image;
	}
};

// TODO: extract common base and inherit specialization from it
class render_task {

public:
// constructors
	render_task( PyObject *Renderer, dictionary_source *Input, std::shared_ptr<python_rt> Target ) :
        m_renderer( Renderer ), m_input( Input ), m_target( Target )
    {}
// methods
	void run();
	void upload();
    void cancel();
	auto target() const -> std::shared_ptr<python_rt> { return m_target; }

private:
// members
    PyObject *m_renderer {nullptr};
    dictionary_source *m_input { nullptr };
	std::shared_ptr<python_rt> m_target { nullptr };
};

class python_taskqueue {

public:
// types
    struct task_request {

        std::string const &renderer;
        dictionary_source *input;
		std::shared_ptr<python_rt> target;
    };
// constructors
    python_taskqueue() = default;
// methods
    // initializes the module. returns true on success
    auto init() -> bool;
    // shuts down the module
    void exit();
    // adds specified task along with provided collection of data to the work queue. returns true on success
    auto insert( task_request const &Task ) -> bool;
    // executes python script stored in specified file. returns true on success
    auto run_file( std::string const &File, std::string const &Path = "" ) -> bool;
    // acquires the python gil and sets the main thread as current
    void acquire_lock();
    // releases the python gil and swaps the main thread out
    void release_lock();

	void update();

private:
// types
    static int const WORKERCOUNT { 1 };
    using worker_array = std::array<std::thread, WORKERCOUNT >;
    using rendertask_sequence = threading::lockable< std::deque<render_task *> >;
	using uploadtask_sequence = threading::lockable< std::deque<render_task *> >;
// methods
    auto fetch_renderer( std::string const Renderer ) -> PyObject *;
	void run(GLFWwindow *Context, rendertask_sequence &Tasks, uploadtask_sequence &Upload_Tasks, threading::condition_variable &Condition, std::atomic<bool> &Exit );
    void error();
// members
    PyObject *m_main { nullptr };
    PyObject *m_stderr { nullptr };
    PyThreadState *m_mainthread{ nullptr };
    worker_array m_workers;
    threading::condition_variable m_condition; // wakes up the workers
    std::atomic<bool> m_exit { false }; // signals the workers to quit
    std::unordered_map<std::string, PyObject *> m_renderers; // cache of python classes
    rendertask_sequence m_tasks;
	uploadtask_sequence m_uploadtasks;
    bool m_initialized { false };
};

#endif
