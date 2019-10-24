
#include "stdafx.h"
#include "editorview.h"
//#include "../ui/guimanager.h"
//#include "../ui/debugvisualizer.h"

using namespace graphic;
using namespace framework;


namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;
using namespace ax;
using ax::Widgets::IconType;
static ed::EditorContext* m_Editor = nullptr;


enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

enum class PinKind
{
	Output,
	Input
};

enum class NodeType
{
	Blueprint,
	Simple,
	Tree,
	Comment
};

struct Node;

struct Pin
{
	ed::PinId   ID;
	::Node*     Node;
	std::string Name;
	PinType     Type;
	PinKind     Kind;

	Pin(int id, const char* name, PinType type) :
		ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
	{
	}
};

struct Node
{
	ed::NodeId ID;
	std::string Name;
	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	NodeType Type;
	ImVec2 Size;

	std::string State;
	std::string SavedState;

	Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
		ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
	{
	}
};

struct Link
{
	ed::LinkId ID;

	ed::PinId StartPinID;
	ed::PinId EndPinID;

	ImColor Color;

	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
	{
	}
};



void ImGuiEx_BeginColumn()
{
	ImGui::BeginGroup();
}

void ImGuiEx_NextColumn()
{
	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::BeginGroup();
}

void ImGuiEx_EndColumn()
{
	ImGui::EndGroup();
}


cEditorView::cEditorView(const string &name)
	: framework::cDockWindow(name)
{
}

cEditorView::~cEditorView()
{
}


bool cEditorView::Init(cRenderer &renderer)
{

	return true;
}


void cEditorView::OnUpdate(const float deltaSeconds)
{
}


void cEditorView::OnRender(const float deltaSeconds)
{
	//RenderSimpleNode();
	RenderBlueprint();
}


void cEditorView::RenderSimpleNode()
{
	namespace ed = ax::NodeEditor;

	static ed::EditorContext* g_Context = nullptr;
	if (!g_Context)
	{
		ed::Config config;
		g_Context = ed::CreateEditor(&config);
	}

	ed::SetCurrentEditor(g_Context);
	ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	int uniqueId = 1;
	// Start drawing nodes.
	//ed::BeginNode(uniqueId++);
	//	ImGui::Text("Node A");
	//	ed::BeginPin(uniqueId++, ed::PinKind::Input);
	//		ImGui::Text("-> In");
	//	ed::EndPin();
	//	ImGui::SameLine();
	//	ed::BeginPin(uniqueId++, ed::PinKind::Output);
	//		ImGui::Text("Out ->");
	//	ed::EndPin();
	//	ed::EndNode();
	//ed::End();

	ed::NodeId nodeA_Id = uniqueId++;
	ed::PinId nodeA_InputPinId = uniqueId++;
	ed::PinId nodeA_OutputPinId = uniqueId++;

	struct LinkInfo
	{
		ed::LinkId Id;
		ed::PinId  InputId;
		ed::PinId  OutputId;
	};
	static bool g_FirstFrame = true;
	static ImVector<LinkInfo> g_Links; // List of live links. It is dynamic unless you want to create read-only view over nodes.

	if (g_FirstFrame)
		ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
	ed::BeginNode(nodeA_Id);
		ImGui::Text("Node A");
		ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
			ImGui::Text("-> In");
		ed::EndPin();
		ImGui::SameLine();
		ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
			ImGui::Text("Out ->");
		ed::EndPin();
	ed::EndNode();

	// Submit Node B
	ed::NodeId nodeB_Id = uniqueId++;
	ed::PinId nodeB_InputPinId1 = uniqueId++;
	ed::PinId nodeB_InputPinId2 = uniqueId++;
	ed::PinId nodeB_OutputPinId = uniqueId++;

	if (g_FirstFrame)
		ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
	ed::BeginNode(nodeB_Id);
		ImGui::Text("Node B");
		ImGuiEx_BeginColumn();
			ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
				ImGui::Text("-> In1");
			ed::EndPin();
			ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
				ImGui::Text("-> In2");
			ed::EndPin();
			ImGuiEx_NextColumn();
			ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
				ImGui::Text("Out ->");
			ed::EndPin();
		ImGuiEx_EndColumn();
	ed::EndNode();

	// Submit Links
	for (auto& linkInfo : g_Links)
		ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

	// Handle creation action, returns true if editor want to create new object (node or link)
	static int g_NextLinkId = 100;
	if (ed::BeginCreate())
	{
		ed::PinId inputPinId, outputPinId;
		if (ed::QueryNewLink(&inputPinId, &outputPinId))
		{
			if (inputPinId && outputPinId)
			{
				if (ed::AcceptNewItem())
				{
					g_Links.push_back({ ed::LinkId(g_NextLinkId++), inputPinId, outputPinId });
					ed::Link(g_Links.back().Id, g_Links.back().InputId, g_Links.back().OutputId);
				}
			}
		}
	}
	ed::EndCreate();

	// Handle deletion action
	if (ed::BeginDelete())
	{
		ed::LinkId deletedLinkId;
		while (ed::QueryDeletedLink(&deletedLinkId))
		{
			if (ed::AcceptDeletedItem())
			{
				for (auto& link : g_Links)
				{
					if (link.Id == deletedLinkId)
					{
						g_Links.erase(&link);
						break;
					}
				}
			}
		}

		ed::NodeId deletedNodeId;
		while (ed::QueryDeletedNode(&deletedNodeId))
		{
			if (ed::AcceptDeletedItem())
			{
			}
		}
	}
	ed::EndDelete(); // Wrap up deletion action

	ed::End();
	ed::SetCurrentEditor(nullptr);
	g_FirstFrame = false;
}


void cEditorView::RenderBlueprint()
{
	static std::vector<Node> s_Nodes;
	static std::vector<Link> s_Links;


}
