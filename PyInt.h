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

#ifdef _DEBUG
#undef _DEBUG // bez tego macra Py_DECREF powoduja problemy przy linkowaniu
#include "Python.h"
#define _DEBUG
#else
#include "Python.h"
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

// collection of keyword-value pairs
// NOTE: since our python dictionary operates on a few types, most of the class was hardcoded for simplicity
struct dictionary_source {
// types
    template <typename Type_>
    using keyvaluepair_sequence = std::vector<std::pair<std::string, Type_>>;
// members
    keyvaluepair_sequence<double> floats;
    keyvaluepair_sequence<int> integers;
    keyvaluepair_sequence<bool> bools;
    keyvaluepair_sequence<std::string> strings;
// methods
    inline void insert( std::string const &Key, double const Value )      { floats.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, int const Value )         { integers.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, bool const Value )        { bools.emplace_back( Key, Value ); }
    inline void insert( std::string const &Key, std::string const Value ) { strings.emplace_back( Key, Value ); }
};

// TODO: extract common base and inherit specialization from it
class render_task {

public:
// constructors
    render_task( PyObject *Renderer, dictionary_source *Input, GLuint Target ) :
        m_renderer( Renderer ), m_input( Input ), m_target( Target )
    {}
// methods
	void run();
	void upload();
    void cancel();
    auto target() const -> texture_handle { return m_target; }

private:
// members
    PyObject *m_renderer {nullptr};
    dictionary_source *m_input { nullptr };
    GLuint m_target { 0 };

	unsigned char *m_image = nullptr;
	int m_width, m_height;
	GLenum m_components, m_format;
};

class python_taskqueue {

public:
// types
    struct task_request {

        std::string const &renderer;
        dictionary_source *input;
        GLuint target;
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
