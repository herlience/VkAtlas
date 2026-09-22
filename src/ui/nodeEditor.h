#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <unordered_set>

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

        inline void draw(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, const char* title = "AST Node Editor");
        inline void clearLayout();

    private:
        ax::NodeEditor::EditorContext* m_ctx = nullptr;
        bool m_needsLayout = true;
        size_t m_lastNodeCount = 0;

        inline void DrawPinIcon(bool isConnected, ImU32 color);

        inline void calculateNodeLayout(uint32_t nodeid, int depth, float& currentY, const VKA::DATA::ASTTree& tree, std::unordered_set<uint32_t>& visited) const;
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
    }

    inline nodeEditor::nodeEditor() {
        if (!m_ctx) m_ctx = ed::CreateEditor();
    }

    inline nodeEditor::~nodeEditor() {
        if (m_ctx) { ed::DestroyEditor(m_ctx); m_ctx = nullptr; }
    }

    inline void nodeEditor::DrawPinIcon(bool isConnected, ImU32 color) {
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

    inline void nodeEditor::calculateNodeLayout(uint32_t nodeid, int depth, float& currentY, const VKA::DATA::ASTTree& tree, std::unordered_set<uint32_t>& visited) const {

        if (visited.count(nodeid)) return;
        visited.insert(nodeid);

        const VKA::DATA::ASTNode* nodePtr = nullptr;
        for (const auto& n : tree.astnodes) {
            if (n.id == nodeid) { nodePtr = &n; break; }
        }
        if (!nodePtr) return;

        uint64_t packedId = detail::packNodeId(tree.treeid, nodePtr->id);
        ed::SetNodePosition(ed::NodeId(packedId), ImVec2(depth * 350.0f, currentY));

        currentY += 160.0f; 

        if (auto* f = std::get_if<VKA::DATA::FunctionCallNodeData>(&nodePtr->data)) {
            for (auto argId : f->arguments_indices)
                calculateNodeLayout(argId, depth + 1, currentY, tree, visited);
        }
        else if (auto* v = std::get_if<VKA::DATA::VariableDeclNodeData>(&nodePtr->data)) {
            if (v->init_expression != UINT32_MAX)
                calculateNodeLayout(v->init_expression, depth + 1, currentY, tree, visited);
        }
        else if (auto* b = std::get_if<VKA::DATA::BlockNodeData>(&nodePtr->data)) {
            for (auto stmtId : b->statements_indices)
                calculateNodeLayout(stmtId, depth, currentY, tree, visited); 
        }
    }

    inline void nodeEditor::draw(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, const char* title) {
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

            const ImU32 headerCol = detail::GetNodeHeaderColor(n.type);

            ed::BeginNode(nodeId);

            ImGui::TextColored(ImColor(headerCol), "%s", detail::ToString(n.type));
            ImGui::SameLine();
            ImGui::TextDisabled("#%u", n.id);
            ImGui::Separator();

            if (n.input_pin_id != UINT32_MAX) {
                ed::BeginPin(ed::PinId(n.input_pin_id), ed::PinKind::Input);
                DrawPinIcon(false, headerCol);
                ImGui::SameLine();
                ImGui::TextUnformatted("In");
                ed::EndPin();
            }

            if (auto* b = std::get_if<VKA::DATA::BlockNodeData>(&n.data)) {
                ImGui::Text("Statements: %zu", b->statements_indices.size());
            }
            else if (auto* v = std::get_if<VKA::DATA::VariableDeclNodeData>(&n.data)) {
                ImGui::Text("%s %s", v->type.c_str(), v->name.c_str());
            }
            else if (auto* f = std::get_if<VKA::DATA::FunctionCallNodeData>(&n.data)) {
                ImGui::Text("%s(...)", f->function_name.c_str());
                // Argümanları içeriğe metin olarak dök (pin kalabalığı yapmasın, linkler yeterli)
                ImGui::TextDisabled("Args: %zu", f->arguments_indices.size());
            }
            else if (auto* e = std::get_if<VKA::DATA::ExpressionNodeData>(&n.data)) {
                ImGui::TextWrapped("%s%s", e->is_address_of ? "&" : "", e->text.c_str());
            }

            if (n.output_pin_id != UINT32_MAX) {
                ed::BeginPin(ed::PinId(n.output_pin_id), ed::PinKind::Output);
                ImGui::TextUnformatted("Out");
                ImGui::SameLine();
                DrawPinIcon(false, headerCol);
                ed::EndPin();
            }

            ImGui::Separator();
            ImGui::TextDisabled("line: %u", n.line);

            ed::EndNode();
            ImGui::PopID();
        }

        for (const auto& link : ctx.links) {
            ed::Link(ed::LinkId(link.id), ed::PinId(link.startnpinid), ed::PinId(link.endpinid));
        }

        ed::End();

        if (m_needsLayout || m_lastNodeCount != tree.astnodes.size()) {
            float startY = 0.0f;
            std::unordered_set<uint32_t> visited;

            for (const auto& n : tree.astnodes) {
                calculateNodeLayout(n.id, 0, startY, tree, visited);
            }

            m_lastNodeCount = tree.astnodes.size();
            m_needsLayout = false;
        }

        ed::SetCurrentEditor(nullptr);
    }

} // VKA::UI