#include "stdafx.h"
#include "launcher/launchermode.h"
#include "launcher/launcheruilayer.h"
#include "application.h"
#include "simulation.h"
#include "Globals.h"

launcher_mode::launcher_mode()
{
	m_userinterface = std::make_shared<launcher_ui>();
}

bool launcher_mode::init()
{
	return true;
}

bool launcher_mode::update()
{
	return true;
}

void launcher_mode::enter()
{
	Application.set_cursor( GLFW_CURSOR_NORMAL );

	simulation::is_ready = false;

	Application.set_title(Global.AppName);
}

void launcher_mode::exit()
{

}

void launcher_mode::on_key(const int Key, const int Scancode, const int Action, const int Mods)
{
	Global.shiftState = ( Mods & GLFW_MOD_SHIFT ) ? true : false;
	Global.ctrlState = ( Mods & GLFW_MOD_CONTROL ) ? true : false;
	m_userinterface->on_key(Key, Action);
}
