#include <iostream>
#include <type_traits>

using std::cout;
using std::endl;

class Foo {

};

class Foo2 {
	
};

int main(void) {
	cout << "Hello world\n";
	bool val = std::is_pod<int>::value; // Is int a Plain Old Datatype? -> std::is_pod is implemented by the compiler.
	bool val2 = std::is_pod<Foo>::value; // Bool is also a Plain Old Datatype -- What distinguishes between a POD and an object?
	cout << std::boolalpha; // Tell the output stream to print booleans as true/false as opposed to 1/0
	cout << val << endl; // True
	cout << val2 << endl; // True
}