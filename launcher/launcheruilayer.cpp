#include "stdafx.h"
#include "launcher/launcheruilayer.h"

launcher_ui::launcher_ui()
{
	add_external_panel(&m_scenerylist_panel);
	add_external_panel(&m_keymapper_panel);
	add_external_panel(&m_mainmenu_panel);
	add_external_panel(&m_vehiclepicker_panel);
}

bool launcher_ui::on_key(const int Key, const int Action)
{
	if (m_keymapper_panel.key(Key))
		return true;

	return ui_layer::on_key(Key, Action);
}
