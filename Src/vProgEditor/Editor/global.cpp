
#include "stdafx.h"
#include "global.h"


// 실행 인자값 두 번째 값을 리턴한다.
// 두번째 인자: configure 파일 경로 값을 지정한다.
// 없다면 config.txt 를 리턴한다.
string GetConfigFileNameFromCommandLine()
{
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argc <= 1)
		return "config.txt";
	return common::wstr2str(argv[1]);
}


cGlobal::cGlobal()
	: m_config(GetConfigFileNameFromCommandLine())
//	, m_guiMgr(NULL)
//	, m_dbgVisualizer(NULL)
{
	//m_map.m_textSize = 0.15f;
	//m_map.m_textOffsetY = 0.3f;
	//m_map.m_vertexSize = 0.1f;
	//m_map.m_showNodeName = false;
}

cGlobal::~cGlobal()
{
	m_editMgr.Clear();
}


bool cGlobal::Init(HWND hWnd, graphic::cRenderer &renderer)
{
	using namespace graphic;
	using namespace framework;
	namespace ed = ax::NodeEditor;

	m_editMgr.Init(renderer);
	m_editMgr.ReadDefinitionFile("function_definition.txt");
	ed::SetCurrentEditor(m_editMgr.m_editor);

	if (0)
	{
		vprog::cNode* node;
		node = m_editMgr.Generate_InputActionNode();      ed::SetNodePosition(node->m_id, ImVec2(-252, 220));
		node = m_editMgr.Generate_BranchNode();           ed::SetNodePosition(node->m_id, ImVec2(-300, 351));
		node = m_editMgr.Generate_DoNNode();              ed::SetNodePosition(node->m_id, ImVec2(-238, 504));
		node = m_editMgr.Generate_OutputActionNode();     ed::SetNodePosition(node->m_id, ImVec2(71, 80));
		node = m_editMgr.Generate_SetTimerNode();         ed::SetNodePosition(node->m_id, ImVec2(168, 316));

		node = m_editMgr.Generate_TreeSequenceNode();     ed::SetNodePosition(node->m_id, ImVec2(1028, 329));
		node = m_editMgr.Generate_TreeTaskNode();         ed::SetNodePosition(node->m_id, ImVec2(1204, 458));
		node = m_editMgr.Generate_TreeTask2Node();        ed::SetNodePosition(node->m_id, ImVec2(868, 538));

		node = m_editMgr.Generate_Comment();              ed::SetNodePosition(node->m_id, ImVec2(112, 576));
		node = m_editMgr.Generate_Comment();              ed::SetNodePosition(node->m_id, ImVec2(800, 224));

		node = m_editMgr.Generate_LessNode();             ed::SetNodePosition(node->m_id, ImVec2(366, 652));
		node = m_editMgr.Generate_WeirdNode();            ed::SetNodePosition(node->m_id, ImVec2(144, 652));
		node = m_editMgr.Generate_MessageNode();          ed::SetNodePosition(node->m_id, ImVec2(-348, 698));
		node = m_editMgr.Generate_PrintStringNode();      ed::SetNodePosition(node->m_id, ImVec2(-69, 652));

		m_editMgr.BuildNodes();

		//s_SaveIcon = ImGui_LoadTexture("Data/ic_save_white_24dp.png");
		//s_RestoreIcon = ImGui_LoadTexture("Data/ic_restore_white_24dp.png");

		framework::vprog::cNode &from = m_editMgr.m_nodes[0];
		framework::vprog::cNode &to1 = m_editMgr.m_nodes[4];
		framework::vprog::cNode &to2 = m_editMgr.m_nodes[7];
		m_editMgr.AddLink(from.m_outputs[1].id, to1.m_inputs[0].id);
		//m_editMgr.AddLink(from.m_outputs[0].id, to2.m_inputs[0].id);
	}

	//m_map.Init(renderer);
	//m_map.m_showInfluenceMap = true;

	//const string mapFileName = m_config.GetString("map-filename");
	//const string scenarioFileName = m_config.GetString("scenario-filename");
	//if (!m_map.ReadPathFile(mapFileName, scenarioFileName))
	//{
	//	::MessageBoxA(hWnd, "Read Map Error", "Error", MB_OK);
	//	return false;
	//}

	//m_textMgr.Create(512, 512);
	return true;
}


// cCommandView가 생성된 후, 호출되어야 한다.
// cGuiManager, DebugVisualizer 를 생성한다.
bool cGlobal::InitRemoteDebugger()
{
	//m_guiMgr = new scene::cGuiManager();
	//m_guiMgr->Init((framework::cDockWindow*)m_cmdView);

	//m_dbgVisualizer = new scene::cDebugVisualizer();
	//m_dbgVisualizer->Init((framework::cDockWindow*)m_cmdView, &m_remoteDebugger);

	return true;
}


// Get Current Mouse Pos 3D Location
Vector2 cGlobal::GetMouse3DPos(const ImVec2 &mousePt)
{
	const Ray ray = graphic::GetMainCamera().GetRay((int)mousePt.x, (int)mousePt.y);
	const Plane ground(Vector3(0, 1, 0), 0);
	const Vector3 tPos = ground.Pick(ray.orig, ray.dir);
	const Vector2 pickPos(tPos.x, tPos.z);
	return pickPos;
}


// Get Current Mouse Pos 3D Location
// - zoom recovery
Vector2 cGlobal::GetMouse3DOriginalPos(const ImVec2 &mousePt)
{
	const Ray ray = graphic::GetMainCamera().GetRay((int)mousePt.x, (int)mousePt.y);
	const Plane ground(Vector3(0, 1, 0), 0);
	const float zoom = graphic::GetMainCamera().GetZoom();
	const Vector3 tPos = ground.Pick(ray.orig, ray.dir) / zoom;
	const Vector2 pickPos(tPos.x, tPos.z);
	return pickPos;
}
