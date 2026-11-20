#include "VKAResource.hpp"

namespace VKA::PARSER::RESOURCE {
	std::vector<VKA::DATA::ASTNode> VKAResource::collectVariables(VKA::DATA::ASTTree tree) {
		cursor = 0;
		source = tree.astnodes;
		while (cursor < source.size()) {
			VKA::DATA::ASTNode currentnode = tree.astnodes[cursor];

			if (currentnode.type == VKA::DATA::ASTNodeType::VariableDecl) {
				variabletable.push_back(currentnode);
				advance();
			}
			else {
				continue;
			}
		}

		return variabletable;
	}

	VKA::DATA::ASTNode VKAResource::advance() {
		VKA::DATA::ASTNode node = source[cursor];
		cursor++;
		return node;
	}
}