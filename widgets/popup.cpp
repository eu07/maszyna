#include "stdafx.h"
#include "widgets/popup.h"

ui::popup::popup(ui_panel &panel) : m_parent(panel) {}

ui::popup::~popup() {}

bool ui::popup::render()
{
	if (!m_id.size())
	{
		m_id = "popup:" + std::to_string(id++);
		ImGui::OpenPopup(m_id.c_str());
	}

	if (!ImGui::BeginPopup(m_id.c_str()))
		return true;

	render_content();

	ImGui::EndPopup();

	return false;
}

int ui::popup::id = 0;
