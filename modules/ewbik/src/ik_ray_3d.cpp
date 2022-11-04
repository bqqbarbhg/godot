/*************************************************************************/
/*  ray_3d.cpp                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "ik_ray_3d.h"

#include "core/math/vector3.h"

IKRay3D::IKRay3D() {
}

IKRay3D::IKRay3D(Vector3 p_p1, Vector3 p_p2) {
	working_vector = p_p1;
	point_1 = p_p1;
	point_2 = p_p2;
}

Vector3 IKRay3D::closest_point_to(const Vector3 p_point) {
	working_vector = p_point;
	working_vector = working_vector - this->point_1;
	Vector3 heading = this->heading();
	heading.normalize();
	const real_t scale = working_vector.dot(heading);
	return get_scaled_to(scale);
}

Vector3 IKRay3D::closest_point_to_strict(const Vector3 &p_point) {
	Vector3 in_point = p_point;
	in_point = in_point - point_1;
	const Vector3 new_heading = heading();
	const real_t scale = (in_point.dot(new_heading) / (new_heading.length() * in_point.length())) * (in_point.length() / new_heading.length());

	if (scale <= 0) {
		return this->point_1;
	} else if (scale >= 1) {
		return this->point_2;
	}
	return get_multipled_by(scale);
}

Vector3 IKRay3D::heading() {
	working_vector = point_2;
	return working_vector - point_1;
}

void IKRay3D::set_align_to(Ref<IKRay3D> p_target) {
	point_1 = p_target->point_1;
	point_2 = p_target->point_2;
}

void IKRay3D::set_heading(Vector3 &p_new_head) {
	point_2 = point_1;
	point_2 = p_new_head;
}

Vector3 IKRay3D::get_heading(const Vector3 &p_set_to) const {
	Vector3 set_to = point_2;
	set_to -= this->point_1;
	return set_to;
}

Ref<IKRay3D> IKRay3D::get_2d_copy() {
	return get_2d_copy(IKRay3D::Z);
}

Ref<IKRay3D> IKRay3D::get_2d_copy(const int &p_collapse_on_axis) {
	Ref<IKRay3D> result = this;
	if (p_collapse_on_axis == IKRay3D::X) {
		result->point_1.x = 0.0f;
		result->point_2.x = 0.0f;
	}
	if (p_collapse_on_axis == IKRay3D::Y) {
		result->point_1.y = 0.0f;
		result->point_2.y = 0.0f;
	}
	if (p_collapse_on_axis == IKRay3D::Z) {
		result->point_1.z = 0.0f;
		result->point_2.z = 0.0f;
	}

	return result;
}

Vector3 IKRay3D::get_origin() {
	return point_1;
}

real_t IKRay3D::get_length() {
	working_vector = point_2;
	return (working_vector - point_1).length();
}

void IKRay3D::set_magnitude(const real_t &p_new_mag) {
	working_vector = point_2;
	Vector3 dir = working_vector - point_1;
	dir = dir * p_new_mag;
	this->set_heading(dir);
}

real_t IKRay3D::scaled_projection(const Vector3 &p_input) {
	working_vector = p_input;
	working_vector = working_vector - this->point_1;
	Vector3 heading = this->heading();
	real_t headingMag = heading.length();
	real_t workingVectorMag = working_vector.length();
	if (workingVectorMag == 0 || headingMag == 0) {
		return 0;
	}
	return (working_vector.dot(heading) / (headingMag * workingVectorMag)) * (workingVectorMag / headingMag);
}

void IKRay3D::set_divide(const real_t &p_divisor) {
	point_2 = point_2 - point_1;
	point_2 = point_2 / p_divisor;
	point_2 = point_2 + point_1;
}

void IKRay3D::set_multiply(const real_t &p_scalar) {
	point_2 = point_2 - point_1;
	point_2 = point_2 * p_scalar;
	point_2 = point_2 + point_1;
}

Vector3 IKRay3D::get_multipled_by(const real_t &p_scalar) {
	Vector3 result = this->heading();
	result = result * p_scalar;
	result = result + point_1;
	return result;
}

Vector3 IKRay3D::get_divided_by(const real_t &p_divisor) {
	Vector3 result = heading();
	result = result * p_divisor;
	result = result + point_1;
	return result;
}

Vector3 IKRay3D::get_scaled_to(const real_t &scale) {
	Vector3 result = heading();
	result.normalize();
	result *= scale;
	result += point_1;
	return result;
}

void IKRay3D::elongate(real_t amt) {
	Vector3 midPoint = (point_1 + point_2) * 0.5f;
	Vector3 p1Heading = point_1 - midPoint;
	Vector3 p2Heading = point_2 - midPoint;
	Vector3 p1Add = p1Heading.normalized() * amt;
	Vector3 p2Add = p2Heading.normalized() * amt;

	this->point_1 = p1Heading + p1Add + midPoint;
	this->point_2 = p2Heading + p2Add + midPoint;
}

Ref<IKRay3D> IKRay3D::copy() {
	return Ref<IKRay3D>(memnew(IKRay3D(this->point_1, this->point_2)));
}

void IKRay3D::reverse() {
	Vector3 temp = this->point_1;
	this->point_1 = this->point_2;
	this->point_2 = temp;
}

Ref<IKRay3D> IKRay3D::get_reversed() {
	return memnew(IKRay3D(this->point_2, this->point_1));
}

Ref<IKRay3D> IKRay3D::get_ray_scaled_to(real_t scalar) {
	return memnew(IKRay3D(point_1, get_scaled_to(scalar)));
}

void IKRay3D::point_with(Ref<IKRay3D> r) {
	if (this->heading().dot(r->heading()) < 0) {
		this->reverse();
	}
}

void IKRay3D::point_with(Vector3 heading) {
	if (this->heading().dot(heading) < 0) {
		this->reverse();
	}
}

Ref<IKRay3D> IKRay3D::getRayScaledBy(real_t scalar) {
	return Ref<IKRay3D>(memnew(IKRay3D(point_1, get_multipled_by(scalar))));
}

Vector3 IKRay3D::setToInvertedTip(Vector3 vec) {
	vec.x = (point_1.x - point_2.x) + point_1.x;
	vec.y = (point_1.y - point_2.y) + point_1.y;
	vec.z = (point_1.z - point_2.z) + point_1.z;
	return vec;
}

void IKRay3D::contractTo(real_t percent) {
	// contracts both ends of a ray toward its center such that the total length of
	// the ray is the percent % of its current length;
	real_t halfPercent = 1 - ((1 - percent) / 2.0f);

	point_1 = point_1.lerp(point_2, halfPercent);
	point_2 = point_2.lerp(point_1, halfPercent);
}

void IKRay3D::translateTo(Vector3 newLocation) {
	working_vector = point_2;
	working_vector = working_vector - point_1;
	working_vector = working_vector + newLocation;
	point_2 = working_vector;
	point_1 = newLocation;
}

void IKRay3D::translateTipTo(Vector3 newLocation) {
	working_vector = newLocation;
	Vector3 transBy = working_vector - point_2;
	this->translateBy(transBy);
}

void IKRay3D::translateBy(Vector3 toAdd) {
	point_1 += toAdd;
	point_2 += toAdd;
}

void IKRay3D::normalize() {
	this->set_magnitude(1);
}

Vector3 IKRay3D::intercepts2D(Ref<IKRay3D> r) {
	Vector3 result = point_1;

	real_t p0_x = this->point_1.x;
	real_t p0_y = this->point_1.y;
	real_t p1_x = this->point_2.x;
	real_t p1_y = this->point_2.y;

	real_t p2_x = r->point_1.x;
	real_t p2_y = r->point_1.y;
	real_t p3_x = r->point_2.x;
	real_t p3_y = r->point_2.y;

	real_t s1_x, s1_y, s2_x, s2_y;
	s1_x = p1_x - p0_x;
	s1_y = p1_y - p0_y;
	s2_x = p3_x - p2_x;
	s2_y = p3_y - p2_y;

	real_t t;
	t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	return result = Vector3(p0_x + (t * s1_x), p0_y + (t * s1_y), 0.0f);
}

Vector3 IKRay3D::closestPointToSegment3D(Ref<IKRay3D> r) {
	Vector3 closestToThis = r->closest_point_to_ray_3d_strict(this);
	return this->closest_point_to(closestToThis);
}

Vector3 IKRay3D::closest_point_to_ray_3d(Ref<IKRay3D> r) {
	working_vector = point_2;
	Vector3 u = working_vector - this->point_1;
	working_vector = r->point_2;
	Vector3 v = working_vector - r->point_1;
	working_vector = this->point_1;
	Vector3 w = working_vector - r->point_1;
	real_t a = u.dot(u);
	real_t b = u.dot(v);
	real_t c = v.dot(v);
	real_t d = u.dot(w);
	real_t e = v.dot(w);
	real_t D = a * c - b * b;
	real_t sc;

	// compute the line parameters of the two closest points
	if (D < FLT_TRUE_MIN) {
		sc = 0.0f;
	} else {
		sc = (b * e - c * d) / D;
	}

	return getRayScaledBy(sc)->p2();
}

Vector3 IKRay3D::closest_point_to_ray_3d_strict(Ref<IKRay3D> r) {
	working_vector = point_2;
	Vector3 u = working_vector - this->point_1;
	working_vector = r->point_2;
	Vector3 v = working_vector - r->point_1;
	working_vector = this->point_1;
	Vector3 w = working_vector - r->point_1;
	real_t a = u.dot(u); // always >= s0
	real_t b = u.dot(v);
	real_t c = v.dot(v); // always >= 0
	real_t d = u.dot(w);
	real_t e = v.dot(w);
	real_t D = a * c - b * b; // always >= 0
	real_t sc; // tc

	// compute the line parameters of the two closest points
	if (D < FLT_TRUE_MIN) {
		sc = 0.0f;
	} else {
		sc = (b * e - c * d) / D;
	}

	Vector3 result;
	if (sc < 0) {
		result = this->point_1;
	} else if (sc > 1) {
		result = this->point_2;
	} else {
		result = this->getRayScaledBy(sc)->p2();
	}

	return result;
}

Ref<IKRay3D> IKRay3D::get_perpendicular_2d() {
	Vector3 heading = this->heading();
	working_vector = Vector3(heading.x - 1.0f, heading.x, 0.0f);
	return Ref<IKRay3D>(memnew(IKRay3D(this->point_1, working_vector + this->point_1)));
}

Vector3 IKRay3D::intersects_plane(Vector3 ta, Vector3 tb, Vector3 tc) {
	Vector3 uvw;
	tta = ta;
	ttb = tb;
	ttc = tc;
	tta -= point_1;
	ttb -= point_1;
	ttc -= point_1;

	Vector3 result = planeIntersectTest(tta, ttb, ttc, uvw);
	return result + point_1;
}

int IKRay3D::intersects_sphere(Vector3 sphereCenter, real_t radius, Vector3 &S1, Vector3 &S2) {
	Vector3 tp1 = point_1 - sphereCenter;
	Vector3 tp2 = point_2 - sphereCenter;
	int result = intersects_sphere(tp1, tp2, radius, S1, S2);
	S1 += sphereCenter;
	S2 += sphereCenter;
	return result;
}

void IKRay3D::p1(Vector3 in) {
	this->point_1 = in;
}

void IKRay3D::p2(Vector3 in) {
	this->point_2 = in;
}

Vector3 IKRay3D::p2() {
	return point_2;
}

void IKRay3D::setP2(Vector3 p_p2) {
	this->point_2 = p_p2;
}

Vector3 IKRay3D::p1() {
	return point_1;
}

void IKRay3D::setP1(Vector3 p_p1) {
	this->point_1 = p_p1;
}

int IKRay3D::intersects_sphere(Vector3 rp1, Vector3 rp2, float radius, Vector3 &S1, Vector3 &S2) {
	Vector3 direction = rp2 - rp1;
	Vector3 e = direction; // e=ray.dir
	e.normalize(); // e=g/|g|
	Vector3 h = point_1;
	h = Vector3(0.0f, 0.0f, 0.0f);
	h = h - rp1; // h=r.o-c.M
	float lf = e.dot(h); // lf=e.h
	float radpow = radius * radius;
	float hdh = h.length_squared();
	float lfpow = lf * lf;
	float s = radpow - hdh + lfpow; // s=r^2-h^2+lf^2
	if (s < 0.0f) {
		return 0; // no intersection points ?
	}
	s = Math::sqrt(s); // s=sqrt(r^2-h^2+lf^2)

	int result = 0;
	if (lf < s) {
		if (lf + s >= 0) {
			s = -s; // swap S1 <-> S2}
			result = 1; // one intersection point
		}
	} else {
		result = 2; // 2 intersection points
	}

	S1 = e * (lf - s);
	S1 += rp1; // S1=A+e*(lf-s)
	S2 = e * (lf + s);
	S2 += rp1; // S2=A+e*(lf+s)
	return result;
}
