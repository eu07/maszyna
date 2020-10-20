#pragma once

class ui_panel;

namespace ui
{

class popup
{
	std::string m_id;
	static int id;

  public:
	popup(ui_panel &panel);
	virtual ~popup();

    virtual bool render();

  protected:
	ui_panel &m_parent;
	virtual void render_content() = 0;
};

} // namespace ui
