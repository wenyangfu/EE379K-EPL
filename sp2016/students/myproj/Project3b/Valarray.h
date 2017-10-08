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
#include <cmath>
#include <iterator>

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

template <typename T>
struct choose_sqrt_type;

template <typename T>
using SqrtType = typename choose_sqrt_type<T>::type;

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

template <typename T1, typename T2>
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

template <typename T>
struct choose_sqrt_type {
	// Figure out what type to promote to for the sqrt() member
	// function of Wrap/valarray.
	static constexpr int double_rank = 3;
	static constexpr bool is_complex = SRank<T>::is_complex;
	// Choose between type = double and type = std::complex<double>
	using type = typename SType<double_rank, is_complex>::type;
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
// Iterator for all proxies (and Wraps). 
// If T is const, then this iterator behaves
// as a const_iterator.
//////////////////////////////////////////////////
template <typename Proxy, typename T>
class ProxyIterator : public std::iterator<std::random_access_iterator_tag, T> 
{
	using Same = ProxyIterator<Proxy, T>;
public:
	Proxy* proxy;
	uint64_t index;

	ProxyIterator(Proxy* _proxy, int _index) : proxy(_proxy), index(_index) {}
 
	// Pre-increment operator.
	ProxyIterator& operator++() {
		++index;
		return *this;
	}

	// Post increment operator.
	ProxyIterator operator++(int) {
		Same it{*this};
		++index;
		return it;
	}

	// Pre-decrement operator.
	ProxyIterator& operator--() {
		--index;
		return *this;
	}

	// Post decrement operator.
	ProxyIterator operator--(int) {
		Same it{*this};
		--index;
		return it;
	}

	// Addition operator.
	ProxyIterator operator+(int64_t n) {
		return Same{proxy, index + n};
	}

	// Subtraction operator.
	ProxyIterator operator-(int64_t n) {
		return Same{proxy, index - n};
	}

	ProxyIterator& operator+=(int64_t n) {
		index += n;
		return *this;
	}

	ProxyIterator& operator-=(int64_t n) {
		index -= n;
		return *this;
	}

	// Dereference operator.
	T operator*() {
		return proxy->operator[](index);
	}

	T& operator->() { return operator*(); }

	// [] operator.
	T operator[](int64_t n) {
		return proxy->operator[](index + n);
	}
 
	// Equivalence operator.
	bool operator==(const ProxyIterator& rhs) const {
		return this->index == rhs.index;
	}
	// Inequivalence operator.
	bool operator!=(const ProxyIterator& rhs) const {
		return !operator==(rhs);
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
	// value_type apply_op(uint64_t k, Op op = Op{}) const {
	// 	value_type l = left[k]; // left operand
	// 	value_type r = right[k]; // right operand
	// 	return op(l, r);
	// }

	// Apply binary operation tree to the kth pair of elements.
	// Basically what's going on :)
	value_type operator[](uint64_t k) const {
		if (k >= size()) { /* Outta bounds */ }
		value_type l = left[k]; // left operand
		value_type r = right[k]; // right operand
		return Op{}(l, r);
		// return apply_op(k, Op{});
	}

	using iterator = ProxyIterator<BinaryProxy, value_type>;
	using const_iterator = ProxyIterator<BinaryProxy, const value_type>;

	iterator begin() { return iterator(this, 0); }

	const_iterator begin() const { return const_iterator(this, 0); }

	iterator end() { return iterator(this, this->size()); }

	const_iterator end() const { return const_iterator(this, this->size()); }

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
	UnaryType data;
	Op op;

	// If unary operator is passed in with no parameters,
	// then just default construct it.
	UnaryProxy(UnaryType _data): data(_data), op() {}

	// If unary operator is passed in with parameters,
	// such as MultVal(-2), then copy construct it.
	UnaryProxy(UnaryType _data, Op _op): data(_data), op(_op) {}

	uint64_t size() const { return data.size(); }

	value_type operator[](uint64_t k) const {
		if (k > size()) { /*Error*/ }
		return op(data[k]);
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
	using S::iterator;
	using S::const_iterator;
	
	Wrap() : S() {}

	// Valarray/proxy constructor.
	Wrap(const S& s) : S(s) {}

	// Copy constructor
	template <typename T>
	Wrap(const Wrap<T>& lhs) {
		for (uint64_t k = 0; k < lhs.size(); k++) { 
			this->push_back(lhs[k]); 
		}
	}

	// Copy assignment operator
	template <typename T>
	Wrap<S>& operator=(Wrap<T> const& rhs) {
		uint64_t size = rhs.size();
		for (uint64_t k = 0; k < size; k++) {
			this->operator[](k) = rhs[k];
		}
		return *this;
	}

	/** Accumulate function
	 *  Add/multiply all elements in the valarray using the given function object
	 */
	template <typename Op>
	value_type accumulate(Op op) {
		if (this-> size() < 1) { /* Can't accumulate with no elements */
			return 0;
		}
		value_type result = this->operator[](0);
		for (int i = 1; i < this->size(); i++) {
			result = op(result, this->operator[](i));
		}
		return result;
	}

	/** 
	 *  Sum function
	 *  Sum all elements in the valarray using std::plus<>
	 */
	value_type sum() {
		value_type result{};
		for (int i = 0; i < this->size(); i++) {
			result += S::operator[](i);
		}
		return result;
	}

	/**
	 *  Apply the function to all elements in the valarray
	 *  using the given function object
	 */
	template <typename UnaryOp>
	Wrap<UnaryProxy<S, UnaryOp>> apply(UnaryOp op) {
		// Cast wrap into vector or binary proxy or unary proxy,
		// whatever is contained inside, then construct a Unary proxy
		// from it.
		UnaryProxy<S, UnaryOp> result((S const&) *this, op);
		// Wrap the Unary proxy and return.
		return Wrap<UnaryProxy<S, UnaryOp>>(result);
	}

	// template <typename SqrtOp>
	// Wrap<UnaryProxy<S, epl::Sqrt<S>>>

	// Uhh, I'm too lazy to figure out the return type
	// I'll let apply() do the work for me 
	auto sqrt() {
		return apply(Sqrt<value_type>());
	}
	// Print function
	std::ostream& print(std::ostream& out) const {
		for (int i = 0; i < this->size() - 1; i++) {
			out << this->operator[](i) << ",";
		}
		out << S::operator[](this->size() - 1) << std::endl;
		return out;
	}

	/**
	 * Square root functor, used for sqrt() method
	 * in the Wrap class.
	 */
	template <typename T>
	class Sqrt {
	public:
		// T _val;
		using result_type = SqrtType<T>;
		using argument_type = T;

		Sqrt() {} // Default constructor, do nothing lol

		result_type operator()(T val) const {
			return std::sqrt(val);
		}
	};
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

template <typename T1, typename T2> 
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

template <typename T1, typename T2> 
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

template <typename T1, typename T2> 
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
template <typename T1, typename T2> 
Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::divides<>>> 
operator/(const T1& lhs, const Wrap<T2>& rhs) {
	BinaryProxy<ScalarWrap<T1>, T2, std::divides<>> result(ScalarWrap<T1>(lhs), (T2 const&)rhs);
	return Wrap<BinaryProxy<ScalarWrap<T1>, T2, std::divides<>>>(result);
}

/**
 * Unary negation
 */
template <typename T>
Wrap<UnaryProxy<T, std::negate<>>>
operator-(const Wrap<T>& lhs) {
	UnaryProxy<T, std::negate<>> result((T const&)lhs);
	return Wrap<UnaryProxy<T, std::negate<>>>(result);
}


};

#endif /* _Valarray_h */

