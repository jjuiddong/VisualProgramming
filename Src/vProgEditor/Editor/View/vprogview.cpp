
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
	
	if (ImGui::Button("Refresh"))
	{
		editMgr.ReadDefinitionFile("vprog_definition.txt");
	}

	RenderDefinition("Event List", vprog::eNodeType::Event);
	RenderDefinition("Control List", vprog::eNodeType::Control);
	RenderDefinition("Macro List", vprog::eNodeType::Macro);
	RenderDefinition("Function List", vprog::eNodeType::Function);
	//RenderDefinition("Operator List", vprog::eNodeType::Operator);
	RenderOperatorList();
	RenderDefinition("Variable List", vprog::eNodeType::Variable);
	RenderSymbolTable();
}


// render definition specific node type
void cVProgView::RenderDefinition(const StrId &headerName
	, const vprog::eNodeType::Enum type)
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


// operator list
// render type combobox
void cVProgView::RenderOperatorList()
{
	vprog::cEditManager &editMgr = g_global->m_editMgr;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Operator List"))
	{
		const char *comboStr = "Bool\0Int\0Float\0String\0\0";
		static int combo = 1;
		vprog::ePinType::Enum type = vprog::ePinType::Bool;
		ImGui::Combo("Type", &combo, comboStr);
		switch (combo)
		{
		case 0: type = vprog::ePinType::Bool; break;
		case 1: type = vprog::ePinType::Int; break;
		case 2: type = vprog::ePinType::Float; break;
		case 3: type = vprog::ePinType::String; break;
		}

		for (auto &def : editMgr.m_definitions)
		{
			if (def.m_type != vprog::eNodeType::Operator)
				continue;

			// check operator type
			if (!def.m_inputs.empty()
				&& (def.m_inputs[0].type != type))
				continue;

			if (ImGui::Selectable(def.m_name.c_str(), false
				, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseDoubleClicked(0))
				{
					if (vprog::cNode *node
						= editMgr.Generate_ReservedDefinition(def.m_name, def.m_varName))
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


// render symboltable, edit symbol value
void cVProgView::RenderSymbolTable()
{
	namespace ed = ax::NodeEditor;
	vprog::cEditManager &editMgr = g_global->m_editMgr;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Symbol Table"))
	{
		for (auto &kv : editMgr.m_symbTable.m_symbols)
		{
			const ed::PinId pid = kv.first;
			vprog::cSymbolTable::sValue &value = kv.second;
			variant_t &var = value.var;
			vprog::cNode *node = editMgr.FindContainNode(pid);
			if (!node)
				continue; // error occurred
			vprog::sPin *pin = editMgr.FindPin(pid);
			if (!pin)
				continue; // error occurred
			if (node->m_type != vprog::eNodeType::Variable)
				continue; // error occurred
			if (node->m_outputs.empty())
				continue; // error occurred

			StrId treeId;
			treeId.Format("(%d)", node->m_id.Get());
			if (ImGui::TreeNodeEx(treeId.c_str(), 0, node->m_varName.c_str()))
			{
				ImGui::InputText("##varname", node->m_varName.m_str, node->m_varName.SIZE);

				switch (pin->type)
				{
				case vprog::ePinType::Bool:
				{
					const char *comboStr = "False\0True\0\0";
					ImGui::Combo("##var", (int*)&var.boolVal, comboStr);
				}
				break;
				case vprog::ePinType::Int:
					ImGui::InputInt("##var", &var.intVal);
					break;
				case vprog::ePinType::Float:
					ImGui::InputFloat("##var", &var.fltVal);
					break;
				case vprog::ePinType::String:
				{
					common::Str128 tmpStr = value.str;
					if (ImGui::InputText("##varstring", tmpStr.m_str, tmpStr.SIZE))
						value.str = tmpStr.c_str();
				}
				break;
				}
				ImGui::TreePop();
			}
		}
		ImGui::Spacing();
	}
}
