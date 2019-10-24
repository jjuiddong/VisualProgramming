//
// 2019-10-24, jjuiddong
// Global Variable
//
#pragma once


class cEditorView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init(HWND hWnd);
	bool InitRemoteDebugger();
	Vector2 GetMouse3DPos(const ImVec2 &mousePt);
	Vector2 GetMouse3DOriginalPos(const ImVec2 &mousePt);


public:
	common::cConfig m_config;
	cEditorView *m_editView;
};
