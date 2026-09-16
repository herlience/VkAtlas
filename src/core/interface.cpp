#include "interface.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>

#include <fstream>

#include <imgui.h>

#include "../parser/VKACustomParser/VKALexer.hpp"
#include "../parser/VKACustomParser/VKAParser.hpp"
#include "../common/vkainfo.h"

// Node Editor header'ını projene dahil ediyoruz
#include "../ui/nodeEditor.h"

namespace VKA::CORE::INTERFACE {

    // Node Editor tuvalinin ve parse edilen ağacın her frame hayatta kalması için statik yapıyoruz
    static VKA::UI::nodeEditor s_astEditor;
    static VKA::DATA::ASTTree s_currentTree;
    static bool s_showNodeEditor = false;

    void renderUI(VKA::DATA::GraphContext& context) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize;

        ImGui::Begin("VkAtlas Converter", nullptr, window_flags);

        // EĞER ÇEVİRİ YAPILDIYSA VE AĞAÇ DOLUYSA: Node Editor Tuvalini Göster
        if (s_showNodeEditor && !s_currentTree.astnodes.empty()) {
            
            if (ImGui::Button("<- Back to Files", ImVec2(150, 30))) {
                s_showNodeEditor = false; // Dosya seçme ekranına geri dön
            }
            ImGui::Separator();
            
            // Düğüm tuvalini devasa ana penceremizin tam içine gömüyoruz
            s_astEditor.draw(s_currentTree);
        }
        // EĞER HENÜZ ÇEVİRİ YAPILMADIYSA: Dosya Seçme Ekranını Göster
        else {
            if (g_selectedfiles.empty()) {
                ImVec2 boxSize = ImVec2(400, 200);
                ImVec2 winSize = ImGui::GetContentRegionAvail();
                ImVec2 cursorPadding = ImVec2((winSize.x - boxSize.x) * 0.5f, (winSize.y - boxSize.y) * 0.5f);

                ImGui::SetCursorPos(cursorPadding);

                if (ImGui::Button("Drag files here\n\nor click to select file", boxSize)) {
                    openfileDialog(); 
                }
            }
            else {
                ImGui::Text("Selected files (%d):", (int)g_selectedfiles.size());
                ImGui::Separator();

                ImGui::BeginChild("FileList", ImVec2(0, -50), true);

                for (size_t i = 0; i < g_selectedfiles.size(); i++) {
                    const auto& file = g_selectedfiles[i];

                    std::string fileName = file.filename().string();
                    std::string fileExt = file.extension().string();

                    ImGui::BulletText("[%s]  %s", fileExt.c_str(), fileName.c_str());

                    ImGui::SameLine();
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::SmallButton("Delete")) {
                        g_selectedfiles.erase(g_selectedfiles.begin() + i);
                    }
                    ImGui::PopID();
                }

                ImGui::EndChild();

                if (ImGui::Button("Add more files")) {
                    openfileDialog();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear the list")) {
                    g_selectedfiles.clear();
                }

                ImGui::Separator();
                
                ImGui::SetNextItemWidth(-1); 

                // CONVERT BUTONU
                if (ImGui::Button("CONVERT", ImVec2(-1, 40))) {
                    VKA::PARSER::LEXER::VKALexer lexer;
                    VKA::DATA::DataFromParser globalsource;
                    
                    // Önceki çeviriden kalan eski ağacı temizle
                    s_currentTree.astnodes.clear(); 

                    for (auto& path : g_selectedfiles) {
                        std::string sourcecode = readFileToString(path);
                        std::string pathstr = path.string();

                        std::vector<VKA::DATA::Token> tokensfromcode = lexer.tokenize(sourcecode, pathstr, context);
                        
                        // Veriyi yerel değişken yerine statik s_currentTree içine yazıyoruz
                        VKA::PARSER::CUSTOM::VKAParser parser{ std::move(tokensfromcode), globalsource };
                        parser.parse(s_currentTree);
                    }

                    // Node editor'ü görünür yap ve ızgara (layout) düzenini sıfırla
                    s_showNodeEditor = true;
                    s_astEditor.clearLayout();

                    for (const auto& node : s_currentTree.astnodes) {
                        std::string tokentype = astNodeTypeToString(node.type);
                        VKA_DEBUG_MSG(tokentype);
                    }
                }
            }
        }

        ImGui::End();
    }

    std::string astNodeTypeToString(VKA::DATA::ASTNodeType type) {
        switch (type) {
        case VKA::DATA::ASTNodeType::Block:        return "Block";
        case VKA::DATA::ASTNodeType::VariableDecl: return "VariableDecl";
        case VKA::DATA::ASTNodeType::FunctionCall: return "FunctionCall";
        case VKA::DATA::ASTNodeType::Expression:   return "Expression";
        default:                                   return "Unknown";
        }
    }

    void drop_callback(GLFWwindow* window, int count, const char** paths) {
        for (int i = 0; i < count; i++) {
            g_selectedfiles.push_back(paths[i]);
        }
    }

    void openfileDialog() {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "All Files\0*.*\0Vulkan XML / C++\0*.xml;*.h;*.hpp;*.cpp\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            g_selectedfiles.push_back(szFile);
        }
    }

    std::string readFileToString(const std::filesystem::path& filePath) {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
            return "";
        }

        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        const auto fileSize = file.tellg();
        if (fileSize <= 0) {
            return "";
        }

        std::string buffer;
        buffer.resize(static_cast<size_t>(fileSize));

        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), fileSize);

        return buffer;
    }
}