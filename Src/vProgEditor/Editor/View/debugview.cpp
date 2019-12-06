
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
	m_remoteDebugger.Clear();
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

	m_remoteDebugger.Process(deltaSeconds);
	m_debugger.Process(deltaSeconds);
	m_interpreter.Process(deltaSeconds);
}


void cDebugView::OnRender(const float deltaSeconds)
{
	RenderLocalDebugging();
	ImGui::Spacing();
	RenderRemoteDebugging();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Step (F10)"))
	{
		if (eState::Debug == m_state)
			m_debugger.OneStep();
		else
			m_remoteDebugger.OneStep();
	}

	ImGui::SameLine();
	if (ImGui::Button("Run (F5)"))
	{
		if (eState::Debug == m_state)
			m_debugger.Run();
		else
			m_remoteDebugger.DebugRun();
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
		if (eState::Debug == m_state)
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
		else if (eState::RemoteDebug == m_state)
		{
			for (auto &vm : m_interpreter.m_vms)
			{
				// update debug info from host remote debugger
				auto it = m_remoteDebugger.m_vmDbgs.find(vm->m_name);
				if (m_remoteDebugger.m_vmDbgs.end() != it)
				{
					if (!it->second.empty())
					{
						vm->m_reg.idx = it->second.front();
						it->second.pop();
					}
				}
									   
				g_global->m_codeView->SetHighLightLine((int)vm->m_reg.idx);

				ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
				if (ImGui::TreeNode(vm->m_name.c_str()))
				{
					ImGui::TextUnformatted("Register Information");

					ImGui::TextUnformatted("index");
					ImGui::SameLine();
					ImGui::InputInt("##index", (int*)&vm->m_reg.idx
						, ImGuiInputTextFlags_ReadOnly);

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
		}//~eState::RemoteDebug

	}
}


// render local debugging information
void cDebugView::RenderLocalDebugging()
{
	vprog::cEditManager &editMgr = g_global->m_editMgr;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Local Debugging"))
	{
		if (!editMgr.IsLoad())
		{
			ImGui::Text("No File to Debug");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
		}
		else if (m_remoteDebugger.IsRun())
		{
			ImGui::Text("Remote Debugger Running");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
		}
		else
		{
			if (ImGui::Button("Debugging"))
			{
				if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
					, "Debug?", "CONFIRM", MB_YESNO))
				{
					const StrPath fileName = g_global->m_editMgr.m_fileName;
					vprog::cVProgFile vprogFile;
					vprogFile.Read(fileName);

					StrPath icodeFileName = fileName.GetFileNameExceptExt();
					icodeFileName += ".icode";
					common::script::cIntermediateCode code;
					vprogFile.GenerateIntermediateCode(code);
					if (!code.Write(icodeFileName))
						return;

					m_debugger.Clear();
					m_interpreter.Clear();

					if (!m_interpreter.Init(icodeFileName, this, this))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error!! Read IntermediateCode", "ERROR"
							, MB_OK | MB_ICONERROR);
						return;
					}

					if (!m_debugger.Init(&m_interpreter))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error!! Debugger Initialize", "ERROR"
							, MB_OK | MB_ICONERROR);
						return;
					}

					g_global->m_codeView->ReadIntermediateFile(icodeFileName);

					m_state = eState::Debug;
				}
			} // ~button "Debug"
		} //~if isload?

		if (!m_remoteDebugger.IsRun()
			&& m_debugger.IsRun())
		{
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
		} // ~debugger isLoad?
	}//~imgui::header
}


void cDebugView::RenderRemoteDebugging()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Remote Debugging"))
	{
		static Str16 ip = "127.0.0.1";
		static int port = 55555;

		if (m_remoteDebugger.IsRun() && m_debugger.IsLoad())
		{
			ImGui::TextUnformatted("- Debugger Running -");
			ImGui::Spacing();
		}
		else
		{
			if (m_remoteDebugger.IsRun())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 0.1f, 1));
				ImGui::Text("- Connect Remote Debugger -");
				ImGui::Text("%s:%d", ip.c_str(), port);
				ImGui::Spacing();
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::TextUnformatted("- Remote Debugger Host IP Setting -");

				ImGui::PushItemWidth(100);
				ImGui::TextUnformatted("IP:     ");
				ImGui::SameLine();
				ImGui::InputText("##hostip", ip.m_str, ip.SIZE);
				ImGui::TextUnformatted("Port: ");
				ImGui::SameLine();
				ImGui::InputInt("##port", &port);
				ImGui::PopItemWidth();
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.1f, 1));
				if (ImGui::Button("Connect & Debugging"))
				{
					m_remoteDebugger.Init(network2::cRemoteDebugger::eDebugMode::Remote
						, ip, port, &m_debugger, this);
					if (m_remoteDebugger.Start())
					{
						//g_global->ReadVProgFile(fileName);
						m_state = eState::RemoteDebug;
					}
					else
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error!! Connect Host Server", "ERROR"
							, MB_OK | MB_ICONERROR);
					}

				}
				ImGui::PopStyleColor(3);
			}
		} //~debugger isload?

		if (m_remoteDebugger.IsRun())
		{
			//ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1));
			if (ImGui::Button("Debug Cancel"))
			{
				if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
					, "Debug Cancel?", "CONFIRM", MB_YESNO))
				{
					m_remoteDebugger.Terminate();
					g_global->m_codeView->ClearCode();
				}
			}
			ImGui::PopStyleColor(3);
		} // ~debugger isLoad?


	} // ~imgui::header
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
		common::WStr128 wstr = (LPCTSTR)var.bstrVal;
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

	vector<common::cSimpleData2::sRule> rules;
	rules.push_back({ 0, "event_trigger", 1, -1 });
	rules.push_back({ 1, "input", 2, -1 });
	rules.push_back({ 2, "input", 2, 1 });
	rules.push_back({ 2, "event_trigger", 1, 0 });
	common::cSimpleData2 sdata(rules);
	sdata.Read(fileName);
	RETV(!sdata.m_root, false);

	for (auto &p : sdata.m_root->children)
	{
		if (p->name == "event_trigger")
		{
			sEventTrigger evt;
			evt.name = sdata.Get<string>(p, "name", "Event");
			evt.evtName = sdata.Get<string>(p, "eventname", "Event");
			for (auto &c : p->children)
			{
				if (c->name == "input")
				{
					variant_t var;
					StrId varName = sdata.Get<string>(c, "name", "Input");
					StrId varType = sdata.Get<string>(c, "type", "String");
					string value = sdata.Get<string>(c, "value", "0");
					if (varType == "Bool")
						var.vt = VT_BOOL;
					else if (varType == "Int")
						var.vt = VT_INT;
					else if (varType == "Float")
						var.vt = VT_R4;
					else if (varType == "String")
						var.vt = VT_BSTR;
					else
						assert(!"cDebugView::ReadEventTriggerListFile()");
					var = common::str2variant(var.vt, value);

					if (!varName.empty())
						evt.vars[varName] = common::copyvariant(var);
					common::clearvariant(var);
				}
			} //~for input
			m_events.push_back(evt);
		} //~if event_trigger
	} //~for root children

	return true;
}


// remote debugging protocol handler
bool cDebugView::UpdateInformation(remotedbg::UpdateInformation_Packet &packet)
{ 
	// read vprog file, icode file
	// intermedatecode filename -> vprog filename
	// change file extends *.icode -> *.vprog
	StrPath icodeFileName = packet.fileName;
	StrPath vprogFileName = icodeFileName.GetFileNameExceptExt();
	vprogFileName += ".vprog";
	g_global->ReadVProgFile(vprogFileName);

	// run interpreter, debugger to simulate host intepreter
	m_debugger.Clear();
	m_interpreter.Clear();

	if (!m_interpreter.Init(icodeFileName, this, this))
	{
		//::MessageBoxA(m_owner->getSystemHandle()
		//	, "Error!! Read IntermediateCode", "ERROR"
		//	, MB_OK | MB_ICONERROR);
		return true;
	}

	if (!m_debugger.Init(&m_interpreter))
	{
		//::MessageBoxA(m_owner->getSystemHandle()
		//	, "Error!! Debugger Initialize", "ERROR"
		//	, MB_OK | MB_ICONERROR);
		return true;
	}

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
	else if (funcName == "IsRobotMove")
	{
		variant_t node;
		symbolTable.Get(scopeName, "Robot Name", node);
		symbolTable.Set(scopeName, "Move", variant_t((bool)1));
		common::clearvariant(node);
	}
	else if (funcName == "ReqMove")
	{
		symbolTable.Set(scopeName, "Result", variant_t((int)0));
	}
	else if ((funcName == "SetJob_Seqwork")
		|| (funcName == "SetJob_Move")
		)
	{
		symbolTable.Set(scopeName, "Result", variant_t((bool)true));
	}
	else if ((funcName == "GetCurrentWork")
		)
	{
		symbolTable.Set(scopeName, "Result", variant_t((bool)false));
	}
	else if (funcName == "ReqLoading")
	{
		symbolTable.Set(scopeName, "Result", variant_t((int)0));
	}
	else if (funcName == "ReqUnloading")
	{
		symbolTable.Set(scopeName, "Result", variant_t((int)0));
	}
	else if (funcName == "ErrorState")
	{
		variant_t errCode;
		symbolTable.Get(scopeName, "ErrorCode", errCode);
		common::clearvariant(errCode);
	}
	else if (funcName == "Print String")
	{
		const string str = symbolTable.Get<string>(scopeName, "In String");
		g_global->m_consoleView->AddString("%s", str.c_str());
	}

	g_global->m_consoleView->AddString("call %s()", funcName.c_str());
	return 0;
}

