
#include "stdafx.h"
#include "debugview.h"
#include "consoleview.h"
#include "codeview.h"


cDebugView::cDebugView(const string &name)
	: framework::cDockWindow(name)
	, m_state(eState::Stop)
{
}

cDebugView::~cDebugView()
{
	m_debugger.Clear();
	m_interpreter.Clear();
}


bool cDebugView::Init(graphic::cRenderer &renderer)
{
	return true;
}


void cDebugView::OnUpdate(const float deltaSeconds)
{
	RET(eState::Stop == m_state);

	m_interpreter.Process(deltaSeconds);
	m_debugger.Process(deltaSeconds);
}


void cDebugView::OnRender(const float deltaSeconds)
{
	if (ImGui::Button("Debug"))
	{
		const StrPath fileName = "vprog.txt";

		vprog::cVProgFile vprogFile;
		vprogFile.Read(fileName);
		common::script::cIntermediateCode code;
		vprogFile.GenerateIntermediateCode(code);
		if (!code.Write("il.txt"))
			return;
		if (!m_interpreter.Init("il.txt", this, this))
			return;
		if (!m_debugger.Init(&m_interpreter))
			return;

		g_global->m_codeView->ReadVProgFile(fileName);

		m_state = eState::Debug;
	}

	if (ImGui::Button("Event Seqwork"))
	{
		common::script::cEvent evt("SeqWork Event");
		evt.m_vars.insert({ "SeqWork Event::Loading", variant_t((int)0) });
		evt.m_vars.insert({ "SeqWork Event::l Layer", variant_t((int)0) });
		evt.m_vars.insert({ "SeqWork Event::Picking", variant_t((int)0) });
		evt.m_vars.insert({ "SeqWork Event::Unloading", variant_t((int)0) });
		evt.m_vars.insert({ "SeqWork Event::u Layer", variant_t((int)0) });
		m_interpreter.PushEvent(evt);
	}

	if (ImGui::Button("Step"))
	{
		m_debugger.OneStep();
	}

	if (ImGui::Button("Debug Cancel"))
	{
		m_debugger.Terminate();
	}

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Virtual Machine"))
	{
		for (auto &vm : m_interpreter.m_vms)
		{
			g_global->m_codeView->SetHighLightLine((int)vm->m_reg.idx);

			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode(vm->m_name.c_str()))
			{
				ImGui::TextUnformatted("Register Information");

				ImGui::TextUnformatted("index");
				ImGui::SameLine();
				ImGui::InputInt("##index", (int*)&vm->m_reg.idx);

				for (uint i = 0; i < ARRAYSIZE(vm->m_reg.val); ++i)
				{
					auto &var = vm->m_reg.val[i];
					StrId id;
					id.Format("reg[%d]", i);
					RenderVariant(id, var);
				}

				ImGui::TreePop();
			}
		}
	}
}


// render variant type
void cDebugView::RenderVariant(const StrId &name, INOUT variant_t &var)
{
	if (VT_EMPTY == var.vt)
		return;

	ImGui::TextUnformatted(name.c_str());
	ImGui::SameLine();

	switch (var.vt)
	{
	case VT_BOOL:
		ImGui::InputInt("##bool type", (int*)&var.bVal);
		break;
	case VT_INT:
		ImGui::InputInt("##int type", &var.intVal);
		break;
	case VT_R4:
		ImGui::InputFloat("##float type", &var.fltVal);
		break;
	case VT_BSTR:
	{
		WStr128 wstr = (LPCTSTR)var.bstrVal;
		ImGui::TextUnformatted(wstr.str().c_str());
	}
	break;
	default:
		// not set regiser
		break;
	}
}


// interpreter callback function
int cDebugView::Function(common::script::cSymbolTable &symbolTable
	, const string &scopeName
	, const string &funcName
	, void *arg)
{
	if (funcName == "GetFrontNode")
	{
		variant_t node;
		symbolTable.Get(scopeName, "Node", node);
		symbolTable.Set(scopeName, "Front", variant_t((int)10));
		common::clearvariant(node);
	}
	else if (funcName == "ReqMove")
	{
		symbolTable.Set(scopeName, "Result", variant_t((int)1));
	}
	else if (funcName == "ReqLoading")
	{
		int a = 0;
	}
	else if (funcName == "ReqUnloading")
	{
		int a = 0;
	}
	else if (funcName == "ErrorState")
	{
		variant_t errCode;
		symbolTable.Get(scopeName, "ErrorCode", errCode);
		common::clearvariant(errCode);
	}

	g_global->m_consoleView->AddString("call %s()", funcName.c_str());
	return 0;
}
