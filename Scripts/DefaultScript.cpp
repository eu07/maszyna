#include <Scripts/DefaultScript.h>
#include <stdafx.h>


#include <Logs.h>
void Start() // Called at start of program
{
	WriteLog("Scripting start EVENT");
}
void Update() // Called Every simulation update
{
	WriteLog("Scripting Update EVENT");
}