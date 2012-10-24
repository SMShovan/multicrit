#include "LabelSet.hpp"
#include <iostream>
#include <algorithm>

struct Label {
	Label(int first, int second): first_weight(first), second_weight(second) {};
	typedef unsigned int value_type;
	value_type first_weight;
	value_type second_weight;
	bool operator==(const Label& other ) const {
		return first_weight == other.first_weight && second_weight == other.second_weight;
	}
};

void assertTrue(bool cond, std::string msg) {
	if (!cond) {
		std::cout << "FAILED: " << msg << std::endl;
		exit(-1);
	}
}

bool contains(const LabelSet<Label> set, const Label label) {
	return std::find(set.begin(), set.end(), label) != set.end();
}

int main() {
	LabelSet<Label> set;
	assertTrue(set.size() == 0, "Should be empty");

	Label label = Label(1,10);
	set.add(label);
	assertTrue(set.size() == 1, "Should only contain single label");
	assertTrue(contains(set, label), "Should contain single label");

	set.add(Label(10,1));
	assertTrue(set.size() == 2, "Should have added non-dominated label");

	set.add(Label(10,1));
	assertTrue(set.size() == 2, "Should not have added duplicated label");

	set.add(Label(5,5));
	assertTrue(set.size() == 3, "Should have fitted non-dominated into sequence");

	set.add(Label(5,6));
	assertTrue(set.size() == 3, "Should not have added dominated label (x-coord conflict)");

	set.add(Label(6,6));
	assertTrue(set.size() == 3, "Should not have added dominated label (in field)");

	set.add(Label(4,6));
	assertTrue(set.size() == 4, "Should have fitted non-dominated into sequence");

	set.add(Label(8,4));
	assertTrue(set.size() == 5, "Should have fitted non-dominated into sequence");

	set.add(Label(7,3));
	assertTrue(set.size() == 5, "Should have replaced label");
	assertTrue(contains(set, Label(7,3)), "Should have added new label");
	assertTrue(!contains(set, Label(8,4)), "Should have removed dominated label");

	set.add(Label(4,3));
	assertTrue(set.size() == 3, "Should have replaced range of dominated labels");
	assertTrue(contains(set, Label(4,3)), "Label should have remained");
	assertTrue(contains(set, Label(10,1)), "Label should have remained");
	assertTrue(contains(set, Label(1,10)), "Label should have remained");



	set = LabelSet<Label>();
	assertTrue(set.size() == 0, "Should be empty");

	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");

	set.add(Label(5,5));
	assertTrue(set.size() == 1, "Should have removed dominated label with x-coord conflict");
	assertTrue(contains(set, Label(5,5)), "Label should have remained");



	set = LabelSet<Label>();
	assertTrue(set.size() == 0, "Should be empty");

	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");

	set.add(Label(4,6));
	assertTrue(set.size() == 1, "Should have removed dominated label with y-coord conflict");
	assertTrue(contains(set, Label(4,6)), "Label should have remained");



	set = LabelSet<Label>();
	assertTrue(set.size() == 0, "Should be empty");

	set.add(Label(4,6));
	assertTrue(set.size() == 1, "Should have fitted non-dominated into sequence");

	set.add(Label(5,6));
	assertTrue(set.size() == 1, "Should not have added dominated label with y-coord conflict");
	assertTrue(contains(set, Label(4,6)), "Original label should have remained");


	std::cout << "Tests passed successfully." << std::endl;
	return 0;
}