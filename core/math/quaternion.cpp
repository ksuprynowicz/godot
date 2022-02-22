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
	// Modify quaternions for shortest path
	// https://math.stackexchange.com/questions/2650188/super-confused-by-squad-algorithm-for-quaternion-interpolation
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
	return prep.slerp_choose(ret, 1.0f);
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
	// When the theta is small, we need to avoid 0 division, so approxmate the value with using taylor expansion, the theta will be divided by 48.
	// https://www.cs.cmu.edu/~spiff/moedit99/expmap.pdf
	if (d < Math::pow(CMP_EPSILON, 0.25)) {
		real_t cos_angle = Math::cos(p_angle * 0.5f);
		real_t s = 0.5 + d * d * 0.0208;
		x = p_axis.x * s;
		y = p_axis.y * s;
		z = p_axis.z * s;
		w = cos_angle;
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

Basis Quaternion::slerp_choose(const Basis &p_to, const real_t &p_weight) const {
	Method method = find_method(*this, p_to);
	Basis result;
	switch (method) {
		default: {
			interpolate_basis_linear(*this, p_to, result, p_weight);
		} break;
		case INTERP_SLERP: {
			result = _basis_slerp_unchecked(*this, p_to, p_weight);
		} break;
		case INTERP_SCALED_SLERP: {
			interpolate_basis_scaled_slerp(*this, p_to, result, p_weight);
		} break;
	}
	return result;
}

Quaternion Quaternion::_basis_to_quaternion_unchecked(const Basis &p_basis) const {
	Basis m = p_basis;
	real_t trace = m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
	real_t temp[4];

	if (trace > 0.0) {
		real_t s = Math::sqrt(trace + 1.0);
		temp[3] = (s * 0.5);
		s = 0.5 / s;

		temp[0] = ((m.elements[2][1] - m.elements[1][2]) * s);
		temp[1] = ((m.elements[0][2] - m.elements[2][0]) * s);
		temp[2] = ((m.elements[1][0] - m.elements[0][1]) * s);
	} else {
		int i = m.elements[0][0] < m.elements[1][1]
				? (m.elements[1][1] < m.elements[2][2] ? 2 : 1)
				: (m.elements[0][0] < m.elements[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.elements[i][i] - m.elements[j][j] - m.elements[k][k] + 1.0);
		temp[i] = s * 0.5;
		s = 0.5 / s;

		temp[3] = (m.elements[k][j] - m.elements[j][k]) * s;
		temp[j] = (m.elements[j][i] + m.elements[i][j]) * s;
		temp[k] = (m.elements[k][i] + m.elements[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

Quaternion Quaternion::_quat_slerp_unchecked(const Quaternion &p_from, const Quaternion &p_to, real_t p_fraction) const {
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = p_from.dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0) {
		cosom = -cosom;
		to1.x = -p_to.x;
		to1.y = -p_to.y;
		to1.z = -p_to.z;
		to1.w = -p_to.w;
	} else {
		to1.x = p_to.x;
		to1.y = p_to.y;
		to1.z = p_to.z;
		to1.w = p_to.w;
	}

	// calculate coefficients

	// This check could possibly be removed as we dealt with this
	// case in the find_method() function, but is left for safety, it probably
	// isn't a bottleneck.
	if ((1.0 - cosom) > CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_fraction) * omega) / sinom;
		scale1 = Math::sin(p_fraction * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0 - p_fraction;
		scale1 = p_fraction;
	}
	// calculate final values
	return Quaternion(
			scale0 * p_from.x + scale1 * to1.x,
			scale0 * p_from.y + scale1 * to1.y,
			scale0 * p_from.z + scale1 * to1.z,
			scale0 * p_from.w + scale1 * to1.w);
}

Basis Quaternion::_basis_slerp_unchecked(Basis p_from, Basis p_to, real_t p_fraction) const {
	Quaternion from = _basis_to_quaternion_unchecked(p_from);
	Quaternion to = _basis_to_quaternion_unchecked(p_to);

	Basis b(_quat_slerp_unchecked(from, to, p_fraction));
	return b;
}

void Quaternion::interpolate_basis_scaled_slerp(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) const {
	Basis b_prev = p_prev;
	Basis b_curr = p_curr;

	// normalize both and find lengths
	Vector3 lengths_prev = _basis_orthonormalize(b_prev);
	Vector3 lengths_curr = _basis_orthonormalize(b_curr);

	r_result = _basis_slerp_unchecked(b_prev, b_curr, p_fraction);

	// now the result is unit length basis, we need to scale
	Vector3 lengths_lerped = lengths_prev + ((lengths_curr - lengths_prev) * p_fraction);

	// keep a note that the column / row order of the basis is weird,
	// so keep an eye for bugs with this.
	r_result[0] *= lengths_lerped;
	r_result[1] *= lengths_lerped;
	r_result[2] *= lengths_lerped;
}

void Quaternion::interpolate_basis_linear(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) const {
	// interpolate basis
	for (int n = 0; n < 3; n++) {
		r_result.elements[n] = p_prev.elements[n].lerp(p_curr.elements[n], p_fraction);
	}

	// It turns out we need to guard against zero scale basis.
	// This is kind of silly, as we should probably fix the bugs elsewhere in Godot that can't deal with
	// zero scale, but until that time...
	for (int n = 0; n < 3; n++) {
		Vector3 &axis = r_result[n];

		// not ok, this could cause errors due to bugs elsewhere,
		// so we will bodge set this to a small value
		const real_t smallest = 0.0001;
		const real_t smallest_squared = smallest * smallest;
		if (axis.length_squared() < smallest_squared) {
			// setting a different component to the smallest
			// helps prevent the situation where all the axes are pointing in the same direction,
			// which could be a problem for e.g. cross products..
			axis[n] = smallest;
		}
	}
}

// Return length of the vector3.
real_t Quaternion::_vector3_normalize(Vector3 &p_vec) const {
	real_t lengthsq = p_vec.length_squared();
	if (lengthsq == 0) {
		p_vec.x = p_vec.y = p_vec.z = 0;
		return 0.0;
	}
	real_t length = Math::sqrt(lengthsq);
	p_vec.x /= length;
	p_vec.y /= length;
	p_vec.z /= length;
	return length;
}

// returns lengths
Vector3 Quaternion::_basis_orthonormalize(Basis &r_basis) const {
	// Gram-Schmidt Process

	Vector3 x = r_basis.get_axis(0);
	Vector3 y = r_basis.get_axis(1);
	Vector3 z = r_basis.get_axis(2);

	Vector3 lengths;

	lengths.x = _vector3_normalize(x);
	y = (y - x * (x.dot(y)));
	lengths.y = _vector3_normalize(y);
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	lengths.z = _vector3_normalize(z);

	r_basis.set_axis(0, x);
	r_basis.set_axis(1, y);
	r_basis.set_axis(2, z);

	return lengths;
}

Quaternion::Method Quaternion::_test_basis(Basis p_basis, bool r_needed_normalize, Quaternion &r_quat) const {
	Vector3 axis_lengths = Vector3(p_basis.get_axis(0).length_squared(),
			p_basis.get_axis(1).length_squared(),
			p_basis.get_axis(2).length_squared());

	bool is_non_unit_scale = r_needed_normalize ||
			!(Math::is_equal_approx((real_t)axis_lengths.x, (real_t)1.0, (real_t)0.001) && Math::is_equal_approx((real_t)axis_lengths.y, (real_t)1.0, (real_t)0.001) && Math::is_equal_approx((real_t)axis_lengths.z, (real_t)1.0, (real_t)0.001));
	if (is_non_unit_scale) {
		// If the basis is not normalized (at least approximately), it will fail the checks needed for slerp.
		// So we try to detect a scaled (but not sheared) basis, which we *can* slerp by normalizing first,
		// and lerping the scales separately.

		// if any of the axes are really small, it is unlikely to be a valid rotation, or is scaled too small to deal with float error
		const real_t slerp_epsilon = 0.00001;
		if ((axis_lengths.x < slerp_epsilon) ||
				(axis_lengths.y < slerp_epsilon) ||
				(axis_lengths.z < slerp_epsilon)) {
			return INTERP_LERP;
		}

		// normalize the basis
		Basis norm_basis = p_basis;

		axis_lengths.x = Math::sqrt(axis_lengths.x);
		axis_lengths.y = Math::sqrt(axis_lengths.y);
		axis_lengths.z = Math::sqrt(axis_lengths.z);

		norm_basis.set_axis(0, norm_basis.get_axis(0) / axis_lengths.x);
		norm_basis.set_axis(1, norm_basis.get_axis(1) / axis_lengths.y);
		norm_basis.set_axis(2, norm_basis.get_axis(2) / axis_lengths.z);

		// This doesn't appear necessary, as the later checks will catch it
		// if (!_basis_is_orthogonal_any_scale(norm_basis)) {
		// return INTERP_LERP;
		// }

		p_basis = norm_basis;

		// Orthonormalize not necessary as normal normalization(!) works if the
		// axes are orthonormal.
		// p_basis.orthonormalize();

		// if we needed to normalize one of the two basis, we will need to normalize both,
		// regardless of whether the 2nd needs it, just to make sure it takes the path to return
		// INTERP_SCALED_LERP on the 2nd call of _test_basis.
		r_needed_normalize = true;
	}

	// Apply less stringent tests than the built in slerp, the standard Godot slerp
	// is too susceptible to float error to be useful
	real_t det = p_basis.determinant();
	if (!Math::is_equal_approx(det, 1, (real_t)0.01)) {
		return INTERP_LERP;
	}

	if (!_basis_is_orthogonal(p_basis)) {
		return INTERP_LERP;
	}

	// This could possibly be less stringent too, check this.
	r_quat = _basis_to_quaternion_unchecked(p_basis);
	if (!r_quat.is_normalized()) {
		return INTERP_LERP;
	}

	return r_needed_normalize ? INTERP_SCALED_SLERP : INTERP_SLERP;
}

// This check doesn't seem to be needed but is preserved in case of bugs.
bool Quaternion::_basis_is_orthogonal_any_scale(const Basis &p_basis) const {
	Vector3 cross = p_basis.get_axis(0).cross(p_basis.get_axis(1));
	real_t l = _vector3_normalize(cross);
	//	// too small numbers, revert to lerp
	if (l < 0.001) {
		return false;
	}

	const real_t epsilon = 0.9995;

	real_t dot = cross.dot(p_basis.get_axis(2));
	if (dot < epsilon) {
		return false;
	}

	cross = p_basis.get_axis(1).cross(p_basis.get_axis(2));
	l = _vector3_normalize(cross);
	// too small numbers, revert to lerp
	if (l < 0.001) {
		return false;
	}

	dot = cross.dot(p_basis.get_axis(0));
	if (dot < epsilon) {
		return false;
	}

	return true;
}

bool Quaternion::_basis_is_orthogonal(const Basis &p_basis, real_t p_epsilon) const {
	Basis identity;
	Basis m = p_basis * p_basis.transposed();
	bool is_m0_orthogonal = Math::is_equal_approx(m[0].x, identity[0].x, p_epsilon) && Math::is_equal_approx(m[0].y, identity[0].y, p_epsilon) && Math::is_equal_approx(m[0].z, identity[0].z, p_epsilon);
	bool is_m1_orthogonal = Math::is_equal_approx(m[1].x, identity[1].x, p_epsilon) && Math::is_equal_approx(m[1].y, identity[1].y, p_epsilon) && Math::is_equal_approx(m[1].z, identity[1].z, p_epsilon);
	bool is_m2_orthogonal = Math::is_equal_approx(m[2].x, identity[2].x, p_epsilon) && Math::is_equal_approx(m[2].y, identity[2].y, p_epsilon) && Math::is_equal_approx(m[2].z, identity[2].z, p_epsilon);
	// Less stringent tests than the standard Godot slerp
	if (!is_m0_orthogonal || !is_m1_orthogonal || !is_m2_orthogonal) {
		return false;
	}
	return true;
}

Quaternion::Method Quaternion::find_method(const Basis &p_a, const Basis &p_b) const {
	bool needed_normalize = false;

	Quaternion q0;
	Method method = _test_basis(p_a, needed_normalize, q0);
	if (method == INTERP_LERP) {
		return method;
	}

	Quaternion q1;
	method = _test_basis(p_b, needed_normalize, q1);
	if (method == INTERP_LERP) {
		return method;
	}

	// Are they close together?
	// Apply the same test that will revert to lerp as
	// is present in the slerp routine.
	real_t cosine = Math::abs(q0.dot(q1));
	if ((1.0 - cosine) <= CMP_EPSILON) {
		return INTERP_LERP;
	}

	return method;
}