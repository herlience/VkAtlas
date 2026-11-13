#pragma once

#include <cstdint>
#include <cctype>
#include <vector>
#include <string>
#include <variant>
#include <unordered_set>
#include <unordered_map>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "imgui_node_editor.h"

#include "../common/VkAtlasData.h"

namespace ed = ax::NodeEditor;

namespace VKA::UI {

    enum class PinCategory : uint8_t {
        Exec,       // white execution arrow
        Boolean,
        Integer,
        Float,
        String,
        Enum,       // Vulkan enums / flags 
        Struct,
        Handle,     // Vulkan handles 
        Delegate,   // PFN_* function pointers
        Array,      // a list of children (function arguments)
        Wildcard    // unknown / untyped
    };

    class nodeEditor {
    public:
        inline nodeEditor();
        inline ~nodeEditor();

        inline void draw(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, const char* title = "AST Node Editor");
        inline void clearLayout();

    private:
        struct NodeVisual {
            VKA::DATA::Pin inPin;                    // from ASTNode::input_pin_id  (+ name from GraphContext::pin_map)
            VKA::DATA::Pin outPin;                   // from ASTNode::output_pin_id (+ name from GraphContext::pin_map)
            PinCategory    inCategory   = PinCategory::Exec;
            PinCategory    outCategory  = PinCategory::Exec;
            bool           inConnected  = false;
            bool           outConnected = false;
            bool           inByRef      = false;     
            bool           compact      = false;     // header-less "getter" style node (expressions)
            bool           literal      = false;     // value drawn inside an inline literal box
            ImU32          headerColor  = 0;
            std::string    title;
            std::string    subtitle;
            std::string    value;                    // compact nodes: displayed text (already fitted)
            std::string    paramInfo;                // matching Vulkan parameter (tooltip)
        };

        ax::NodeEditor::EditorContext* m_ctx = nullptr;
        bool m_needsLayout = true;
        size_t m_lastNodeCount = 0;

        bool m_styleApplied = false;
        bool m_visualsDirty = true;
        size_t m_visualLinkCount = 0;
        size_t m_visualPinMapCount = 0;
        uint32_t m_visualTreeId = 0;
        std::vector<NodeVisual> m_visuals;                        // same order as ASTTree::astnodes
        std::unordered_map<uint32_t, size_t> m_nodeIndex;         // ASTNode::id -> index in ASTTree::astnodes
        std::unordered_map<uint32_t, PinCategory> m_pinCategory;  // pin id -> category (wire colours)

        ed::NodeId m_tooltipNode;
        float m_tooltipTimer = 0.0f;

        inline void applyBlueprintStyle();
        inline void rebuildVisuals(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx);
        inline void drawNode(const VKA::DATA::ASTNode& node, const NodeVisual& visual, ed::NodeId nodeId, ed::NodeId hoveredNode, ed::PinId hoveredPin) const;
        inline void drawNodeTooltip(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, ed::NodeId hoveredNode);

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

        namespace bp {
            constexpr ImU32 GraphBg        = IM_COL32( 38,  38,  38, 255);
            constexpr ImU32 GraphGrid      = IM_COL32(  0,   0,   0,  64);
            constexpr ImU32 BodyTop        = IM_COL32( 26,  26,  28, 240);
            constexpr ImU32 BodyBottom     = IM_COL32( 14,  14,  16, 240);
            constexpr ImU32 CompactTop     = IM_COL32( 54,  54,  58, 245);
            constexpr ImU32 CompactBottom  = IM_COL32( 30,  30,  33, 245);
            constexpr ImU32 NodeBorder     = IM_COL32(  0,   0,   0, 235);
            constexpr ImU32 Selection      = IM_COL32(255, 166,   0, 255);
            constexpr ImU32 Hover          = IM_COL32(255, 255, 255, 100);
            constexpr ImU32 Title          = IM_COL32(255, 255, 255, 255);
            constexpr ImU32 Subtitle       = IM_COL32(205, 205, 205, 175);
            constexpr ImU32 PinLabel       = IM_COL32(220, 220, 220, 255);
            constexpr ImU32 ParamLabel     = IM_COL32(165, 165, 165, 255);
            constexpr ImU32 ValueBoxBg     = IM_COL32(  8,   8,  10, 220);
            constexpr ImU32 ValueBoxBorder = IM_COL32(255, 255, 255,  34);
            constexpr ImU32 ValueText      = IM_COL32(236, 236, 236, 255);
            constexpr ImU32 WarningBanner  = IM_COL32(232, 170,  24, 255);
            constexpr ImU32 ErrorBanner    = IM_COL32(196,  28,  28, 255);
        }

        inline ImU32 GetNodeHeaderColor(VKA::DATA::ASTNodeType t) {
            using T = VKA::DATA::ASTNodeType;
            switch (t) {
            case T::Block:        return IM_COL32(158,  24,  24, 255);  // Event    - red
            case T::VariableDecl: return IM_COL32(  0, 112, 106, 255);  // Variable - teal (real colour follows the variable type)
            case T::FunctionCall: return IM_COL32( 26,  88, 164, 255);  // Function - blue
            case T::Expression:   return IM_COL32( 64, 132,  50, 255);  // Pure     - green
            default:              return IM_COL32(100, 100, 100, 255);
            }
        }

        inline ImU32 GetPinColor(PinCategory c) {
            switch (c) {
            case PinCategory::Exec:     return IM_COL32(255, 255, 255, 255);
            case PinCategory::Boolean:  return IM_COL32(170,  20,  20, 255);
            case PinCategory::Integer:  return IM_COL32( 31, 227, 175, 255);
            case PinCategory::Float:    return IM_COL32(160, 250,  68, 255);
            case PinCategory::String:   return IM_COL32(250,   0, 208, 255);
            case PinCategory::Enum:     return IM_COL32(  0, 128, 114, 255);
            case PinCategory::Struct:   return IM_COL32(  0,  96, 214, 255);
            case PinCategory::Handle:   return IM_COL32(  0, 168, 242, 255);
            case PinCategory::Delegate: return IM_COL32(255,  56,  56, 255);
            case PinCategory::Array:    return IM_COL32(200, 200, 200, 255);
            default:                    return IM_COL32(140, 140, 140, 255);
            }
        }

        inline ImU32 ScaleColor(ImU32 c, float f) {
            const ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
            return ImGui::ColorConvertFloat4ToU32(ImVec4(v.x * f, v.y * f, v.z * f, v.w));
        }

        inline ImU32 MixColor(ImU32 a, ImU32 b, float t) {
            return ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a), ImGui::ColorConvertU32ToFloat4(b), t));
        }

        inline ImU32 WithAlpha(ImU32 c, int alpha) {
            return (c & ~IM_COL32_A_MASK) | (static_cast<ImU32>(ImClamp(alpha, 0, 255)) << IM_COL32_A_SHIFT);
        }

        inline ed::PinKind ToEditorPinKind(VKA::DATA::PinKind kind) {
            return kind == VKA::DATA::PinKind::Output ? ed::PinKind::Output : ed::PinKind::Input;
        }

        // ---- type -> pin category (visual heuristics only) -------------------------------------

        inline bool EndsWith(const std::string& s, const char* suffix) {
            const size_t n = std::char_traits<char>::length(suffix);
            return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
        }

        // "const VkInstanceCreateInfo*" -> "VkInstanceCreateInfo", "unsigned int" -> "int"
        inline std::string BaseTypeName(const std::string& type) {
            auto isIdent = [](char ch) {
                const unsigned char u = static_cast<unsigned char>(ch);
                return std::isalnum(u) || u == '_' || u == ':';
            };
            std::string last;
            size_t i = 0;
            while (i < type.size()) {
                if (!isIdent(type[i])) { ++i; continue; }
                size_t j = i;
                while (j < type.size() && isIdent(type[j])) ++j;
                std::string token = type.substr(i, j - i);
                if (token != "const" && token != "volatile" && token != "struct" && token != "enum" && token != "class" && token != "signed")
                    last = std::move(token);
                i = j;
            }
            return last;
        }

        // "VkPipelineStageFlags2KHR" -> "VkPipelineStageFlags", "VkPresentModeKHR" -> "VkPresentMode"
        inline std::string StripVendorSuffix(std::string s) {
            size_t end = s.size();
            size_t upper = 0;
            while (end > 0 && std::isupper(static_cast<unsigned char>(s[end - 1]))) { --end; ++upper; }
            if (upper < 2) end = s.size();
            while (end > 0 && std::isdigit(static_cast<unsigned char>(s[end - 1]))) --end;
            s.resize(end);
            return s;
        }

        inline bool IsVulkanHandle(const std::string& base) {
            static const std::unordered_set<std::string> handles = {
                "VkInstance", "VkPhysicalDevice", "VkDevice", "VkQueue", "VkSemaphore", "VkCommandBuffer", "VkFence",
                "VkDeviceMemory", "VkBuffer", "VkImage", "VkEvent", "VkQueryPool", "VkBufferView", "VkImageView",
                "VkShaderModule", "VkPipelineCache", "VkPipelineLayout", "VkRenderPass", "VkPipeline",
                "VkDescriptorSetLayout", "VkSampler", "VkDescriptorPool", "VkDescriptorSet", "VkFramebuffer",
                "VkCommandPool", "VkSamplerYcbcrConversion", "VkDescriptorUpdateTemplate", "VkPrivateDataSlot",
                "VkSurfaceKHR", "VkSwapchainKHR", "VkDisplayKHR", "VkDisplayModeKHR", "VkDebugReportCallbackEXT",
                "VkDebugUtilsMessengerEXT", "VkValidationCacheEXT", "VkAccelerationStructureKHR",
                "VkAccelerationStructureNV", "VkDeferredOperationKHR", "VkIndirectCommandsLayoutNV",
                "VkIndirectCommandsLayoutEXT", "VkIndirectExecutionSetEXT", "VkMicromapEXT", "VkShaderEXT",
                "VkVideoSessionKHR", "VkVideoSessionParametersKHR", "VkPipelineBinaryKHR", "VkOpticalFlowSessionNV",
                "VkCuModuleNVX", "VkCuFunctionNVX", "VkCudaModuleNV", "VkCudaFunctionNV",
                "VkPerformanceConfigurationINTEL", "VkBufferCollectionFUCHSIA"
            };
            return handles.count(base) > 0;
        }

        inline PinCategory ClassifyType(const std::string& type, bool isPtr, const VKA::DATA::GraphContext& ctx) {
            const std::string base = BaseTypeName(type);
            if (base.empty()) return PinCategory::Wildcard;
            isPtr = isPtr || type.find('*') != std::string::npos;

            static const std::unordered_set<std::string> boolTypes   = { "bool", "VkBool32" };
            static const std::unordered_set<std::string> floatTypes  = { "float", "double" };
            static const std::unordered_set<std::string> stringTypes = { "string", "std::string", "string_view", "std::string_view" };
            static const std::unordered_set<std::string> intTypes    = {
                "int", "unsigned", "short", "long", "size_t", "int8_t", "int16_t", "int32_t", "int64_t",
                "uint8_t", "uint16_t", "uint32_t", "uint64_t", "VkDeviceSize", "VkDeviceAddress", "VkSampleMask"
            };

            if (boolTypes.count(base))      return PinCategory::Boolean;
            if (floatTypes.count(base))     return PinCategory::Float;
            if (base == "char")             return isPtr ? PinCategory::String : PinCategory::Integer;
            if (stringTypes.count(base))    return PinCategory::String;
            if (intTypes.count(base))       return PinCategory::Integer;
            if (base.rfind("PFN_", 0) == 0) return PinCategory::Delegate;
            if (IsVulkanHandle(base))       return PinCategory::Handle;
            if (ctx.enumspecs.count(base))  return PinCategory::Enum;

            const std::string core = StripVendorSuffix(base);
            if (EndsWith(core, "Flags") || EndsWith(core, "FlagBits")) return PinCategory::Enum;
            if (base == "void")             return PinCategory::Wildcard;
            if (ctx.typespecs.count(base))  return PinCategory::Struct;   // handles and enums are filtered above

            if (base.size() > 2 && base[0] == 'V' && base[1] == 'k') {   // fallback when vk.xml is not loaded
                static const char* enumSuffixes[] = { "Type", "Mode", "Op", "Topology", "Tiling", "Factor", "Filter", "Level", "Result" };
                if (base == "VkFormat") return PinCategory::Enum;
                for (const char* suffix : enumSuffixes)
                    if (EndsWith(core, suffix)) return PinCategory::Enum;
                return PinCategory::Struct;
            }
            return PinCategory::Wildcard;
        }

        inline bool IsNumberLiteral(const std::string& s, bool& isFloat) {
            isFloat = false;
            const size_t i = (!s.empty() && (s[0] == '-' || s[0] == '+')) ? 1 : 0;
            if (i >= s.size()) return false;
            const bool leadingDot = s[i] == '.' && i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1]));
            if (!std::isdigit(static_cast<unsigned char>(s[i])) && !leadingDot) return false;
            const bool hex = s.size() > i + 1 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X');
            if (!hex)
                isFloat = s.find_first_of(".eE") != std::string::npos || EndsWith(s, "f") || EndsWith(s, "F");
            return true;
        }

        inline PinCategory ClassifyExpression(const VKA::DATA::ExpressionNodeData& e, const VKA::DATA::GraphContext& ctx, bool& isLiteral) {
            const std::string& t = e.text;
            bool isFloat = false;
            isLiteral = true;
            if (t.empty())                                                       { isLiteral = false; return PinCategory::Wildcard; }
            if (t.front() == '"')                                                return PinCategory::String;
            if (t.front() == '\'')                                               return PinCategory::Integer;
            if (t == "true" || t == "false" || t == "VK_TRUE" || t == "VK_FALSE") return PinCategory::Boolean;
            if (t == "nullptr" || t == "NULL" || t == "VK_NULL_HANDLE")          return PinCategory::Handle;
            if (t == "UINT32_MAX" || t == "UINT64_MAX" || t == "SIZE_MAX")       return PinCategory::Integer;
            if (IsNumberLiteral(t, isFloat))                                     return isFloat ? PinCategory::Float : PinCategory::Integer;
            if (ctx.enumspecs.count(t) || t.rfind("VK_", 0) == 0)                return PinCategory::Enum;
            isLiteral = false;
            return PinCategory::Wildcard;
        }

        inline void ShadeVerts(ImDrawList* dl, int vtxStart, const ImVec2& p0, const ImVec2& p1, ImU32 c0, ImU32 c1) {
            const ImVec2 axis     = p1 - p0;
            const float  lenSq    = ImLengthSqr(axis);
            const float  invLenSq = lenSq > 0.0f ? 1.0f / lenSq : 0.0f;
            const ImVec4 a        = ImGui::ColorConvertU32ToFloat4(c0);
            const ImVec4 b        = ImGui::ColorConvertU32ToFloat4(c1);
            for (int i = vtxStart; i < dl->VtxBuffer.Size; ++i) {
                ImDrawVert& vert = dl->VtxBuffer[i];
                ImVec4 c = ImLerp(a, b, ImSaturate(ImDot(vert.pos - p0, axis) * invLenSq));
                c.w *= static_cast<float>((vert.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
                vert.col = ImGui::ColorConvertFloat4ToU32(c);
            }
        }

        inline void FillGradient(ImDrawList* dl, const ImVec2& a, const ImVec2& b, float rounding, ImDrawFlags flags, ImU32 c0, ImU32 c1, bool horizontal) {
            const int vtxStart = dl->VtxBuffer.Size;
            dl->AddRectFilled(a, b, IM_COL32_WHITE, rounding, flags);
            ShadeVerts(dl, vtxStart, a, horizontal ? ImVec2(b.x, a.y) : ImVec2(a.x, b.y), c0, c1);
        }

        inline void DrawShadow(ImDrawList* dl, const ImVec2& a, const ImVec2& b, float rounding) {
            const ImVec2 offset(0.0f, 3.0f);
            for (int i = 0; i < 4; ++i) {
                const float grow = 1.0f + 1.75f * static_cast<float>(i);
                dl->AddRectFilled(a - ImVec2(grow, grow) + offset, b + ImVec2(grow, grow) + offset,
                                  IM_COL32(0, 0, 0, 36 - i * 8), rounding + grow);
            }
        }

        inline void AddTextShadowed(ImDrawList* dl, const ImVec2& pos, ImU32 col, const char* text) {
            dl->AddText(pos + ImVec2(1.0f, 1.0f), IM_COL32(0, 0, 0, 160), text);
            dl->AddText(pos, col, text);
        }

        inline std::string FitText(const std::string& text, float maxWidth) {
            if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;
            const float dotsWidth = ImGui::CalcTextSize("...").x;
            std::string out = text;
            while (!out.empty() && ImGui::CalcTextSize(out.c_str()).x + dotsWidth > maxWidth) {
                while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0) == 0x80) out.pop_back(); // keep UTF-8 valid
                if (!out.empty()) out.pop_back();
            }
            return out + "...";
        }

        inline ImVec2 DrawPinIcon(ImDrawList* dl, const ImVec2& center, float size, PinCategory category, bool connected, bool hovered, bool byRef) {
            ImU32 col = GetPinColor(category);
            if (hovered) col = MixColor(col, IM_COL32_WHITE, 0.35f);
            const float thickness = ImMax(1.25f, size * 0.11f);

            if (category == PinCategory::Exec) {   // execution pin: pentagon arrow
                const float w = size * 0.62f, h = size * 0.72f;
                const float x0 = center.x - w * 0.5f, x1 = center.x + w * 0.5f;
                const float y0 = center.y - h * 0.5f, y1 = center.y + h * 0.5f;
                const ImVec2 pts[5] = { ImVec2(x0, y0), ImVec2(x0 + w * 0.5f, y0), ImVec2(x1, center.y), ImVec2(x0 + w * 0.5f, y1), ImVec2(x0, y1) };
                if (connected) dl->AddConvexPolyFilled(pts, 5, col);
                else           dl->AddPolyline(pts, 5, col, ImDrawFlags_Closed, thickness);
                return ImVec2(x0, x1);
            }

            if (category == PinCategory::Array) {  // array pin: 3x3 grid
                const float cell  = ImFloor(size * 0.2f);
                const float gap   = ImMax(1.0f, ImFloor(size * 0.07f));
                const float total = cell * 3.0f + gap * 2.0f;
                const ImVec2 origin(ImFloor(center.x - total * 0.5f), ImFloor(center.y - total * 0.5f));
                const ImU32 cellCol = connected ? col : WithAlpha(col, 110);
                for (int y = 0; y < 3; ++y)
                    for (int x = 0; x < 3; ++x) {
                        const ImVec2 a = origin + ImVec2(x * (cell + gap), y * (cell + gap));
                        dl->AddRectFilled(a, a + ImVec2(cell, cell), cellCol);
                    }
                return ImVec2(origin.x, origin.x + total);
            }

            const float  r = ImFloor(size * 0.28f) + 0.5f;
            const ImVec2 c(center.x - size * 0.10f, center.y);
            float right = c.x + r;

            if (category == PinCategory::Delegate) {      // delegate pin: square
                if (connected) dl->AddRectFilled(c - ImVec2(r, r), c + ImVec2(r, r), col);
                else           dl->AddRect(c - ImVec2(r, r), c + ImVec2(r, r), col, 0.0f, 0, thickness);
            }
            else if (byRef) {                             // pass-by-reference pin: diamond
                const float d = r + 1.0f;
                const ImVec2 pts[4] = { ImVec2(c.x - d, c.y), ImVec2(c.x, c.y - d), ImVec2(c.x + d, c.y), ImVec2(c.x, c.y + d) };
                if (connected) dl->AddConvexPolyFilled(pts, 4, col);
                else           dl->AddPolyline(pts, 4, col, ImDrawFlags_Closed, thickness);
                right = c.x + d;
            }
            else {                                        // value pin: circle
                if (connected) dl->AddCircleFilled(c, r, col);
                else           dl->AddCircle(c, r - thickness * 0.5f, col, 0, thickness);
            }

            // small direction arrow next to the pin (UE4 style)
            const float ax = right + ImMax(1.5f, size * 0.10f);
            const float ah = ImMax(2.0f, size * 0.16f);
            dl->AddTriangleFilled(ImVec2(ax, c.y - ah), ImVec2(ax + ah, c.y), ImVec2(ax, c.y + ah), col);
            return ImVec2(c.x - r, ax + ah);
        }

        // Small white glyph in the node header (function "f", event diamond, variable pill).
        inline void DrawHeaderIcon(ImDrawList* dl, const ImVec2& c, float s, VKA::DATA::ASTNodeType type, ImU32 accent) {
            using T = VKA::DATA::ASTNodeType;
            const ImU32 white = IM_COL32(255, 255, 255, 235);
            const float t     = ImMax(1.5f, s * 0.11f);
            switch (type) {
            case T::FunctionCall: {
                dl->AddBezierCubic(c + ImVec2( 0.28f * s, -0.36f * s), c + ImVec2( 0.16f * s, -0.50f * s),
                                   c + ImVec2(-0.04f * s, -0.48f * s), c + ImVec2(-0.06f * s, -0.22f * s), white, t);
                dl->AddLine(c + ImVec2(-0.06f * s, -0.22f * s), c + ImVec2(-0.12f * s, 0.30f * s), white, t);
                dl->AddBezierCubic(c + ImVec2(-0.12f * s,  0.30f * s), c + ImVec2(-0.14f * s, 0.45f * s),
                                   c + ImVec2(-0.26f * s,  0.48f * s), c + ImVec2(-0.36f * s, 0.38f * s), white, t);
                dl->AddLine(c + ImVec2(-0.30f * s, -0.06f * s), c + ImVec2(0.18f * s, -0.06f * s), white, t);
                break;
            }
            case T::Block: {
                const float r = s * 0.42f, r2 = r * 0.45f;
                const ImVec2 outer[4] = { ImVec2(c.x, c.y - r),  ImVec2(c.x + r, c.y),  ImVec2(c.x, c.y + r),  ImVec2(c.x - r, c.y) };
                const ImVec2 inner[4] = { ImVec2(c.x, c.y - r2), ImVec2(c.x + r2, c.y), ImVec2(c.x, c.y + r2), ImVec2(c.x - r2, c.y) };
                dl->AddPolyline(outer, 4, white, ImDrawFlags_Closed, t);
                dl->AddConvexPolyFilled(inner, 4, white);
                break;
            }
            case T::VariableDecl: {
                const ImVec2 a(c.x - s * 0.44f, c.y - s * 0.22f), b(c.x + s * 0.44f, c.y + s * 0.22f);
                dl->AddRectFilled(a, b, accent, s * 0.22f);
                dl->AddRect(a, b, IM_COL32(255, 255, 255, 200), s * 0.22f, 0, 1.0f);
                break;
            }
            default:
                dl->AddCircleFilled(c, s * 0.3f, accent);
                break;
            }
        }
    }

    inline nodeEditor::nodeEditor() {
        if (!m_ctx) m_ctx = ed::CreateEditor();
    }

    inline nodeEditor::~nodeEditor() {
        if (m_ctx) { ed::DestroyEditor(m_ctx); m_ctx = nullptr; }
    }

    inline void nodeEditor::clearLayout() {
        m_needsLayout = true;
        m_visualsDirty = true;
    }

    inline void nodeEditor::applyBlueprintStyle() {
        ed::Style& style = ed::GetStyle();

        // Node frame, header, pins and selection are drawn by drawNode(), so the built-in chrome is switched off.
        style.NodePadding             = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.NodeRounding            = 6.0f;
        style.NodeBorderWidth         = 0.0f;
        style.HoveredNodeBorderWidth  = 0.0f;
        style.SelectedNodeBorderWidth = 0.0f;
        style.PinRounding             = 0.0f;
        style.PinBorderWidth          = 0.0f;
        style.PinRadius               = 0.0f;
        style.PinArrowSize            = 0.0f;
        style.PinArrowWidth           = 0.0f;
        style.SnapLinkToPinDir        = 0.0f;
        style.HighlightConnectedLinks = 0.0f;

        auto set = [&style](ed::StyleColor index, ImU32 color) { style.Colors[index] = ImGui::ColorConvertU32ToFloat4(color); };
        set(ed::StyleColor_Bg,                  detail::bp::GraphBg);
        set(ed::StyleColor_Grid,                detail::bp::GraphGrid);
        set(ed::StyleColor_NodeBg,              IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_NodeBorder,          IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_HovNodeBorder,       IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_SelNodeBorder,       IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_NodeSelRect,         IM_COL32(255, 255, 255, 16));
        set(ed::StyleColor_NodeSelRectBorder,   IM_COL32(255, 255, 255, 120));
        set(ed::StyleColor_HovLinkBorder,       IM_COL32(255, 255, 255, 110));
        set(ed::StyleColor_SelLinkBorder,       detail::bp::Selection);
        set(ed::StyleColor_HighlightLinkBorder, detail::bp::Selection);
        set(ed::StyleColor_LinkSelRect,         IM_COL32(255, 255, 255, 16));
        set(ed::StyleColor_LinkSelRectBorder,   IM_COL32(255, 255, 255, 120));
        set(ed::StyleColor_PinRect,             IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_PinRectBorder,       IM_COL32(0, 0, 0, 0));
        set(ed::StyleColor_Flow,                IM_COL32(255, 255, 255, 255));
        set(ed::StyleColor_FlowMarker,          IM_COL32(255, 255, 255, 255));
        set(ed::StyleColor_GroupBg,             IM_COL32(0, 0, 0, 110));
        set(ed::StyleColor_GroupBorder,         IM_COL32(255, 255, 255, 40));
    }

    inline void nodeEditor::rebuildVisuals(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx) {
        using namespace VKA::DATA;

        m_visuals.assign(tree.astnodes.size(), NodeVisual{});
        m_nodeIndex.clear();
        m_pinCategory.clear();
        for (size_t i = 0; i < tree.astnodes.size(); ++i)
            m_nodeIndex.emplace(tree.astnodes[i].id, i);

        // Pins that take part in a link are drawn "filled", like connected pins in Blueprint.
        std::unordered_set<uint32_t> connectedPins;
        connectedPins.reserve(ctx.links.size() * 2);
        for (const LinkArrow& link : ctx.links) {
            connectedPins.insert(link.startnpinid);
            connectedPins.insert(link.endpinid);
        }

        // child id -> parent (and argument slot), taken from the AST data
        struct ParentRef { size_t parent; size_t argument; };
        std::unordered_map<uint32_t, ParentRef> parents;
        for (size_t i = 0; i < tree.astnodes.size(); ++i) {
            const ASTNode& p = tree.astnodes[i];
            if (const auto* f = std::get_if<FunctionCallNodeData>(&p.data)) {
                for (size_t a = 0; a < f->arguments_indices.size(); ++a)
                    parents.emplace(f->arguments_indices[a], ParentRef{ i, a });
            }
            else if (const auto* vd = std::get_if<VariableDeclNodeData>(&p.data)) {
                if (vd->init_expression != UINT32_MAX)
                    parents.emplace(vd->init_expression, ParentRef{ i, SIZE_MAX });
            }
            else if (const auto* b = std::get_if<BlockNodeData>(&p.data)) {
                for (uint32_t stmt : b->statements_indices)
                    parents.emplace(stmt, ParentRef{ i, SIZE_MAX });
            }
        }

        auto makePin = [&ctx](uint32_t pinId, uint32_t nodeId, PinKind kind) {
            Pin pin;
            pin.id      = pinId;
            pin.node_id = nodeId;
            pin.kind    = kind;
            if (pinId != UINT32_MAX) {
                const auto it = ctx.pin_map.find(pinId);
                if (it != ctx.pin_map.end()) pin.name = it->second.name;
            }
            return pin;
        };
        auto findSpec = [&ctx](const std::string& name) -> const VulkanCommandSpec* {
            const auto it = ctx.commandspecs.find(name);
            return it != ctx.commandspecs.end() ? &it->second : nullptr;
        };

        for (size_t i = 0; i < tree.astnodes.size(); ++i) {
            const ASTNode& n = tree.astnodes[i];
            NodeVisual&    v = m_visuals[i];

            v.inPin        = makePin(n.input_pin_id,  n.id, PinKind::Input);
            v.outPin       = makePin(n.output_pin_id, n.id, PinKind::Output);
            v.inConnected  = n.input_pin_id  != UINT32_MAX && connectedPins.count(n.input_pin_id)  > 0;
            v.outConnected = n.output_pin_id != UINT32_MAX && connectedPins.count(n.output_pin_id) > 0;
            v.headerColor  = detail::GetNodeHeaderColor(n.type);

            // What is this node plugged into? (decides the colour of its input pin / incoming wire)
            const ASTNode*         parent = nullptr;
            const VulkanParamSpec* param  = nullptr;
            if (const auto it = parents.find(n.id); it != parents.end()) {
                parent = &tree.astnodes[it->second.parent];
                if (it->second.argument != SIZE_MAX) {
                    if (const auto* pf = std::get_if<FunctionCallNodeData>(&parent->data)) {
                        const VulkanCommandSpec* spec = findSpec(pf->function_name);
                        if (spec && it->second.argument < spec->parameters.size())
                            param = &spec->parameters[it->second.argument];
                    }
                }
            }
            const auto* parentVar     = parent ? std::get_if<VariableDeclNodeData>(&parent->data) : nullptr;
            const bool  parentIsBlock = parent && std::holds_alternative<BlockNodeData>(parent->data);
            const std::string lineInfo = "line " + std::to_string(n.line);

            if (const auto* b = std::get_if<BlockNodeData>(&n.data)) {
                const size_t count = b->statements_indices.size();
                v.title       = "Block";
                v.subtitle    = std::to_string(count) + (count == 1 ? " statement" : " statements") + "  |  " + lineInfo;
                v.inCategory  = PinCategory::Exec;
                v.outCategory = PinCategory::Exec;
                if (v.outPin.name.empty()) v.outPin.name = "Then";
            }
            else if (const auto* vd = std::get_if<VariableDeclNodeData>(&n.data)) {
                const PinCategory typeCategory = detail::ClassifyType(vd->type, false, ctx);
                v.title       = vd->name.empty() ? "<unnamed>" : vd->name;
                v.subtitle    = (vd->type.empty() ? std::string("auto") : vd->type) + "  |  " + lineInfo;
                v.headerColor = detail::ScaleColor(detail::GetPinColor(typeCategory), 0.62f);
                v.inCategory  = PinCategory::Exec;
                v.outCategory = typeCategory;
                if (v.outPin.name.empty()) v.outPin.name = "Init";
            }
            else if (const auto* f = std::get_if<FunctionCallNodeData>(&n.data)) {
                const VulkanCommandSpec* spec = findSpec(f->function_name);
                v.title = f->function_name.empty() ? "Function Call" : f->function_name;
                if (spec)
                    v.subtitle = (spec->returntype.empty() || spec->returntype == "void") ? std::string("Vulkan Command") : "Returns " + spec->returntype;
                else
                    v.subtitle = "Function Call";
                v.subtitle += "  |  " + lineInfo;

                if (param)          v.inCategory = detail::ClassifyType(param->type, param->isPtr, ctx);
                else if (parentVar) v.inCategory = detail::ClassifyType(parentVar->type, false, ctx);
                else                v.inCategory = PinCategory::Exec;
                v.outCategory = PinCategory::Array;
                if (v.outPin.name.empty()) v.outPin.name = "Args (" + std::to_string(f->arguments_indices.size()) + ")";
            }
            else if (const auto* e = std::get_if<ExpressionNodeData>(&n.data)) {
                bool literal = false;
                const PinCategory own = detail::ClassifyExpression(*e, ctx, literal);
                v.compact = true;
                v.literal = literal;
                v.inByRef = e->is_address_of;

                if (param)              v.inCategory = detail::ClassifyType(param->type, param->isPtr, ctx);
                else if (parentVar)     v.inCategory = detail::ClassifyType(parentVar->type, false, ctx);
                else if (parentIsBlock) v.inCategory = PinCategory::Exec;
                else                    v.inCategory = own;
                v.outCategory = v.inCategory == PinCategory::Exec ? own : v.inCategory;

                if (param) {
                    if (v.inPin.name.empty()) v.inPin.name = param->name;
                    v.paramInfo = std::string(param->isConst ? "const " : "") + param->type + (param->isPtr ? "* " : " ") + param->name;
                }
                v.value = detail::FitText((e->is_address_of ? "&" : "") + e->text, 280.0f);
            }

            if (n.input_pin_id  != UINT32_MAX) m_pinCategory[n.input_pin_id]  = v.inCategory;
            if (n.output_pin_id != UINT32_MAX) m_pinCategory[n.output_pin_id] = v.outCategory;
        }

        m_visualLinkCount   = ctx.links.size();
        m_visualPinMapCount = ctx.pin_map.size();
        m_visualTreeId      = tree.treeid;
        m_visualsDirty      = false;
    }

    inline void nodeEditor::drawNode(const VKA::DATA::ASTNode& n, const NodeVisual& v, ed::NodeId nodeId, ed::NodeId hoveredNode, ed::PinId hoveredPin) const {
        namespace bp = detail::bp;

        const float lineH  = ImGui::GetTextLineHeight();
        const float iconSz = ImFloor(lineH + 3.0f);
        const float rowH   = ImFloor(ImMax(lineH, iconSz) + 6.0f);
        const float padX   = v.compact ? 8.0f : 10.0f;
        const float padY   = 6.0f;
        const bool  hasIn  = n.input_pin_id  != UINT32_MAX;
        const bool  hasOut = n.output_pin_id != UINT32_MAX;

        const char* bannerText      = nullptr;
        ImU32       bannerColor     = 0;
        ImU32       bannerTextColor = 0;
        if (n.status == VKA::DATA::NodeStatus::Warning) {
            bannerText = "WARNING!"; bannerColor = bp::WarningBanner; bannerTextColor = IM_COL32(24, 24, 24, 255);
        }
        else if (n.status == VKA::DATA::NodeStatus::Error) {
            bannerText = "ERROR!";   bannerColor = bp::ErrorBanner;   bannerTextColor = IM_COL32(255, 255, 255, 255);
        }
        const float bannerH = bannerText ? ImFloor(lineH + 6.0f) : 0.0f;

        auto pinWidth = [&](const VKA::DATA::Pin& pin) {
            return iconSz + (pin.name.empty() ? 0.0f : 4.0f + ImGui::CalcTextSize(pin.name.c_str()).x);
        };
        const float inW  = hasIn  ? pinWidth(v.inPin)  : 0.0f;
        const float outW = hasOut ? pinWidth(v.outPin) : 0.0f;

        float headerH = 0.0f, bodyH = 0.0f, nodeW = 0.0f, valueW = 0.0f;
        if (!v.compact) {
            const float titleW = ImGui::CalcTextSize(v.title.c_str()).x;
            const float subW   = v.subtitle.empty() ? 0.0f : ImGui::CalcTextSize(v.subtitle.c_str()).x;
            headerH = ImFloor(padY + lineH + (v.subtitle.empty() ? 0.0f : lineH + 2.0f) + padY);
            bodyH   = ImFloor(padY + rowH + padY);
            nodeW   = ImMax(padX + iconSz + 6.0f + ImMax(titleW, subW) + padX + 6.0f, padX + inW + 40.0f + outW + padX);
            nodeW   = ImMax(nodeW, 170.0f);
        }
        else {
            valueW = ImGui::CalcTextSize(v.value.c_str()).x + (v.literal ? 12.0f : 0.0f);
            bodyH  = ImFloor(5.0f + rowH + 5.0f);
            nodeW  = padX + (hasIn ? inW + 8.0f : 0.0f) + valueW + (hasOut ? 8.0f + outW : 0.0f) + padX;
            nodeW  = ImMax(nodeW, 60.0f);
        }
        if (bannerText)
            nodeW = ImMax(nodeW, ImGui::CalcTextSize(bannerText).x + padX * 2.0f);
        nodeW = ImFloor(nodeW);

        const float nodeH    = headerH + bodyH + bannerH;
        const float rounding = v.compact ? ImMin(12.0f, ImFloor(bodyH * 0.5f)) : 6.0f;

        ed::BeginNode(nodeId);

        const ImVec2 nodeMin = ImGui::GetCursorScreenPos();
        const ImVec2 nodeMax = nodeMin + ImVec2(nodeW, nodeH);
        ImGui::Dummy(ImVec2(nodeW, nodeH)); // fixes the node size, everything else is placed inside it

        ImDrawList* dl       = ImGui::GetWindowDrawList();
        const bool  selected = ed::IsNodeSelected(nodeId);
        const bool  hovered  = hoveredNode && hoveredNode == nodeId;

        // ---- body
        detail::DrawShadow(dl, nodeMin, nodeMax, rounding);
        detail::FillGradient(dl, nodeMin, nodeMax, rounding, ImDrawFlags_RoundCornersAll,
                             v.compact ? bp::CompactTop : bp::BodyTop, v.compact ? bp::CompactBottom : bp::BodyBottom, false);

        // ---- header
        if (!v.compact) {
            const ImVec2 headerMax(nodeMax.x, nodeMin.y + headerH);
            detail::FillGradient(dl, nodeMin, headerMax, rounding, ImDrawFlags_RoundCornersTop,
                                 v.headerColor, detail::WithAlpha(detail::ScaleColor(v.headerColor, 0.35f), 210), true);
            detail::FillGradient(dl, nodeMin, ImVec2(nodeMax.x, nodeMin.y + ImFloor(headerH * 0.5f)), rounding, ImDrawFlags_RoundCornersTop,
                                 IM_COL32(255, 255, 255, 38), IM_COL32(255, 255, 255, 0), false);
            dl->AddLine(ImVec2(nodeMin.x + 1.0f, headerMax.y - 0.5f), ImVec2(nodeMax.x - 1.0f, headerMax.y - 0.5f), IM_COL32(0, 0, 0, 160), 1.0f);

            const float textX = nodeMin.x + padX + iconSz + 6.0f;
            detail::DrawHeaderIcon(dl, ImVec2(nodeMin.x + padX + iconSz * 0.5f, nodeMin.y + padY + lineH * 0.5f), iconSz, n.type, detail::GetPinColor(v.outCategory));
            detail::AddTextShadowed(dl, ImVec2(textX, nodeMin.y + padY), bp::Title, v.title.c_str());
            if (!v.subtitle.empty())
                dl->AddText(ImVec2(textX, nodeMin.y + padY + lineH + 2.0f), bp::Subtitle, v.subtitle.c_str());
        }

        // ---- status strip
        if (bannerText) {
            const ImVec2 bannerMin(nodeMin.x, nodeMax.y - bannerH);
            const ImVec2 textSize = ImGui::CalcTextSize(bannerText);
            dl->AddRectFilled(bannerMin, nodeMax, bannerColor, rounding, ImDrawFlags_RoundCornersBottom);
            dl->AddText(ImVec2(ImFloor(nodeMin.x + (nodeW - textSize.x) * 0.5f), ImFloor(bannerMin.y + (bannerH - textSize.y) * 0.5f)), bannerTextColor, bannerText);
        }

        // ---- outline, hover and selection highlight
        dl->AddRect(nodeMin, nodeMax, bannerText ? detail::WithAlpha(bannerColor, 200) : bp::NodeBorder, rounding, 0, 1.0f);
        if (selected) {
            dl->AddRect(nodeMin - ImVec2(3.0f, 3.0f), nodeMax + ImVec2(3.0f, 3.0f), detail::WithAlpha(bp::Selection, 60), rounding + 3.0f, 0, 4.0f);
            dl->AddRect(nodeMin - ImVec2(1.5f, 1.5f), nodeMax + ImVec2(1.5f, 1.5f), bp::Selection, rounding + 1.5f, 0, 2.0f);
        }
        else if (hovered) {
            dl->AddRect(nodeMin - ImVec2(1.0f, 1.0f), nodeMax + ImVec2(1.0f, 1.0f), bp::Hover, rounding + 1.0f, 0, 1.5f);
        }

        // ---- pins
        const float rowTop  = nodeMin.y + headerH + ImFloor((bodyH - rowH) * 0.5f);
        const float rowMidY = rowTop + ImFloor(rowH * 0.5f);

        auto submitPin = [&](const VKA::DATA::Pin& pin, PinCategory category, bool connected, bool byRef, float x, float width, ImU32 labelColor) {
            const bool  isInput = pin.kind == VKA::DATA::PinKind::Input;
            const float edgeX   = isInput ? nodeMin.x : nodeMax.x;

            ImGui::SetCursorScreenPos(ImVec2(x, rowTop));
            ed::BeginPin(ed::PinId(pin.id), detail::ToEditorPinKind(pin.kind));
            ImGui::Dummy(ImVec2(width, rowH));
            ed::PinPivotRect(ImVec2(edgeX, rowMidY), ImVec2(edgeX, rowMidY)); // wires end on the node border...
            ed::EndPin();

            const bool   pinHovered = hoveredPin && hoveredPin == ed::PinId(pin.id);
            const ImVec2 iconCenter(isInput ? x + iconSz * 0.5f : x + width - iconSz * 0.5f, rowMidY);
            const ImVec2 glyph = detail::DrawPinIcon(dl, iconCenter, iconSz, category, connected, pinHovered, byRef);

            if (connected) { // ...and a short stub joins the border to the glyph, so the wire looks continuous
                const float thickness = category == PinCategory::Exec ? 3.0f : 2.0f;
                if (isInput) dl->AddLine(ImVec2(edgeX, rowMidY), ImVec2(glyph.x, rowMidY), detail::GetPinColor(category), thickness);
                else         dl->AddLine(ImVec2(glyph.y, rowMidY), ImVec2(edgeX, rowMidY), detail::GetPinColor(category), thickness);
            }

            if (!pin.name.empty()) {
                const ImVec2 textSize = ImGui::CalcTextSize(pin.name.c_str());
                const float  textX    = isInput ? x + iconSz + 4.0f : x + width - iconSz - 4.0f - textSize.x;
                dl->AddText(ImVec2(ImFloor(textX), ImFloor(rowMidY - textSize.y * 0.5f)), labelColor, pin.name.c_str());
            }
        };

        if (!v.compact) {
            if (hasIn)  submitPin(v.inPin,  v.inCategory,  v.inConnected,  false, nodeMin.x + padX, inW, bp::PinLabel);
            if (hasOut) submitPin(v.outPin, v.outCategory, v.outConnected, false, nodeMax.x - padX - outW, outW, bp::PinLabel);
        }
        else {
            float x = nodeMin.x + padX;
            if (hasIn) {
                submitPin(v.inPin, v.inCategory, v.inConnected, v.inByRef, x, inW, bp::ParamLabel);
                x += inW + 8.0f;
            }

            const float textY = ImFloor(rowMidY - lineH * 0.5f);
            if (v.literal) { 
                const ImVec2 boxMin(x, textY - 3.0f), boxMax(x + valueW, textY + lineH + 3.0f);
                dl->AddRectFilled(boxMin, boxMax, bp::ValueBoxBg, 3.0f);
                dl->AddRect(boxMin, boxMax, bp::ValueBoxBorder, 3.0f);
                dl->AddText(ImVec2(x + 6.0f, textY), bp::ValueText, v.value.c_str());
            }
            else {
                detail::AddTextShadowed(dl, ImVec2(x, textY), bp::Title, v.value.c_str());
            }

            if (hasOut)
                submitPin(v.outPin, v.outCategory, v.outConnected, false, nodeMax.x - padX - outW, outW, bp::PinLabel);
        }

        ed::EndNode();
    }

    inline void nodeEditor::drawNodeTooltip(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, ed::NodeId hoveredNode) {
        using namespace VKA::DATA;

        if (!hoveredNode || hoveredNode != m_tooltipNode) {
            m_tooltipNode  = hoveredNode;
            m_tooltipTimer = 0.0f;
            return;
        }
        m_tooltipTimer += ImGui::GetIO().DeltaTime;
        if (m_tooltipTimer < 0.45f || ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right))
            return;

        const uint64_t packed = static_cast<uint64_t>(hoveredNode.Get());
        if (static_cast<uint32_t>(packed >> 32) != tree.treeid)
            return;
        const auto found = m_nodeIndex.find(static_cast<uint32_t>(packed & 0xFFFFFFFFull));
        if (found == m_nodeIndex.end() || found->second >= tree.astnodes.size() || found->second >= m_visuals.size())
            return;

        const ASTNode&    n      = tree.astnodes[found->second];
        const NodeVisual& v      = m_visuals[found->second];
        const ImU32       accent = v.compact ? detail::GetPinColor(v.inCategory) : v.headerColor;

        ed::Suspend();
        if (ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);

            ImGui::TextColored(ImColor(detail::MixColor(accent, IM_COL32_WHITE, 0.3f)), "%s", detail::ToString(n.type));
            ImGui::SameLine();
            ImGui::TextDisabled("#%u  |  line %u", n.id, n.line);
            ImGui::Separator();

            if (const auto* b = std::get_if<BlockNodeData>(&n.data)) {
                ImGui::Text("Statements: %zu", b->statements_indices.size());
            }
            else if (const auto* vd = std::get_if<VariableDeclNodeData>(&n.data)) {
                ImGui::Text("%s %s", vd->type.c_str(), vd->name.c_str());
                ImGui::TextDisabled("%s", vd->init_expression != UINT32_MAX ? "Has initializer" : "No initializer");
            }
            else if (const auto* f = std::get_if<FunctionCallNodeData>(&n.data)) {
                ImGui::Text("%s(...)", f->function_name.c_str());
                ImGui::TextDisabled("Arguments: %zu", f->arguments_indices.size());
                if (const auto spec = ctx.commandspecs.find(f->function_name); spec != ctx.commandspecs.end())
                    ImGui::TextDisabled("Returns: %s", spec->second.returntype.c_str());
            }
            else if (const auto* e = std::get_if<ExpressionNodeData>(&n.data)) {
                ImGui::Text("%s%s", e->is_address_of ? "&" : "", e->text.c_str());
                if (!v.paramInfo.empty())
                    ImGui::TextDisabled("Parameter: %s", v.paramInfo.c_str());
            }

            if (n.status != NodeStatus::Success) {
                const bool isError = n.status == NodeStatus::Error;
                ImGui::Separator();
                ImGui::TextColored(isError ? ImVec4(1.0f, 0.38f, 0.38f, 1.0f) : ImVec4(1.0f, 0.76f, 0.25f, 1.0f), "%s%s%s",
                                   isError ? "Error" : "Warning", n.statusMsg.empty() ? "" : ": ", n.statusMsg.c_str());
            }

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ed::Resume();
    }

    inline void nodeEditor::calculateNodeLayout(uint32_t nodeid, int depth, float& currentY, const VKA::DATA::ASTTree& tree, std::unordered_set<uint32_t>& visited) const {
        if (visited.count(nodeid)) return;
        visited.insert(nodeid);

        const VKA::DATA::ASTNode* nodePtr = nullptr;
        for (const auto& n : tree.astnodes) {
            if (n.id == nodeid) { nodePtr = &n; break; }
        }
        if (!nodePtr) return;

        uint64_t packedId = detail::packNodeId(tree.treeid, nodePtr->id);

        ed::SetNodePosition(ed::NodeId(static_cast<uintptr_t>(packedId)), ImVec2(depth * 320.0f, currentY));

        float initialY = currentY;
        currentY += 150.0f;

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
                calculateNodeLayout(stmtId, depth + 1, currentY, tree, visited);
        }
    }

    inline void nodeEditor::draw(const VKA::DATA::ASTTree& tree, const VKA::DATA::GraphContext& ctx, const char* title) {
        if (!m_ctx) m_ctx = ed::CreateEditor();

        ed::SetCurrentEditor(m_ctx);

        if (!m_styleApplied) {
            applyBlueprintStyle();
            m_styleApplied = true;
        }

        if (m_needsLayout || m_lastNodeCount != tree.astnodes.size()) {
            float startY = 0.0f;
            std::unordered_set<uint32_t> visited;

            auto isChild = [&](uint32_t id) {
                for (const auto& parent : tree.astnodes) {
                    if (auto* f = std::get_if<VKA::DATA::FunctionCallNodeData>(&parent.data)) {
                        for (auto argId : f->arguments_indices) if (argId == id) return true;
                    }
                    else if (auto* v = std::get_if<VKA::DATA::VariableDeclNodeData>(&parent.data)) {
                        if (v->init_expression == id) return true;
                    }
                    else if (auto* b = std::get_if<VKA::DATA::BlockNodeData>(&parent.data)) {
                        for (auto stmtId : b->statements_indices) if (stmtId == id) return true;
                    }
                }
                return false;
                };

            for (const auto& n : tree.astnodes) {
                if (!isChild(n.id)) {
                    calculateNodeLayout(n.id, 0, startY, tree, visited);
                }
            }

            m_lastNodeCount = tree.astnodes.size();
            m_needsLayout = false;
        }

        if (m_visualsDirty || m_visuals.size() != tree.astnodes.size() || m_visualLinkCount != ctx.links.size()
            || m_visualPinMapCount != ctx.pin_map.size() || m_visualTreeId != tree.treeid) {
            rebuildVisuals(tree, ctx);
        }

        ed::Begin("ASTNodeEditor");

        const ed::NodeId hoveredNode = ed::GetHoveredNode();
        const ed::PinId  hoveredPin  = ed::GetHoveredPin();

        for (size_t i = 0; i < tree.astnodes.size(); ++i) {
            const auto& n = tree.astnodes[i];
            const uint64_t packedId = detail::packNodeId(tree.treeid, n.id);
            const ed::NodeId nodeId(static_cast<uintptr_t>(packedId));

            ImGui::PushID(static_cast<int>(packedId));
            drawNode(n, m_visuals[i], nodeId, hoveredNode, hoveredPin);
            ImGui::PopID();
        }

        for (const auto& link : ctx.links) {
            PinCategory category = PinCategory::Wildcard;
            if (const auto end = m_pinCategory.find(link.endpinid); end != m_pinCategory.end())
                category = end->second;
            else if (const auto start = m_pinCategory.find(link.startnpinid); start != m_pinCategory.end())
                category = start->second;

            const ImVec4 color     = ImGui::ColorConvertU32ToFloat4(detail::GetPinColor(category));
            const float  thickness = category == PinCategory::Exec ? 3.0f : 2.0f;
            ed::Link(ed::LinkId(link.id), ed::PinId(link.startnpinid), ed::PinId(link.endpinid), color, thickness);
        }

        drawNodeTooltip(tree, ctx, hoveredNode);

        ed::End();
        ed::SetCurrentEditor(nullptr);
    }

} // VKA::UI
