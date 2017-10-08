#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <cstdint>
#include <stdexcept>
#include <utility>

//Utility gives std::rel_ops which will fill in relational
//iterator operations so long as you provide the
//operators discussed in class.  In any case, ensure that
//all operations listed in this website are legal for your
//iterators:
//http://www.cplusplus.com/reference/iterator/RandomAccessIterator/
using namespace std::rel_ops;

namespace epl{

class invalid_iterator {
    public:
    enum SeverityLevel {SEVERE,MODERATE,MILD,WARNING};
    SeverityLevel level;    

    invalid_iterator(SeverityLevel level = SEVERE){ this->level = level; }
    virtual const char* what() const {
    switch(level){
      case WARNING:   return "Warning"; // not used in Spring 2015
      case MILD:      return "Mild";
      case MODERATE:  return "Moderate";
      case SEVERE:    return "Severe";
      default:        return "ERROR"; // should not be used
    }
    }
};

template <typename T>
class vector{
    //////////////////////////
    // Private fields
    //////////////////////////
private:
    uint64_t length; // current size of internal array
    uint64_t capacity; // capacity of internal array
    uint64_t front; // index where circular array begins
    T* data; // Internal array
    uint64_t version_mild; // Mild exception version #
    uint64_t version_moderate; // Moderate exception version #
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
        init_version_num();
    }

    // Explicit size constructor
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
        init_version_num();
    }

    // Explicit initializer list constructor
    vector(std::initializer_list<T> l) {
        alloc(l.size());
        init_version_num();
        for (T obj : l) { push_back(obj); }
    }

    // Explicit iterator constructor
    template <typename It>
    vector(It begin, It end) {
        // typename std::iterator_traits<It>::iterator_category category();
        init_version_num();
        // make_from_iterator(begin, end, category);
        alloc(default_capacity);
        while (begin != end) {
            this->push_back(*begin);
            begin++;
        }
    }

    /* Vector constructor from random-access iterators */
    template <typename It>
    void make_from_iterator(It begin, It end, std::random_access_iterator_tag) {
        uint64_t n = end - begin;
        alloc(n); // Allocates in one go
        while (begin != end) {
            this->push_back(*begin);
            begin++;
        }

    }

    template <typename It>
    void make_from_iterator(It begin, It end, std::forward_iterator_tag) {
        alloc(default_capacity);
        while (begin != end) {
            this->push_back(*begin);
            begin++;
        }
    }

    // Copy constructor - copy from other object
    vector(const vector<T>& that) { 
        copy(that); 
        init_version_num();
    }

    // Move constructor - shallow copy other vector's pointers,
    // and null out pointers in the other vector.
    vector(vector<T>&& that)
        : length(that.length), capacity(that.capacity),
        front(that.front), data(that.data)//, version_mild(that.version_mild),
        //version_moderate(that.version_moderate)
    {
        init_version_num();
        that.length = 0;
        that.capacity = 0;
        that.front = 0;
        that.data = nullptr;
        that.version_mild = 0;
        that.version_moderate = 0;
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
            version_mild++;
            version_moderate++;
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
            this->front = rhs.front;
            this->data = rhs.data;
            this->version_mild++;
            this->version_moderate++;

            // Null out rhs
            rhs.length = 0;
            rhs.capacity = 0;
            rhs.front = 0;
            rhs.data = nullptr;
            rhs.version_mild = 0;
            rhs.version_moderate = 0;
        }
        return *this;
    }

    // vector index operator
    T& operator[](uint64_t k) {
        if (k >= length) { throw std::out_of_range("vector access out of bounds"); }
        else { return data[(k + front) % capacity]; }
    }

    // constant vector index operator
    const T& operator[](uint64_t k) const {
        if (k >= length) { throw std::out_of_range("vector access out of bounds"); }
        else { return data[(k + front) % capacity]; }
    }

    //////////////////////////
    // Public Functions
    //////////////////////////

    // Accessor - returns the number of constructed objects in the vector
    uint64_t size(void) const { return length; }

    // Mutator - adds a new value to the next available free space. If there isn't
    // enough space left, double the size of the array
    
    //Illustration:
    //  |--obj1--|
    //  |--obj2--|
    //  |--obj3--|
    //  |--obj4--|
    //  |--FREE--| <- INSERT NEXT OBJECT HERE
    //  |--FREE--|
    //  |--FREE--|
    //  |--FREE--|
    
    void push_back(const T& that) {
        version_mild++;
        // Double the array's capacity
        if (length == capacity) {
            // Reallocate the buffer
            std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
            // Insert the element
            new (&(this->data[(front + length) % capacity])) T{that};
            // Copy items from old buffer to new buffer
            resize_move(tup);
        }
        else {
            // Insert element via copy construction and placement new
            new (&(this->data[(front + length) % capacity])) T{that};
        }
        length += 1;
    }

    void push_back(T&& that) {
        version_mild++;
        // Perform amortized doubling for array
        if (length == capacity) {
            // Reallocate the buffer
            std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
            // Insert element via move construction and placement new
            new (&(this->data[(front + length) % capacity])) T{ std::move(that) };
            // Copy items from old buffer to new buffer
            resize_move(tup);
        }
        else {
            // Insert element via copy construction and placement new
            new (&(this->data[(front + length) % capacity])) T{ std::move(that) };
        }
        length += 1;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        version_mild++;
        // Perform amortized doubling for array
        if (length == capacity) {
            // Reallocate the buffer
            std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
            // Insert element via move construction and placement new
            new (&(this->data[(front + length) % capacity])) T{ std::forward<Args>(args)... };
            // Copy items from old buffer to new buffer
            resize_move(tup);
        }
        else {
            // Insert element via copy construction and placement new
            new (&(this->data[(front + length) % capacity])) T{ std::forward<Args>(args)... };
        }
        length += 1;
    }

    // Mutator - adds an element at beginning of vector. Actual implementation
    // will add the element to the next available space beginning from the end
    // of the array.

    //Illustration:
    //  |--obj2--|
    //  |--obj3--|
    //  |--obj4--|
    //  |--obj5--|
    //  |--FREE--| 
    //  |--FREE--|
    //  |--FREE--| <- INSERT NEXT OBJECT HERE
    //  |--obj1--| 
    void push_front(const T& that) {
        version_mild++;
        if (length == capacity) {
            // Reallocate the buffer
            std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
            // Insert the element
            front = (front - 1) % capacity;
            new (&(this->data[front])) T{that};
            // Copy items from old buffer to new buffer
            resize_move(tup);
        }
        else {
            front = (front - 1) % capacity;
            new (&(this->data[front])) T{that};
        }
        length += 1;
    }

    // Same as above, but move constructs the argument
    void push_front(T&& that) {
        version_mild++;
        if (length == capacity) {
            // Reallocate the buffer
            std::tuple<T*, uint64_t, uint64_t> tup = resize_double();
            // Insert the element
            front = (front - 1) % capacity;
            new (&(this->data[front])) T{ std::move(that) };
            // Copy items from old buffer to new buffer
            resize_move(tup);
        }
        else {
            front = (front - 1) % capacity;
            new (&(this->data[front])) T{ std::move(that) };
        }
        length += 1;
    }

    // Mutator - removes the value at the end of the circular array.
    void pop_back(void) {
        version_mild++;
        if (length > 0) {
            (&data[(front + length - 1) % capacity])->~T();
            length -= 1;
        }
        else {
            throw std::out_of_range("Vector has no more elements!");
        }
    }

    // Mutator - removes the value at the head of the circular array.
    void pop_front(void) {
        version_mild++;
        if (length > 0) {
            (&data[front])->~T();
            length -= 1;
            // shift head of circular array forward
            front = (front + 1) % capacity;
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
        front = 0;
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
        front = 0;
        capacity = init_cap;
        data = reinterpret_cast<T*>(::operator new(init_cap * sizeof(T)));
    }

    /* Doubles the vector's capacity 
     * Part 1 of amortized doubling implementation */
    std::tuple<T*, uint64_t, uint64_t> resize_double(void) {
        // Back up old values
        T* old_storage = this->data;
        uint64_t old_cap = this->capacity;
        uint64_t old_front = this->front;

        capacity *= 2; // Double size of the array
        alloc(capacity, length);
        version_moderate++;

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

    /* Initialize the version numbers
       for the vector. Used for sanity checks
       between a vector and its iterator. */
    void init_version_num() {
        version_mild = 42;
        version_moderate = 42;
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
        this->front = that.front;
        // use placement new to adhere to Phase B requirements
        // instead of assignment operator.
        for (uint64_t k = 0; k < that.length; k += 1) {
            new (&data[(k + front) % capacity]) T(that[k]);
        }
    }
    
    /* This function will delete any existing objects living
     * in the buffer, and also deallocate the buffer, assuming
     * that the buffer has already been allocated.
     */
    void destroy() {
        // Completely release existing memory
        for (int k = 0; k < length; k++) {
            (&data[(k + front) % capacity])->~T();
        }
        // Deallocate existing buffer
        ::operator delete(this->data);
    }

    vector<T>* self = this;

    //////////////////////////
    // Iterator Code
    //////////////////////////

    // Compile time constant: whether iterator is constant or not.
    template <bool is_const_iter = true>
    class vec_iterator
        : public std::iterator<std::random_access_iterator_tag, T>
    {
        // Type: Pointer to a vector
        typedef typename std::conditional<is_const_iter, const vector<T>*, vector<T>*>::type vector_ptr_type;
        // Type: Reference to a value in vector
        typedef typename std::conditional<is_const_iter, const T&, T&>::type value_ref_type;
        // Type: value in a vector
        typedef typename std::conditional<is_const_iter, const T, T>::type value_type;

        //////////////////////////
        // Public Methods
        //////////////////////////

        /**
         * Default constructor.
         */
    public:
        vec_iterator()
            : m_vector_ptr(nullptr), m_index(0), 
            m_version_mild(42), m_version_moderate(42) {}
        
        /**
         * Constructor that takes in a pointer to a vector
         */
        vec_iterator(vector_ptr_type vector_ptr,
            uint64_t version_mild, uint64_t version_moderate)
        : m_vector_ptr(vector_ptr), m_index(42), 
        m_version_mild(version_mild), 
        m_version_moderate(version_moderate) {}

        /**
         * Constructor that takes in a pointer from vector, and an index. 
         */
        vec_iterator(vector_ptr_type vector_ptr, uint64_t index,
            uint64_t version_mild, uint64_t version_moderate)
        : m_vector_ptr(vector_ptr),
          m_index(index), m_version_mild(version_mild), 
          m_version_moderate(version_moderate) {}
 
        /**
         * Copy constructor. 
         * Allows for implicit conversion from a regular iterator to a const_iterator,
         * but not the other way around.
         */
        vec_iterator(const vec_iterator<false>& other)
            : m_vector_ptr(other.m_vector_ptr), m_index(other.m_index), 
            m_version_mild(other.m_version_mild), 
            m_version_moderate(other.m_version_moderate) {}

        /**
         * Copy constructor. 
         * Allows for construction of a const_iterator from a const_iterator,
         * but not construction of iterator from const_Iterator.
         * Try SFINAE enable_if stuff
         */
        vec_iterator(const vec_iterator<true>& other)
            : m_vector_ptr(other.m_vector_ptr), m_index(other.m_index), 
            m_version_mild(other.m_version_mild), 
            m_version_moderate(other.m_version_moderate) {}

/*        void operator=(const vec_iterator& rhs) {
            m_vector_ptr = rhs.vector_ptr;
            m_index = rhs.m_index;
            m_version_mild = rhs.m_version_mild;
            m_version_moderate = rhs.m_version_moderate;
        }*/

        value_ref_type operator*() { 
            is_valid();
            return (*m_vector_ptr)[m_index]; 
        }

        value_ref_type operator->() { return operator*(); }

        // Equality operator
        bool operator==(const vec_iterator& rhs) {
            is_valid();
            return m_index == rhs.m_index;
        }

        // Inequality operator
        bool operator!=(const vec_iterator& rhs) {
            is_valid();
            return !operator==(rhs);
        }

        // Greater than operator
        bool operator>(const vec_iterator& rhs) {
            is_valid();
            return m_index > rhs.m_index;
        }

        // Less than operator
        bool operator<(const vec_iterator& rhs) {
            is_valid();
            return m_index < rhs.m_index;
        }

        // Less than and equal operator
        bool operator<=(const vec_iterator& rhs) {
            is_valid();
            return !operator>(rhs);
        }

        // Less than and equal operator
        bool operator>=(const vec_iterator& rhs) {
            is_valid();
            return !operator<(rhs);
        }

        // Addition with integer
        vec_iterator operator+(uint64_t n) {
            is_valid();
            return vec_iterator(m_vector_ptr, m_index + n, m_version_mild, m_version_moderate);
        }

        vec_iterator& operator+=(uint64_t n) {
            is_valid();
            m_index += n;
            return *this;
        }

        // Subtraction with integer
        vec_iterator operator-(uint64_t n) {
            is_valid();
            return vec_iterator(m_vector_ptr, m_index - n, m_version_mild, m_version_moderate);
        }

        vec_iterator operator-=(uint64_t n) {
            is_valid();
            if (m_index < n) {} // Throw exception?
            m_index -= n;
            return *this;
        }

        // Subtraction with iterator
        uint64_t operator-(const vec_iterator& rhs) {
            is_valid();
            return m_index - rhs.m_index;
        }

        // Prefix increment
        vec_iterator& operator++() {
            is_valid();
            ++m_index;
            if (m_index > m_vector_ptr->length) {	
                throw invalid_iterator(invalid_iterator::MILD); 
            }
            return *this;
        }

        // Postfix increment
        vec_iterator& operator++(int) {
            is_valid();
            ++m_index;
            if (m_index > m_vector_ptr->length) {   
                throw invalid_iterator(invalid_iterator::MILD); 
            }
            return *this;
        }

        // Postfix decrement
        vec_iterator& operator--() {
            is_valid();
            --m_index;
            return *this;
        }

        // Postfix decrement
        vec_iterator& operator--(int) {
            is_valid();
            --m_index;
            return *this;
        }


        // One way conversion: iterator -> const_iterator

        /*template <bool is_const_iter = true>
        class vec_iterator
        : public std::iterator<std::random_access_iterator_tag, T> */

        operator vec_iterator <false>() const
        {
            return vec_iterator<true>(m_vector_ptr, 
                m_index, m_version_mild, m_version_moderate);
        }



        // value_ref_type operator=(value_ref_type rhs) {
        //     // Destruct existing object,
        //     // replace with new object :)
        //     &((self->data)[(m_index + self->front) % self->capacity])->~T();
            
        // }


    private:
        //////////////////////////
        // Private Helpers
        //////////////////////////

        /**
         * Check if the iterator is valid.
         * If not, then throw an appropriate exception.
         */
        void is_valid() {
            // TODO:
            // Many of the warnings are placeholders right now.

            // iterator out of bounds or vector ptr is invalid
            if (m_vector_ptr == nullptr || m_index < 0
                || m_index > m_vector_ptr->length) { // Iterator out of bounds (incremented too much)
                throw invalid_iterator(invalid_iterator::SEVERE);
            } 
            else if (m_version_moderate != m_vector_ptr->version_moderate) {
                throw invalid_iterator(invalid_iterator::MODERATE);
            }
            else if (m_version_mild != m_vector_ptr->version_mild){
                throw invalid_iterator(invalid_iterator::MILD);
            }
        }

        //////////////////////////
        // Private Fields
        //////////////////////////
        vector_ptr_type m_vector_ptr; // Member pointer to vector
        uint64_t m_index; // Index to iterator
        uint64_t m_version_mild; // Version # of iterator, used for mild exception checks
        uint64_t m_version_moderate; // Version # of iterator, used for moderate exception checks
        friend vector;
    };

public:
    /**
     * Shorthand for a regular iterator (non-const) for vector.
     */
    typedef vec_iterator<false> iterator;
 
    /**
     * Shorthand for a constant iterator (const_iterator) for vector.
     */
    typedef vec_iterator<true> const_iterator;

    friend iterator;
    friend const_iterator;

    //////////////////////////
    // Iterator Functions
    //////////////////////////
    iterator begin() { return iterator(self, 0, version_mild, version_moderate); }
    iterator end() { return iterator(self, length, version_mild, version_moderate); }
    const_iterator begin() const { return const_iterator(self, 0, version_mild, version_moderate); }
    const_iterator end() const { return const_iterator(self, length, version_mild, version_moderate); }
};

} //namespace epl

#endif
