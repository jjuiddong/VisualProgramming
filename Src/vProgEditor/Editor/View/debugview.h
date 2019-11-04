//
// 2019-11-04, jjuiddong
// Visual Programming Debug View
//	- debugging vprog
//
#pragma once


class cDebugView : public framework::cDockWindow
				 , public common::script::iFunctionCallback
{
public:
	cDebugView(const string &name);
	virtual ~cDebugView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void RenderVariant(const StrId &name, INOUT variant_t &var);

	// interpreter callback function override
	virtual int Function(common::script::cSymbolTable &symbolTable
		, const string &scopeName
		, const string &funcName
		, void *arg) override;


public:
	enum class eState {Stop, Debug};
	eState m_state;
	common::script::cInterpreter m_interpreter;
	common::script::cDebugger m_debugger;
};

