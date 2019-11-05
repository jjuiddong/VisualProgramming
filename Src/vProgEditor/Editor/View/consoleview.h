//
// 2019-11-04, jjuiddong
// Visual Programming Console View
//	- display console view
//
#pragma once


class cConsoleView : public framework::cDockWindow
{
public:
	cConsoleView(const string &name);
	virtual ~cConsoleView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	void AddString(const char* fmt, ...);


public:
	vector<string> m_outputs;
	int m_movScrollLine; // when update string
};

