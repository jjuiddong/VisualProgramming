//
// 2019-10-28, jjuiddong
// Visual Programming View
//	- listing function, operator, variable
//
#pragma once


class cVProgView : public framework::cDockWindow
{
public:
	cVProgView(const string &name);
	virtual ~cVProgView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:
	void RenderDefinition(const StrId &headerName, const vprog::eNodeType::Enum type);
	void RenderOperatorList();
	void RenderSymbolTable();
};
