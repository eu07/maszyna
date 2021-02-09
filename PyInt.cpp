/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "PyInt.h"

#include "dictionary.h"
#include "application.h"
#include "Logs.h"
#include "Globals.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

void render_task::run() {

    // convert provided input to a python dictionary
    auto *input = PyDict_New();
    if (input == nullptr) {
		cancel();
		return;
	}
    for( auto const &datapair : m_input->floats )   { auto *value{ PyGetFloat( datapair.second ) }; PyDict_SetItemString( input, datapair.first.c_str(), value ); Py_DECREF( value ); }
    for( auto const &datapair : m_input->integers ) { auto *value{ PyGetInt( datapair.second ) }; PyDict_SetItemString( input, datapair.first.c_str(), value ); Py_DECREF( value ); }
    for( auto const &datapair : m_input->bools )    { auto *value{ PyGetBool( datapair.second ) }; PyDict_SetItemString( input, datapair.first.c_str(), value ); }
    for( auto const &datapair : m_input->strings )  { auto *value{ PyGetString( datapair.second.c_str() ) }; PyDict_SetItemString( input, datapair.first.c_str(), value ); Py_DECREF( value ); }
    for (auto const &datapair : m_input->vec2_lists) {
        PyObject *list = PyList_New(datapair.second.size());

        for (size_t i = 0; i < datapair.second.size(); i++) {
            auto const &vec = datapair.second[i];
                        WriteLog("passing " + glm::to_string(vec));

            PyObject *tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyGetFloat(vec.x)); // steals ref
            PyTuple_SetItem(tuple, 1, PyGetFloat(vec.y)); // steals ref

            PyList_SetItem(list, i, tuple); // steals ref
        }

        PyDict_SetItemString(input, datapair.first.c_str(), list);
        Py_DECREF(list);
    }
	delete m_input;
	m_input = nullptr;

    // call the renderer
    auto *output { PyObject_CallMethod( m_renderer, "render", "O", input ) };
    Py_DECREF( input );

    if( output != nullptr ) {
        auto *outputwidth { PyObject_CallMethod( m_renderer, "get_width", nullptr ) };
        auto *outputheight { PyObject_CallMethod( m_renderer, "get_height", nullptr ) };
        // upload texture data
        if( ( outputwidth != nullptr )
         && ( outputheight != nullptr )
		 && m_target) {
			int width = PyInt_AsLong( outputwidth );
			int height = PyInt_AsLong( outputheight );
			int components, format;

            const unsigned char *image = reinterpret_cast<const unsigned char *>( PyString_AsString( output ) );

			std::lock_guard<std::mutex> guard(m_target->mutex);
			if (m_target->image)
				delete[] m_target->image;

			if (!Global.gfx_usegles)
			{
				int size = width * height * 3;
				format = GL_SRGB8;
				components = GL_RGB;
				m_target->image = new unsigned char[size];
				memcpy(m_target->image, image, size);
			}
			else
			{
				format = GL_SRGB8_ALPHA8;
				components = GL_RGBA;
				m_target->image = new unsigned char[width * height * 4];

				int w = width;
				int h = height;
				for (int y = 0; y < h; y++)
					for (int x = 0; x < w; x++)
					{
						m_target->image[(y * w + x) * 4 + 0] = image[(y * w + x) * 3 + 0];
						m_target->image[(y * w + x) * 4 + 1] = image[(y * w + x) * 3 + 1];
						m_target->image[(y * w + x) * 4 + 2] = image[(y * w + x) * 3 + 2];
						m_target->image[(y * w + x) * 4 + 3] = 0xFF;
					}
			}

			m_target->width = width;
			m_target->height = height;
			m_target->components = components;
			m_target->format = format;
			m_target->timestamp = std::chrono::high_resolution_clock::now();
        }
        if( outputheight != nullptr ) { Py_DECREF( outputheight ); }
        if( outputwidth  != nullptr ) { Py_DECREF( outputwidth ); }
        Py_DECREF( output );
    }
}

void render_task::upload()
{
	if (Global.python_uploadmain && m_target->image)
	{
		glBindTexture(GL_TEXTURE_2D, m_target->shared_tex);
		glTexImage2D(
		    GL_TEXTURE_2D, 0,
		    m_target->format,
		    m_target->width, m_target->height, 0,
		    m_target->components, GL_UNSIGNED_BYTE, m_target->image);

		if (Global.python_mipmaps)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		if (Global.python_threadedupload)
			glFlush();
	}

	delete this;
}

void render_task::cancel() {

    delete m_input;
    delete this;
}

// initializes the module. returns true on success
auto python_taskqueue::init() -> bool {

	crashreport_add_info("python.threadedupload", Global.python_threadedupload ? "yes" : "no");
	crashreport_add_info("python.uploadmain", Global.python_uploadmain ? "yes" : "no");

#ifdef _WIN32
	if (sizeof(void*) == 8)
		Py_SetPythonHome("python64");
	else
		Py_SetPythonHome("python");
#elif __linux__
	if (sizeof(void*) == 8)
		Py_SetPythonHome("linuxpython64");
	else
		Py_SetPythonHome("linuxpython");
#elif __APPLE__
	if (sizeof(void*) == 8)
		Py_SetPythonHome("macpython64");
	else
		Py_SetPythonHome("macpython");
#endif
    Py_InitializeEx(0);

    PyEval_InitThreads();

	PyObject *stringiomodule { nullptr };
	PyObject *stringioclassname { nullptr };
	PyObject *stringioobject { nullptr };

    // do the setup work while we hold the lock
    m_main = PyImport_ImportModule("__main__");
    if (m_main == nullptr) {
        ErrorLog( "Python Interpreter: __main__ module is missing" );
        goto release_and_exit;
    }

    stringiomodule = PyImport_ImportModule( "cStringIO" );
    stringioclassname = (
        stringiomodule != nullptr ?
            PyObject_GetAttrString( stringiomodule, "StringIO" ) :
            nullptr );
    stringioobject = (
        stringioclassname != nullptr ?
            PyObject_CallObject( stringioclassname, nullptr ) :
            nullptr );
    m_stderr = { (
        stringioobject == nullptr ? nullptr :
        PySys_SetObject( "stderr", stringioobject ) != 0 ? nullptr :
        stringioobject ) };

    if( false == run_file( "abstractscreenrenderer" ) ) { goto release_and_exit; }

    // release the lock, save the state for future use
    m_mainthread = PyEval_SaveThread();

    WriteLog( "Python Interpreter: setup complete" );

    // init workers
    for( auto &worker : m_workers ) {

		GLFWwindow *openglcontextwindow = nullptr;
		if (Global.python_threadedupload)
			openglcontextwindow = Application.window( -1 );
        worker = std::thread(
                    &python_taskqueue::run, this,
		            openglcontextwindow, std::ref( m_tasks ), std::ref(m_uploadtasks), std::ref( m_condition ), std::ref( m_exit ) );

        if( false == worker.joinable() ) { return false; }
    }

    m_initialized = true;

    return true;

release_and_exit:
    PyEval_ReleaseLock();
    return false;
}

// shuts down the module
void python_taskqueue::exit() {
    if (!m_initialized)
        return;

    // let the workers know we're done with them
    m_exit = true;
    m_condition.notify_all();
    // let them free up their shit before we proceed
    for( auto &worker : m_workers ) {
	if (worker.joinable())
	        worker.join();
    }
    // get rid of the leftover tasks
    // with the workers dead we don't have to worry about concurrent access anymore
    for( auto *task : m_tasks.data ) {
        task->cancel();
    }
    // take a bow
    acquire_lock();
    Py_Finalize();
}

// adds specified task along with provided collection of data to the work queue. returns true on success
auto python_taskqueue::insert( task_request const &Task ) -> bool {

    if( !m_initialized
     || ( false == Global.python_enabled )
     || ( Task.renderer.empty() )
     || ( Task.input == nullptr )
     || ( Task.target == 0 ) ) { return false; }

    auto *renderer { fetch_renderer( Task.renderer ) };
    if( renderer == nullptr ) { return false; }

    auto *newtask { new render_task( renderer, Task.input, Task.target ) };
    bool newtaskinserted { false };
    // acquire a lock on the task queue and add the new task
    {
        std::lock_guard<std::mutex> lock( m_tasks.mutex );
        // check the task list for a pending request with the same target
        for( auto &task : m_tasks.data ) {
            if( task->target() == Task.target ) {
                // replace pending task in the slot with the more recent one
                task->cancel();
                task = newtask;
                newtaskinserted = true;
                break;
            }
        }
        if( false == newtaskinserted ) {
            m_tasks.data.emplace_back( newtask );
        }
    }
    // potentially wake a worker to handle the new task
    m_condition.notify_one();
    // all done
    return true;
}

// executes python script stored in specified file. returns true on success
auto python_taskqueue::run_file( std::string const &File, std::string const &Path ) -> bool {

    auto const lookup { FileExists( { Path + File, "python/local/" + File }, { ".py" } ) };
    if( lookup.first.empty() ) { return false; }

    std::ifstream inputfile { lookup.first + lookup.second };
    std::string input;
    input.assign( std::istreambuf_iterator<char>( inputfile ), std::istreambuf_iterator<char>() );

    if( PyRun_SimpleString( input.c_str() ) != 0 ) {
        error();
        return false;
    }

    return true;
}

// acquires the python gil and sets the main thread as current
void python_taskqueue::acquire_lock() {

    PyEval_RestoreThread( m_mainthread );
}

// releases the python gil and swaps the main thread out
void python_taskqueue::release_lock() {

    PyEval_SaveThread();
}

auto python_taskqueue::fetch_renderer( std::string const Renderer ) ->PyObject * {

    auto const lookup { m_renderers.find( Renderer ) };
    if( lookup != std::end( m_renderers ) ) {
        return lookup->second;
    }
    // try to load specified renderer class
    auto const path { substr_path( Renderer ) };
    auto const file { Renderer.substr( path.size() ) };
    PyObject *renderer { nullptr };
    PyObject *rendererarguments { nullptr };
    PyObject *renderername { nullptr };
    acquire_lock();
    {
        if( m_main == nullptr ) {
            ErrorLog( "Python Renderer: __main__ module is missing" );
            goto cache_and_return;
        }

        if( false == run_file( file, path ) ) {
            goto cache_and_return;
        }
        renderername = PyObject_GetAttrString( m_main, file.c_str() );
        if( renderername == nullptr ) {
            ErrorLog( "Python Renderer: class \"" + file + "\" not defined" );
            goto cache_and_return;
        }
        rendererarguments = Py_BuildValue( "(s)", path.c_str() );
        if( rendererarguments == nullptr ) {
            ErrorLog( "Python Renderer: failed to create initialization arguments" );
            goto cache_and_return;
        }
        renderer = PyObject_CallObject( renderername, rendererarguments );

        if( PyErr_Occurred() != nullptr ) {
            error();
            renderer = nullptr;
        }

cache_and_return:
        // clean up after yourself
        if( rendererarguments != nullptr ) {
            Py_DECREF( rendererarguments );
        }
    }
    release_lock();
    // cache the failures as well so we don't try again on subsequent requests
    m_renderers.emplace( Renderer, renderer );
    return renderer;
}

void python_taskqueue::run( GLFWwindow *Context, rendertask_sequence &Tasks, uploadtask_sequence &Upload_Tasks, threading::condition_variable &Condition, std::atomic<bool> &Exit ) {

	if (Context)
		glfwMakeContextCurrent( Context );

    // create a state object for this thread
    PyEval_AcquireLock();
    auto *threadstate { PyThreadState_New( m_mainthread->interp ) };
    PyEval_ReleaseLock();

    render_task *task { nullptr };

    while( false == Exit.load() ) {
        // regardless of the reason we woke up prime the spurious wakeup flag for the next time
        Condition.spurious( true );
        // keep working as long as there's any scheduled tasks
        do {
            task = nullptr;
            // acquire a lock on the task queue and potentially grab a task from it
            {
                std::lock_guard<std::mutex> lock( Tasks.mutex );
                if( false == Tasks.data.empty() ) {
                    // fifo
                    task = Tasks.data.front();
                    Tasks.data.pop_front();
                }
            }
            if( task != nullptr ) {
                // swap in my thread state
                PyEval_RestoreThread( threadstate );
                {
                    // execute python code
                    task->run();
					if (Context)
						task->upload();
					else
					{
						std::lock_guard<std::mutex> lock(Upload_Tasks.mutex);
						Upload_Tasks.data.push_back(task);
					}
					if( PyErr_Occurred() != nullptr )
						error();
                }
                // clear the thread state
                PyEval_SaveThread();
            }
            // TBD, TODO: add some idle time between tasks in case we're on a single thread cpu?
        } while( task != nullptr );
        // if there's nothing left to do wait until there is
        // but check every now and then on your own to minimize potential deadlock situations
        Condition.wait_for( std::chrono::seconds( 5 ) );
    }
    // clean up thread state data
    PyEval_AcquireLock();
    PyThreadState_Swap( nullptr );
    PyThreadState_Clear( threadstate );
    PyThreadState_Delete( threadstate );
    PyEval_ReleaseLock();
}

void python_taskqueue::update()
{
	std::lock_guard<std::mutex> lock(m_uploadtasks.mutex);

	for (auto &task : m_uploadtasks.data)
		task->upload();

	m_uploadtasks.data.clear();
}

void
python_taskqueue::error() {

    if( m_stderr != nullptr ) {
        // std err pythona jest buforowane
        PyErr_Print();
        auto *errortext { PyObject_CallMethod( m_stderr, "getvalue", nullptr ) };
        ErrorLog( PyString_AsString( errortext ) );
        // czyscimy bufor na kolejne bledy
        PyObject_CallMethod( m_stderr, "truncate", "i", 0 );
    }
    else {
        // nie dziala buffor pythona
        PyObject *type, *value, *traceback;
        PyErr_Fetch( &type, &value, &traceback );
        if( type == nullptr ) {
            ErrorLog( "Python Interpreter: don't know how to handle null exception" );
        }
        PyErr_NormalizeException( &type, &value, &traceback );
        if( type == nullptr ) {
            ErrorLog( "Python Interpreter: don't know how to handle null exception" );
        }
        auto *typetext { PyObject_Str( type ) };
        if( typetext != nullptr ) {
            ErrorLog( PyString_AsString( typetext ) );
        }
        if( value != nullptr ) {
            ErrorLog( PyString_AsString( value ) );
        }
        auto *tracebacktext { PyObject_Str( traceback ) };
        if( tracebacktext != nullptr ) {
            ErrorLog( PyString_AsString( tracebacktext ) );
        }
        else {
            WriteLog( "Python Interpreter: failed to retrieve the stack traceback" );
        }
    }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
