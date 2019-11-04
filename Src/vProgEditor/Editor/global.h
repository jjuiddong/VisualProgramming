//
// 2019-10-24, jjuiddong
// Global Variable
//
#pragma once


class cEditorView;
class cVProgView;
class cDebugView;
class cConsoleView;
class cCodeView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(HWND hWnd, graphic::cRenderer &renderer);
	bool InitRemoteDebugger();
	Vector2 GetMouse3DPos(const ImVec2 &mousePt);
	Vector2 GetMouse3DOriginalPos(const ImVec2 &mousePt);


public:
	common::cConfig m_config;
	vprog::cEditManager m_editMgr;

	cEditorView *m_editView;
	cVProgView *m_vprogView;
	cDebugView *m_dbgView;
	cConsoleView *m_consoleView;
	cCodeView *m_codeView;
};
