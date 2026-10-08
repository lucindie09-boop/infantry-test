#include "test_node.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

using namespace godot;

void TestNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add", "a", "b"), &TestNode::add);
	ClassDB::bind_method(D_METHOD("get_hello"), &TestNode::get_hello);
}

int TestNode::add(int a, int b) const {
	return a + b;
}

String TestNode::get_hello() const {
	return "Hello from TestNode (C++ GDExtension)!";
}