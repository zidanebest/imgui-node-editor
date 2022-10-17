#include "dialogue_editor.h"

#include <fstream>
#include <filesystem>
#include <iostream>

#include "imgui_internal.h"
namespace fs = std::filesystem;

int Main(int argc, char** argv){
    NodeGraph exampe("Simple", argc, argv);

    if(exampe.Create()) return exampe.Run();

    return 0;
}

Node& NodeGraph::FindNode(ed::NodeId id){ for(auto& node : m_Nodes){ if(node.NodeId == id){ return node; } } }

int NodeGraph::FindStartNodeId(){
    for(auto& node:m_Nodes){
        if(node.Type=="Start"){
            return node.ID;
        }
    }
    return -1;
}

Pin& NodeGraph::FindPin(ed::PinId id){
    for(auto& node : m_Nodes){
        auto pin = FindPin(id, node.InPins);
        if(pin) return *pin;
        pin = FindPin(id, node.OutPins);
        if(pin) return *pin;
    }
}


Pin* NodeGraph::FindPin(ed::PinId id, std::vector<Pin>& pin_arr){
    for(auto pin_ptr = pin_arr.begin(); pin_ptr != pin_arr.end();){
        if(pin_ptr->PinId == id){ return &*pin_ptr; }
        else{
            // 在subpin中找
            for(auto sub_pin_ptr = pin_ptr->SubPins.begin(); sub_pin_ptr != pin_ptr->SubPins.end();){
                if(sub_pin_ptr->PinId == id){ return &*sub_pin_ptr; }
                else{ ++sub_pin_ptr; }
            }
            ++pin_ptr;
        }
    }
    return nullptr;
}

int NodeGraph::GetNodeId(ed::PinId id){
    for(auto& node : m_Nodes){
        auto pin = FindPin(id, node.InPins);
        if(pin) return node.ID;
        pin = FindPin(id, node.OutPins);
        if(pin) return node.ID;
    }
}

// -1 failed 0 false 1 true
int NodeGraph::IsOutPin(ed::PinId id){
    for(auto& node : m_Nodes){
        auto pin = FindPin(id, node.InPins);
        if(pin) return 0;
        pin = FindPin(id, node.OutPins);
        if(pin) return 1;
    }
    return -1;
}

Node& NodeGraph::CreateNode(int id){
    if(id == -1){ id = GetNextId(); }
    ed::NodeId node_id(id);
    m_Nodes.emplace_back();
    auto& node  = m_Nodes.back();
    node.ID     = id;
    node.NodeId = node_id;
    return node;
}

Pin& NodeGraph::CreatePin(std::vector<Pin>& pin_arr, int id){
    if(id == -1){ id = GetNextId(); }
    ed::PinId pin_id(id);
    pin_arr.emplace_back();
    auto& pin = pin_arr.back();
    pin.ID    = id;
    pin.PinId = pin_id;
    return pin;
}

void NodeGraph::CreateSubPins(Pin& pin, int num){
    for(int i = 0; i < num; ++i){
        auto& sub_pin     = CreatePin(pin.SubPins);
        sub_pin.Kind      = pin.Kind;
        sub_pin.Name      = pin.Name;
        sub_pin.IsVariant = false;
        sub_pin.State     = PinState::Free;
    }
}

LinkInfo& NodeGraph::CreateLink(int id){
    if(id == -1){ id = GetNextId(); }
    ed::LinkId link_id(id);
    m_Links.emplace_back();
    auto& link  = m_Links.back();
    link.ID     = id;
    link.LinkId = link_id;
    return link;
}

void NodeGraph::RemoveNode(ed::NodeId id){
    for(auto node_ptr = m_Nodes.begin(); node_ptr != m_Nodes.end();){
        if(node_ptr->NodeId == id){
            for(auto pin_ptr = node_ptr->InPins.begin(); pin_ptr != node_ptr->InPins.end(); ++pin_ptr){
                ClearPin(*pin_ptr);
            }
            for(auto pin_ptr = node_ptr->OutPins.begin(); pin_ptr != node_ptr->OutPins.end(); ++pin_ptr){
                ClearPin(*pin_ptr);
            }
            RestoreId(node_ptr->ID);
            node_ptr = m_Nodes.erase(node_ptr);
            return;
        }
        else{ ++node_ptr; }
    }
}

void NodeGraph::RemovePin(ed::PinId id){
    for(auto node_ptr = m_Nodes.begin(); node_ptr != m_Nodes.end(); ++node_ptr){
        if(RemovePin(id, node_ptr->InPins)){ return; }
        if(RemovePin(id, node_ptr->OutPins)){ return; }
    }
}

bool NodeGraph::RemovePin(ed::PinId id, std::vector<Pin>& pin_arr){
    for(auto pin_ptr = pin_arr.begin(); pin_ptr != pin_arr.end();){
        if(pin_ptr->PinId == id){
            ClearPin(*pin_ptr);
            pin_ptr = pin_arr.erase(pin_ptr);
            return true;
        }
        else{
            // 在subpin中找
            for(auto sub_pin_ptr = pin_ptr->SubPins.begin(); sub_pin_ptr != pin_ptr->SubPins.end();){
                if(sub_pin_ptr->PinId == id){
                    ClearPin(*sub_pin_ptr);
                    sub_pin_ptr = pin_ptr->SubPins.erase(sub_pin_ptr);
                    return true;
                }
                else{ ++sub_pin_ptr; }
            }
            ++pin_ptr;
        }
    }
    return false;
}

void NodeGraph::ClearPin(Pin& pin_ptr){
    if(pin_ptr.IsVariant && !pin_ptr.SubPins.empty()){
        for(auto sub_pin_ptr = pin_ptr.SubPins.begin(); sub_pin_ptr != pin_ptr.SubPins.end(); ++sub_pin_ptr){
            RemoveLink(sub_pin_ptr->PinId);
            RestoreId(sub_pin_ptr->ID);
        }
        pin_ptr.SubPins.clear();
    }
    RemoveLink(pin_ptr.PinId);
    RestoreId(pin_ptr.ID);
}

void NodeGraph::RemoveLink(ed::LinkId id){
    for(auto link_ptr = m_Links.begin(); link_ptr != m_Links.end();){
        if(link_ptr->LinkId == id){
            RestoreId(link_ptr->ID);
            link_ptr = m_Links.erase(link_ptr);
            return;
        }
        else{ ++link_ptr; }
    }
}

void NodeGraph::RemoveLink(ed::PinId id){
    for(auto link_ptr = m_Links.begin(); link_ptr != m_Links.end();){
        if(link_ptr->InPin == id || link_ptr->OutPin == id){
            RestoreId(link_ptr->ID);
            link_ptr = m_Links.erase(link_ptr);
        }
        else{ ++link_ptr; }
    }
}


void NodeGraph::DrawNode(Node& node){

    auto& io = ImGui::GetIO();

    ed::BeginNode(node.ID);
    {
        //int id = 0;
        ImGui::Text("%s : %s", node.Type.data(), node.Describe.data());

        ImGui::BeginGroup();
        for(auto& pin : node.InPins){
            std::string pin_flag = "";
            if(pin.Kind == PinKind::Exec){ pin_flag = "=> "; }
            if(pin.Kind == PinKind::Pin){ pin_flag = "O "; }
            if(pin.IsVariant){
                std::string text(pin_flag + pin.Name + "_1");
                ed::BeginPin(pin.PinId, ed::PinKind::Input);
                ImGui::TextUnformatted(text.data());
                ed::EndPin();
                ImGui::SameLine();

                ImGui::PushID(m_NextUniqueId++);
                if(ImGui::Button("-")){
                    if(!pin.SubPins.empty()){
                        ClearPin(pin.SubPins.back());
                        pin.SubPins.pop_back();
                    }
                }
                ImGui::PopID();

                ImGui::SameLine();

                ImGui::PushID(m_NextUniqueId++);
                if(ImGui::Button("+")){
                    auto& new_pin     = CreatePin(pin.SubPins);
                    new_pin.Name      = pin.Name;
                    new_pin.Kind      = pin.Kind;
                    new_pin.IsVariant = false;
                    new_pin.State     = PinState::Free;
                }
                ImGui::PopID();


                if(!pin.SubPins.empty()){
                    for(int i = 0; i < pin.SubPins.size(); ++i){
                        ed::BeginPin(pin.SubPins[i].PinId, ed::PinKind::Input);
                        ImGui::TextUnformatted(
                            std::string(pin_flag + pin.Name + "_" + std::to_string(i + 2)).data());
                        ed::EndPin();
                    }
                }
            }
            else{
                ed::BeginPin(pin.PinId, ed::PinKind::Input);
                ImGui::TextUnformatted(std::string(pin_flag + pin.Name).data());
                ed::EndPin();
            }
        }

        ImGui::EndGroup();
        ImGui::SameLine(0, 100);
        ImGui::BeginGroup();
        for(auto& pin : node.OutPins){
            std::string pin_flag = "";
            if(pin.Kind == PinKind::Exec){ pin_flag = "=> "; }
            if(pin.Kind == PinKind::Pin){ pin_flag = "O "; }
            if(pin.IsVariant){
                std::string text(pin_flag + pin.Name + "_1");
                ed::BeginPin(pin.PinId, ed::PinKind::Output);
                ImGui::TextUnformatted(text.data());
                ed::EndPin();
                ImGui::SameLine();

                ImGui::PushID(m_NextUniqueId++);
                if(ImGui::Button("-")){
                    if(!pin.SubPins.empty()){
                        ClearPin(pin.SubPins.back());
                        pin.SubPins.pop_back();
                    }
                }
                ImGui::PopID();

                ImGui::SameLine();

                ImGui::PushID(m_NextUniqueId++);
                if(ImGui::Button("+")){
                    auto& new_pin     = CreatePin(pin.SubPins);
                    new_pin.Name      = pin.Name;
                    new_pin.Kind      = pin.Kind;
                    new_pin.IsVariant = false;
                    new_pin.State     = PinState::Free;
                }
                ImGui::PopID();

                if(!pin.SubPins.empty()){
                    for(int i = 0; i < pin.SubPins.size(); ++i){
                        ed::BeginPin(pin.SubPins[i].PinId, ed::PinKind::Output);
                        ImGui::TextUnformatted(
                            std::string(pin_flag + pin.Name + "_" + std::to_string(i + 2)).data());
                        ed::EndPin();
                    }
                }
            }
            else{
                ed::BeginPin(pin.PinId, ed::PinKind::Output);
                ImGui::TextUnformatted(std::string(pin_flag + pin.Name).data());
                ed::EndPin();
            }
        }
        ImGui::EndGroup();
    }
    ed::EndNode();
}

void NodeGraph::DrawRightPanel(){
    ImGui::Begin("Detail Panel");

    /*if(ImGui::BeginMenu("mmm")){
        ImGui::MenuItem("abc");
        ImGui::MenuItem("efg");
        ImGui::EndMenu();
    }
    static int current=0;
    const char* combo[]={"aaa","bbb","ccc"};
    ImGui::Combo("combo",&current,combo,sizeof(combo)/sizeof(*combo));*/

    // zoom 
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("Zoom to Content")) ed::NavigateToContent();
    ImGui::PopID();
    ImGui::SameLine();

    // save
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("Save")){ DumpAll(); }
    ImGui::PopID();

    std::vector<ed::NodeId> selectedNodes;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    selectedNodes.resize(nodeCount);
    if(selectedNodes.empty()){
        ImGui::End();
        return;
    }

    // name
    auto& node = FindNode(selectedNodes.back());
    ImGui::Text("%s: ", node.Type.data());
    ImGui::SameLine();


    // describe
    char describe[128] = {'\0'};
    for(int i = 0; i < node.Describe.size(); ++i){ describe[i] = node.Describe[i]; }
    describe[node.Describe.size()] = '\0';
    ImGui::PushID(m_NextUniqueId++);
    ImGui::InputText("", describe, sizeof(describe) / sizeof(*describe));
    ImGui::PopID();
    node.Describe = std::move(describe);


    for(auto& pin : node.InPins){
        if(pin.Kind == PinKind::Pin){
            if(pin.IsVariant == true){
                ImGui::PushID(m_NextUniqueId++);
                ImGui::Text("%s_1", pin.Name.data());
                ImGui::PopID();
                DrawValue(pin.Value);
                for(int i = 0; i < pin.SubPins.size(); ++i){
                    ImGui::PushID(m_NextUniqueId++);
                    ImGui::Text("%s_%d", pin.SubPins[i].Name.data(), i + 2);
                    ImGui::PopID();
                    DrawValue(pin.SubPins[i].Value);
                }
            }
            else{
                ImGui::PushID(m_NextUniqueId++);
                ImGui::Text("%s", pin.Name.data());
                ImGui::PopID();
                DrawValue(pin.Value);
            }
        }
    }

    //DrawValue(m_str);
    //DrawValue(m_float);
    /*DrawNumber(m_float);
    DrawString(m_str);
    DrawArray(m_arr);*/
    // for(auto& pin:node.InPins){
    //     ImGui::TextUnformatted("pins");
    // }

    ImGui::End();
}

int NodeGraph::GetNextId(){
    if(m_FreeIds.empty()){ return m_NextId++; }
    else{
        int ret = m_FreeIds.front();
        m_FreeIds.pop_front();
        return ret;
    }
}

std::vector<int> NodeGraph::GetNextIds(int n){
    if(n == 0) return std::vector<int>();
    std::vector<int> ret;
    for(int i = 0; i < n; ++i){ ret.push_back(GetNextId()); }
    return ret;
}

void NodeGraph::RestoreId(int id){ m_FreeIds.push_front(id); }

void NodeGraph::RestoreId(std::vector<Pin>& pin_arr){ for(auto& pin : pin_arr){ RestoreId(pin.ID); } }

void NodeGraph::DrawValue(json::value& val){

    int i = static_cast<int>(val.type());

    ImGui::PushID(m_NextUniqueId++);
    ImGui::PushItemWidth(80);
    ImGui::Combo("value type", &i, var_type, sizeof(var_type) / sizeof(*var_type));
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::Indent();
    switch(static_cast<json::type_t>(i)){
        case json::type_t::number: DrawNumber(val);
            break;
        case json::type_t::string: DrawString(val);
            break;
        case json::type_t::boolean: DrawBoolean(val);
            break;
        case json::type_t::array: DrawArray(val);
            break;
        /*case json::type_t::object: DrawObject(val);
            break;*/
        default: DrawNull(val);
            break;
    }
    ImGui::Unindent();
}

void NodeGraph::DrawNull(json::value& val){ val = json::value(); }

void NodeGraph::DrawBoolean(json::value& val){
    auto ptr = val.get_ptr<json::boolean>();
    bool b   = ptr ? *ptr : false;
    ImGui::PushID(m_NextUniqueId++);
    ImGui::Checkbox("", &b);
    ImGui::PopID();
    val = b;
}

void NodeGraph::DrawNumber(json::value& val){
    auto   ptr = val.get_ptr<json::number>();
    double f   = ptr ? *ptr : 0;
    ImGui::PushID(m_NextUniqueId++);
    ImGui::InputDouble("", &f);
    ImGui::PopID();
    val = f;
}

void NodeGraph::DrawString(json::value& val){
    auto ptr = val.get_ptr<json::string>();

    char str[128] = {'\0'};
    if(ptr){
        for(int i = 0; i < ptr->size(); ++i){ str[i] = (*ptr)[i]; }
        str[ptr->size()] = '\0';
    }
    ImGui::PushID(m_NextUniqueId++);
    ImGui::InputText("", str, sizeof(str) / sizeof(*str));
    ImGui::PopID();
    val = std::move(str);
}

/*std::tuple<bool,std::string> NodeGraph::DrawString(const std::string& s){

    char str[128] = {'\0'};
    for(int i = 0; i < s.size(); ++i){ str[i] = s[i]; }
        str[s.size()] = '\0';
    
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::InputText("", str, sizeof(str) / sizeof(*str))){
        ImGui::PopID();
        return std::tuple<bool,std::string>(true,std::move(str));
    }
    ImGui::PopID();
    return std::tuple<bool,std::string>(false,"");
}*/


void NodeGraph::DrawArray(json::value& val){

    auto ptr = val.get_ptr<json::array>();

    json::array arr;
    if(ptr){ arr = *ptr; }
    int t = 0;
    if(!arr.empty()){ t = static_cast<int>(arr[0].type()); }
    int store_t = t;
    ImGui::PushID(m_NextUniqueId++);
    ImGui::PushItemWidth(80);
    if(ImGui::Combo("element type", &t, var_type, sizeof(var_type) / sizeof(*var_type))){
        if(store_t != t && static_cast<json::type_t>(t) != json::type_t::null){
            arr.clear();
            arr.emplace_back();
        }
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("+")){ arr.emplace_back(); }
    ImGui::SameLine();
    ImGui::PopID();
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("-")){ if(arr.size() > 1){ arr.pop_back(); } }
    ImGui::PopID();

    ImGui::Indent();
    for(int i = 0; i < arr.size(); ++i){
        switch(static_cast<json::type_t>(t)){
            case json::type_t::number: DrawNumber(arr[i]);
                break;
            case json::type_t::string: DrawString(arr[i]);
                break;
            case json::type_t::boolean: DrawBoolean(arr[i]);
                break;
            case json::type_t::array: DrawArray(arr[i]);
                break;
            /*case json::type_t::object: DrawObject(arr[i]);
                break;*/
            default: DrawNull(arr[i]);
                break;
        }
    }
    val = std::move(arr);
    ImGui::Unindent();

}

void NodeGraph::FillCreateInfos(){
    fs::path path(m_CreateInfosPath);
    if(!exists(path)){ std::cerr << "directory path not exit" << std::endl; }
    fs::directory_entry entry(path);
    if(entry.status().type() != std::filesystem::file_type::directory){
        std::cerr << "create infos path is not a directory path!" << std::endl;
    }
    fs::directory_iterator list(path);
    for(auto& it : list){
        std::string   file_name = it.path().filename().string();
        std::string   full_path = m_CreateInfosPath + '/' + file_name;
        std::string   data;
        std::ifstream file(full_path);
        if(file){
            file.seekg(0, std::ios_base::end);
            auto size = static_cast<size_t>(file.tellg());
            file.seekg(0, std::ios_base::beg);

            data.reserve(size);
            data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }
        m_CreateInfos.emplace_back(json::value::parse(data));
    }
}

void NodeGraph::Load(){
    std::string   data;
    std::ifstream file(m_FilePath);
    if(file){
        file.seekg(0, std::ios_base::end);
        auto size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios_base::beg);

        data.reserve(size);
        data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        
        json::value load_data;
        load_data = json::value::parse(data);
        //
        //UnDumpNodes(load_data["nodes"]);

        auto& nodes_data = load_data["nodes"];
        if(nodes_data.is_object()){ for(const auto& pair : nodes_data.get<json::object>()){ UnDumpNode(pair.second); } }
        
        UnDumpLinks(load_data["links"]);

        // UnDump Unique id state
        UnDumpUniqueIds(load_data);
        
    }
    
}

PinKind GetPinKind(std::string pin_kind){
    if(pin_kind=="exec"||pin_kind=="in_execs"||pin_kind=="out_execs")
        return PinKind::Exec;
    else
        return PinKind::Pin;
}

std::vector<Pin>& GetNodePinTypeArr(Node& node,std::string pin_type){
    if(pin_type=="in"||pin_type=="in_execs"||pin_type=="in_pins")
        return node.InPins;
    else
        return node.OutPins;
}

void NodeGraph::UnDumpNode(const json::value& data){
    auto& node    = CreateNode((int)data["id"].get<json::number>());
    node.Type     = data["type"].get<json::string>();
    node.Describe = data["describe"].get<json::string>();
    UnDumpPins(node,data);
}

void NodeGraph::UnDumpPins(Node& node, const json::value& data){
    // UnDump Pins
    auto UnDumpPin = [this,&data,&node](const char* pin_kind_type){
        if(data.contains(pin_kind_type)){
            auto& execs_data = data[pin_kind_type];
            for(auto& pin_data_pair : execs_data.get<json::object>()){
                auto& pin_data = pin_data_pair.second;
                auto& pin = CreatePin(GetNodePinTypeArr(node, pin_kind_type), (int)pin_data["id"].get<json::number>());
                pin.Name = pin_data["name"].get<json::string>();
                pin.Kind = GetPinKind(pin_kind_type);
                pin.IsVariant = pin_data["variant"].get<json::boolean>();
                pin.State = PinState::Free;
                pin.Value = pin_data["value"];

                if(pin.IsVariant == true && pin_data.contains("sub_pins")&&pin_data["sub_pins"].is_object()){
                    auto& sub_pins_data = pin_data["sub_pins"];
                    for(auto& sub_pin_data_pair : sub_pins_data.get<json::object>()){
                        auto& sub_pin_data = sub_pin_data_pair.second;
                        auto& sub_pin      = CreatePin(pin.SubPins, (int)sub_pin_data["id"].get<json::number>());
                        sub_pin.Name       = sub_pin_data["name"].get<json::string>();
                        sub_pin.Kind       = pin.Kind;
                        sub_pin.IsVariant  = false;
                        sub_pin.State      = PinState::Free;
                        sub_pin.Value      = sub_pin_data["value"];
                    }
                }
            }
        }
    };
    UnDumpPin("in_execs");
    UnDumpPin("out_execs");
    UnDumpPin("in_pins");
    UnDumpPin("out_pins");
    
}

void NodeGraph::UnDumpLinks(const json::value& data){
    if(data.is_object()){
        for(auto& link_data_pair:data.get<json::object>()){
            auto& link_data=link_data_pair.second;
            auto& link = CreateLink((int)link_data["id"].get<json::number>());
            int input_id=link_data.contains("in_exec_id") ? (int)link_data["in_exec_id"].get<json::number>():(int)link_data["in_pin_id"].get<json::number>();
            int output_id=link_data.contains("out_exec_id") ? (int)link_data["out_exec_id"].get<json::number>():(int)link_data["out_pin_id"].get<json::number>();
            link.InPin  = input_id;
            link.OutPin = output_id;
        }
    }
}

void NodeGraph::UnDumpUniqueIds(const json::value& data){
    m_NextId=(int)data["next_id"].get<json::number>();
    for(auto& free_id:data["free_ids"].get<json::array>()){
        m_FreeIds.push_back((int)free_id.get<json::number>());
    }
}

/*
void NodeGraph::DrawObject(json::value& val){
    auto ptr = val.get_ptr<json::object>();

    json::object obj;
    if(ptr){ obj = *ptr; }
    int t = 0;
    if(!obj.empty()){ t = static_cast<int>(obj.begin()->second.type()); }
    int store_t = t;
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Combo("element type", &t, var_type, sizeof(var_type) / sizeof(*var_type))){
        if(store_t != t && static_cast<json::type_t>(t) != json::type_t::null){
            obj.clear();
            /*char input[128]={'\0'};
            ImGui::PushID(m_NextUniqueId++);
            ImGui::InputText("",input,sizeof(input) / sizeof(*input));
            ImGui::PopID();
            obj.insert(std::make_pair(input, json::value()));#1#
            obj.insert(std::make_pair("", json::value()));
        }
    }
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("+")){
        obj.insert(std::make_pair("", json::value()));
    }
    ImGui::SameLine();
    ImGui::PopID();
    
    ImGui::PushID(m_NextUniqueId++);
    if(ImGui::Button("-")){ if(obj.size() > 1){ obj.erase(--obj.end()); } }
    ImGui::PopID();

    ImGui::Indent();
    
    for(auto b = obj.begin(); b != obj.end();){
        ImGui::PushItemWidth(50);
        auto result=DrawString(b->first);
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        
        switch(static_cast<json::type_t>(t)){
            case json::type_t::number: DrawNumber(b->second);
                break;
            case json::type_t::string: DrawString(b->second);
                break;
            case json::type_t::boolean: DrawBoolean(b->second);
                break;
            case json::type_t::array: DrawArray(b->second);
            default: break;
        }
        if(std::get<0>(result)==true){
            obj.insert(std::make_pair(std::get<1>(result),b->second));
            obj.erase(b++);
        }
        else{
            ++b;
        }
    }
    val = std::move(obj);
    ImGui::Unindent();
}
*/

json::value NodeGraph::DumpNode(Node& node){
    json::value node_data;
    node_data["describe"] = node.Describe;
    node_data["id"]       = (double)node.ID;
    node_data["type"]     = node.Type;
    for(int i = 0; i < node.InPins.size(); ++i){
        auto& pin = node.InPins[i];
        if(pin.Kind == PinKind::Exec){ node_data["in_execs"][std::to_string(pin.ID)] = DumpPin(pin); }
        else{ node_data["in_pins"][std::to_string(pin.ID)] = DumpPin(pin); }
    }
    for(int i = 0; i < node.OutPins.size(); ++i){
        auto& pin = node.OutPins[i];
        if(pin.Kind == PinKind::Exec){ node_data["out_execs"][std::to_string(pin.ID)] = DumpPin(pin); }
        else{ node_data["out_pins"][std::to_string(pin.ID)] = DumpPin(pin); }
    }
    return node_data;
}

json::value NodeGraph::DumpPin(Pin& pin){
    json::value pin_data;
    pin_data["name"]    = pin.Name;
    pin_data["id"]      = (double)pin.ID;
    pin_data["value"]   = pin.Value;
    pin_data["variant"] = pin.IsVariant;
    if(pin.IsVariant){
        for(int i = 0; i < pin.SubPins.size(); ++i){
            auto& sub_pin                                    = pin.SubPins[i];
            pin_data["sub_pins"][std::to_string(sub_pin.ID)] = DumpPin(sub_pin);
        }
    }

    return pin_data;
}

void NodeGraph::DumpAll(){
    json::value save_data;
    json::value nodes_data;
    json::value links_data;
    json::value ids_data;


    // Dump Nodes & Pins
    for(int i = 0; i < m_Nodes.size(); ++i){
        auto& node                          = m_Nodes[i];
        nodes_data[std::to_string(node.ID)] = DumpNode(node);
    }

    save_data["nodes"] = nodes_data;

    // Dump Links
    for(int i = 0; i < m_Links.size(); ++i){
        auto&       link = m_Links[i];
        json::value link_data;
        link_data["id"]   = (double)link.ID;
        auto& in_pin      = FindPin(link.InPin);
        auto& out_pin     = FindPin(link.OutPin);
        auto  in_node_id  = GetNodeId(link.InPin);
        auto  out_node_id = GetNodeId(link.OutPin);

        if(in_pin.Kind == PinKind::Exec){
            link_data["in_exec_id"]  = (double)in_pin.ID;
            link_data["out_exec_id"] = (double)out_pin.ID;
            link_data["out_node_id"] = (double)out_node_id;
            link_data["in_node_id"]  = (double)in_node_id;
        }
        else{
            link_data["in_pin_id"]   = (double)in_pin.ID;
            link_data["out_pin_id"]  = (double)out_pin.ID;
            link_data["out_node_id"] = (double)out_node_id;
            link_data["in_node_id"]  = (double)in_node_id;
        }
        links_data[std::to_string(link.ID)] = link_data;
    }
    save_data["links"] = links_data;

    // Dump Unique Ids
    save_data["next_id"]=(double)m_NextId;
    std::vector<json::value> free_ids;
    free_ids.reserve(m_FreeIds.size());
    for(auto& free_id:m_FreeIds){
        free_ids.push_back((double)free_id);
    }
    save_data["free_ids"]=free_ids;

    // Dump StartNode Id
    save_data["start_id"]=(double)FindStartNodeId();
    
    auto data = save_data.dump(2);
    std::ofstream settingsFile(m_FilePath);
    if(settingsFile) settingsFile << data;
}


void NodeGraph::OnStart(){
    ed::Config config;
    config.SettingsFile = "Simple.json";
    config.UserPointer  = this;
    
    FillCreateInfos();
    Load();
    m_Context = ed::CreateEditor(&config);

}

void NodeGraph::OnStop(){ ed::DestroyEditor(m_Context); }




void NodeGraph::OnFrame(float deltaTime){
    auto& io = ImGui::GetIO();

    // ImGui::ShowDemoWindow();

    auto& t = *m_Context;
    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
    ImGui::Separator();

    ed::SetCurrentEditor(m_Context);

    m_NextUniqueId = 100000;

    DrawRightPanel();


    ed::Begin("My Editor", ImVec2(0.0, 0.0f));

    ed::Suspend();
    if(ed::ShowBackgroundContextMenu()){ ImGui::OpenPopup("Create New Node"); }
    ed::Resume();

    static int a = 1;
    ed::Suspend();
    if(ImGui::BeginPopup("Create New Node")){
        for(auto& create_info : m_CreateInfos){
            auto&       node_info = create_info["node"];
            auto&       pins_info = create_info["pins"];
            std::string node_type = node_info["type"].get<json::string>();
            if(ImGui::MenuItem(node_type.c_str())){
                auto& node = CreateNode();
                node.Type  = node_type;
                for(auto& pin_info : pins_info.get<json::array>()){
                    auto& pin=CreatePin(GetNodePinTypeArr(node,pin_info["type"].get<json::string>()));
                    pin.Name      = pin_info["name"].get<json::string>();
                    pin.Kind      = GetPinKind(pin_info["kind"].get<json::string>());
                    pin.IsVariant = pin_info["is_variant"].get<json::boolean>();
                    pin.State     = PinState::Free;
                    
                }
            }
        }
        ImGui::EndPopup();
    }
    ed::Resume();
    if(a > 1){
        ed::BeginNode(1000);
        ImGui::TextUnformatted("hello");
        ed::EndNode();
    }

    // Start drawing nodes.
    for(auto& node : m_Nodes){ DrawNode(node); }

    for(auto& linkInfo : m_Links) ed::Link(linkInfo.LinkId, linkInfo.OutPin, linkInfo.InPin);


    /// Handle creation action ---------------------------------------------------------------------------
    if(ed::BeginCreate()){
        ed::PinId inputPinId, outputPinId;
        if(ed::QueryNewLink(&outputPinId, &inputPinId)){
            if(inputPinId && outputPinId && inputPinId != outputPinId){
                if(ed::AcceptNewItem()){
                    auto& link = CreateLink();
                    if(IsOutPin(outputPinId) == 1){
                        link.InPin  = inputPinId;
                        link.OutPin = outputPinId;
                    }
                    else if(IsOutPin(outputPinId) == 0){
                        link.InPin  = outputPinId;
                        link.OutPin = inputPinId;
                    }
                }
            }
        }
    }
    ed::EndCreate();

    // Handle deletion action ---------------------------------------------------------------------------
    if(ed::BeginDelete()){
        ed::LinkId deletedLinkId;
        while(ed::QueryDeletedLink(&deletedLinkId)){ if(ed::AcceptDeletedItem()){ RemoveLink(deletedLinkId); } }

        ed::NodeId deleted_node_id;
        while(ed::QueryDeletedNode(&deleted_node_id)){
            if(ed::AcceptDeletedItem()){
                RemoveNode(deleted_node_id);
                ed::DeleteNode(deleted_node_id);
            }
        }
    }
    ed::EndDelete();

    ed::End();
    ed::SetCurrentEditor(nullptr);
}
