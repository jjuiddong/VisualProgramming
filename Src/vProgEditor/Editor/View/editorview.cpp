
#include "stdafx.h"
#include "editorview.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

using namespace graphic;
using namespace framework;

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;
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

static const int            s_PinIconSize = 24;
static std::vector<Node>    s_Nodes;
static std::vector<Link>    s_Links;
static cTexture *s_HeaderBackground = nullptr;

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

static int s_NextId = 1;
static int GetNextId()
{
	return s_NextId++;
}

static ed::LinkId GetNextLinkId()
{
	return ed::LinkId(GetNextId());
}



static Node* FindNode(ed::NodeId id)
{
	for (auto& node : s_Nodes)
		if (node.ID == id)
			return &node;

	return nullptr;
}

static Link* FindLink(ed::LinkId id)
{
	for (auto& link : s_Links)
		if (link.ID == id)
			return &link;

	return nullptr;
}

static Pin* FindPin(ed::PinId id)
{
	if (!id)
		return nullptr;

	for (auto& node : s_Nodes)
	{
		for (auto& pin : node.Inputs)
			if (pin.ID == id)
				return &pin;

		for (auto& pin : node.Outputs)
			if (pin.ID == id)
				return &pin;
	}

	return nullptr;
}

static bool IsPinLinked(ed::PinId id)
{
	if (!id)
		return false;

	for (auto& link : s_Links)
		if (link.StartPinID == id || link.EndPinID == id)
			return true;

	return false;
}

static bool CanCreateLink(Pin* a, Pin* b)
{
	if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
		return false;

	return true;
}

//static void DrawItemRect(ImColor color, float expand = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRect(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color);
//};

//static void FillItemRect(ImColor color, float expand = 0.0f, float rounding = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRectFilled(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color, rounding);
//};

static void BuildNode(Node* node)
{
	for (auto& input : node->Inputs)
	{
		input.Node = node;
		input.Kind = PinKind::Input;
	}

	for (auto& output : node->Outputs)
	{
		output.Node = node;
		output.Kind = PinKind::Output;
	}
}

static Node* SpawnInputActionNode()
{
	s_Nodes.emplace_back(GetNextId(), "InputAction Fire", ImColor(255, 128, 128));
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Delegate);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Pressed", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Released", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnBranchNode()
{
	s_Nodes.emplace_back(GetNextId(), "Branch");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnDoNNode()
{
	s_Nodes.emplace_back(GetNextId(), "Do N");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Enter", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "N", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Exit", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Counter", PinType::Int);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnOutputActionNode()
{
	s_Nodes.emplace_back(GetNextId(), "OutputAction");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Sample", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Event", PinType::Delegate);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnPrintStringNode()
{
	s_Nodes.emplace_back(GetNextId(), "Print String");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnMessageNode()
{
	s_Nodes.emplace_back(GetNextId(), "", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Message", PinType::String);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnSetTimerNode()
{
	s_Nodes.emplace_back(GetNextId(), "Set Timer", ImColor(128, 195, 248));
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Object", PinType::Object);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Function Name", PinType::Function);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Time", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Looping", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnLessNode()
{
	s_Nodes.emplace_back(GetNextId(), "<", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnWeirdNode()
{
	s_Nodes.emplace_back(GetNextId(), "o.O", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTraceByChannelNode()
{
	s_Nodes.emplace_back(GetNextId(), "Single Line Trace by Channel", ImColor(255, 128, 64));
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Start", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "End", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Channel", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Complex", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Actors to Ignore", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Draw Debug Type", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Ignore Self", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Out Hit", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeSequenceNode()
{
	s_Nodes.emplace_back(GetNextId(), "Sequence");
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeTaskNode()
{
	s_Nodes.emplace_back(GetNextId(), "Move To");
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeTask2Node()
{
	s_Nodes.emplace_back(GetNextId(), "Random Wait");
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnComment()
{
	s_Nodes.emplace_back(GetNextId(), "Test Comment");
	s_Nodes.back().Type = NodeType::Comment;
	s_Nodes.back().Size = ImVec2(300, 200);

	return &s_Nodes.back();
}

void BuildNodes()
{
	for (auto& node : s_Nodes)
		BuildNode(&node);
}


ImColor GetIconColor(PinType type)
{
	switch (type)
	{
	default:
	case PinType::Flow:     return ImColor(255, 255, 255);
	case PinType::Bool:     return ImColor(220, 48, 48);
	case PinType::Int:      return ImColor(68, 201, 156);
	case PinType::Float:    return ImColor(147, 226, 74);
	case PinType::String:   return ImColor(124, 21, 153);
	case PinType::Object:   return ImColor(51, 150, 215);
	case PinType::Function: return ImColor(218, 0, 183);
	case PinType::Delegate: return ImColor(255, 48, 48);
	}
};

void DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
	IconType iconType;
	ImColor  color = GetIconColor(pin.Type);
	color.Value.w = alpha / 255.0f;
	switch (pin.Type)
	{
	case PinType::Flow:     iconType = IconType::Flow;   break;
	case PinType::Bool:     iconType = IconType::Circle; break;
	case PinType::Int:      iconType = IconType::Circle; break;
	case PinType::Float:    iconType = IconType::Circle; break;
	case PinType::String:   iconType = IconType::Circle; break;
	case PinType::Object:   iconType = IconType::Circle; break;
	case PinType::Function: iconType = IconType::Circle; break;
	case PinType::Delegate: iconType = IconType::Square; break;
	default:
		return;
	}

	ax::Widgets::Icon(ImVec2(s_PinIconSize, s_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
};


cEditorView::cEditorView(const string &name)
	: framework::cDockWindow(name)
{
}

cEditorView::~cEditorView()
{
}


bool cEditorView::Init(cRenderer &renderer)
{
	ed::Config config;
	m_Editor = ed::CreateEditor(&config);
	ed::SetCurrentEditor(m_Editor);

	Node* node;
	node = SpawnInputActionNode();      ed::SetNodePosition(node->ID, ImVec2(-252, 220));
	node = SpawnBranchNode();           ed::SetNodePosition(node->ID, ImVec2(-300, 351));
	node = SpawnDoNNode();              ed::SetNodePosition(node->ID, ImVec2(-238, 504));
	node = SpawnOutputActionNode();     ed::SetNodePosition(node->ID, ImVec2(71, 80));
	node = SpawnSetTimerNode();         ed::SetNodePosition(node->ID, ImVec2(168, 316));

	node = SpawnTreeSequenceNode();     ed::SetNodePosition(node->ID, ImVec2(1028, 329));
	node = SpawnTreeTaskNode();         ed::SetNodePosition(node->ID, ImVec2(1204, 458));
	node = SpawnTreeTask2Node();        ed::SetNodePosition(node->ID, ImVec2(868, 538));

	node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(112, 576));
	node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(800, 224));

	node = SpawnLessNode();             ed::SetNodePosition(node->ID, ImVec2(366, 652));
	node = SpawnWeirdNode();            ed::SetNodePosition(node->ID, ImVec2(144, 652));
	node = SpawnMessageNode();          ed::SetNodePosition(node->ID, ImVec2(-348, 698));
	node = SpawnPrintStringNode();      ed::SetNodePosition(node->ID, ImVec2(-69, 652));

	s_HeaderBackground = cResourceManager::Get()->LoadTexture(renderer, "BlueprintBackground.png");
	//s_SaveIcon = ImGui_LoadTexture("Data/ic_save_white_24dp.png");
	//s_RestoreIcon = ImGui_LoadTexture("Data/ic_restore_white_24dp.png");


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

	ed::SetCurrentEditor(m_Editor);
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
	namespace ed = ax::NodeEditor;

	ed::SetCurrentEditor(m_Editor);

	static ed::NodeId contextNodeId = 0;
	static ed::LinkId contextLinkId = 0;
	static ed::PinId  contextPinId = 0;
	static bool createNewNode = false;
	static Pin* newNodeLinkPin = nullptr;
	static Pin* newLinkPin = nullptr;

	ed::Begin("My Editor");
	{
		auto cursorTopLeft = ImGui::GetCursorScreenPos();
		const int bgWidth = s_HeaderBackground->m_imageInfo.Width;
		const int bgHeight = s_HeaderBackground->m_imageInfo.Height;
		util::BlueprintNodeBuilder builder(s_HeaderBackground->m_texSRV, bgWidth, bgHeight);

		for (auto& node : s_Nodes)
		{
			if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
				continue;

			const auto isSimple = node.Type == NodeType::Simple;

			bool hasOutputDelegates = false;
			for (auto& output : node.Outputs)
			{
				if (output.Type == PinType::Delegate)
					hasOutputDelegates = true;
			}

			builder.Begin(node.ID);
			if (!isSimple)
			{
				builder.Header(node.Color);
				ImGui::Spring(0);
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::Spring(1);
				ImGui::Dummy(ImVec2(0, 28));
				if (hasOutputDelegates)
				{
					ImGui::BeginVertical("delegates", ImVec2(0, 28));
					ImGui::Spring(1, 0);
					for (auto& output : node.Outputs)
					{
						if (output.Type != PinType::Delegate)
							continue;

						auto alpha = ImGui::GetStyle().Alpha;
						if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
							alpha = alpha * (48.0f / 255.0f);

						ed::BeginPin(output.ID, ed::PinKind::Output);
						ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
						ed::PinPivotSize(ImVec2(0, 0));
						ImGui::BeginHorizontal(output.ID.AsPointer());
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
						if (!output.Name.empty())
						{
							ImGui::TextUnformatted(output.Name.c_str());
							ImGui::Spring(0);
						}
						DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
						ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
						ImGui::EndHorizontal();
						ImGui::PopStyleVar();
						ed::EndPin();

						//DrawItemRect(ImColor(255, 0, 0));
					}
					ImGui::Spring(1, 0);
					ImGui::EndVertical();
					ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				}
				else
					ImGui::Spring(0);
				builder.EndHeader();
			}

			for (auto& input : node.Inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				builder.Input(input.ID);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
				ImGui::Spring(0);
				if (!input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}
				if (input.Type == PinType::Bool)
				{
					ImGui::Button("Hello");
					ImGui::Spring(0);
				}
				ImGui::PopStyleVar();
				builder.EndInput();
			} //~input

			if (isSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::Spring(1, 0);
			}

			for (auto& output : node.Outputs)
			{
				if (!isSimple && output.Type == PinType::Delegate)
					continue;

				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				builder.Output(output.ID);
				if (output.Type == PinType::String)
				{
					static char buffer[128] = "Edit Me\nMultiline!";
					static bool wasActive = false;

					ImGui::PushItemWidth(100.0f);
					ImGui::InputText("##edit", buffer, 127);
					ImGui::PopItemWidth();
					if (ImGui::IsItemActive() && !wasActive)
					{
						ed::EnableShortcuts(false);
						wasActive = true;
					}
					else if (!ImGui::IsItemActive() && wasActive)
					{
						ed::EnableShortcuts(true);
						wasActive = false;
					}
					ImGui::Spring(0);
				}
				if (!output.Name.empty())
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.Name.c_str());
				}
				ImGui::Spring(0);
				DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
				ImGui::PopStyleVar();
				builder.EndOutput();
			} //~output

			builder.End();
		} // ~for nodes

		for (auto& link : s_Links)
			ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		if (!createNewNode)
		{
			if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
			{
				auto showLabel = [](const char* label, ImColor color)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize(label);

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

					auto rectMin = ImGui::GetCursorScreenPos() - padding;
					auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
					ImGui::TextUnformatted(label);
				};

				ed::PinId startPinId = 0, endPinId = 0;
				if (ed::QueryNewLink(&startPinId, &endPinId))
				{
					auto startPin = FindPin(startPinId);
					auto endPin = FindPin(endPinId);

					newLinkPin = startPin ? startPin : endPin;

					if (startPin->Kind == PinKind::Input)
					{
						std::swap(startPin, endPin);
						std::swap(startPinId, endPinId);
					}

					if (startPin && endPin)
					{
						if (endPin == startPin)
						{
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (endPin->Kind == startPin->Kind)
						{
							showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						//else if (endPin->Node == startPin->Node)
						//{
						//    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
						//    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
						//}
						else if (endPin->Type != startPin->Type)
						{
							showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
						}
						else
						{
							showLabel("+ Create Link", ImColor(32, 45, 32, 180));
							if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
							{
								s_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
								s_Links.back().Color = GetIconColor(startPin->Type);
							}
						}
					}
				}

				ed::PinId pinId = 0;
				if (ed::QueryNewNode(&pinId))
				{
					newLinkPin = FindPin(pinId);
					if (newLinkPin)
						showLabel("+ Create Node", ImColor(32, 45, 32, 180));

					if (ed::AcceptNewItem())
					{
						createNewNode = true;
						newNodeLinkPin = FindPin(pinId);
						newLinkPin = nullptr;
						ed::Suspend();
						ImGui::OpenPopup("Create New Node");
						ed::Resume();
					}
				}
			}
			else
			{
				newLinkPin = nullptr;
			}

			ed::EndCreate();


			if (ed::BeginDelete())
			{
				ed::LinkId linkId = 0;
				while (ed::QueryDeletedLink(&linkId))
				{
					if (ed::AcceptDeletedItem())
					{
						auto id = std::find_if(s_Links.begin(), s_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
						if (id != s_Links.end())
							s_Links.erase(id);
					}
				}

				ed::NodeId nodeId = 0;
				while (ed::QueryDeletedNode(&nodeId))
				{
					if (ed::AcceptDeletedItem())
					{
						auto id = std::find_if(s_Nodes.begin(), s_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
						if (id != s_Nodes.end())
							s_Nodes.erase(id);
					}
				}
			}
			ed::EndDelete();


		}//~!createNewNode
	} // ~ed::Begin("My Editor");

	auto openPopupPosition = ImGui::GetMousePos();


	ed::Suspend();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("Create New Node"))
	{
		auto newNodePostion = openPopupPosition;
		//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

		//auto drawList = ImGui::GetWindowDrawList();
		//drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

		Node* node = nullptr;
		if (ImGui::MenuItem("Input Action"))
			node = SpawnInputActionNode();
		if (ImGui::MenuItem("Output Action"))
			node = SpawnOutputActionNode();
		if (ImGui::MenuItem("Branch"))
			node = SpawnBranchNode();
		if (ImGui::MenuItem("Do N"))
			node = SpawnDoNNode();
		if (ImGui::MenuItem("Set Timer"))
			node = SpawnSetTimerNode();
		if (ImGui::MenuItem("Less"))
			node = SpawnLessNode();
		if (ImGui::MenuItem("Weird"))
			node = SpawnWeirdNode();
		if (ImGui::MenuItem("Trace by Channel"))
			node = SpawnTraceByChannelNode();
		if (ImGui::MenuItem("Print String"))
			node = SpawnPrintStringNode();
		ImGui::Separator();
		if (ImGui::MenuItem("Comment"))
			node = SpawnComment();
		ImGui::Separator();
		if (ImGui::MenuItem("Sequence"))
			node = SpawnTreeSequenceNode();
		if (ImGui::MenuItem("Move To"))
			node = SpawnTreeTaskNode();
		if (ImGui::MenuItem("Random Wait"))
			node = SpawnTreeTask2Node();
		ImGui::Separator();
		if (ImGui::MenuItem("Message"))
			node = SpawnMessageNode();

		if (node)
		{
			createNewNode = false;

			ed::SetNodePosition(node->ID, newNodePostion);

			if (auto startPin = newNodeLinkPin)
			{
				auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

				for (auto& pin : pins)
				{
					if (CanCreateLink(startPin, &pin))
					{
						auto endPin = &pin;
						if (startPin->Kind == PinKind::Input)
							std::swap(startPin, endPin);

						s_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
						s_Links.back().Color = GetIconColor(startPin->Type);

						break;
					}
				}
			}
		}

		ImGui::EndPopup();
	}
	else
	{
		createNewNode = false;
	}

	ImGui::PopStyleVar();
	ed::Resume();

	ed::End();
}
