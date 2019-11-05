
#include "stdafx.h"
#include "consoleview.h"


cConsoleView::cConsoleView(const string &name)
	: framework::cDockWindow(name)
	, m_movScrollLine(-1)
{
}

cConsoleView::~cConsoleView()
{
}


bool cConsoleView::Init(graphic::cRenderer &renderer)
{
	return true;
}


void cConsoleView::OnUpdate(const float deltaSeconds)
{
}


void cConsoleView::OnRender(const float deltaSeconds)
{
	if (ImGui::Button("Clear"))
	{
		m_outputs.clear();
	}

	if (ImGui::BeginChild("Console String Window", ImVec2(0,0), true))
	{
		for (uint i = 0; i < m_outputs.size(); ++i)
		{
			auto &str = m_outputs[i];
			ImGui::Selectable(str.c_str());
		}

		if (m_movScrollLine != (int)m_outputs.size())
		{
			ImGui::SetScrollHere();
			m_movScrollLine = (int)m_outputs.size();
		}
	}
	ImGui::EndChild();
}


void cConsoleView::AddString(const char* fmt, ...)
{
	char buff[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf_s(buff, sizeof(buff) - 1, _TRUNCATE, fmt, args);
	va_end(args);
	m_outputs.push_back(buff);

	m_movScrollLine = -1;
}
