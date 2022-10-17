#pragma once


# include <imgui_node_editor.h>
# include <application.h>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>
#include"json.h"


namespace ed = ax::NodeEditor;
namespace json = example_json;

const char* var_type[] = {"null", "object", "array", "string", "boolean", "number"};


#pragma optimize("","off")
enum class PinKind{
    Exec,
    Pin
};

enum class PinState{
    Link,
    Free,
    Value
};

struct Pin{
    int ID;

    ed::PinId PinId;

    std::string Name;

    PinKind Kind;

    std::vector<Pin> SubPins;

    bool IsVariant;

    PinState State;

    json::value Value;
};

struct Node{
    int ID;

    ed::NodeId NodeId;

    std::string Type;

    std::string Describe;

    std::vector<Pin> InPins;

    std::vector<Pin> OutPins;
};

struct LinkInfo{
    int ID;

    ed::LinkId LinkId;

    ed::PinId InPin;

    ed::PinId OutPin;
};

struct NodeGraph :
    public Application{
    using Application::Application;

    /* Node Pin & Link*/
    Node& FindNode(ed::NodeId id);

    int FindStartNodeId();

    Pin& FindPin(ed::PinId id);

    Pin* FindPin(ed::PinId id, std::vector<Pin>& pin_arr);

    int GetNodeId(ed::PinId id);

    int IsOutPin(ed::PinId id);

    LinkInfo& FindLink(ed::LinkId id);

    Node& CreateNode(int id = -1);

    Pin& CreatePin(std::vector<Pin>& pin_arr, int id = -1);

    void CreateSubPins(Pin& pin, int num);

    LinkInfo& CreateLink(int id = -1);

    void RemoveNode(ed::NodeId);

    void RemovePin(ed::PinId);

    bool RemovePin(ed::PinId, std::vector<Pin>&);

    void ClearPin(Pin& pin_ptr);

    void RemoveLink(ed::LinkId);

    void RemoveLink(ed::PinId);

    /* Draw */
    void DrawNode(Node& node);

    void DrawRightPanel();

    void DrawValue(json::value& val);

    void DrawNull(json::value& val);

    void DrawBoolean(json::value& val);

    void DrawNumber(json::value& val);

    void DrawString(json::value& val);

    //std::tuple<bool,std::string> NodeGraph::DrawString(const std::string& s);
    void DrawArray(json::value& val);

    //void DrawObject(json::value& val);

    /* UniqueId operate */
    int GetNextId();

    std::vector<int> GetNextIds(int n);

    void RestoreId(int id);

    void RestoreId(std::vector<Pin>& pin_arr);

    /* life cycle */
    void OnStart() override;

    void OnStop() override;

    void OnFrame(float deltaTime) override;
    
    /* load & save */

    void FillCreateInfos();
    
    void Load();
    
    void UnDumpNode(const json::value& data);

    void UnDumpPins(Node& node,const json::value& data);

    void UnDumpLinks(const json::value& data);

    void UnDumpUniqueIds(const json::value& data);

    // void UnDumpLinks(const json::value& data);
    json::value DumpNode(Node& node);

    json::value DumpPin(Pin& pin);

    void DumpAll();

    ed::EditorContext* m_Context = nullptr;

    std::deque<int> m_FreeIds;

    int m_NextId=1;

    int m_NextUniqueId;

    std::vector<Node> m_Nodes;

    std::vector<LinkInfo> m_Links;

    std::string m_FilePath = "C:/MainDic/Pan01/demo/EM/Content/Script/BluePrints/Story/GraphData/dialogue.json";

    std::string m_CreateInfosPath="C:/MainDic/Pan01/demo/EM/Content/Script/BluePrints/Story/NodesMeta";

    std::vector<json::value> m_CreateInfos;
};

#pragma optimize("","on")
