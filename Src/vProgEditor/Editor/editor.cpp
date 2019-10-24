//
// Visual Programming Editor
//
#include "stdafx.h"
#include "view/editorview.h"
//#include "view/infoview.h"


class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;
};


using namespace graphic;
using namespace framework;

INIT_FRAMEWORK3(cViewer);

cGlobal *g_global = NULL;

cViewer::cViewer()
{
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");

	m_windowName = L"vProg Editor";
	m_isLazyMode = true;
	const RECT r = { 0, 0, 1024, 768 };
	//const RECT r = { 0, 0, 1280, 960 };
	m_windowRect = r;
	//m_minDeltaTime = 1.f;
}

cViewer::~cViewer()
{
	SAFE_DELETE(g_global);
}


bool cViewer::OnInit()
{
	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 0.1f, 10000.0f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.3f, 0.3f, 0.3f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	m_gui.SetContext();

	g_global = new cGlobal();
	g_global->Init(m_hWnd);

	bool result = false;
	cEditorView *cmdView = new cEditorView("Editor");
	cmdView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL);
	result = cmdView->Init(m_renderer);
	assert(result);

	//c3DView *view = new c3DView("3D Map");
	//view->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL);
	//bool result = view->Init(m_renderer);
	//assert(result);

	//cInfoView *infoView = new cInfoView("Information");
	//infoView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, view, 0.25f
	//	, framework::eDockSizingOption::PIXEL);
	//result = infoView->Init();
	//assert(result);

	//g_global->m_3dView = view;
	//g_global->m_infoView = infoView;

	m_gui.SetContext();
	m_gui.SetStyleColorsDark();

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.code)
		{
		case sf::Keyboard::Escape: close(); break;
		}
		break;
	}
}
