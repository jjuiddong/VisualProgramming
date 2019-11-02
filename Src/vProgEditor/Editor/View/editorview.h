//
// 2019-10-24, jjuiddong
// Visual Programming Editor View
//
#pragma once


class cEditorView : public framework::cDockWindow
				  , public common::script::iFunctionCallback
{
public:
	cEditorView(const string &name);
	virtual ~cEditorView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void RenderSimpleNode();
	void RenderBlueprint();

	// interpreter callback function override
	virtual int Function(common::script::cSymbolTable &symbolTable
		, const string &scopeName
		, const string &funcName
		, void *arg) override;

};
