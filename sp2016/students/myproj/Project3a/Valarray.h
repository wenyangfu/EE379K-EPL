// Valarray.h

/* Put your solution in this file, we expect to be able to use
 * your epl::valarray class by simply saying #include "Valarray.h"
 *
 * We will #include "Vector.h" to get the epl::vector<T> class 
 * before we #include "Valarray.h". You are encouraged to test
 * and develop your class using std::vector<T> as the base class
 * for your epl::valarray<T>
 * you are required to submit your project with epl::vector<T>
 * as the base class for your epl::valarray<T>
 */
#include <vector>
#include <complex>
#include <functional>
#include "Vector.h"
#include <iostream>

#ifndef _Valarray_h
#define _Valarray_h

namespace epl {

using epl::vector;


////////////////////////////////////
// Some Forward Declarations
////////////////////////////////////
template <typename LType, typename RType, typename Op> class BinaryProxy;
template <typename T, typename Op> class UnaryProxy;
template <typename S> class Wrap; // Valarray/Proxy wrapper
template <typename T> class valarray_hidden; // True valarray
template <typename T> class ScalarWrap; // Wrap
// template <typename T> using valarray = Wrap<valarray_hidden<T>>;
template <typename T> using valarray = Wrap<vector<T>>;

template <typename T1, typename T2>
struct choose_type;

template <typename T1, typename T2>
using ChooseType = typename choose_type<T1, T2>::type;

////////////////////////////////////
// Type Promotion
////////////////////////////////////

template <typename>
struct SRank;

// Simple ranks: int, float, double
template <> struct SRank<int> { 
	static constexpr int value = 1;  
	static constexpr bool is_complex = false;
};
template <> struct SRank<float> {
	static constexpr int value = 2;
	static constexpr bool is_complex = false; 
};
template <> struct SRank<double> {
	static constexpr int value = 3;
	static constexpr bool is_complex = false; 
};

template <typename T> struct SRank<std::complex<T>> {
	static constexpr int value = SRank<T>::value;
	static constexpr bool is_complex = true;
};

// template <typename T> struct SRank<valarray_hidden<T>> {
// 	static constexpr int value = SRank<T>::value;
// 	static constexpr bool is_complex = SRank<T>::is_complex;
// };
// template <typename T1, typename T2, typename Op> struct SRank<BinaryProxy<T1, T2, Op>> {
// 	static constexpr int value = SRank<ChooseType<T1, T2>>::value;
// 	static constexpr bool is_complex = SRank<ChooseType<T1, T2>>::is_complex;
// };
// template <typename T> struct SRank<Wrap<T>> {
// 	static constexpr int value = SRank<T>::value;
// 	static constexpr bool is_complex = SRank<T>::is_complex;
// };
// template <typename T> struct SRank<ScalarWrap<T>> {
// 	static constexpr int value = SRank<T>::value;
// 	static constexpr bool is_complex = SRank<T>::is_complex;
// };

template <int, bool>
struct SType;

template <> struct SType<1, false> { using type = int; };
template <> struct SType<2, false> { using type = float; };
template <> struct SType<3, false> { using type = double; };
template <> struct SType<2, true> { using type = std::complex<float>; };
template <> struct SType<3, true> { using type = std::complex<double>; };

template<typename T1, typename T2>
struct choose_type {
	// Figure out what type to promote to.
	static constexpr int t1_rank = SRank<T1>::value;
	static constexpr int t2_rank = SRank<T2>::value;
	static constexpr bool t1_complex = SRank<T1>::is_complex;
	static constexpr bool t2_complex = SRank<T2>::is_complex;

	static constexpr int max_rank = (t1_rank > t2_rank) ? t1_rank : t2_rank;
	static constexpr bool is_complex = t1_complex || t2_complex;

	using type = typename SType<max_rank, is_complex>::type;
};

// template <typename T1, typename T2>
// using ChooseType = typename choose_type<T1, T2>::type;

////////////////////////////////////
// Choose between ref/no reference
////////////////////////////////////
template <typename T> struct choose_ref { using type = T; };

template <typename T> struct choose_ref<vector<T>> {
	using type = vector<T> const&;
};

template <typename T> using ChooseRef = typename choose_ref<T>::type;

//////////////////////////////////////////////////
// Iterator for all proxies. If T is const, then 
// this iterator behaves as a const_iterator.
//////////////////////////////////////////////////
template<typename Proxy, typename T>
class ProxyIterator {
public:
    using iterator = ProxyIterator<Proxy, T>;

    Proxy* proxy;
    uint64_t index;

    ProxyIterator(Proxy* p, int i) : proxy(p), index(i) {}

    // T operator*() { return proxy->operator[](index); }
 
    // Pre-increment operator.
    ProxyIterator& operator++() {
        ++index;
        return *this;
    }

    // Post increment operator.
    ProxyIterator operator++(int) {
        iterator t{*this};
        ++index;
        return t;
    }

    // Pre-decrement operator.
    ProxyIterator& operator--() {
        --index;
        return *this;
    }

    // Post decrement operator.
    ProxyIterator operator--(int) {
        iterator t{*this};
        --index;
        return t;
    }

    // Addition operator.
    ProxyIterator operator+(int64_t n) {
        return iterator{proxy, index + n};
    }

    // Subtraction operator.
    ProxyIterator operator-(int64_t n) {
        return iterator{proxy, index - n};
    }

    ProxyIterator& operator+=(int64_t n) {
        index += n;
        return *this;
    }

    ProxyIterator& operator-=(int64_t n) {
        index -= n;
        return *this;
    }

    // [] operator.
    T operator[](int64_t n) {
        return proxy->operator[](index + n);
    }
 
 	// Equivalence operator.
    bool operator == (const ProxyIterator& rhs) const {
        return this->index == rhs.index ? true : false;
    }

};

/////////////////////////////////////////////////
// Proxy for all binary operations.
// Required for lazy evaluation.
/////////////////////////////////////////////////
template <typename LType, typename RType, typename Op>
class BinaryProxy {
	using LeftType = ChooseRef<LType>;
	using RightType = ChooseRef<RType>;
	LeftType left;
	RightType right;
public:
	using value_type = ChooseType<typename LType::value_type, typename RType::value_type>;

	BinaryProxy(LeftType const& l, RightType const& r) : 
		left(l), right(r) {}
	uint64_t size() const { return std::min(left.size(), right.size()); }

	// Apply operation to a parse tree (Basically what's going on)
	value_type apply_op(uint64_t k, Op op = Op{}) const {
		value_type l = left[k]; // left operand
		value_type r = right[k]; // right operand
		return op(l, r);
	}

	// Apply binary operation to the kth element.
	value_type operator[](uint64_t k) const {
		if (k >= size()) { /* Outta bounds */ }
		return apply_op(k, Op{});
	}

	// TODO: Add iterator begin() and end()

	// using iterator = ProxyIterator<BinaryProxy, T>;
 //    using const_iterator = ProxyIterator<BinaryProxy, const T>;

 //    iterator begin() { return iterator(this, 0); }

 //    const_iterator begin() const { return const_iterator(this, 0); }

 //    iterator end() { return iterator(this, this->size()); }

 //    const_iterator end() const { return const_iterator(this, this->size()); }

	void print(std::ostream& out) const {
		left.print(out);
		right.print(out);
	}

};

/////////////////////////////////////////////////
// Proxy for all unary operations. 
// Required for lazy evaluation.
/////////////////////////////////////////////////
template <typename T, typename Op>
class UnaryProxy {
public:
	using UnaryType = ChooseRef<T>;
	using value_type = typename T::value_type;
	T data;

	UnaryProxy(UnaryType _data): data(_data) {}

	uint64_t size() const { return data.size(); }

	value_type operator[](uint64_t k) const {
		if (k > size()) { /*Error*/ }
		return Op{}(data[k]);
	}

    using iterator = ProxyIterator<UnaryProxy, T>;
    using const_iterator = ProxyIterator<UnaryProxy, const T>;

    iterator begin() { return iterator(this, 0); }

    const_iterator begin() const { return const_iterator(this, 0); }

    iterator end() { return iterator(this, this->size()); }

    const_iterator end() const { return const_iterator(this, this->size()); }

	void print(std::ostream& out) const {
		data.print(out);
	}

};

////////////////////////////////////
// Wrapper for Valarrays and Proxies
////////////////////////////////////

template <typename S>
class Wrap : public S {
public:
	using value_type = typename S::value_type;
	using S::S;
	Wrap() : S() {}

	// Valarray/proxy constructor.
	Wrap(const S& s) : S(s) {}

	// Copy constructor
	template<typename T>
	Wrap(const Wrap<T>& lhs) {
		for (uint64_t k = 0; k < lhs.size(); k++) { 
			this->push_back(lhs[k]); 
		}
	}

	// Copy assignment operator
	template<typename T>
	Wrap<S>& operator=(Wrap<T> const& rhs) {
		uint64_t size = rhs.size();
		for (uint64_t k = 0; k < size; k++) {
			this->operator[](k) = rhs[k];
		}
		return *this;
	}

	// Print function
	std::ostream& print(std::ostream& out) const {
		for (int i = 0; i < this->size() - 1; i++) {
			out << this->operator[](i) << ",";
		}
		out << S::operator[](this->size() - 1) << std::endl;
		return out;
	}


};

// Output stream operator for wrapped Valarrays and Proxies
template <typename S>
std::ostream& operator<<(std::ostream& out, Wrap<S> const& rhs)
{
	rhs.print(out);
	return out;
}

////////////////////////////////////
// Wrapper for all Scalars
////////////////////////////////////

template <typename T>
class ScalarWrap {
	T value;
public:
	using value_type = T;
	ScalarWrap(T val) : value(val) {}

	T operator[](uint64_t index) const { return value; }

	uint64_t size() const { return -1; /* Ensures cross platform max*/ }

	void print(std::ostream& out) const { out << value << std::endl; }
};

// Output stream operator for wrapped scalars
template <typename S>
std::ostream& operator<<(std::ostream& out, ScalarWrap<S> const& rhs)
{
	rhs.print(out);
	return out;
}

////////////////////////////////////
// Implementation of Hidden Valarray
////////////////////////////////////
template <typename T>
class valarray_hidden : public vector<T> {
	using Vector = vector<T>;
	using Same = valarray_hidden<T>;
	Vector& vec;
public:
	using value_type = T;
	using vector<T>::vector;
	valarray_hidden() : Vector() {}
	// Not really a copy constructor, just copy the reference
	// So we don't have to copy the vector :)
	valarray_hidden(const Same& rhs) : vec((const Vector &)rhs) {} //Vector((Vector)rhs) {}
	valarray_hidden(const Vector& rhs) : vec(rhs) {} // Vector(rhs) {}
	T operator[](uint64_t index) {
		return vec[index];
	}
};



////////////////////////////////////
// Operators for Wrap
////////////////////////////////////

/**
 * Addition operator for two valarray/proxy Wraps.
 * Truncate the longer valarray to the length of the shorter valarray,
 * and combine the two together.
 */

template <typename T1, typename T2> 
Wrap <BinaryProxy<T1, T2, std::plus<>>> 
operator+(const Wrap<T1>& lhs, const Wrap<T2>& rhs) {
	BinaryProxy <T1, T2, std::plus<>> result((T1 const&)lhs, (T2 const&)rhs);
	return Wrap<BinaryProxy<T1, T2, std::plus<>>>(result);
}

/**
 * Addition operator for valarray/proxy lhs and Scalar rhs.
 * Add scalar to every element in valarray.
 */

template <typename T1, typename T2> 
Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::plus<>>> 
operator+(const Wrap<T1>& lhs, const T2& rhs) {
	BinaryProxy<T1, ScalarWrap<T2>, std::plus<>> result((T1 const&)lhs, ScalarWrap<T2>(rhs));
	return Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::plus<>>>(result);
}


/**
 * Addition operator for Scalar lhs and valarray/proxy rhs.
 * Add scalar to every element in valarray.
 */

template<typename T1, typename T2> 
Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::plus<>>> 
operator+(const T1& lhs, const Wrap<T2>& rhs) {
	BinaryProxy<ScalarWrap<T1>, T2, std::plus<>> result(ScalarWrap<T1>(lhs), (T2 const&)rhs);
	return Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::plus<>>>(result);
}


/**
 * Subtraction operator for two valarray/proxy Wraps.
 * Truncate the longer valarray to the length of the shorter valarray,
 * and combine the two together.
 */

template <typename T1, typename T2> 
Wrap <BinaryProxy<T1, T2, std::minus<>>> 
operator-(const Wrap<T1>& lhs, const Wrap<T2>& rhs) {
	BinaryProxy <T1, T2, std::minus<>> result((T1 const&)lhs, (T2 const&)rhs);
	return Wrap<BinaryProxy<T1, T2, std::minus<>>>(result);
}

/**
 * Subtraction operator for valarray/proxy lhs and Scalar rhs.
 * Add scalar to every element in valarray.
 */

template <typename T1, typename T2> 
Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::minus<>>> 
operator-(const Wrap<T1>& lhs, const T2& rhs) {
	BinaryProxy<T1, ScalarWrap<T2>, std::minus<>> result((T1 const&)lhs, ScalarWrap<T2>(rhs));
	return Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::minus<>>>(result);
}


/**
 * Subtraction operator for Scalar lhs and valarray/proxy rhs.
 * Add scalar to every element in valarray.
 */

template<typename T1, typename T2> 
Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::minus<>>> 
operator-(const T1& lhs, const Wrap<T2>& rhs) {
	BinaryProxy<ScalarWrap<T1>, T2, std::minus<>> result(ScalarWrap<T1>(lhs), (T2 const&)rhs);
	return Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::minus<>>>(result);
}

/**
 * Multiplication operator for two valarray/proxy Wraps.
 * Truncate the longer valarray to the length of the shorter valarray,
 * and combine the two together.
 */

template <typename T1, typename T2> 
Wrap <BinaryProxy<T1, T2, std::multiplies<>>> 
operator*(const Wrap<T1>& lhs, const Wrap<T2>& rhs) {
	BinaryProxy <T1, T2, std::multiplies<>> result((T1 const&)lhs, (T2 const&)rhs);
	return Wrap<BinaryProxy<T1, T2, std::multiplies<>>>(result);
}

/**
 * Multiplication operator for valarray/proxy lhs and Scalar rhs.
 * Add scalar to every element in valarray.
 */

template <typename T1, typename T2> 
Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::multiplies<>>> 
operator*(const Wrap<T1>& lhs, const T2& rhs) {
	BinaryProxy<T1, ScalarWrap<T2>, std::multiplies<>> result((T1 const&)lhs, ScalarWrap<T2>(rhs));
	return Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::multiplies<>>>(result);
}


/**
 * Multiplication operator for Scalar lhs and valarray/proxy rhs.
 * Add scalar to every element in valarray.
 */

template<typename T1, typename T2> 
Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::multiplies<>>> 
operator*(const T1& lhs, const Wrap<T2>& rhs) {
	BinaryProxy<ScalarWrap<T1>, T2, std::multiplies<>> result(ScalarWrap<T1>(lhs), (T2 const&)rhs);
	return Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::multiplies<>>>(result);
}

/**
 * Division operator for two valarray/proxy Wraps.
 * Truncate the longer valarray to the length of the shorter valarray,
 * and combine the two together.
 */

template <typename T1, typename T2> 
Wrap <BinaryProxy<T1, T2, std::divides<>>> 
operator/(const Wrap<T1>& lhs, const Wrap<T2>& rhs) {
	BinaryProxy <T1, T2, std::divides<>> result((T1 const&)lhs, (T2 const&)rhs);
	return Wrap<BinaryProxy<T1, T2, std::divides<>>>(result);
}

/**
 * Division operator for valarray/proxy lhs and Scalar rhs.
 * Add scalar to every element in valarray.
 */

template <typename T1, typename T2> 
Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::divides<>>> 
operator/(const Wrap<T1>& lhs, const T2& rhs) {
	BinaryProxy<T1, ScalarWrap<T2>, std::divides<>> result((T1 const&)lhs, ScalarWrap<T2>(rhs));
	return Wrap<BinaryProxy<T1, ScalarWrap<T2>, std::divides<>>>(result);
}


/**
 * Division operator for Scalar lhs and valarray/proxy rhs.
 * Add scalar to every element in valarray.
 */

template<typename T1, typename T2> 
Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::divides<>>> 
operator/(const T1& lhs, const Wrap<T2>& rhs) {
	BinaryProxy<ScalarWrap<T1>, T2, std::divides<>> result(ScalarWrap<T1>(lhs), (T2 const&)rhs);
	return Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::divides<>>>(result);
}

template<typename T>
Wrap<UnaryProxy<T, std::negate<>>>
operator-(const Wrap<T>& lhs) {
	UnaryProxy<T, std::negate<>> result((T const&)lhs);
	return Wrap<UnaryProxy<T, std::negate<>>>(result);
}


};

#endif /* _Valarray_h */

