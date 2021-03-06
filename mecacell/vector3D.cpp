#include "vector3D.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <random>
#include <cstdlib>
#include "rotation.h"
#include "quaternion.h"
#include "tools.h"

using namespace std;
namespace MecaCell {

double Vector3D::dot(const Vector3D &v) const { return x * v.x + y * v.y + z * v.z; }

Vector3D Vector3D::cross(const Vector3D &v) const {
	return Vector3D((y * v.z - z * v.y), (z * v.x - x * v.z), (x * v.y - y * v.x));
}

void Vector3D::random() {
	std::normal_distribution<double> nDist(0.0, 1.0);
	x = nDist(globalRand);
	y = nDist(globalRand);
	z = nDist(globalRand);
	normalize();
}

Vector3D Vector3D::randomUnit() {
	Vector3D v;
	v.random();
	return v;
}

Vector3D Vector3D::deltaDirection(double amount) {
	std::normal_distribution<double> nDist(0.0, amount);
	return Vector3D(x + nDist(globalRand), y + nDist(globalRand), z + nDist(globalRand)).normalized();
}

Vector3D Vector3D::zero() { return Vector3D(0, 0, 0); }

bool Vector3D::isZero() const { return (x == 0 && y == 0 && z == 0); }

void Vector3D::operator/=(const double &d) {
	x /= d;
	y /= d;
	z /= d;
}

void Vector3D::operator*=(const double &d) {
	x *= d;
	y *= d;
	z *= d;
}

void Vector3D::operator+=(const Vector3D &v) {
	x += v.x;
	y += v.y;
	z += v.z;
}

Vector3D Vector3D::operator+(const Vector3D &v) const { return Vector3D(x + v.x, y + v.y, z + v.z); }
Vector3D Vector3D::operator-(const Vector3D &v) const { return Vector3D(x - v.x, y - v.y, z - v.z); }
Vector3D Vector3D::operator-(const double &v) const { return Vector3D(x - v, y - v, z - v); }
Vector3D Vector3D::operator+(const double &v) const { return Vector3D(x + v, y + v, z + v); }
Vector3D Vector3D::operator/(const double &s) const { return Vector3D(x / s, y / s, z / s); }
Vector3D Vector3D::operator/(const Vector3D &v) const { return Vector3D(x / v.x, y / v.y, z / v.z); }
Vector3D Vector3D::operator-() const { return Vector3D(-x, -y, -z); }

bool Vector3D::operator>=(const double &v) const { return (x >= v && y >= v && z >= v); }
bool Vector3D::operator<=(const double &v) const { return (x <= v && y <= v && z <= v); }
bool Vector3D::operator>(const double &v) const { return (x > v && y > v && z > v); }
bool Vector3D::operator<(const double &v) const { return (x < v && y < v && z < v); }

double Vector3D::length() const { return sqrt(x * x + y * y + z * z); }
double Vector3D::sqlength() const { return (x * x + y * y + z * z); }

double Vector3D::getX() const { return x; }
double Vector3D::getY() const { return y; }
double Vector3D::getZ() const { return z; }

void Vector3D::normalize() { *this = *this / length(); }

Vector3D Vector3D::normalized() const {
	double l = length();
	return Vector3D(x / l, y / l, z / l);
}

std::string Vector3D::toString() {
	std::stringstream s;
	s.precision(500);
	s << "(" << x << " , " << y << ", " << z << ")";
	return s.str();
}

int Vector3D::getHash(int a, int b) {
	unsigned int A = (unsigned int)(a >= 0 ? 2 * a : -2 * a - 1);
	unsigned int B = (unsigned int)(b >= 0 ? 2 * b : -2 * b - 1);
	int C = ((A >= B ? A * A + A + B : A + B * B) / 2);
	return (a < 0 && b < 0) || (a >= 0 && b >= 0) ? C : -C - 1;
}

std::size_t Vector3D::getHash() const { return getHash(x, getHash(y, z)); }

void Vector3D::iterateTo(Vector3D const &v, const std::function<void(const Vector3D &)> &fun, int inc) {
	int im, iM, jm, jM, km, kM;
	if (x < v.x) {
		im = double2int(x);
		iM = double2int(v.x);
	} else {
		im = double2int(v.x);
		iM = double2int(x);
	}
	if (y < v.y) {
		jm = double2int(y);
		jM = double2int(v.y);
	} else {
		jm = double2int(v.y);
		jM = double2int(y);
	}
	if (z < v.z) {
		km = double2int(z);
		kM = double2int(v.z);
	} else {
		km = double2int(v.z);
		kM = double2int(z);
	}
	for (int i = im; i <= iM; i += inc) {
		for (int j = jm; j <= jM; j += inc) {
			for (int k = km; k <= kM; k += inc) {
				fun(Vector3D(i, j, k));
			}
		}
	}
}

Vector3D Vector3D::ortho() const {
	if (y == 0 && x == 0) {
		return Vector3D(0, 1, 0);
	}
	return Vector3D(-y, x, 0);
}
Vector3D Vector3D::ortho(Vector3D v) const {
	if ((v - *this).sqlength() > 0.000000001) {
		Vector3D res = cross(v);
		if (res.sqlength() > 0.000000000001) return cross(v);
	}
	return ortho();
}

Vector3D Vector3D::rotated(const double &angle, const Vector3D &vec) const {
	double halfangle = angle * 0.5;
	Vector3D v = vec * sin(halfangle);
	Vector3D vcV = 2.0 * v.cross(*this);
	return *this + cos(halfangle) * vcV + v.cross(vcV);
}

Vector3D Vector3D::rotated(const Rotation<Vector3D> &r) const {
	double halfangle = r.teta * 0.5;
	Vector3D v = r.n * sin(halfangle);
	Vector3D vcV = 2.0 * v.cross(*this);
	return *this + cos(halfangle) * vcV + v.cross(vcV);
}
// return Quaternion(r.teta, r.n) * *this; }

Rotation<Vector3D> Vector3D::rotateRotation(const Rotation<Vector3D> &start,
                                            const Rotation<Vector3D> &offset) {
	return Rotation<Vector3D>(start.n.rotated(offset), start.teta);
}

Rotation<Vector3D> Vector3D::addRotations(const Rotation<Vector3D> &R0, const Rotation<Vector3D> &R1) {
	Quaternion q2 = Quaternion(R1.teta, R1.n) * Quaternion(R0.teta, R0.n);
	q2.normalize();
	return q2.toAxisAngle();
}

void Vector3D::addAsAngularVelocity(const Vector3D &v, Rotation<Vector3D> &r) {
	double dTeta = v.length();
	Vector3D n0(0, 1, 0);
	if (dTeta > 0) {
		n0 = v / dTeta;
	}
	r = addRotations(r, Rotation<Vector3D>(n0, dTeta));
}


double Vector3D::rayCast(const Vector3D &o, const Vector3D &n, const Vector3D &p, const Vector3D &r) {
	// returns l such that p + l.r lies on the plane defined by its normal n and an offset o
	// l > 0 means that the ray hits the plane, l < 0 means that the ray dos not face the plane
	// l = 0 means that the ray is parallel to the plane or that p is on the plane
	double nr = n.dot(r);
	return (nr == 0) ? 0 : n.dot(o - p) / nr;
}

Vector3D Vector3D::getProjectionOnPlane(const Vector3D &o, const Vector3D &n, const Vector3D &p) {
	// returns the projection of p onto a plane defined by its normal n and an offset o
	return p - (n.dot(p - o) * n);
}

Vector3D Vector3D::getProjection(const Vector3D &origin, const Vector3D &B, const Vector3D &P) {
	// returns the projected P point onto the origin -> B vector
	Vector3D a = B - origin;
	return origin + a * (a.dot(P - origin) / a.sqlength());
}

Rotation<Vector3D> Vector3D::getRotation(const Vector3D &v0, const Vector3D &v1) {
	Rotation<Vector3D> res;
	res.teta = acos(min(1.0, max(-1.0, v0.dot(v1))));
	Vector3D cross = v0.cross(v1);
	if (cross.sqlength() == 0) {
		cross = Vector3D(0, 1, 0);
	}
	res.n = cross;
	return res;
}

Rotation<Vector3D> Vector3D::getRotation(const Basis<Vector3D> &b0, const Basis<Vector3D> &b1) {
	return getRotation(b0.X, b0.Y, b1.X, b1.Y);
}

Rotation<Vector3D> Vector3D::getRotation(const Vector3D &X0, const Vector3D &Y0, const Vector3D &X1,
                                         const Vector3D &Y1) {
	Quaternion q0(X0.normalized(), X1.normalized());
	Vector3D Ytmp = q0 * Y0;
	Ytmp.normalize();
	Quaternion qres = Quaternion(Ytmp, Y1.normalized()) * q0;
	qres.normalize();
	return qres.toAxisAngle();
}

Vector3D operator*(const Vector3D &v, const double &s) { return Vector3D(v.x * s, v.y * s, v.z * s); }
Vector3D operator*(const double &s, const Vector3D &v) { return Vector3D(v.x * s, v.y * s, v.z * s); }

bool operator==(const Vector3D &a, const Vector3D &b) { return (a.x == b.x && a.y == b.y && a.z == b.z); }
bool operator!=(const Vector3D &a, const Vector3D &b) { return !operator==(a, b); }

ostream &operator<<(ostream &out, const Vector3D &v) {
	out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return out;
}
}
