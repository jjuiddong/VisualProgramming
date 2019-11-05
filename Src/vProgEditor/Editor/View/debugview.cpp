
#include "stdafx.h"
#include "debugview.h"
#include "consoleview.h"
#include "codeview.h"
#include "editorview.h"


//----------------------------------------------------------------
// cDebugView::sEventTrigger
//----------------------------------------------------------------
cDebugView::sEventTrigger::sEventTrigger() {}
cDebugView::sEventTrigger::~sEventTrigger() {clear();}
cDebugView::sEventTrigger::sEventTrigger(const sEventTrigger &rhs) {operator=(rhs);}
cDebugView::sEventTrigger& cDebugView::sEventTrigger::operator=(const sEventTrigger &rhs) {
	if (this != &rhs)
	{
		clear();
		name = rhs.name;
		evtName = rhs.evtName;
		for (auto &kv : rhs.vars)
			vars[kv.first] = common::copyvariant(kv.second);
	}
	return *this;
}
void cDebugView::sEventTrigger::clear() {
	name.clear();
	evtName.clear();
	for (auto &kv : vars)
		common::clearvariant(kv.second);
	vars.clear();
}



//----------------------------------------------------------------
// cDebugView
//----------------------------------------------------------------
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
	ReadEventTriggerListFile("event_trigger_list.txt");

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
		if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
			, "Debug?", "CONFIRM", MB_YESNO))
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

			g_global->ReadVProgFile(fileName);

			m_state = eState::Debug;
		}
	}

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1));
	if (ImGui::Button("Debug Cancel"))
	{
		if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
			, "Debug Cancel?", "CONFIRM", MB_YESNO))
		{
			m_debugger.Terminate();
			g_global->m_codeView->ClearCode();
		}
	}
	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Step"))
	{
		m_debugger.OneStep();
	}

	ImGui::SameLine();
	if (ImGui::Button("Run"))
	{
		m_debugger.Run();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	RenderEventTriggerList();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

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


// render event trigger list
void cDebugView::RenderEventTriggerList()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Event Trigger List"))
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.1f, 1));
		if (ImGui::Button("Refresh"))
			ReadEventTriggerListFile("event_trigger_list.txt");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Read \"event_trigger_list.txt\" file");
		ImGui::PopStyleColor(3);

		if (ImGui::BeginChild("event child window", ImVec2(0, 100), true))
		{
			for (auto &trigger : m_events)
			{
				if (ImGui::Button(trigger.name.c_str()))
				{
					common::script::cEvent evt(trigger.evtName);
					for (auto &kv : trigger.vars)
						evt.m_vars.insert({ kv.first.c_str()
							, common::copyvariant(kv.second) });
					m_interpreter.PushEvent(evt);
				}
			}
		}
		ImGui::EndChild();
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


// read event trigger list file
// - event trigger file format
//
//event_trigger
//	name "event name1"
//	input
//		name "var1"
//		type Int
//		value 0
//	input
//		name "var2"
//		type String
//		value "aaa"
//
//event_trigger
//	name "event name2"
//	input
//		name "var1"
//		type float
//		value 0.01
//
bool cDebugView::ReadEventTriggerListFile(const StrPath &fileName)
{
	m_events.clear();

	using namespace std;
	ifstream ifs(fileName.c_str());
	if (!ifs.is_open())
		return false;

	sEventTrigger evt;
	StrId varName;
	variant_t var;
	var.vt = VT_EMPTY;

	int state = 0;
	string line;
	while (getline(ifs, line))
	{
		common::trim(line);
		vector<string> toks;
		common::tokenizer_space(line, toks);
		if (toks.empty())
			continue;

		switch (state)
		{
		case 0:
			if (toks[0] == "event_trigger")
			{
				state = 1;
				evt.clear();
			}
			break;

		case 1: // event parsing
			if ((toks[0] == "name") && (toks.size() >= 2))
			{
				evt.name = toks[1];
			}
			if ((toks[0] == "eventname") && (toks.size() >= 2))
			{
				evt.evtName = toks[1];
			}
			else if (toks[0] == "input")
			{
				state = 2;
				varName.clear();
			}
			break;

		case 2: // input parsing
			if ((toks[0] == "type") && (toks.size() >= 2))
			{
				if (toks[1] == "Bool")
					var.vt = VT_BOOL;
				else if (toks[1] == "Int")
					var.vt = VT_INT;
				else if (toks[1] == "Float")
					var.vt = VT_R4;
				else if (toks[1] == "String")
					var.vt = VT_BSTR;
				else
					assert(!"cDebugView::ReadEventTriggerListFile()");
			}
			else if ((toks[0] == "name") && (toks.size() >= 2))
			{
				varName = toks[1];
			}
			else if ((toks[0] == "value") && (toks.size() >= 2))
			{
				var = common::str2variant(var.vt, toks[1]);
			}
			else if (toks[0] == "input")
			{
				if (!varName.empty())
					evt.vars[varName] = common::copyvariant(var);

				state = 2;
				varName.clear();
				common::clearvariant(var);
			}
			else if ((toks[0] == "event_trigger"))
			{
				if (!varName.empty())
					evt.vars[varName] = common::copyvariant(var);

				if (!evt.name.empty())
					m_events.push_back(evt);

				state = 1;
				evt.clear();
				common::clearvariant(var);
			}
			break;
		} //~switch
	} //~while

	if (!varName.empty())
		evt.vars[varName] = common::copyvariant(var);

	if (!evt.name.empty())
		m_events.push_back(evt);

	evt.clear();
	common::clearvariant(var);
	return true;
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
	else if (funcName == "IsRobotReady")
	{
		variant_t node;
		symbolTable.Get(scopeName, "Robot Name", node);
		symbolTable.Set(scopeName, "Ready", variant_t((bool)1));
		common::clearvariant(node);
	}
	else if (funcName == "IsRobotWork")
	{
		variant_t node;
		symbolTable.Get(scopeName, "Robot Name", node);
		symbolTable.Set(scopeName, "Work", variant_t((bool)1));
		common::clearvariant(node);
	}
	else if (funcName == "ReqMove")
	{
		symbolTable.Set(scopeName, "Result", variant_t((int)0));
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

