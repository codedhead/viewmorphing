#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_


/*
// The default versions of these are fine for release builds; for debug
// we define them so that we can add the Assert checks.
Vec3(const Vec3 &v) {
	Assert(!v.hasNaNs());
	x = v.x; y = v.y; z = v.z;
}

Vec3 &operator=(const Vec3 &v) {
	Assert(!v.hasNaNs());
	x = v.x; y = v.y; z = v.z;
	return *this;
}
*/

#include <math.h>
#define Assert
#include <float.h>
#define isnan _isnan
#define isinf(f) (!_finite((f)))

class Point2;
class Point3;
class Vec2 {
public:
	Vec2() { x = y = 0.f; }
	explicit Vec2(float f) { x = y = f; }
	Vec2(float xx, float yy)
		: x(xx), y(yy) {
			Assert(!hasNaNs());
	}
	bool isZero() const { return x==0.f&&y==0.f; }
	bool hasZeros() const { return x==0.f||y==0.f; }
	bool hasNaNs() const { return isnan(x) || isnan(y); }
	bool isNormalized()const
	{
		float len2=x*x+y*y;
		return len2>1.f-1e-5&&len2<1.f+1e-5;
	}
	explicit Vec2(const Point2 &p);

	Vec2 reciprocal()const
	{
		Assert(!hasZeros());
		return Vec2(1.f/x, 1.f/y);
	}

	// unary -
	Vec2 operator-() const { return Vec2(-x, -y); }
	// +
	Vec2 operator+(const Vec2 &v) const {
		Assert(!v.hasNaNs());
		return Vec2(x + v.x, y + v.y);
	}
	Vec2& operator+=(const Vec2 &v) {
		Assert(!v.hasNaNs());
		x += v.x; y += v.y;
		return *this;
	}
	// -
	Vec2 operator-(const Vec2 &v) const {
		Assert(!v.hasNaNs());
		return Vec2(x - v.x, y - v.y);
	}
	Vec2& operator-=(const Vec2 &v) {
		Assert(!v.hasNaNs());
		x -= v.x; y -= v.y;
		return *this;
	}
	// *
	Vec2 operator*(float f) const {
		Assert(!isnan(f));
		return Vec2(f*x, f*y);
	}
	Vec2 &operator*=(float f) {
		Assert(!isnan(f));
		x *= f; y *= f;
		return *this;
	}
	friend inline Vec2 operator*(float f,const Vec2& v)
	{
		return v*f;
	}
	// /
	Vec2 operator/(float f) const {
		Assert(f != 0);
		float inv = 1.f / f;
		return Vec2(x * inv, y * inv);
	}
	Vec2 &operator/=(float f) {
		Assert(f != 0);
		float inv = 1.f / f;
		x *= inv; y *= inv;
		return *this;
	}
	// dot
	float operator*(const Vec2& v) const
	{
		Assert(!v.hasNaNs());
		return x*v.x+y*v.y;
	}
	float absDot(const Vec2& v) const
	{
		return fabsf(*this*v);
	}

	// face
	Vec2 faceForward(const Vec2& v) const
	{
		return (*this*v<0.f)?-*this:*this;
	}
	// index
	float operator[](int i) const {
		Assert(i >= 0 && i <= 1);
		return (&x)[i];
	}
	float &operator[](int i) {
		Assert(i >= 0 && i <= 1);
		return (&x)[i];
	}
	// normalize
	float length2() const { Assert(!hasNaNs()); return x*x + y*y; }
	float length() const { Assert(!hasNaNs()); return sqrtf(length2()); }
	Vec2& hat()
	{
		return (*this/=length());
	}
	float normalize()
	{
		float ret=length();
		*this/=ret;
		return ret;
	}
	float normalize2()
	{
		float ret=length2();
		*this/=sqrtf(ret);
		return ret;
	}
	// equal
	bool operator==(const Vec2 &v) const {
		return x == v.x && y == v.y;
	}
	bool operator!=(const Vec2 &v) const {
		return x != v.x || y != v.y;
	}

	float x, y;
};


class Vec3 {
public:
	Vec3() { x = y = z = 0.f; }
	explicit Vec3(float f) { x = y = z = f; }
	Vec3(float xx, float yy, float zz)
		: x(xx), y(yy), z(zz) {
			Assert(!hasNaNs());
	}
	bool isZero() const { return x==0.f&&y==0.f&&z==0.f; }
	bool hasZeros() const { return x==0.f||y==0.f||z==0.f; }
	bool hasNaNs() const { return isnan(x) || isnan(y) || isnan(z); }
	bool isNormalized()const
	{
		float len2=x*x+y*y+z*z;
		return len2>1.f-1e-5&&len2<1.f+1e-5;
	}
	explicit Vec3(const Point3 &p);

	Vec3 reciprocal()const
	{
		Assert(!hasZeros());
		return Vec3(1.f/x, 1.f/y, 1.f/z);
	}

	// unary -
	Vec3 operator-() const { return Vec3(-x, -y, -z); }
	// +
	Vec3 operator+(const Vec3 &v) const {
		Assert(!v.hasNaNs());
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	Vec3& operator+=(const Vec3 &v) {
		Assert(!v.hasNaNs());
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	// -
	Vec3 operator-(const Vec3 &v) const {
		Assert(!v.hasNaNs());
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	Vec3& operator-=(const Vec3 &v) {
		Assert(!v.hasNaNs());
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	// *
	Vec3 operator*(float f) const {
		Assert(!isnan(f));
		return Vec3(f*x, f*y, f*z);
	}
	Vec3 &operator*=(float f) {
		Assert(!isnan(f));
		x *= f; y *= f; z *= f;
		return *this;
	}
	friend inline Vec3 operator*(float f,const Vec3& v)
	{
		return v*f;
	}
	// /
	Vec3 operator/(float f) const {
		Assert(f != 0);
		float inv = 1.f / f;
		return Vec3(x * inv, y * inv, z * inv);
	}
	Vec3 &operator/=(float f) {
		Assert(f != 0);
		float inv = 1.f / f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	// dot
	float operator*(const Vec3& v) const
	{
		Assert(!v.hasNaNs());
		return x*v.x+y*v.y+z*v.z;
	}
	float absDot(const Vec3& v) const
	{
		return fabsf(*this*v);
	}
	// cross
	Vec3 operator^(const Vec3& v) const
	{
		Assert(!v.hasNaNs());
		return (Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x));
	}
	// face
	Vec3 faceForward(const Vec3& v) const
	{
		return (*this*v<0.f)?-*this:*this;
	}
	// index
	float operator[](int i) const {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	float &operator[](int i) {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	// normalize
	float length2() const { Assert(!hasNaNs()); return x*x + y*y + z*z; }
	float length() const { Assert(!hasNaNs()); return sqrtf(length2()); }
	Vec3& hat()
	{
		return (*this/=length());
	}
	float normalize()
	{
		float ret=length();
		*this/=ret;
		return ret;
	}
	float normalize2()
	{
		float ret=length2();
		*this/=sqrtf(ret);
		return ret;
	}
	// equal
	bool operator==(const Vec3 &v) const {
		return x == v.x && y == v.y && z == v.z;
	}
	bool operator!=(const Vec3 &v) const {
		return x != v.x || y != v.y || z != v.z;
	}

	float x, y, z;
};

class Point2 {
public:
	Point2() { x = y = 0.f; }
	explicit Point2(float f) { x = y = f; }
	explicit Point2(float* f) { x=f[0];y=f[1]; }
	explicit Point2(double* f) { x=(float)f[0];y=(float)f[1]; }

	Point2(float xx, float yy)
		: x(xx), y(yy) {
			Assert(!hasNaNs());
	}

	bool hasNaNs() const {
		return isnan(x) || isnan(y);
	}

	// +
	Point2 operator+(const Vec2 &v) const {
		Assert(!v.hasNaNs());
		return Point2(x + v.x, y + v.y);
	}
	Point2 &operator+=(const Vec2 &v) {
		Assert(!v.hasNaNs());
		x += v.x; y += v.y;
		return *this;
	}
	// -
	Vec2 operator-(const Point2 &p) const {
		Assert(!p.hasNaNs());
		return Vec2(x - p.x, y - p.y);
	}
	Point2 operator-(const Vec2 &v) const {
		Assert(!v.hasNaNs());
		return Point2(x - v.x, y - v.y);
	}
	Point2 &operator-=(const Vec2 &v) {
		Assert(!v.hasNaNs());
		x -= v.x; y -= v.y;
		return *this;
	}
	// lerp
	Point2 &operator+=(const Point2 &p) {
		Assert(!p.hasNaNs());
		x += p.x; y += p.y;
		return *this;
	}
	Point2 operator+(const Point2 &p) const {
		Assert(!p.hasNaNs());
		return Point2(x + p.x, y + p.y);
	}
	// *
	Point2 operator* (float f) const
	{
		Assert(!isnan(f));
		return Point2(f*x, f*y);
	}
	Point2 &operator*=(float f) {
		Assert(!isnan(f));
		x *= f; y *= f;
		return *this;
	}
	friend inline Point2 operator*(float f,const Point2& p) {
		return p*f;
	}
	// /
	Point2 operator/ (float f) const {
		Assert(f!=0.f&&!isnan(f));
		float inv = 1.f/f;
		return Point2(inv*x, inv*y);
	}
	Point2 &operator/=(float f) {
		Assert(f!=0.f&&!isnan(f));
		float inv = 1.f/f;
		x *= inv; y *= inv;
		return *this;
	}
	// index
	float operator[](int i) const {
		Assert(i >= 0 && i <= 1);
		return (&x)[i];
	}
	float &operator[](int i) {
		Assert(i >= 0 && i <= 1);
		return (&x)[i];
	}
	// distance
	float distance(const Point2& p) const
	{
		return sqrtf((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y));
	}
	float distance2(const Point2& p) const
	{
		return ((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y));
	}
	// equal
	bool operator==(const Point2 &p) const {
		return x == p.x && y == p.y;
	}
	bool operator!=(const Point2 &p) const {
		return x != p.x || y != p.y;
	}

	float x, y;
};

class Point3 {
public:
	Point3() { x = y = z = 0.f; }
	explicit Point3(float f) { x = y = z = f; }
	explicit Point3(float* f) { x=f[0];y=f[1];z=f[2]; }
	explicit Point3(double* f) { x=(float)f[0];y=(float)f[1];z=(float)f[2]; }

	Point3(float xx, float yy, float zz)
		: x(xx), y(yy), z(zz) {
			Assert(!hasNaNs());
	}

	bool hasNaNs() const {
		return isnan(x) || isnan(y) || isnan(z);
	}

	// +
	Point3 operator+(const Vec3 &v) const {
		Assert(!v.hasNaNs());
		return Point3(x + v.x, y + v.y, z + v.z);
	}
	Point3 &operator+=(const Vec3 &v) {
		Assert(!v.hasNaNs());
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	// -
	Vec3 operator-(const Point3 &p) const {
		Assert(!p.hasNaNs());
		return Vec3(x - p.x, y - p.y, z - p.z);
	}
	Point3 operator-(const Vec3 &v) const {
		Assert(!v.hasNaNs());
		return Point3(x - v.x, y - v.y, z - v.z);
	}
	Point3 &operator-=(const Vec3 &v) {
		Assert(!v.hasNaNs());
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	// lerp
	Point3 &operator+=(const Point3 &p) {
		Assert(!p.hasNaNs());
		x += p.x; y += p.y; z += p.z;
		return *this;
	}
	Point3 operator+(const Point3 &p) const {
		Assert(!p.hasNaNs());
		return Point3(x + p.x, y + p.y, z + p.z);
	}
	// *
	Point3 operator* (float f) const
	{
		Assert(!isnan(f));
		return Point3(f*x, f*y, f*z);
	}
	Point3 &operator*=(float f) {
		Assert(!isnan(f));
		x *= f; y *= f; z *= f;
		return *this;
	}
	friend inline Point3 operator*(float f,const Point3& p) {
		return p*f;
	}
	// /
	Point3 operator/ (float f) const {
		Assert(f!=0.f&&!isnan(f));
		float inv = 1.f/f;
		return Point3(inv*x, inv*y, inv*z);
	}
	Point3 &operator/=(float f) {
		Assert(f!=0.f&&!isnan(f));
		float inv = 1.f/f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	// index
	float operator[](int i) const {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	float &operator[](int i) {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	// distance
	float distance(const Point3& p) const
	{
		return sqrtf((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));
	}
	float distance2(const Point3& p) const
	{
		return ((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));
	}
	// equal
	bool operator==(const Point3 &p) const {
		return x == p.x && y == p.y && z == p.z;
	}
	bool operator!=(const Point3 &p) const {
		return x != p.x || y != p.y || z != p.z;
	}

	float x, y, z;
};

inline Vec2::Vec2(const Point2 &p)
: x(p.x), y(p.y) {
	Assert(!hasNaNs());
}
inline Vec3::Vec3(const Point3 &p)
: x(p.x), y(p.y), z(p.z) {
	Assert(!hasNaNs());
}

struct Line2
{
	Line2(){}
	Line2(const Point2& a,const Point2& b)
		:p1(a),p2(b){}

	float length()
	{
		return (p2-p1).length();
	}
	float length2()
	{
		return (p2-p1).length2();
	}
	Vec2 vec()
	{
		return (p2-p1);
	}


	Point2 p1,p2;
};

/*
inline void coordSystem(const Vec3& v1,Vec3 *v2,Vec3 *v3)
{
	// assume normalized
	//v1.normalize();
	if (fabsf(v1.x) > fabsf(v1.y)) {
		float invLen = 1.f / sqrtf(v1.x*v1.x + v1.z*v1.z);
		*v2 = Vec3(-v1.z * invLen, 0.f, v1.x * invLen);
	}
	else {
		float invLen = 1.f / sqrtf(v1.y*v1.y + v1.z*v1.z);
		*v2 = Vec3(0.f, v1.z * invLen, -v1.y * invLen);
	}
	*v3 = v1^*v2;
}*/

#endif