#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <variant>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "imgui_node_editor.h"

#include "../common/VkAtlasData.h"

namespace ed = ax::NodeEditor;

namespace VKA::UI {

    class nodeEditor {
    public:
        inline nodeEditor();
        inline ~nodeEditor();

        inline void draw(const VKA::DATA::ASTTree& tree, const char* title = "AST Node Editor");
        inline void clearLayout();

    private:
        ax::NodeEditor::EditorContext* m_ctx = nullptr;
        bool m_needsLayout = true;
        size_t m_lastNodeCount = 0;

        inline ImVec2 gridPosForIndex(size_t index, float xStep = 260.0f, float yStep = 160.0f, int cols = 5) const;
        inline void DrawUEPinIcon(bool isConnected, ImU32 color);
    };

        namespace detail {
        inline uint64_t packNodeId(uint32_t treeId, uint32_t nodeId) {
            return (static_cast<uint64_t>(treeId) << 32) | static_cast<uint64_t>(nodeId);
        }
        inline const char* ToString(VKA::DATA::ASTNodeType t) {
            using T = VKA::DATA::ASTNodeType;
            switch (t) {
            case T::Block:        return "Block";
            case T::VariableDecl: return "VariableDecl";
            case T::FunctionCall: return "FunctionCall";
            case T::Expression:   return "Expression";
            default:              return "Unknown";
            }
        }
        // Unreal Engine Tür Renkleri (Header için)
        inline ImU32 GetNodeHeaderColor(VKA::DATA::ASTNodeType t) {
            using T = VKA::DATA::ASTNodeType;
            switch (t) {
            case T::Block:        return IM_COL32(180, 50, 50, 255);   // Event / Flow Kırmızı
            case T::VariableDecl: return IM_COL32(0, 160, 150, 255);   // Variable Cyan/Turkuaz
            case T::FunctionCall: return IM_COL32(25, 120, 230, 255);  // Function Mavi
            case T::Expression:   return IM_COL32(80, 180, 70, 255);   // Pure Expression Yeşil
            default:              return IM_COL32(120, 120, 120, 255);
            }
        }
                // Node'a bağlı stabil pin id üretici (node anahtarına göre port ve yön)
        inline ed::PinId MakePinId(uint64_t nodePacked, uint16_t port, bool isInput) {
            const uint64_t key = (nodePacked << 8) | (static_cast<uint64_t>(port) << 1) | (isInput ? 1ull : 0ull);
            return ed::PinId(static_cast<uintptr_t>(key));
        }
    }



    inline nodeEditor::nodeEditor() {
        if (!m_ctx) m_ctx = ed::CreateEditor();
    }

    inline nodeEditor::~nodeEditor() {
        if (m_ctx) { ed::DestroyEditor(m_ctx); m_ctx = nullptr; }
    }

    inline ImVec2 nodeEditor::gridPosForIndex(size_t index, float xStep, float yStep, int cols) const {
        const int col = static_cast<int>(index % cols);
        const int row = static_cast<int>(index / cols);
        return ImVec2(col * xStep, row * yStep);
    }

    // Minik UE stili Daire Pin İkonu
    inline void nodeEditor::DrawUEPinIcon(bool isConnected, ImU32 color) {
        const float size = 10.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 center = pos + ImVec2(size * 0.5f, size * 0.5f);

        if (isConnected)
            drawList->AddCircleFilled(center, size * 0.4f, color);
        else
            drawList->AddCircle(center, size * 0.4f, color, 12, 2.0f);

        ImGui::Dummy(ImVec2(size + 4.0f, size));
    }

    inline void nodeEditor::clearLayout() { m_needsLayout = true; }

        inline void nodeEditor::draw(const VKA::DATA::ASTTree& tree, const char* title) {
        if (!m_ctx) m_ctx = ed::CreateEditor();

        ed::SetCurrentEditor(m_ctx);
        ed::Begin("ASTNodeEditor");

        std::vector<ed::NodeId> createdNodeIds;
        createdNodeIds.reserve(tree.astnodes.size());

        for (const auto& n : tree.astnodes) {
            const uint64_t packedId = detail::packNodeId(tree.treeid, n.id);
            const ed::NodeId nodeId(static_cast<int>(packedId));
            createdNodeIds.push_back(nodeId);

            ImGui::PushID(static_cast<int>(packedId));

            // Header renkleri (UE tarzı)
            const ImU32 headerCol = detail::GetNodeHeaderColor(n.type);
            const ImU32 headerColHover = IM_COL32(((headerCol>>0)&0xFF) + 15 > 255 ? 255 : ((headerCol>>0)&0xFF) + 15,
                                                  ((headerCol>>8)&0xFF) + 15 > 255 ? 255 : ((headerCol>>8)&0xFF) + 15,
                                                  ((headerCol>>16)&0xFF) + 15 > 255 ? 255 : ((headerCol>>16)&0xFF) + 15, 255);
            const ImU32 headerColSelect = IM_COL32(((headerCol>>0)&0xFF) + 30 > 255 ? 255 : ((headerCol>>0)&0xFF) + 30,
                                                   ((headerCol>>8)&0xFF) + 30 > 255 ? 255 : ((headerCol>>8)&0xFF) + 30,
                                                   ((headerCol>>16)&0xFF) + 30 > 255 ? 255 : ((headerCol>>16)&0xFF) + 30, 255);

                        ed::BeginNode(nodeId);

            // Başlık (UE tarzı renkli metin)
            ImGui::TextColored(ImColor(headerCol), "%s", detail::ToString(n.type));
            ImGui::SameLine();
            ImGui::TextDisabled("#%u", n.id);
            ImGui::Separator();

            // Sütunlar yok, her şeyi tertemiz alt alta diziyoruz
            // Sol/Giriş Pinleri
            if (n.type == VKA::DATA::ASTNodeType::Block) {
                ed::BeginPin(detail::MakePinId(packedId, 100, true), ed::PinKind::Input);
                DrawUEPinIcon(false, headerCol);
                ImGui::SameLine(); ImGui::TextUnformatted("Exec In");
                ed::EndPin();
            } else if (n.type == VKA::DATA::ASTNodeType::FunctionCall) {
                if (const auto* f = std::get_if<VKA::DATA::FunctionCallNodeData>(&n.data)) {
                    for (size_t ai = 0; ai < f->arguments_indices.size(); ++ai) {
                        ed::BeginPin(detail::MakePinId(packedId, static_cast<uint16_t>(ai), true), ed::PinKind::Input);
                        DrawUEPinIcon(false, headerCol);
                        ImGui::SameLine(); ImGui::Text("arg%zu", ai);
                        ed::EndPin();
                    }
                }
            }

            // Düğümün ana içeriği (metinler)
            if (auto* b = std::get_if<VKA::DATA::BlockNodeData>(&n.data)) {
                ImGui::Text("Statements: %zu", b->statements_indices.size());
            } else if (auto* v = std::get_if<VKA::DATA::VariableDeclNodeData>(&n.data)) {
                ImGui::Text("%s %s", v->type.c_str(), v->name.c_str());
            } else if (auto* f = std::get_if<VKA::DATA::FunctionCallNodeData>(&n.data)) {
                ImGui::Text("%s(...)", f->function_name.c_str());
            } else if (auto* e = std::get_if<VKA::DATA::ExpressionNodeData>(&n.data)) {
                ImGui::TextWrapped("%s%s", e->is_address_of ? "&" : "", e->text.c_str());
            }

            // Sağ/Çıkış Pinleri
            if (n.type == VKA::DATA::ASTNodeType::Block) {
                ed::BeginPin(detail::MakePinId(packedId, 101, false), ed::PinKind::Output);
                ImGui::TextUnformatted("Exec Out");
                ImGui::SameLine(); DrawUEPinIcon(false, headerCol);
                ed::EndPin();
            } else if (n.type == VKA::DATA::ASTNodeType::VariableDecl) {
                if (const auto* v = std::get_if<VKA::DATA::VariableDeclNodeData>(&n.data)) {
                    ed::BeginPin(detail::MakePinId(packedId, 0, false), ed::PinKind::Output);
                    ImGui::Text("%s", v->name.c_str());
                    ImGui::SameLine(); DrawUEPinIcon(false, headerCol);
                    ed::EndPin();
                }
            } else if (n.type == VKA::DATA::ASTNodeType::FunctionCall) {
                ed::BeginPin(detail::MakePinId(packedId, 0, false), ed::PinKind::Output);
                ImGui::TextUnformatted("ret");
                ImGui::SameLine(); DrawUEPinIcon(false, headerCol);
                ed::EndPin();
            } else if (n.type == VKA::DATA::ASTNodeType::Expression) {
                ed::BeginPin(detail::MakePinId(packedId, 0, false), ed::PinKind::Output);
                ImGui::TextUnformatted("value");
                ImGui::SameLine(); DrawUEPinIcon(false, headerCol);
                ed::EndPin();
            }

            // Alt bilgi
            ImGui::Separator();
            ImGui::TextDisabled("line: %u", n.line);

            ed::EndNode();

            ImGui::PopID();

        }

        ed::End();

        if (m_needsLayout || m_lastNodeCount != tree.astnodes.size()) {
            for (size_t i = 0; i < createdNodeIds.size(); ++i) {
                ed::SetNodePosition(createdNodeIds[i], gridPosForIndex(i));
            }
            m_lastNodeCount = tree.astnodes.size();
            m_needsLayout = false;
        }

        ed::SetCurrentEditor(nullptr);
    }


} // namespace VKA::UI