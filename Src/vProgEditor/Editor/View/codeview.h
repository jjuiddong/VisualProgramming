//
// 2019-11-04, jjuiddong
// Visual Programming Code View
//	- intermediate code view
//
#pragma once


class cCodeView : public framework::cDockWindow
{
public:
	cCodeView(const string &name);
	virtual ~cCodeView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;

	bool ReadVProgFile(const StrPath &fileName);
	bool ReadIntermediateFile(const StrPath &fileName);
	bool SetCode(const common::script::cIntermediateCode &icode);
	bool SetHighLightLine(const int line);
	bool ClearCode();


protected:
	string ConvertInstructionToString(const common::script::sInstruction &inst);


public:
	common::script::cIntermediateCode m_code;
	vector<string> m_strs;
	int m_highlightLine;
	int m_movScrollLine; // when update highlight line
	ax::NodeEditor::LinkId m_flowLinkId;
};

