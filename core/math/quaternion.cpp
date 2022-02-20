/*************************************************************************/
/*  quaternion.cpp                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "quaternion.h"

#include "core/math/basis.h"
#include "core/string/print_string.h"

real_t Quaternion::angle_to(const Quaternion &p_to) const {
	real_t d = dot(p_to);
	return Math::acos(CLAMP(d * d * 2 - 1, -1, 1));
}

// get_euler_xyz returns a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses XYZ convention (Z is the first rotation).
Vector3 Quaternion::get_euler_xyz() const {
	Basis m(*this);
	return m.get_euler(Basis::EULER_ORDER_XYZ);
}

// get_euler_yxz returns a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Vector3 Quaternion::get_euler_yxz() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Vector3(0, 0, 0), "The quaternion must be normalized.");
#endif
	Basis m(*this);
	return m.get_euler(Basis::EULER_ORDER_YXZ);
}

void Quaternion::operator*=(const Quaternion &p_q) {
	real_t xx = w * p_q.x + x * p_q.w + y * p_q.z - z * p_q.y;
	real_t yy = w * p_q.y + y * p_q.w + z * p_q.x - x * p_q.z;
	real_t zz = w * p_q.z + z * p_q.w + x * p_q.y - y * p_q.x;
	w = w * p_q.w - x * p_q.x - y * p_q.y - z * p_q.z;
	x = xx;
	y = yy;
	z = zz;
}

Quaternion Quaternion::operator*(const Quaternion &p_q) const {
	Quaternion r = *this;
	r *= p_q;
	return r;
}

bool Quaternion::is_equal_approx(const Quaternion &p_quaternion) const {
	return Math::is_equal_approx(x, p_quaternion.x) && Math::is_equal_approx(y, p_quaternion.y) && Math::is_equal_approx(z, p_quaternion.z) && Math::is_equal_approx(w, p_quaternion.w);
}

real_t Quaternion::length() const {
	return Math::sqrt(length_squared());
}

void Quaternion::normalize() {
	*this /= length();
}

Quaternion Quaternion::normalized() const {
	return *this / length();
}

bool Quaternion::is_normalized() const {
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON); //use less epsilon
}

Quaternion Quaternion::inverse() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The quaternion must be normalized.");
#endif
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::log() const {
	Quaternion src = *this;
	Vector3 src_v = src.get_axis() * src.get_angle();
	return Quaternion(src_v.x, src_v.y, src_v.z, 0);
}

Quaternion Quaternion::exp() const {
	Quaternion src = *this;
	Vector3 src_v = Vector3(src.x, src.y, src.z);
	float theta = src_v.length();
	if (theta < CMP_EPSILON) {
		return Quaternion();
	}
	return Quaternion(src_v.normalized(), theta);
}

Quaternion Quaternion::slerp(const Quaternion &p_to, const real_t &p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion must be normalized.");
#endif
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1 = -p_to;
	} else {
		to1 = p_to;
	}

	// calculate coefficients

	if ((1.0f - cosom) > CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_weight) * omega) / sinom;
		scale1 = Math::sin(p_weight * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_weight;
		scale1 = p_weight;
	}
	// calculate final values
	return Quaternion(
			scale0 * x + scale1 * to1.x,
			scale0 * y + scale1 * to1.y,
			scale0 * z + scale1 * to1.z,
			scale0 * w + scale1 * to1.w);
}

Quaternion Quaternion::slerpni(const Quaternion &p_to, const real_t &p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion must be normalized.");
#endif
	const Quaternion &from = *this;

	real_t dot = from.dot(p_to);

	if (Math::absf(dot) > 0.9999f) {
		return from;
	}

	real_t theta = Math::acos(dot),
		   sinT = 1.0f / Math::sin(theta),
		   newFactor = Math::sin(p_weight * theta) * sinT,
		   invFactor = Math::sin((1.0f - p_weight) * theta) * sinT;

	return Quaternion(invFactor * from.x + newFactor * p_to.x,
			invFactor * from.y + newFactor * p_to.y,
			invFactor * from.z + newFactor * p_to.z,
			invFactor * from.w + newFactor * p_to.w);
}

Quaternion Quaternion::cubic_slerp(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, const real_t &p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion must be normalized.");
#endif
	Quaternion ret = *this;
	Quaternion prep = (ret - p_pre_a).length_squared() < (ret + p_pre_a).length_squared() ? p_pre_a : p_pre_a * -1.0f;
	Quaternion q_b = (ret - p_b).length_squared() < (ret + p_b).length_squared() ? p_b : p_b * -1.0f;
	Quaternion post_b = (p_b - p_post_b).length_squared() < (p_b + p_post_b).length_squared() ? p_post_b : p_post_b * -1.0f;

	// calculate coefficients
	if ((1.0 - Math::abs(dot(p_b))) > CMP_EPSILON) {
		Quaternion ln_ret = ret.log();
		Quaternion ln_to = q_b.log();
		Quaternion ln_pre = prep.log();
		Quaternion ln_post = post_b.log();
		Quaternion ln = Quaternion(0, 0, 0, 0);
		ln.x = Math::cubic_interpolate(ln_ret.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
		ln.y = Math::cubic_interpolate(ln_ret.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
		ln.z = Math::cubic_interpolate(ln_ret.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
		ret = ln.exp();
	} else {
		ret.x = Math::cubic_interpolate(ret.x, q_b.x, prep.x, post_b.x, p_weight);
		ret.y = Math::cubic_interpolate(ret.y, q_b.y, prep.y, post_b.y, p_weight);
		ret.z = Math::cubic_interpolate(ret.z, q_b.z, prep.z, post_b.z, p_weight);
		ret.w = Math::cubic_interpolate(ret.w, q_b.w, prep.w, post_b.w, p_weight);
	}
	// calculate final values
	return ret;
}

Quaternion::operator String() const {
	return "(" + String::num_real(x, false) + ", " + String::num_real(y, false) + ", " + String::num_real(z, false) + ", " + String::num_real(w, false) + ")";
}

Vector3 Quaternion::get_axis() const {
	if (Math::abs(w) > 1 - CMP_EPSILON) {
		return Vector3(x, y, z);
	}
	real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
	return Vector3(x * r, y * r, z * r);
}

float Quaternion::get_angle() const {
	return 2 * Math::acos(w);
}

Quaternion::Quaternion(const Vector3 &p_axis, real_t p_angle) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 must be normalized.");
#endif
	real_t d = p_axis.length();
	if (d == 0) {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	} else {
		real_t sin_angle = Math::sin(p_angle * 0.5f);
		real_t cos_angle = Math::cos(p_angle * 0.5f);
		real_t s = sin_angle / d;
		x = p_axis.x * s;
		y = p_axis.y * s;
		z = p_axis.z * s;
		w = cos_angle;
	}
}

// Euler constructor expects a vector containing the Euler angles in the format
// (ax, ay, az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Quaternion::Quaternion(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.y * 0.5f;
	real_t half_a2 = p_euler.x * 0.5f;
	real_t half_a3 = p_euler.z * 0.5f;

	// R = Y(a1).X(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	x = sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3;
	y = sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3;
	z = -sin_a1 * sin_a2 * cos_a3 + cos_a1 * cos_a2 * sin_a3;
	w = sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3;
}
