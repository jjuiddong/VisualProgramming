
#include "stdafx.h"
#include "vprogview.h"


cVProgView::cVProgView(const string &name)
	: framework::cDockWindow(name)
{
}

cVProgView::~cVProgView()
{
}


bool cVProgView::Init(graphic::cRenderer &renderer)
{
	return true;
}


void cVProgView::OnUpdate(const float deltaSeconds)
{
}


void cVProgView::OnRender(const float deltaSeconds)
{
	vprog::cEditManager &editMgr = g_global->m_editMgr;

	RenderDefinition("Event List", vprog::NodeType::Event);
	RenderDefinition("Control List", vprog::NodeType::Control);
	RenderDefinition("Function List", vprog::NodeType::Function);
	RenderDefinition("Operator List", vprog::NodeType::Operator);
}


// render definition specific node type
void cVProgView::RenderDefinition(const StrId &headerName
	, const vprog::NodeType::Enum type)
{
	vprog::cEditManager &editMgr = g_global->m_editMgr;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader(headerName.c_str()))
	{
		for (auto &def : editMgr.m_definitions)
		{
			if (def.m_type != type)
				continue;

			if (ImGui::Selectable(def.m_name.c_str(), false
				, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseDoubleClicked(0))
				{
					if (vprog::cNode *node 
						= editMgr.Generate_ReservedDefinition(def.m_name))
					{
						// 화면의 중앙에 출력되게 한다.
						namespace ed = ax::NodeEditor;
						ed::SetCurrentEditor(editMgr.m_editor);

						const ImVec2 size = ed::GetScreenSize();
						const ImVec2 pos = ed::ScreenToCanvas(ImVec2(size.x / 2.f, size.y / 2.f));
						ed::SetNodePosition(node->m_id, pos);
					}
				}
			}
		}
		ImGui::Spacing();
	}
}
