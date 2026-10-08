extends Node

func _ready() -> void:
	var node := TestNode.new()
	print("TestNode.add(2, 3) = ", node.add(2, 3))
	print(node.get_hello())
	node.free()
	get_tree().quit(0)