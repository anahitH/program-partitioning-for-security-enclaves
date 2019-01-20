#pragma once

#include <limits>
#include <cmath>

namespace vazgen {

template <typename T>
class NumberType
{
public:
    static constexpr T POS_INFINITY = std::numeric_limits<T>::max();
    static constexpr T NEG_INFINITY = std::numeric_limits<T>::min();

    NumberType(T value = T())
        : m_value(value)
    {
    }

    NumberType(const NumberType& ) = default;
    NumberType(NumberType& ) = default;
    NumberType& operator =(const NumberType&) = default;
    NumberType& operator =(NumberType&&) = default;

    NumberType& operator =(T value)
    {
        m_value = value;
        return *this;
    }

    operator T() const
    {
        return m_value;
    }

    bool isPosInfinity() const
    {
        return m_value == POS_INFINITY;
    }

    bool isNegInfinity() const
    {
        return m_value == NEG_INFINITY;
    }

    T getValue() const
    {
        return m_value;
    }

    NumberType& operator +=(const NumberType& i);
    NumberType& operator -=(const NumberType& i);

    NumberType& operator ++();
    NumberType operator ++(int );

private:
    T m_value;
}; // class NumberType

template class NumberType<double>;
template class NumberType<int>;

using Integer = NumberType<int>;
using Double = NumberType<double>;

template <typename T>
NumberType<T> operator +(const NumberType<T>& i1, const NumberType<T>& i2);
template <typename T>
NumberType<T> operator -(const NumberType<T>& i1, const NumberType<T>& i2);
template <typename T>
NumberType<T> operator *(const NumberType<T>& i1, const NumberType<T>& i2);

template <typename T>
T safe_sum(const NumberType<T>& i1, const NumberType<T>& i2)
{
    if ((i1.isPosInfinity() && !i2.isNegInfinity())
            || (i2.isPosInfinity() && !i1.isNegInfinity())) {
        return NumberType<T>::POS_INFINITY;
    }
    if ((i1.isPosInfinity() && i2.isNegInfinity())
            || i2.isPosInfinity() && i1.isNegInfinity()) {
        return 0;
    }
    if ((i1.isNegInfinity() && !i2.isPosInfinity())
            || (i2.isNegInfinity() && !i1.isPosInfinity())) {
        return NumberType<T>::NEG_INFINITY;
    }
    T sum = (T)i1 + (T)i2;
    if (i1 < 0 && i2 < 0) {
        // underflow
        if (sum > i1) {
            return NumberType<T>::NEG_INFINITY;
        }
    } else if (i1 > 0 && i2 > 0) {
        // overflow
        if (sum < i1) {
            return NumberType<T>::POS_INFINITY;
        }
    }
    return sum;
}

template <typename T>
T safe_mult(const NumberType<T>& i1, const NumberType<T>& i2)
{
    if (i1 == 0 || i2 == 0) {
        return 0;
    }
    if ((i1.isPosInfinity() && !i2.isNegInfinity())
            || (i2.isPosInfinity() && !i1.isNegInfinity())) {
        return i2 > 0 ? NumberType<T>::POS_INFINITY : NumberType<T>::NEG_INFINITY;
    }
    if ((i1.isPosInfinity() && i2.isNegInfinity())
            || i2.isPosInfinity() && i1.isNegInfinity()) {
        return NumberType<T>::NEG_INFINITY;
    }
    if ((i1.isNegInfinity() && !i2.isPosInfinity())
            || (i2.isNegInfinity() && !i1.isPosInfinity())) {
        return i1 > 0 ? NumberType<T>::POS_INFINITY : NumberType<T>::NEG_INFINITY;
    }
    T mult = std::abs((T)i1 * (T)i2);
    // overfolw
    if (mult < std::abs(T(i1))) {
        if (i1 < 0 || i2 < 0) {
            return NumberType<T>::NEG_INFINITY;
        }
        return NumberType<T>::POS_INFINITY;
    }
    return mult;
}

template <typename T>
NumberType<T>& NumberType<T>::operator +=(const NumberType<T>& i)
{
    m_value = safe_sum(*this, i);
    return *this;
}

template <typename T>
NumberType<T>& NumberType<T>::operator -=(const NumberType<T>& i)
{
    m_value = safe_sum(*this, NumberType<T>(-1 * (T)i));
    return *this;
}

template <typename T>
NumberType<T>& NumberType<T>::operator ++()
{
    ++m_value;
    return *this;
}

template <typename T>
NumberType<T> NumberType<T>::operator ++(int )
{
    NumberType<T> tmp = *this;
    ++m_value;
    return tmp;
}

template <typename T>
NumberType<T> operator +(const NumberType<T>& i1, const NumberType<T>& i2)
{
    return safe_sum(i1, i2);
}

template <typename T>
NumberType<T> operator -(const NumberType<T>& i1, const NumberType<T>& i2)
{
    return safe_sum(i1, NumberType<T>(-1 * (T)i2));
}

template <typename T>
NumberType<T> operator *(const NumberType<T>& i1, const NumberType<T>& i2)
{
    return safe_mult(i1, i2);
}

} // namespace vazgen

