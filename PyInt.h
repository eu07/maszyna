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

#ifdef __GNUC__
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

#include "Classes.h"
#include "utilities.h"

#define PyGetFloat(param) PyFloat_FromDouble(param >= 0 ? param : -param)
#define PyGetFloatS(param) PyFloat_FromDouble(param)
#define PyGetInt(param) PyInt_FromLong(param)
#define PyGetBool(param) param ? Py_True : Py_False
#define PyGetString(param) PyString_FromString(param)

// TODO: extract common base and inherit specialization from it
class render_task {

public:
// constructors
    render_task( PyObject *Renderer, PyObject *Input, texture_handle Target ) :
        m_renderer( Renderer ), m_input( Input ), m_target( Target )
    {}
// methods
    void run();
    void cancel();
    auto target() const -> texture_handle { return m_target; }

private:
// members
    PyObject *m_renderer {nullptr};
    PyObject *m_input { nullptr };
    texture_handle m_target { null_handle };
};

class python_taskqueue {

public:
// types
    struct task_request {

        std::string const &renderer;
        PyObject *input;
        texture_handle target;
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

private:
// types
    static int const WORKERCOUNT { 1 };
    using worker_array = std::array<std::unique_ptr<std::thread>, WORKERCOUNT >;
    using rendertask_sequence = threading::lockable< std::deque<render_task *> >;
// methods
    auto fetch_renderer( std::string const Renderer ) -> PyObject *;
    void run( GLFWwindow *Context, rendertask_sequence &Tasks, threading::condition_variable &Condition, std::atomic<bool> &Exit );
    void error();
// members
    PyObject *m_main { nullptr };
    PyObject *m_error { nullptr };
    PyThreadState *m_mainthread{ nullptr };
    worker_array m_workers;
    threading::condition_variable m_condition; // wakes up the workers
    std::atomic<bool> m_exit { false }; // signals the workers to quit
    std::unordered_map<std::string, PyObject *> m_renderers; // cache of python classes
    rendertask_sequence m_tasks;
    bool m_initialized { false };
};

#endif
