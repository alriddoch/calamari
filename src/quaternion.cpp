// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#include "quaternion.h"

#include <cmath>
#include <stdio.h>
#include <string.h>

static inline float square(float f)
{
    return f * f;
}

static float vector_mag(const float self[])
{
    return std::sqrt(square(self[0]) + square(self[1]) + square(self[2]));
}

void vector_mult(float self[], float scalar)
{
    self[0] *= scalar;
    self[1] *= scalar;
    self[2] *= scalar;
}

void vector_add(float self[], float rhs[])
{
    self[0] += rhs[0];
    self[1] += rhs[1];
    self[2] += rhs[2];
}

void vector_sub(float self[], float rhs[])
{
    self[0] += rhs[0];
    self[1] += rhs[1];
    self[2] += rhs[2];
}

float vector_dot(float self[], const float rhs[])
{
    return self[0] * rhs[0] + self[1] * rhs[1] + self[2] * rhs[2];
}

void vector_cross(const float lhs[], const float rhs[], float res[])
{
    res[0] = lhs[1] * rhs[2] - rhs[1] * lhs[2];
    res[1] = lhs[2] * rhs[0] - rhs[2] * lhs[0];
    res[2] = lhs[0] * rhs[1] - rhs[0] * lhs[1];
}

void quaternion_init(Quaternion * const self)
{
    self->vec[0] = 0;
    self->vec[1] = 0;
    self->vec[2] = 0;
    self->w = 1;
}

Quaternion quaternion_mult(const Quaternion * const lhs, const Quaternion * const rhs)
{
  Quaternion res;

  res.w =      lhs->w * rhs->w -      lhs->vec[0] * rhs->vec[0] - lhs->vec[1] * rhs->vec[1] - lhs->vec[2] * rhs->vec[2];
  res.vec[0] = lhs->w * rhs->vec[0] + lhs->vec[0] * rhs->w +      lhs->vec[1] * rhs->vec[2] - lhs->vec[2] * rhs->vec[1];
  res.vec[1] = lhs->w * rhs->vec[1] + lhs->vec[1] * rhs->w +      lhs->vec[2] * rhs->vec[0] - lhs->vec[0] * rhs->vec[2];
  res.vec[2] = lhs->w * rhs->vec[2] + lhs->vec[2] * rhs->w +      lhs->vec[0] * rhs->vec[1] - lhs->vec[1] * rhs->vec[0];

  return res;
}

Quaternion quaternion_rotate(Quaternion * const self, const float axis[], float angle)
{
  Quaternion other;
  float half_angle = angle / 2;

  // Calcalate other, a quaternion giving the rotation of angle about axis.
  other.w = std::cos(half_angle);

  other.vec[0] = axis[0]; other.vec[1] = axis[1]; other.vec[2] = axis[2];
  vector_mult(other.vec, std::sin(half_angle) / vector_mag(axis));

  // m_vec = axis * (CoordType) (sin(half_angle) / axis.mag());

  // Multiply this quaternion by other to rotate it by the desired ammount
  
#if 0
  Quaternion res;

  res.w =      self->w * other.w -      self->vec[0] * other.vec[0] - self->vec[1] * other.vec[1] - self->vec[2] * other.vec[2];
  res.vec[0] = self->w * other.vec[0] + self->vec[0] * other.w +      self->vec[1] * other.vec[2] - self->vec[2] * other.vec[1];
  res.vec[1] = self->w * other.vec[1] + self->vec[1] * other.w +      self->vec[2] * other.vec[0] - self->vec[0] * other.vec[2];
  res.vec[2] = self->w * other.vec[2] + self->vec[2] * other.w +      self->vec[0] * other.vec[1] - self->vec[1] * other.vec[0];

  // printf("Angle: %f\n",  angle);
  // printf("S(%f,%f,%f), %f\n", self->vec[0], self->vec[1], self->vec[2], self->w);
  // printf("O(%f,%f,%f), %f\n", other.vec[0], other.vec[1], other.vec[2], other.w);
  // printf("R(%f,%f,%f), %f\n", res.vec[0], res.vec[1], res.vec[2], res.w);

  return res;
#else
  // return quaternion_mult(self, &other);
  return quaternion_mult(&other, self);
#endif

}

void quaternion_rotmatrix(const Quaternion * q, float matrix[])
{
    // First row
    matrix[ 0] = 1.0f - 2.0f * ( q->vec[1] * q->vec[1] + q->vec[2] * q->vec[2] );
    matrix[ 1] = 2.0f * (q->vec[0] * q->vec[1] + q->vec[2] * q->w);
    matrix[ 2] = 2.0f * (q->vec[0] * q->vec[2] - q->vec[1] * q->w);
    matrix[ 3] = 0.0f;

    // Second row
    matrix[ 4] = 2.0f * ( q->vec[0] * q->vec[1] - q->vec[2] * q->w );
    matrix[ 5] = 1.0f - 2.0f * ( q->vec[0] * q->vec[0] + q->vec[2] * q->vec[2] );
    matrix[ 6] = 2.0f * (q->vec[2] * q->vec[1] + q->vec[0] * q->w );
    matrix[ 7] = 0.0f;

    // Third row
    matrix[ 8] = 2.0f * ( q->vec[0] * q->vec[2] + q->vec[1] * q->w );
    matrix[ 9] = 2.0f * ( q->vec[1] * q->vec[2] - q->vec[0] * q->w );
    matrix[10] = 1.0f - 2.0f * ( q->vec[0] * q->vec[0] + q->vec[1] * q->vec[1] );
    matrix[11] = 0.0f;

    // Fourth row
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1.0f;
}

void quaternion_invert(Quaternion * q)
{
    q->vec[0] = -q->vec[0];
    q->vec[1] = -q->vec[1];
    q->vec[2] = -q->vec[2];
}

#if 0
// The arguments to this function have been swapped over because in
// the form provided in the example code, the result appeared to be
// a rotation in the wrong direction. This may be a bug in my apogee
// code, or it may be this code.

Quaternion::Quaternion(const Vector3D & from, const Vector3D & to) : _set(true)
{
    double cosT = from.dot(to);
    if (cosT > 0.99999f) {
        x = y = z = 0.0;
        w = 1.0;

        return;
    } else if (cosT < -0.99999f) {
        Vector3D t(0.0, from.X(), -from.Y());

        if (t.relMag() < 1e-12) {
            t = Vector3D(-from.Z(), 0.0, from.X());
        }

        t.unit();

        x = t.X();
        y = t.Y();
        z = t.Z();
        w = 0.0;

        return;
    }
    Vector3D t = from.cross(to);

    t.unit();

    double ss = std::sqrt(0.5 * (1.0 - cosT));

    x = t.X() * ss;
    y = t.Y() * ss;
    z = t.Z() * ss;
    w = std::sqrt(0.5 * (1.0 + cosT));
}
#endif
