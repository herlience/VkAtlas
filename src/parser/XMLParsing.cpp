#include "XMLParsing.h"
#include "../common/vkainfo.h"
#include <pugixml/pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {
	using namespace VKA::DATA;

	bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
	bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }

	std::string trim(std::string_view s) {
		size_t begin = 0, end = s.size();
		while (begin < end && isSpace(s[begin])) ++begin;
		while (end > begin && isSpace(s[end - 1])) --end;
		return std::string(s.substr(begin, end - begin));
	}

	bool endsWith(std::string_view s, std::string_view suffix) {
		return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
	}

	bool isIdentifier(std::string_view s) {
		if (s.empty() || std::isdigit(static_cast<unsigned char>(s.front()))) return false;
		return std::all_of(s.begin(), s.end(), isIdentChar);
	}

	bool isWordAt(std::string_view text, size_t pos, size_t length) {
		const bool startOk = pos == 0 || !isIdentChar(text[pos - 1]);
		const bool endOk = pos + length == text.size() || !isIdentChar(text[pos + length]);
		return startOk && endOk;
	}

	bool hasWord(std::string_view text, std::string_view word) {
		for (size_t pos = text.find(word); pos != std::string_view::npos; pos = text.find(word, pos + 1))
			if (isWordAt(text, pos, word.size())) return true;
		return false;
	}

	std::string removeWord(std::string text, std::string_view word) {
		size_t pos = text.find(word);
		while (pos != std::string::npos) {
			if (isWordAt(text, pos, word.size())) {
				size_t end = pos + word.size();
				while (end < text.size() && isSpace(text[end])) ++end;
				text.erase(pos, end - pos);
				pos = text.find(word, pos);
			}
			else {
				pos = text.find(word, pos + word.size());
			}
		}
		return text;
	}

	std::string normalizeDecl(std::string_view s) {
		std::string out;
		bool pendingSpace = false;
		for (char c : s) {
			if (isSpace(c)) {
				pendingSpace = !out.empty();
				continue;
			}
			if (!out.empty()) {
				const char prev = out.back();
				const bool glue = c == '*' || c == '[' || c == ']' || c == ':' || c == ',' || c == ')' || prev == '[' || prev == '(';
				if ((pendingSpace && !glue) || (prev == '*' && isIdentChar(c))) out.push_back(' ');
			}
			pendingSpace = false;
			out.push_back(c);
		}
		return out;
	}

	std::vector<std::string> splitList(std::string_view s, char separator = ',') {
		std::vector<std::string> out;
		size_t start = 0;
		while (start <= s.size()) {
			size_t end = s.find(separator, start);
			if (end == std::string_view::npos) end = s.size();
			std::string item = trim(s.substr(start, end - start));
			if (!item.empty()) out.push_back(std::move(item));
			start = end + 1;
		}
		return out;
	}

	bool listContains(std::string_view list, std::string_view item) {
		for (const std::string& entry : splitList(list))
			if (entry == item) return true;
		return false;
	}

	void addUnique(std::vector<std::string>& list, const std::string& item) {
		if (!item.empty() && std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
	}

	bool parseInteger(std::string_view text, int64_t& out) {
		std::string s = trim(text);
		while (s.size() > 2 && s.front() == '(' && s.back() == ')') s = trim(std::string_view(s).substr(1, s.size() - 2));
		if (s.empty() || !(std::isdigit(static_cast<unsigned char>(s.front())) || s.front() == '-')) return false;

		char* end = nullptr;
		if (s.front() == '-') out = std::strtoll(s.c_str(), &end, 0);
		else                  out = static_cast<int64_t>(std::strtoull(s.c_str(), &end, 0));
		if (end == s.c_str()) return false;
		while (*end == 'U' || *end == 'u' || *end == 'L' || *end == 'l') ++end;
		return *end == '\0';
	}

	bool isVulkanApi(const pugi::xml_node& node, const char* attribute = "api") {
		const pugi::xml_attribute api = node.attribute(attribute);
		return !api || listContains(api.value(), "vulkan");
	}

	void appendText(const pugi::xml_node& node, std::string& out) {
		for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
			if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) out += child.value();
			else if (child.type() == pugi::node_element && std::string_view(child.name()) != "comment") appendText(child, out);
		}
	}

	std::string innerText(const pugi::xml_node& node) {
		std::string text;
		appendText(node, text);
		return trim(text);
	}

	VulkanTypeCategory toCategory(std::string_view category) {
		if (category.empty())          return VulkanTypeCategory::External;
		if (category == "define")      return VulkanTypeCategory::Define;
		if (category == "basetype")    return VulkanTypeCategory::BaseType;
		if (category == "bitmask")     return VulkanTypeCategory::Bitmask;
		if (category == "handle")      return VulkanTypeCategory::Handle;
		if (category == "enum")        return VulkanTypeCategory::Enum;
		if (category == "funcpointer") return VulkanTypeCategory::FuncPointer;
		if (category == "struct")      return VulkanTypeCategory::Struct;
		if (category == "union")       return VulkanTypeCategory::Union;
		return VulkanTypeCategory::Unknown;
	}

	std::vector<std::string> splitLens(std::string_view len, const std::string& altlen) {
		std::vector<std::string> out;
		std::string current;
		int depth = 0;
		const auto flush = [&]() {
			std::string item = trim(current);
			current.clear();
			if (item.empty()) return;
			if (item.rfind("latexmath:", 0) == 0 && !altlen.empty()) item = altlen;
			out.push_back(std::move(item));
		};
		for (char c : len) {
			if (c == '[') ++depth;
			else if (c == ']') --depth;
			if (c == ',' && depth == 0) {
				flush();
				continue;
			}
			current.push_back(c);
		}
		flush();
		return out;
	}

	VulkanVariableSpec parseDeclaration(const pugi::xml_node& node) {
		VulkanVariableSpec v;
		std::string beforeType, beforeName, afterName;
		bool seenType = false, seenName = false;

		for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
			if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
				(seenName ? afterName : beforeName) += child.value();
				if (!seenType && !seenName) beforeType += child.value();
				continue;
			}
			if (child.type() != pugi::node_element) continue;

			const std::string_view tag = child.name();
			if (tag == "type") {
				v.type = trim(child.child_value());
				beforeName += v.type;
				seenType = true;
			}
			else if (tag == "name") {
				v.name = trim(child.child_value());
				v.namealias = child.attribute("alias").value();
				seenName = true;
			}
			else if (tag == "enum") {
				(seenName ? afterName : beforeName) += child.child_value();
			}
			else if (tag == "comment") {
				v.comment = trim(child.child_value());
			}
		}

		v.fulltype = normalizeDecl(removeWord(beforeName, "struct"));
		v.pointerdepth = static_cast<uint32_t>(std::count(beforeName.begin(), beforeName.end(), '*'));
		v.isPtr = v.pointerdepth > 0;
		v.isConst = hasWord(beforeType, "const");

		const std::string suffix = normalizeDecl(afterName);
		for (size_t open = suffix.find('['); open != std::string::npos; open = suffix.find('[', open + 1)) {
			const size_t close = suffix.find(']', open);
			if (close == std::string::npos) break;
			v.arraysizes.push_back(trim(std::string_view(suffix).substr(open + 1, close - open - 1)));
		}
		v.isArray = !v.arraysizes.empty();
		if (const size_t colon = suffix.find(':'); colon != std::string::npos)
			v.bitfieldwidth = static_cast<uint32_t>(std::strtoul(suffix.c_str() + colon + 1, nullptr, 10));
		v.declaration = normalizeDecl(v.fulltype + " " + v.name + suffix);

		const auto attr = [&node](const char* name) { return std::string(node.attribute(name).value()); };
		for (const std::string& level : splitList(node.attribute("optional").value()))
			v.optional.push_back(level == "true");
		v.isOptional = !v.optional.empty() && v.optional.front();
		v.noautovalidity = node.attribute("noautovalidity").as_bool();
		v.len = attr("len");
		v.altlen = attr("altlen");
		v.lens = splitLens(v.len, v.altlen);
		v.values = attr("values");
		v.externsync = attr("externsync");
		v.selector = attr("selector");
		v.selection = attr("selection");
		v.objecttype = attr("objecttype");
		v.limittype = attr("limittype");
		v.validstructs = attr("validstructs");
		v.stride = attr("stride");
		v.flagsextend = attr("flagsextend");
		v.flagsextendmember = attr("flagsextendmember");
		v.featurelink = attr("featurelink");
		v.deprecated = attr("deprecated");
		return v;
	}

	class RegistryParser {
	public:
		explicit RegistryParser(GraphContext& ctx) : m_ctx(ctx) {}

		void parse(const pugi::xml_node& registry) {
			clearRegistry();
			for (pugi::xml_node platform : registry.child("platforms").children("platform"))
				m_platformProtect[platform.attribute("name").value()] = platform.attribute("protect").value();

			parseTypes(registry.child("types"));
			for (pugi::xml_node block : registry.children("enums")) parseEnumBlock(block);
			parseCommands(registry.child("commands"));
			for (pugi::xml_node feature : registry.children("feature")) parseFeature(feature);
			for (pugi::xml_node extension : registry.child("extensions").children("extension")) parseExtension(extension);

			finalize();
		}

	private:
		GraphContext& m_ctx;

		std::unordered_map<std::string, VulkanTypeCategory> m_categories;
		std::unordered_map<std::string, std::string> m_typeAliases;
		std::unordered_map<std::string, std::string> m_commandAliases;
		std::unordered_map<std::string, std::vector<std::string>> m_typeDeps;
		std::unordered_map<std::string, std::vector<std::string>> m_typeEnumDeps;
		std::unordered_map<std::string, std::vector<std::string>> m_commandDeps;

		std::unordered_set<std::string> m_requiredTypes;
		std::unordered_set<std::string> m_requiredCommands;
		std::unordered_set<std::string> m_requiredEnums;
		std::unordered_set<std::string> m_internalFeatures;
		std::unordered_map<std::string, std::vector<std::string>> m_providers;
		std::unordered_map<std::string, VulkanDeprecation> m_deprecations;
		std::unordered_map<std::string, std::string> m_platformProtect;
		std::unordered_map<std::string, std::pair<std::string, size_t>> m_valueIndex;

		void clearRegistry() {
			m_ctx.commandspecs.clear();
			m_ctx.structspecs.clear();
			m_ctx.handlespecs.clear();
			m_ctx.enumtypespecs.clear();
			m_ctx.bitmaskspecs.clear();
			m_ctx.funcpointerspecs.clear();
			m_ctx.basetypespecs.clear();
			m_ctx.definespecs.clear();
			m_ctx.constantspecs.clear();
			m_ctx.featurespecs.clear();
			m_ctx.typecategories.clear();
			m_ctx.enumvalueowners.clear();
			m_ctx.enumspecs.clear();
			m_ctx.typespecs.clear();
		}

		void addTypeDep(const std::string& type, const std::string& dependency) {
			if (!dependency.empty() && dependency != type) m_typeDeps[type].push_back(dependency);
		}

		void addProvider(const std::string& name, const std::string& provider) {
			addUnique(m_providers[name], provider);
		}

		std::string resolveAlias(const std::unordered_map<std::string, std::string>& aliases, std::string name) const {
			for (int guard = 0; guard < 16; ++guard) {
				const auto it = aliases.find(name);
				if (it == aliases.end()) break;
				name = it->second;
			}
			return name;
		}

		void parseTypes(const pugi::xml_node& types) {
			for (pugi::xml_node t : types.children("type")) {
				if (!isVulkanApi(t)) continue;

				const std::string category = t.attribute("category").value();
				if (category == "include") continue;

				std::string name = t.attribute("name").value();
				if (name.empty()) name = trim(t.child_value("name"));
				if (name.empty()) name = trim(t.child("proto").child_value("name"));
				if (name.empty()) continue;

				const VulkanTypeCategory cat = toCategory(category);
				m_categories[name] = cat;
				addTypeDep(name, t.attribute("requires").value());
				addTypeDep(name, t.attribute("bitvalues").value());

				if (const std::string alias = t.attribute("alias").value(); !alias.empty()) {
					m_typeAliases[name] = alias;
					addTypeDep(name, alias);
					continue;
				}

				switch (cat) {
				case VulkanTypeCategory::Define:      parseDefine(t, name); break;
				case VulkanTypeCategory::BaseType:    parseBaseType(t, name); break;
				case VulkanTypeCategory::Bitmask:     parseBitmask(t, name); break;
				case VulkanTypeCategory::Handle:      parseHandle(t, name); break;
				case VulkanTypeCategory::Enum:        m_ctx.enumtypespecs[name].name = name; break;
				case VulkanTypeCategory::FuncPointer: parseFuncPointer(t, name); break;
				case VulkanTypeCategory::Struct:
				case VulkanTypeCategory::Union:       parseStruct(t, name, cat == VulkanTypeCategory::Union); break;
				default: break;
				}
			}
		}

		void parseDefine(const pugi::xml_node& t, const std::string& name) {
			VulkanDefineSpec d;
			d.name = name;
			d.definition = innerText(t);
			d.comment = t.attribute("comment").value();
			for (pugi::xml_node sub : t.children("type")) addTypeDep(name, trim(sub.child_value()));
			m_ctx.definespecs[name] = std::move(d);
		}

		void parseBaseType(const pugi::xml_node& t, const std::string& name) {
			VulkanBaseTypeSpec b;
			b.name = name;
			b.definition = innerText(t);
			b.comment = t.attribute("comment").value();
			if (const pugi::xml_node typeNode = t.child("type")) {
				b.underlying = trim(typeNode.child_value());
				addTypeDep(name, b.underlying);
				for (pugi::xml_node n = typeNode.next_sibling(); n && std::string_view(n.name()) != "name"; n = n.next_sibling()) {
					const std::string_view text = n.value();
					b.underlying.append(static_cast<size_t>(std::count(text.begin(), text.end(), '*')), '*');
				}
			}
			m_ctx.basetypespecs[name] = std::move(b);
		}

		void parseBitmask(const pugi::xml_node& t, const std::string& name) {
			VulkanBitmaskSpec b;
			b.name = name;
			b.basetype = trim(t.child_value("type"));
			b.flagbits = t.attribute("requires").value();
			if (b.flagbits.empty()) b.flagbits = t.attribute("bitvalues").value();
			b.comment = t.attribute("comment").value();
			addTypeDep(name, b.basetype);
			m_ctx.bitmaskspecs[name] = std::move(b);
		}

		void parseHandle(const pugi::xml_node& t, const std::string& name) {
			VulkanHandleSpec h;
			h.name = name;
			const std::string macro = trim(t.child_value("type"));
			h.isDispatchable = macro == "VK_DEFINE_HANDLE";
			h.parents = splitList(t.attribute("parent").value());
			h.objtypeenum = t.attribute("objtypeenum").value();
			h.comment = t.attribute("comment").value();
			addTypeDep(name, macro);
			m_ctx.handlespecs[name] = std::move(h);
		}

		void parseFuncPointer(const pugi::xml_node& t, const std::string& name) {
			VulkanFuncPointerSpec f;
			f.name = name;
			f.comment = t.attribute("comment").value();
			if (const pugi::xml_node proto = t.child("proto")) {
				const VulkanVariableSpec ret = parseDeclaration(proto);
				f.returntype = ret.fulltype;
				addTypeDep(name, ret.type);
				for (pugi::xml_node p : t.children("param")) {
					if (!isVulkanApi(p)) continue;
					f.parameters.push_back(parseDeclaration(p));
					addTypeDep(name, f.parameters.back().type);
				}
			}
			else {
				const std::string text = innerText(t);
				if (const size_t open = text.find('('); text.rfind("typedef", 0) == 0 && open != std::string::npos)
					f.returntype = normalizeDecl(trim(std::string_view(text).substr(7, open - 7)));
				for (pugi::xml_node sub : t.children("type")) addTypeDep(name, trim(sub.child_value()));
			}
			m_ctx.funcpointerspecs[name] = std::move(f);
		}

		void parseStruct(const pugi::xml_node& t, const std::string& name, bool isUnion) {
			VulkanStructSpec s;
			s.name = name;
			s.isUnion = isUnion;
			s.returnedonly = t.attribute("returnedonly").as_bool();
			s.allowduplicate = t.attribute("allowduplicate").as_bool();
			s.requiredlimittype = t.attribute("requiredlimittype").as_bool();
			s.structextends = splitList(t.attribute("structextends").value());
			s.comment = t.attribute("comment").value();
			if (s.comment.empty()) s.comment = trim(t.child_value("comment"));

			for (pugi::xml_node m : t.children("member")) {
				if (!isVulkanApi(m)) continue;
				VulkanMemberSpec member = parseDeclaration(m);
				addTypeDep(name, member.type);
				for (const std::string& size : member.arraysizes)
					if (isIdentifier(size)) m_typeEnumDeps[name].push_back(size);
				if (member.name == "sType") s.stype = member.values;
				s.members.push_back(std::move(member));
			}
			m_ctx.structspecs[name] = std::move(s);
		}

		void parseEnumBlock(const pugi::xml_node& block) {
			const std::string name = block.attribute("name").value();
			const std::string type = block.attribute("type").value();

			if (type == "constants" || name == "API Constants") {
				for (pugi::xml_node e : block.children("enum")) {
					if (!isVulkanApi(e)) continue;
					VulkanConstantSpec c;
					c.name = e.attribute("name").value();
					c.type = e.attribute("type").value();
					c.value = e.attribute("value").value();
					c.aliasof = e.attribute("alias").value();
					c.comment = e.attribute("comment").value();
					if (!c.name.empty()) m_ctx.constantspecs[c.name] = std::move(c);
				}
				return;
			}
			if (name.empty()) return;

			VulkanEnumSpec& spec = m_ctx.enumtypespecs[name];
			spec.name = name;
			spec.isBitmask = type == "bitmask";
			spec.bitwidth = block.attribute("bitwidth").as_uint(32);
			if (const std::string comment = block.attribute("comment").value(); !comment.empty()) spec.comment = comment;
			m_categories.emplace(name, VulkanTypeCategory::Enum);

			for (pugi::xml_node e : block.children("enum"))
				if (isVulkanApi(e)) addEnumValue(name, e, 0);
		}

		void addEnumValue(const std::string& enumName, const pugi::xml_node& e, int64_t extensionNumber) {
			const std::string valueName = e.attribute("name").value();
			if (valueName.empty() || m_valueIndex.count(valueName)) return;

			VulkanEnumValueSpec v;
			v.name = valueName;
			v.aliasof = e.attribute("alias").value();
			if (const pugi::xml_attribute offset = e.attribute("offset")) {
				const pugi::xml_attribute extnumber = e.attribute("extnumber");
				const int64_t number = extnumber ? extnumber.as_int() : extensionNumber;
				v.value = 1000000000LL + (number - 1) * 1000LL + offset.as_int();
				if (std::string_view(e.attribute("dir").value()) == "-") v.value = -v.value;
			}
			else if (const pugi::xml_attribute bitpos = e.attribute("bitpos")) {
				v.bitpos = bitpos.as_int();
				if (v.bitpos >= 0 && v.bitpos < 64) v.value = static_cast<int64_t>(uint64_t{ 1 } << v.bitpos);
			}
			else if (const pugi::xml_attribute value = e.attribute("value")) {
				if (!parseInteger(value.value(), v.value) && isIdentifier(value.value())) v.aliasof = value.value();
			}
			else if (v.aliasof.empty()) {
				return;
			}
			v.deprecated = e.attribute("deprecated").value();
			v.protect = e.attribute("protect").value();
			v.comment = e.attribute("comment").value();

			VulkanEnumSpec& spec = m_ctx.enumtypespecs[enumName];
			if (spec.name.empty()) spec.name = enumName;
			m_valueIndex[valueName] = { enumName, spec.values.size() };
			spec.values.push_back(std::move(v));
		}

		void addConstant(const pugi::xml_node& e, VulkanFeatureSpec& feature) {
			VulkanConstantSpec c;
			c.name = e.attribute("name").value();
			c.aliasof = e.attribute("alias").value();
			c.comment = e.attribute("comment").value();

			const std::string value = e.attribute("value").value();
			int64_t number = 0;
			if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
				c.type = "const char*";
				c.value = value.substr(1, value.size() - 2);
				if (feature.isExtension && feature.nameconstant.empty() && endsWith(c.name, "_EXTENSION_NAME")) feature.nameconstant = c.name;
			}
			else if (parseInteger(value, number)) {
				c.type = "uint32_t";
				c.value = value;
				if (feature.isExtension && endsWith(c.name, "_SPEC_VERSION")) feature.specversion = static_cast<uint32_t>(number);
			}
			else if (isIdentifier(value)) {
				c.aliasof = value;
			}
			else {
				c.value = value;
			}
			m_ctx.constantspecs.emplace(c.name, std::move(c));
		}

		void parseCommands(const pugi::xml_node& commands) {
			for (pugi::xml_node cmd : commands.children("command")) {
				if (!isVulkanApi(cmd)) continue;

				if (const std::string alias = cmd.attribute("alias").value(); !alias.empty()) {
					m_commandAliases[cmd.attribute("name").value()] = alias;
					continue;
				}
				const pugi::xml_node proto = cmd.child("proto");
				if (!proto) continue;

				const VulkanVariableSpec ret = parseDeclaration(proto);
				VulkanCommandSpec spec;
				spec.name = ret.name;
				spec.returntype = ret.fulltype;
				spec.queues = splitList(cmd.attribute("queues").value());
				spec.renderpassscope = cmd.attribute("renderpass").value();
				spec.cmdbufferlevel = splitList(cmd.attribute("cmdbufferlevel").value());
				spec.tasks = splitList(cmd.attribute("tasks").value());
				spec.videocoding = cmd.attribute("videocoding").value();
				spec.conditionalrendering = cmd.attribute("conditionalrendering").value();
				spec.allownoqueues = cmd.attribute("allownoqueues").as_bool();
				spec.isExported = listContains(cmd.attribute("export").value(), "vulkan");
				spec.successcodes = splitList(cmd.attribute("successcodes").value());
				spec.errorcodes = splitList(cmd.attribute("errorcodes").value());
				spec.comment = cmd.attribute("comment").value();

				std::vector<std::string>& deps = m_commandDeps[spec.name];
				deps.push_back(ret.type);
				for (pugi::xml_node p : cmd.children("param")) {
					if (!isVulkanApi(p)) continue;
					spec.parameters.push_back(parseDeclaration(p));
					deps.push_back(spec.parameters.back().type);
				}
				for (pugi::xml_node p : cmd.child("implicitexternsyncparams").children("param"))
					spec.implicitexternsyncparams.push_back(trim(p.child_value()));

				m_ctx.commandspecs[spec.name] = std::move(spec);
			}
		}

		void parseFeature(const pugi::xml_node& feature) {
			if (!isVulkanApi(feature)) return;

			const std::string number = feature.attribute("number").value();
			std::string name = feature.attribute("name").value();
			const bool internal = std::string_view(feature.attribute("apitype").value()) == "internal";
			if (internal) {
				m_internalFeatures.insert(name);
				name = "VK_VERSION_" + number;
				std::replace(name.begin(), name.end(), '.', '_');
			}

			VulkanFeatureSpec& spec = m_ctx.featurespecs[name];
			spec.name = name;
			spec.number = number;
			if (!internal) {
				std::vector<std::string> depends;
				for (const std::string& dep : splitList(feature.attribute("depends").value()))
					if (!m_internalFeatures.count(dep)) depends.push_back(dep);
				spec.depends.clear();
				for (const std::string& dep : depends) spec.depends += (spec.depends.empty() ? "" : ",") + dep;
				spec.comment = feature.attribute("comment").value();
			}
			parseRequires(feature, spec, 0);
		}

		void parseExtension(const pugi::xml_node& extension) {
			if (!isVulkanApi(extension, "supported")) return;

			const std::string name = extension.attribute("name").value();
			VulkanFeatureSpec& spec = m_ctx.featurespecs[name];
			spec.name = name;
			spec.isExtension = true;
			spec.number = extension.attribute("number").value();
			spec.type = extension.attribute("type").value();
			spec.depends = extension.attribute("depends").value();
			spec.platform = extension.attribute("platform").value();
			if (const auto it = m_platformProtect.find(spec.platform); it != m_platformProtect.end()) spec.protect = it->second;
			spec.promotedto = extension.attribute("promotedto").value();
			spec.deprecatedby = extension.attribute("deprecatedby").value();
			spec.obsoletedby = extension.attribute("obsoletedby").value();
			spec.specialuse = extension.attribute("specialuse").value();
			spec.author = extension.attribute("author").value();
			spec.isProvisional = extension.attribute("provisional").as_bool();
			spec.comment = extension.attribute("comment").value();
			parseRequires(extension, spec, extension.attribute("number").as_int());
		}

		void parseRequires(const pugi::xml_node& owner, VulkanFeatureSpec& feature, int64_t extensionNumber) {
			for (pugi::xml_node require : owner.children("require")) {
				if (!isVulkanApi(require)) continue;

				for (pugi::xml_node item : require.children()) {
					const std::string_view tag = item.name();
					const std::string name = item.attribute("name").value();
					if (name.empty()) continue;

					if (tag == "type") {
						addUnique(feature.types, name);
						m_requiredTypes.insert(name);
						addProvider(name, feature.name);
					}
					else if (tag == "command") {
						addUnique(feature.commands, name);
						m_requiredCommands.insert(name);
						addProvider(name, feature.name);
					}
					else if (tag == "feature") {
						addUnique(feature.requiredfeatures, std::string(item.attribute("struct").value()) + "::" + name);
					}
					else if (tag == "enum" && isVulkanApi(item)) {
						addUnique(feature.enums, name);
						m_requiredEnums.insert(name);
						addProvider(name, feature.name);
						if (const std::string extends = item.attribute("extends").value(); !extends.empty())
							addEnumValue(resolveAlias(m_typeAliases, extends), item, extensionNumber);
						else if (item.attribute("value") || item.attribute("alias"))
							addConstant(item, feature);
					}
				}
			}

			for (pugi::xml_node block : owner.children("deprecate")) {
				const std::string link = block.attribute("explanationlink").value();
				for (pugi::xml_node item : block.children()) {
					const std::string_view tag = item.name();
					if (tag != "type" && tag != "command") continue;
					m_deprecations.emplace(item.attribute("name").value(), VulkanDeprecation{ feature.name, item.attribute("supersededby").value(), link });
				}
			}
		}

		void finalize() {
			computeRequiredClosure();
			dropUnrequired();

			for (const auto& [name, category] : m_categories)
				if (m_requiredTypes.count(name)) m_ctx.typecategories[name] = category;

			for (auto& [name, s] : m_ctx.structspecs)      resolveVariables(s.members);
			for (auto& [name, c] : m_ctx.commandspecs)     resolveVariables(c.parameters);
			for (auto& [name, f] : m_ctx.funcpointerspecs) resolveVariables(f.parameters);

			linkStructExtends();
			resolveEnumValueAliases();
			resolveConstantAliases();
			applyMeta();
			createAliasEntries();
			buildLookups();
		}

		void computeRequiredClosure() {
			std::vector<std::string> pending(m_requiredCommands.begin(), m_requiredCommands.end());
			while (!pending.empty()) {
				const std::string name = std::move(pending.back());
				pending.pop_back();
				if (const auto alias = m_commandAliases.find(name); alias != m_commandAliases.end())
					if (m_requiredCommands.insert(alias->second).second) pending.push_back(alias->second);
				if (const auto deps = m_commandDeps.find(name); deps != m_commandDeps.end())
					m_requiredTypes.insert(deps->second.begin(), deps->second.end());
			}

			pending.assign(m_requiredTypes.begin(), m_requiredTypes.end());
			while (!pending.empty()) {
				const std::string name = std::move(pending.back());
				pending.pop_back();
				if (const auto deps = m_typeDeps.find(name); deps != m_typeDeps.end())
					for (const std::string& dep : deps->second)
						if (m_requiredTypes.insert(dep).second) pending.push_back(dep);
				if (const auto enums = m_typeEnumDeps.find(name); enums != m_typeEnumDeps.end())
					m_requiredEnums.insert(enums->second.begin(), enums->second.end());
			}

			pending.assign(m_requiredEnums.begin(), m_requiredEnums.end());
			while (!pending.empty()) {
				const std::string name = std::move(pending.back());
				pending.pop_back();
				const auto it = m_ctx.constantspecs.find(name);
				if (it != m_ctx.constantspecs.end() && !it->second.aliasof.empty())
					if (m_requiredEnums.insert(it->second.aliasof).second) pending.push_back(it->second.aliasof);
			}
		}

		void dropUnrequired() {
			const auto notType = [this](const auto& entry) { return m_requiredTypes.count(entry.first) == 0; };
			std::erase_if(m_ctx.structspecs, notType);
			std::erase_if(m_ctx.handlespecs, notType);
			std::erase_if(m_ctx.enumtypespecs, notType);
			std::erase_if(m_ctx.bitmaskspecs, notType);
			std::erase_if(m_ctx.funcpointerspecs, notType);
			std::erase_if(m_ctx.basetypespecs, notType);
			std::erase_if(m_ctx.definespecs, notType);
			std::erase_if(m_ctx.commandspecs, [this](const auto& entry) { return m_requiredCommands.count(entry.first) == 0; });
			std::erase_if(m_ctx.constantspecs, [this](const auto& entry) { return m_requiredEnums.count(entry.first) == 0; });
		}

		uint32_t resolveArrayLength(std::string size) const {
			for (int guard = 0; guard < 16; ++guard) {
				int64_t number = 0;
				if (parseInteger(size, number)) return static_cast<uint32_t>(number);
				const auto it = m_ctx.constantspecs.find(size);
				if (it == m_ctx.constantspecs.end()) return 0;
				size = it->second.aliasof.empty() ? it->second.value : it->second.aliasof;
			}
			return 0;
		}

		void resolveVariables(std::vector<VulkanVariableSpec>& vars) const {
			for (VulkanVariableSpec& v : vars) {
				const auto it = m_ctx.typecategories.find(v.type);
				v.category = it != m_ctx.typecategories.end() ? it->second : VulkanTypeCategory::Unknown;
				v.isStruct = v.category == VulkanTypeCategory::Struct || v.category == VulkanTypeCategory::Union;
				v.arraylengths.clear();
				for (const std::string& size : v.arraysizes) v.arraylengths.push_back(resolveArrayLength(size));
			}
			for (size_t i = 0; i < vars.size(); ++i) {
				if (vars[i].lens.empty() || !isIdentifier(vars[i].lens.front())) continue;
				for (VulkanVariableSpec& sibling : vars)
					if (sibling.name == vars[i].lens.front()) addUnique(sibling.lengthfor, vars[i].name);
			}
		}

		void linkStructExtends() {
			for (auto& [name, s] : m_ctx.structspecs)
				std::erase_if(s.structextends, [this](const std::string& target) {
					return m_ctx.structspecs.count(resolveAlias(m_typeAliases, target)) == 0;
				});
			for (const auto& [name, s] : m_ctx.structspecs)
				for (const std::string& target : s.structextends)
					if (const auto it = m_ctx.structspecs.find(resolveAlias(m_typeAliases, target)); it != m_ctx.structspecs.end())
						addUnique(it->second.extendedby, name);
			for (auto& [name, s] : m_ctx.structspecs) std::sort(s.extendedby.begin(), s.extendedby.end());
		}

		void resolveEnumValueAliases() {
			for (auto& [name, e] : m_ctx.enumtypespecs) {
				for (VulkanEnumValueSpec& v : e.values) {
					std::string target = v.aliasof;
					for (int guard = 0; guard < 16 && !target.empty(); ++guard) {
						const auto index = m_valueIndex.find(target);
						if (index == m_valueIndex.end()) break;
						const auto owner = m_ctx.enumtypespecs.find(index->second.first);
						if (owner == m_ctx.enumtypespecs.end()) break;
						const VulkanEnumValueSpec& t = owner->second.values[index->second.second];
						if (t.aliasof.empty()) {
							v.value = t.value;
							v.bitpos = t.bitpos;
							break;
						}
						target = t.aliasof;
					}
				}
			}
		}

		void resolveConstantAliases() {
			for (auto& [name, c] : m_ctx.constantspecs) {
				std::string target = c.aliasof;
				for (int guard = 0; guard < 16 && !target.empty(); ++guard) {
					const auto it = m_ctx.constantspecs.find(target);
					if (it == m_ctx.constantspecs.end()) break;
					if (it->second.aliasof.empty()) {
						c.type = it->second.type;
						c.value = it->second.value;
						break;
					}
					target = it->second.aliasof;
				}
			}
		}

		template <typename Spec>
		void applyMeta(const std::string& name, Spec& spec) const {
			if (const auto it = m_providers.find(name); it != m_providers.end()) spec.providedby = it->second;
			else spec.providedby.clear();
			if (const auto it = m_deprecations.find(name); it != m_deprecations.end()) spec.deprecation = it->second;
			else spec.deprecation = {};
		}

		template <typename Map>
		void applyMeta(Map& map) const {
			for (auto& [name, spec] : map) applyMeta(name, spec);
		}

		void applyMeta() {
			applyMeta(m_ctx.structspecs);
			applyMeta(m_ctx.handlespecs);
			applyMeta(m_ctx.enumtypespecs);
			applyMeta(m_ctx.bitmaskspecs);
			applyMeta(m_ctx.funcpointerspecs);
			applyMeta(m_ctx.basetypespecs);
			applyMeta(m_ctx.definespecs);
			applyMeta(m_ctx.constantspecs);
			applyMeta(m_ctx.commandspecs);

			for (auto& [name, e] : m_ctx.enumtypespecs)
				for (VulkanEnumValueSpec& v : e.values) {
					const auto it = m_providers.find(v.name);
					v.providedby = it != m_providers.end() ? it->second : e.providedby;
				}
		}

		template <typename Map>
		void copyAlias(Map& map, const std::string& alias, const std::string& target) {
			const auto it = map.find(target);
			if (it == map.end()) return;
			auto copy = it->second;
			copy.name = alias;
			copy.aliasof = target;
			applyMeta(alias, copy);
			map.insert_or_assign(alias, std::move(copy));
		}

		void createAliasEntries() {
			for (const auto& [alias, declared] : m_typeAliases) {
				if (!m_requiredTypes.count(alias)) continue;
				const std::string target = resolveAlias(m_typeAliases, alias);
				switch (m_categories[alias]) {
				case VulkanTypeCategory::Struct:
				case VulkanTypeCategory::Union:       copyAlias(m_ctx.structspecs, alias, target); break;
				case VulkanTypeCategory::Handle:      copyAlias(m_ctx.handlespecs, alias, target); break;
				case VulkanTypeCategory::Enum:        copyAlias(m_ctx.enumtypespecs, alias, target); break;
				case VulkanTypeCategory::Bitmask:     copyAlias(m_ctx.bitmaskspecs, alias, target); break;
				case VulkanTypeCategory::FuncPointer: copyAlias(m_ctx.funcpointerspecs, alias, target); break;
				case VulkanTypeCategory::BaseType:    copyAlias(m_ctx.basetypespecs, alias, target); break;
				case VulkanTypeCategory::Define:      copyAlias(m_ctx.definespecs, alias, target); break;
				default: break;
				}
			}
			for (const auto& [alias, declared] : m_commandAliases)
				if (m_requiredCommands.count(alias)) copyAlias(m_ctx.commandspecs, alias, resolveAlias(m_commandAliases, alias));
		}

		void buildLookups() {
			for (const auto& [name, e] : m_ctx.enumtypespecs) {
				m_ctx.enumspecs.insert(name);
				m_ctx.typespecs.insert(name);
				if (e.isAlias()) continue;
				for (const VulkanEnumValueSpec& v : e.values) {
					m_ctx.enumspecs.insert(v.name);
					m_ctx.enumvalueowners[v.name] = name;
				}
			}
			for (const auto& [name, c] : m_ctx.constantspecs) m_ctx.enumspecs.insert(name);
			for (const auto& [name, s] : m_ctx.structspecs)   m_ctx.typespecs.insert(name);
			for (const auto& [name, h] : m_ctx.handlespecs)   m_ctx.typespecs.insert(name);
			for (const auto& [name, c] : m_ctx.commandspecs)  m_ctx.typespecs.insert(name);
		}
	};
}

namespace VKA::PARSER::XML {
	std::string XMLParsing::findXMLPath() {
		const char* sdkPathEnv = std::getenv("VULKAN_SDK");
		if (!sdkPathEnv) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "Failed to find VULKAN_SDK! Is VULKAN_SDK installed ? ");
			return "";
		}

		fs::path sdkDir(sdkPathEnv);
		std::vector<fs::path> possiblePaths{
			sdkDir / "share" / "vulkan" / "registry" / "vk.xml",
			sdkDir / "explicit_layer.d" / ".." / "registry" / "vk.xml",
			sdkDir / "Config" / "vk.xml",
			sdkDir / "xml" / "vk.xml"
		};

		for (const auto& path : possiblePaths) {
			if (fs::exists(path)) {
				return path.string();
			}
		}

		try {
			for (const auto& entry : fs::recursive_directory_iterator(sdkDir)) {
				if (entry.is_regular_file() && entry.path().stem() == "vk" && entry.path().extension() == ".xml") {
					return entry.path().string();
				}
			}
		}
		catch (const std::exception& e) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), e.what());
		}

		VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "VULKAN_SDK found but vk.xml was not detected within it");
		return "";
	}

	bool XMLParsing::initAndParse(VKA::DATA::GraphContext& graphcontext) {
		std::string xmlpath = findXMLPath();
		if (xmlpath.empty()) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "XML FILE IS EMPTY");
			return false;
		}
		return parseFile(xmlpath, graphcontext);
	}

	bool XMLParsing::parseFile(const std::string& xmlpath, VKA::DATA::GraphContext& graphcontext) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(xmlpath.c_str(), pugi::parse_default | pugi::parse_ws_pcdata);
		if (!result) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "XML LOAD FAIL: " << result.description());
			return false;
		}

		pugi::xml_node registry = doc.child("registry");
		if (!registry) {
			VKA_ERROR(ErrorTypeToString(ErrorType::VKA_XML_PARSER_ERROR), "There is no <registry> tag!");
			return false;
		}

		RegistryParser parser(graphcontext);
		parser.parse(registry);

		VKA_DEBUG_MSG("Vulkan Specification is loaded to memory: "
			<< graphcontext.commandspecs.size() << " commands, "
			<< graphcontext.structspecs.size() << " structs/unions, "
			<< graphcontext.handlespecs.size() << " handles, "
			<< graphcontext.enumtypespecs.size() << " enums, "
			<< graphcontext.enumvalueowners.size() << " enum values, "
			<< graphcontext.bitmaskspecs.size() << " bitmasks, "
			<< graphcontext.funcpointerspecs.size() << " funcpointers, "
			<< graphcontext.constantspecs.size() << " constants, "
			<< graphcontext.featurespecs.size() << " versions/extensions");
		return true;
	}
} // VKA::PARSER::XML
