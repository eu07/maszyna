#ifndef PyIntH
#define PyIntH

#undef _DEBUG // bez tego macra Py_DECREF powoduja problemy przy linkowaniu

#include "Python.h"
#include "QueryParserComp.hpp"
#include "Model3d.h"
#include <vector>
#include <set>

#define PyGetFloat(param) PyFloat_FromDouble(param>=0?param:-param)
#define PyGetInt(param) PyInt_FromLong(param)
#define PyGetBool(param) param?Py_True:Py_False

struct ltstr
{
	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) < 0;
	}
};

class TPythonInterpreter
{
	protected:
		TPythonInterpreter();
		~TPythonInterpreter() {}
		static TPythonInterpreter* _instance;
		int _screenRendererPriority;
		std::set<const char*, ltstr> _classes;
		PyObject* _main;
		PyObject* _stdErr;
		FILE* _getFile(const char* lookupPath, const char* className);
	public:
		static TPythonInterpreter* getInstance();
		bool loadClassFile(const char* lookupPath, const char* className);
		PyObject* newClass(const char* className);
		PyObject* newClass(const char* className, PyObject* argsTuple);
		int getScreenRendererPriotity() {return _screenRendererPriority;};
		void setScreenRendererPriority(const char* priority);
		void handleError();
};

class TPythonScreenRenderer
{
	protected:
		PyObject* _pyRenderer;
		PyObject* _pyTexture;
		int _textureId;
		PyObject* _pyWidth;
		PyObject* _pyHeight;
	public:
		TPythonScreenRenderer(int textureId, PyObject* renderer);
		~TPythonScreenRenderer();
		void render(PyObject* trainState);
		void cleanup();
		void updateTexture();
};

class TPythonScreens
{
	protected:
		bool _cleanupReadyFlag;
		bool _renderReadyFlag;
		bool _terminationFlag;
		HANDLE _thread;
		DWORD _threadId;
		std::vector<TPythonScreenRenderer * > _screens;
		char* _lookupPath;
		void* _train;
		void _cleanup();
		void _freeTrainState();
		PyObject* _trainState;
	public:
		void reset(void* train);
		void setLookupPath(AnsiString path);
		void init(TQueryParserComp *parser, TModel3d *model, AnsiString name, int cab);
		void update();
		TPythonScreens();
		~TPythonScreens();
		void run();
		void start();
		void finish();
};

#endif // PyIntH
