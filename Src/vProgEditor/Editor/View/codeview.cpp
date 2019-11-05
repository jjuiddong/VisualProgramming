
#include "stdafx.h"
#include "codeview.h"


cCodeView::cCodeView(const string &name)
	: framework::cDockWindow(name)
	, m_highlightLine(-1)
	, m_movScrollLine(-1)
{
}

cCodeView::~cCodeView()
{
}


bool cCodeView::Init(graphic::cRenderer &renderer)
{
	return true;
}


void cCodeView::OnUpdate(const float deltaSeconds)
{
}


void cCodeView::OnRender(const float deltaSeconds)
{
	if (ImGui::BeginChild("iCode Child Window", ImVec2(0, 0), true))
	{
		for (uint i = 0; i < m_strs.size(); ++i)
		{
			auto &str = m_strs[i];
			bool isSelect = (m_highlightLine == (int)i);
			ImGui::Selectable(str.c_str(), &isSelect);
			if (isSelect 
				&& (m_movScrollLine != m_highlightLine))
			{
				ImGui::SetScrollHere();
				m_movScrollLine = m_highlightLine;
			}
		}
	}
	ImGui::EndChild();

	// debug visualizer
	// show flow animation
	if ((uint)m_highlightLine < m_code.m_codes.size())
	{
		const auto &inst = m_code.m_codes[m_highlightLine];
		if ((inst.cmd == script::eCommand::cmt) && (inst.str1 == "flow"))
		{
			// debug information
			// check from-to pin id
			if (vprog::sLink *link 
				= g_global->m_editMgr.FindLink(inst.reg1, inst.reg2))
			{
				if (m_flowLinkId != link->id)
				{
					m_flowLinkId = link->id;
					g_global->m_editMgr.ShowFlow(link->id);
				}
			}
		}
	}
}


// read vprog file
bool cCodeView::ReadVProgFile(const StrPath &fileName)
{
	vprog::cVProgFile vprogFile;
	if (!vprogFile.Read(fileName))
		return false;

	common::script::cIntermediateCode code;
	if (!vprogFile.GenerateIntermediateCode(code))
		return false;

	if (!SetCode(code))
		return false;

	return true;
}


// read intermediate file
bool cCodeView::ReadIntermediateFile(const StrPath &fileName)
{
	common::script::cIntermediateCode code;
	if (!code.Read(fileName))
		return false;

	if (!SetCode(code))
		return false;

	return true;
}


// update code data
bool cCodeView::SetCode(const common::script::cIntermediateCode &icode)
{
	m_strs.clear();
	m_code = icode;

	for (uint i=0; i < icode.m_codes.size(); ++i)
	{
		const auto &code = icode.m_codes[i];
		const string str = ConvertInstructionToString(code);
		const string line = common::format("%4d    %s", i + 1, str.c_str());
		m_strs.push_back(line);
	}
	return true;
}


// set highlight linenum
bool cCodeView::SetHighLightLine(const int line)
{
	m_highlightLine = line;

	// move scroll
	if (m_movScrollLine != line)
		m_movScrollLine = -1;
	return true;
}


// clear intermeidate code
bool cCodeView::ClearCode()
{
	m_code.Clear();
	m_strs.clear();
	m_highlightLine = -1;
	m_movScrollLine = -1;
	m_flowLinkId = 0;
	return true;
}


// convert intermediate code instructio to string
string cCodeView::ConvertInstructionToString(const common::script::sInstruction &inst)
{
	namespace script = common::script;

	stringstream ss;

	ss << script::eCommand::ToString(inst.cmd) << " ";

	switch (inst.cmd)
	{
	case script::eCommand::ldbc:
	case script::eCommand::ldic:
	case script::eCommand::ldfc:
	case script::eCommand::ldsc:
		ss << common::format("val%d", inst.reg1);
		ss << ", " << common::variant2str(inst.var1, true);
		break;

	case script::eCommand::ldcmp:
		ss << common::format("val%d", inst.reg1);
		break;

	case script::eCommand::getb:
	case script::eCommand::geti:
	case script::eCommand::getf:
	case script::eCommand::gets:
	case script::eCommand::setb:
	case script::eCommand::seti:
	case script::eCommand::setf:
	case script::eCommand::sets:
		ss << "\"" << inst.str1 << "\"";
		ss << ", \"" << inst.str2 << "\"";
		ss << ", " << common::format("val%d", inst.reg1);
		break;

	case script::eCommand::eqi:
	case script::eCommand::eqf:
	case script::eCommand::eqs:
	case script::eCommand::lesi:
	case script::eCommand::lesf:
	case script::eCommand::leqi:
	case script::eCommand::leqf:
	case script::eCommand::gri:
	case script::eCommand::grf:
	case script::eCommand::greqi:
	case script::eCommand::greqf:
		ss << common::format("val%d", inst.reg1);
		ss << ", " << common::format("val%d", inst.reg2);
		break;

	case script::eCommand::eqic:
	case script::eCommand::eqfc:
	case script::eCommand::eqsc:
		ss << common::format("val%d", inst.reg1);
		ss << ", " << common::variant2str(inst.var1, true);
		break;


	case script::eCommand::symbolb:
	case script::eCommand::symboli:
	case script::eCommand::symbolf:
	case script::eCommand::symbols:
		ss << "\"" << inst.str1 << "\"";
		ss << ", \"" << inst.str2 << "\"";
		ss << ", " << common::variant2str(inst.var1, true);
		break;

	case script::eCommand::call:
	case script::eCommand::jnz:
	case script::eCommand::jmp:
		ss << "\"" << inst.str1 << "\"";
		break;

	case script::eCommand::label:
		ss.str(""); // clear stringstream
		ss << "\"" << inst.str1 << "\":";
		break;

	case script::eCommand::cmt:
		ss.str(""); // clear stringstream
		ss << "#comment ";
		ss << "\"" << inst.str1 << "\"";
		ss << ", " << inst.reg1;
		ss << ", " << inst.reg2;
		break;

	case script::eCommand::nop:
		break;

	default:
		break;
	}

	return ss.str();
}
