// Vector.h -- header file for Vector data structure project

#pragma once
#ifndef _Vector_h
#define _Vector_h
#include <cstdint>
// #define True_Index(index) (index + this->front_index) % this->capacity

namespace epl{



template <typename T>
class vector {
	// Need to implement const and non-const versions of each function:
	// Copy constructor, move constructor, copy assignment operator, move assignment operator
	// Move semantics.

	//////////////////////////
	// Private fields
	//////////////////////////
private:
	uint64_t length; // current size of internal array
	uint64_t capacity; // capacity of internal array
	uint64_t front_index; // index where circular array begins
	T* data; // Internal array
	static constexpr uint64_t default_capacity = 8;

public:

	//////////////////////////
	// Constructors and Destructors
	//////////////////////////

	// Default constructor
	vector(void) {
		// ::operator new means that it comes from global namespace
		// We are allocating space without initializing objects here.
		alloc(default_capacity);
	}

	// Explicit constructor
	vector(uint64_t n) {
		// Use default constructor if user tries to init vector w/ 0 elements.
		// This guarantees that our invariant of O(1)* inserts is maintained.

		if (n == 0) {
			vector();
		}
		else {
			alloc(n);
			init(n);
		}	
	}

	// Copy constructor - copy from other object
	// and initialize new object
	vector(const vector<T>& that) { copy(that); }

	// Move constructor - shallow copy other vector's pointers,
	// and null out pointers in the other vector.
	vector(vector<T>&& that) 
		: length(that.length), capacity(that.capacity),
		front_index(that.front_index), data(that.data)
	{
		that.length = 0;
		that.capacity = 0;
		that.front_index = 0;
		that.data = nullptr;
	}

	// Destructor
	~vector(void) {
		destroy();
	}

	//////////////////////////
	// Public Operators
	//////////////////////////

	// Copy assignment operator - 
	// delete contents of current object, and deep copy
	// contents of rhs object into this object.
	vector<T>& operator=(const vector<T>& rhs) {
		if (this != &rhs) { // protect against "x = x";
			this->destroy();
			this->copy(rhs);
		}
		return *this;
	}

	// Copy assignment operator w/ move semantics -
	// delete contents of current object, and shallow copy
	// other object into current object.
	vector<T>& operator=(vector<T>&& rhs) {
		if (this != &rhs) { // protect against "x = x";
			this->destroy();

			// Shallow copy
			this->capacity = rhs.capacity;
			this->length = rhs.length;
			this->front_index = rhs.front_index;
			this->data = rhs.data;

			// Null out rhs
			rhs.length = 0;
			rhs.capacity = 0;
			rhs.front_index = 0;
			rhs.data = nullptr;
		}
		return *this;
	}

	// vector index operator
	T& operator[](uint64_t k) {
		if (k >= length) { throw std::out_of_range("vector access out of bounds"); }
		else { return data[(k + front_index) % capacity]; }
	}

	// constant vector index operator
	const T& operator[](uint64_t k) const {
		if (k >= length) { throw std::out_of_range("vector access out of bounds"); }
		else { return data[(k + front_index) % capacity]; }
	}

	//////////////////////////
	// Public Functions
	//////////////////////////

	// Accessor - returns the number of constructed objects in the vector
	uint64_t size(void) const { return length; }

	// Mutator - adds a new value to the next available free space. If there isn't
	// enough space left, double the size of the array
	
	//Illustration:
	//	|--obj1--|
	//	|--obj2--|
	//	|--obj3--|
	//	|--obj4--|
	//	|--FREE--| <- INSERT NEXT OBJECT HERE
	//	|--FREE--|
	//	|--FREE--|
	//	|--FREE--|
	
	void push_back(const T& that) {
		// Double the array's capacity
		if (length == capacity) {
			// Reallocate the buffer
			std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
			// Insert the element
			new (&(this->data[(front_index + length) % capacity])) T{that};
			// Copy items from old buffer to new buffer
			resize_move(tup);
		}
		else {
			// Insert element via copy construction and placement new
			new (&(this->data[(front_index + length) % capacity])) T{that};
		}
		length += 1;
	}

	void push_back(T&& that) {
		// Perform amortized doubling for array
		if (length == capacity) {
			// Reallocate the buffer
			std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
			// Insert element via move construction and placement new
			new (&(this->data[(front_index + length) % capacity])) T{ std::move(that) };
			// Copy items from old buffer to new buffer
			resize_move(tup);
		}
		else {
			// Insert element via copy construction and placement new
			new (&(this->data[(front_index + length) % capacity])) T{ std::move(that) };
		}
		length += 1;
	}

	// Mutator - adds an element at beginning of vector. Actual implementation
	// will add the element to the next available space beginning from the end
	// of the array.

	//Illustration:
	//	|--obj2--|
	//	|--obj3--|
	//	|--obj4--|
	//	|--obj5--|
	//	|--FREE--| 
	//	|--FREE--|
	//	|--FREE--| <- INSERT NEXT OBJECT HERE
	//	|--obj1--| 
	void push_front(const T& that) {
		if (length == capacity) {
			// Reallocate the buffer
			std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
			// Insert the element
			front_index = (front_index - 1) % capacity;
			new (&(this->data[front_index])) T{that};
			// Copy items from old buffer to new buffer
			resize_move(tup);
		}
		else {
			front_index = (front_index - 1) % capacity;
			new (&(this->data[front_index])) T{that};
		}
		length += 1;
	}

	// Same as above, but move constructs the argument
	void push_front(T&& that) {
		if (length == capacity) {
			// Reallocate the buffer
			std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
			// Insert the element
			front_index = (front_index - 1) % capacity;
			new (&(this->data[front_index])) T{ std::move(that) };
			// Copy items from old buffer to new buffer
			resize_move(tup);
		}
		else {
			front_index = (front_index - 1) % capacity;
			new (&(this->data[front_index])) T{ std::move(that) };
		}
		length += 1;
	}

	// Mutator - removes the value at the end of the circular array.
	void pop_back(void) {
		if (length > 0) {
			(&data[(front_index + length - 1) % capacity])->~T();
			length -= 1;
		}
		else {
			throw std::out_of_range("Vector has no more elements!");
		}
	}

	// Mutator - removes the value at the head of the circular array.
	void pop_front(void) {
		if (length > 0) {
			(&data[front_index])->~T();
			length -= 1;
			// shift head of circular array forward
			front_index = (front_index + 1) % capacity;
		}
		else {
			throw std::out_of_range("Vector has no more elements!");
		}
	}

	//////////////////////////
	// Private Helper Functions
	//////////////////////////
private:
	/* allocates memory for the internal buffer,
	 * without constructing objects in place.
	 * init_cap - initial capacity of vector
	 */
	void alloc(uint64_t init_cap) {
		length = 0;
		front_index = 0;
		capacity = init_cap;
		data = static_cast<T*>(::operator new(init_cap * sizeof(T)));
	}

	/* allocates memory for the vector
	 * without constructing objects in place.
	 * init_cap - initial capacity of vector
	 * init_length - initial length of vector
	 */
	void alloc(uint64_t init_cap, uint64_t init_length) {
		length = init_length;
		front_index = 0;
		capacity = init_cap;
		data = reinterpret_cast<T*>(::operator new(init_cap * sizeof(T)));
	}

	/* Doubles the vector's capacity 
	 * Part 1 of amortized doubling implementation */
	std::tuple<T*, uint64_t, uint64_t> resize_double(void) {
		// Back up old values
		T* old_storage = this->data;
		uint64_t old_cap = this->capacity;
		uint64_t old_front = this->front_index;

		capacity *= 2; // Double size of the array
		alloc(capacity, length);

		return std::make_tuple(old_storage, old_cap, old_front);
	}

	/* moves all objects from the old buffer to new buffer
	 * Implements amoritzed doubling for the vector (part 2).
	 * Precondition: this->length must not have been incremented yet
	 */
	void resize_move(std::tuple<T*, uint64_t, uint64_t>& tup) {
		// Unpack tuple elements packed by resize_double()
		T* old_data = std::get<0>(tup);
		uint64_t old_cap = std::get<1>(tup);
		uint64_t old_front = std::get<2>(tup);

		// Move elements from old buffer to new one
		for (uint64_t k = 0; k < length; k += 1) {
			// Map any circular array back to an array where the true beginning
			// starts at index 0. This makes life easier!
			new (&(this->data[k])) T(std::move(old_data[(k + old_front) % old_cap]));
		}

		::operator delete(old_data);
	}


	/* initializes memory for the internal buffer,
	 * assuming that memory has already been allocated.
	 */
	void init(uint64_t init_size) {
		length = init_size;
		for (uint64_t i = 0; i < length; i++) {
			new (&data[i]) T{};
		}
	}

	/* this function assumes that internal bufer
	 * is uninitialized, so copy will initialize
	 * this to an exact copy of that */
	void copy(const vector<T>& that) {
		alloc(that.capacity, that.length); // Allocate new memory
		this->front_index = that.front_index;
		// use placement new to adhere to Phase B requirements
		// instead of assignment operator.
		for (uint64_t k = 0; k < that.length; k += 1) {
			new (&data[(k + front_index) % capacity]) T(that[k]);
		}
	}
	
	/* This function will delete any existing objects living
	 * in the buffer, and also deallocate the buffer, assuming
	 * that the buffer has already been allocated.
	 */
	void destroy() {
		// Completely release existing memory
		for (int k = 0; k < length; k++) {
			this->data[(k + front_index) % capacity].~T();
		}
		// Deallocate existing buffer
		::operator delete(this->data);
	}
};

} //namespace epl

#endif /* _Vector_h */
