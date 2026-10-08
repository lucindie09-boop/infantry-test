#ifndef INFANTRY_TEST_NODE_HPP
#define INFANTRY_TEST_NODE_HPP

#include <godot_cpp/classes/node.hpp>

namespace godot {

class TestNode : public Node {
	GDCLASS(TestNode, Node)

protected:
	static void _bind_methods();

public:
	int add(int a, int b) const;
	String get_hello() const;
};

} // namespace godot

#endif // INFANTRY_TEST_NODE_HPP