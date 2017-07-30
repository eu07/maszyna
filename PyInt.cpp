#include "stdafx.h"
#include "PyInt.h"
#include "renderer.h"
#include "Train.h"
#include "Logs.h"
//#include <process.h>
//#include <windows.h>
//#include <stdlib.h>

TPythonInterpreter *TPythonInterpreter::_instance = NULL;

//#define _PY_INT_MORE_LOG

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

TPythonInterpreter::TPythonInterpreter()
{
    WriteLog("Loading Python ...");
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
#endif
    Py_Initialize();
    _main = PyImport_ImportModule("__main__");
    if (_main == NULL)
    {
        WriteLog("Cannot import Python module __main__");
    }
    PyObject *cStringModule = PyImport_ImportModule("cStringIO");
    _stdErr = NULL;
    if (cStringModule == NULL)
        return;
    PyObject *cStringClassName = PyObject_GetAttrString(cStringModule, "StringIO");
    if (cStringClassName == NULL)
        return;
    PyObject *cString = PyObject_CallObject(cStringClassName, NULL);
    if (cString == NULL)
        return;
    if (PySys_SetObject("stderr", cString) != 0)
        return;
    _stdErr = cString;
}

TPythonInterpreter *TPythonInterpreter::getInstance()
{
    if (!_instance)
    {
        _instance = new TPythonInterpreter();
    }
    return _instance;
}

void
TPythonInterpreter::killInstance() {

	delete _instance;
}

bool TPythonInterpreter::loadClassFile( std::string const &lookupPath, std::string const &className )
{
    std::set<std::string>::const_iterator it = _classes.find(className);
    if (it == _classes.end())
    {
        FILE *sourceFile = _getFile(lookupPath, className);
        if (sourceFile != nullptr)
        {
            fseek(sourceFile, 0, SEEK_END);
            long fsize = ftell(sourceFile);
            char *buffer = (char *)calloc(fsize + 1, sizeof(char));
            fseek(sourceFile, 0, SEEK_SET);
            size_t freaded = fread(buffer, sizeof(char), fsize, sourceFile);
            buffer[freaded] = 0; // z jakiegos powodu czytamy troche mniej i trzczeba dodac konczace
// zero do bufora (mimo ze calloc teoretycznie powiniene zwrocic
// wyzerowana pamiec)
#ifdef _PY_INT_MORE_LOG
            char buf[255];
            sprintf(buf, "readed %d / %d characters for %s", freaded, fsize, className);
            WriteLog(buf);
#endif // _PY_INT_MORE_LOG
            fclose(sourceFile);
            if (PyRun_SimpleString(buffer) != 0)
            {
                handleError();
                return false;
            }
			_classes.insert( className );
/*
            char *classNameToRemember = (char *)calloc(strlen(className) + 1, sizeof(char));
            strcpy(classNameToRemember, className);
            _classes.insert(classNameToRemember);
*/
			free(buffer);
            return true;
        }
        return false;
    }
    return true;
}

PyObject *TPythonInterpreter::newClass( std::string const &className )
{
    return newClass(className, NULL);
}

FILE *TPythonInterpreter::_getFile( std::string const &lookupPath, std::string const &className )
{
	if( false == lookupPath.empty() ) {
		std::string const sourcefilepath = lookupPath + className + ".py";
		FILE *file = fopen( sourcefilepath.c_str(), "r" );
#ifdef _PY_INT_MORE_LOG
		WriteLog( sourceFilePath );
#endif // _PY_INT_MORE_LOG
		if( nullptr != file ) { return file; }
	}
	std::string  sourcefilepath = "python/local/" + className + ".py";
	FILE *file = fopen( sourcefilepath.c_str(), "r" );
#ifdef _PY_INT_MORE_LOG
	WriteLog( sourceFilePath );
#endif // _PY_INT_MORE_LOG
	return file; // either the file, or a nullptr on fail
/*
    char *sourceFilePath;
    if (lookupPath != NULL)
    {
        sourceFilePath = (char *)calloc(strlen(lookupPath) + strlen(className) + 4, sizeof(char));
        strcat(sourceFilePath, lookupPath);
        strcat(sourceFilePath, className);
        strcat(sourceFilePath, ".py");

        FILE *file = fopen(sourceFilePath, "r");
#ifdef _PY_INT_MORE_LOG
        WriteLog(sourceFilePath);
#endif // _PY_INT_MORE_LOG
        free(sourceFilePath);
        if (file != NULL)
        {
            return file;
        }
    }
    char *basePath = "python/local/";
    sourceFilePath = (char *)calloc(strlen(basePath) + strlen(className) + 4, sizeof(char));
    strcat(sourceFilePath, basePath);
    strcat(sourceFilePath, className);
    strcat(sourceFilePath, ".py");

    FILE *file = fopen(sourceFilePath, "r");
#ifdef _PY_INT_MORE_LOG
    WriteLog(sourceFilePath);
#endif // _PY_INT_MORE_LOG
    free(sourceFilePath);
    if (file != NULL)
    {
        return file;
    }
    return NULL;
*/
}

void TPythonInterpreter::handleError()
{
#ifdef _PY_INT_MORE_LOG
    WriteLog("Python Error occured");
#endif // _PY_INT_MORE_LOG
    if (_stdErr != NULL)
    { // std err pythona jest buforowane
        PyErr_Print();
        PyObject *bufferContent = PyObject_CallMethod(_stdErr, "getvalue", NULL);
        PyObject_CallMethod(_stdErr, "truncate", "i", 0); // czyscimy bufor na kolejne bledy
        WriteLog(PyString_AsString(bufferContent));
    }
    else
    { // nie dziala buffor pythona
        if (PyErr_Occurred() != NULL)
        {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (ptype == NULL)
            {
                WriteLog("Don't konw how to handle NULL exception");
            }
            PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
            if (ptype == NULL)
            {
                WriteLog("Don't konw how to handle NULL exception");
            }
            PyObject *pStrType = PyObject_Str(ptype);
            if (pStrType != NULL)
            {
                WriteLog(PyString_AsString(pStrType));
            }
            WriteLog(PyString_AsString(pvalue));
            PyObject *pStrTraceback = PyObject_Str(ptraceback);
            if (pStrTraceback != NULL)
            {
                WriteLog(PyString_AsString(pStrTraceback));
            }
            else
            {
                WriteLog("Python Traceback cannot be shown");
            }
        }
        else
        {
#ifdef _PY_INT_MORE_LOG
            WriteLog("Called python error handler when no error occured!");
#endif // _PY_INT_MORE_LOG
        }
    }
}
PyObject *TPythonInterpreter::newClass(std::string const &className, PyObject *argsTuple)
{
    if (_main == NULL)
    {
        WriteLog("main turned into null");
        return NULL;
    }
    PyObject *classNameObj = PyObject_GetAttrString(_main, className.c_str());
    if (classNameObj == NULL)
    {
#ifdef _PY_INT_MORE_LOG
        char buf[255];
        sprintf(buf, "Python class %s not defined!", className);
        WriteLog(buf);
#endif // _PY_INT_MORE_LOG
        return NULL;
    }
    PyObject *object = PyObject_CallObject(classNameObj, argsTuple);

    if (PyErr_Occurred() != NULL)
    {
        handleError();
        return NULL;
    }
    return object;
}

TPythonScreenRenderer::TPythonScreenRenderer(int textureId, PyObject *renderer)
{
    _textureId = textureId;
    _pyRenderer = renderer;
}

void TPythonScreenRenderer::updateTexture()
{
    int width, height;
    if (_pyWidth == NULL || _pyHeight == NULL)
    {
        WriteLog("Unknown python texture size!");
        return;
    }
    width = PyInt_AsLong(_pyWidth);
    height = PyInt_AsLong(_pyHeight);
    if (_pyTexture != NULL)
    {
        char *textureData = PyString_AsString(_pyTexture);
        if (textureData != NULL)
        {
#ifdef _PY_INT_MORE_LOG
            char buff[255];
            sprintf(buff, "Sending texture id: %d w: %d h: %d", _textureId, width, height);
            WriteLog(buff);
#endif // _PY_INT_MORE_LOG
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            GfxRenderer.Bind(_textureId);
            // setup texture parameters
            if( GLEW_VERSION_1_4 ) {

                glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1.0 );
            }
            else {

                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            }
            // build texture
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData );

#ifdef _PY_INT_MORE_LOG
            GLenum status = glGetError();
            switch (status)
            {
            case GL_INVALID_ENUM:
                WriteLog("An unacceptable value is specified for an enumerated argument. The "
                         "offending function is ignored, having no side effect other than to set "
                         "the error flag.");
                break;
            case GL_INVALID_VALUE:
                WriteLog("A numeric argument is out of range. The offending function is ignored, "
                         "having no side effect other than to set the error flag.");
                break;
            case GL_INVALID_OPERATION:
                WriteLog("The specified operation is not allowed in the current state. The "
                         "offending function is ignored, having no side effect other than to set "
                         "the error flag.");
                break;
            case GL_NO_ERROR:
                WriteLog("No error has been recorded. The value of this symbolic constant is "
                         "guaranteed to be zero.");
                break;
            case GL_STACK_OVERFLOW:
                WriteLog("This function would cause a stack overflow. The offending function is "
                         "ignored, having no side effect other than to set the error flag.");
                break;
            case GL_STACK_UNDERFLOW:
                WriteLog("This function would cause a stack underflow. The offending function is "
                         "ignored, having no side effect other than to set the error flag.");
                break;
            case GL_OUT_OF_MEMORY:
                WriteLog("There is not enough memory left to execute the function. The state of "
                         "OpenGL is undefined, except for the state of the error flags, after this "
                         "error is recorded.");
                break;
            };
#endif // _PY_INT_MORE_LOG
        }
        else
        {
            WriteLog("RAW python texture data is NULL!");
        }
    }
    else
    {
        WriteLog("Python texture object is NULL!");
    }
}

void TPythonScreenRenderer::render(PyObject *trainState)
{
#ifdef _PY_INT_MORE_LOG
    WriteLog("Python rendering texture ...");
#endif // _PY_INT_MORE_LOG
    _pyTexture = PyObject_CallMethod(_pyRenderer, "render", "O", trainState);

    if (_pyTexture == NULL)
    {
        TPythonInterpreter::getInstance()->handleError();
    }
    else
    {
        _pyWidth = PyObject_CallMethod(_pyRenderer, "get_width", NULL);
        if (_pyWidth == NULL)
        {
            TPythonInterpreter::getInstance()->handleError();
        }
        _pyHeight = PyObject_CallMethod(_pyRenderer, "get_height", NULL);
        if (_pyHeight == NULL)
        {
            TPythonInterpreter::getInstance()->handleError();
        }
    }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

TPythonScreenRenderer::~TPythonScreenRenderer()
{
#ifdef _PY_INT_MORE_LOG
    WriteLog("PythonScreenRenderer descturctor called");
#endif // _PY_INT_MORE_LOG
    if (_pyRenderer != NULL)
    {
        Py_CLEAR(_pyRenderer);
    }
    cleanup();
#ifdef _PY_INT_MORE_LOG
    WriteLog("PythonScreenRenderer desctructor finished");
#endif // _PY_INT_MORE_LOG
}

void TPythonScreenRenderer::cleanup()
{
    if (_pyTexture != NULL)
    {
        Py_CLEAR(_pyTexture);
        _pyTexture = NULL;
    }
    if (_pyWidth != NULL)
    {
        Py_CLEAR(_pyWidth);
        _pyWidth = NULL;
    }
    if (_pyHeight != NULL)
    {
        Py_CLEAR(_pyHeight);
        _pyHeight = NULL;
    }
}

void TPythonScreens::reset(void *train)
{
    _terminationFlag = true;
    if (_thread != NULL)
    {
//        WriteLog("Awaiting python thread to end");
		_thread->join();
		delete _thread;
        _thread = nullptr;
    }
    _terminationFlag = false;
    _cleanupReadyFlag = false;
    _renderReadyFlag = false;
    for (std::vector<TPythonScreenRenderer *>::iterator i = _screens.begin(); i != _screens.end();
         ++i)
    {
        delete *i;
    }
#ifdef _PY_INT_MORE_LOG
    WriteLog("Clearing renderer vector");
#endif // _PY_INT_MORE_LOG
    _screens.clear();
    _train = train;
}

void TPythonScreens::init(cParser &parser, TModel3d *model, std::string const &name, int const cab)
{
	std::string asSubModelName, asPyClassName;
	parser.getTokens( 2, false );
	parser
		>> asSubModelName
		>> asPyClassName;
    std::string subModelName = ToLower( asSubModelName );
    std::string pyClassName = ToLower( asPyClassName );
    TSubModel *subModel = model->GetFromName(subModelName.c_str());
    if (subModel == NULL)
    {
        WriteLog( "Python Screen: submodel " + subModelName + " not found - Ignoring screen" );
        return; // nie ma takiego sub modelu w danej kabinie pomijamy
    }
    auto textureId = subModel->GetTextureId();
    if (textureId <= 0)
    {
        WriteLog( "Python Screen: invalid texture id " + std::to_string(textureId) + " - Ignoring screen" );
        return; // sub model nie posiada tekstury lub tekstura wymienna - nie obslugiwana
    }
    TPythonInterpreter *python = TPythonInterpreter::getInstance();
    python->loadClassFile(_lookupPath, pyClassName);
    PyObject *args = Py_BuildValue("(ssi)", _lookupPath.c_str(), name.c_str(), cab);
    if (args == NULL)
    {
        WriteLog("Python Screen: cannot create __init__ arguments");
        return;
    }
    PyObject *pyRenderer = python->newClass(pyClassName, args);
    Py_CLEAR(args);
    if (pyRenderer == NULL)
    {
        WriteLog( "Python Screen: null renderer for " + pyClassName + " - Ignoring screen" );
        return; // nie mozna utworzyc obiektu Pythonowego
    }
    TPythonScreenRenderer *renderer = new TPythonScreenRenderer(textureId, pyRenderer);
    _screens.push_back(renderer);
    WriteLog( "Created python screen " + pyClassName + " on submodel " + subModelName + " (" + std::to_string(textureId) + ")" );
}

void TPythonScreens::update()
{
    if (!_renderReadyFlag)
    {
        return;
    }
    _renderReadyFlag = false;
    for (std::vector<TPythonScreenRenderer *>::iterator i = _screens.begin(); i != _screens.end();
         ++i)
    {
        (*i)->updateTexture();
    }
    _cleanupReadyFlag = true;
}

void TPythonScreens::setLookupPath(std::string const &path)
{
	_lookupPath = path;
}

TPythonScreens::TPythonScreens()
{
    TPythonInterpreter::getInstance()->loadClassFile("", "abstractscreenrenderer");
    _terminationFlag = false;
    _renderReadyFlag = false;
    _cleanupReadyFlag = false;
    _thread = NULL;
}

TPythonScreens::~TPythonScreens()
{
#ifdef _PY_INT_MORE_LOG
    WriteLog("Called python sceeens destructor");
#endif // _PY_INT_MORE_LOG
    reset(NULL);
/*
    if (_lookupPath != NULL)
    {
#ifdef _PY_INT_MORE_LOG
        WriteLog("Freeing lookup path");
#endif // _PY_INT_MORE_LOG
        free(_lookupPath);
    }
*/
}

void TPythonScreens::run()
{
    while (1)
    {
        if (_terminationFlag)
        {
            return;
        }
        TTrain *train = (TTrain *)_train;
        _trainState = train->GetTrainState();
        if (_terminationFlag)
        {
            _freeTrainState();
            return;
        }
        for (std::vector<TPythonScreenRenderer *>::iterator i = _screens.begin();
             i != _screens.end(); ++i)
        {
            (*i)->render(_trainState);
        }
        _freeTrainState();
        if (_terminationFlag)
        {
            _cleanup();
            return;
        }
        _renderReadyFlag = true;
        while (!_cleanupReadyFlag && !_terminationFlag)
        {
#ifdef _WIN32
            Sleep(100);
#elif __linux__
			usleep(100*1000);
#endif
        }
        if (_terminationFlag)
        {
            return;
        }
        _cleanup();
    }
}

void TPythonScreens::finish()
{
    _thread = NULL;
}

void ScreenRendererThread(TPythonScreens* renderer)
{
    renderer->run();
    renderer->finish();
#ifdef _PY_INT_MORE_LOG
    WriteLog("Python Screen Renderer Thread Ends");
#endif // _PY_INT_MORE_LOG
}

void TPythonScreens::start()
{
    if (_screens.size() > 0)
    {
		_thread = new std::thread(ScreenRendererThread, this);
    }
}

void TPythonScreens::_cleanup()
{
    _cleanupReadyFlag = false;
    for (std::vector<TPythonScreenRenderer *>::iterator i = _screens.begin(); i != _screens.end();
         ++i)
    {
        (*i)->cleanup();
    }
}

void TPythonScreens::_freeTrainState()
{
    if (_trainState != NULL)
    {
        PyDict_Clear(_trainState);
        Py_CLEAR(_trainState);
        _trainState = NULL;
    }
}
