// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#include "quaternion.h"

#include <math.h>

static inline float square(float f)
{
    return f * f;
}

static float vector_mag(const float self[])
{
    return sqrt(square(self[0]) + square(self[1]) + square(self[2]));
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

void quaternion_rotate(Quaternion * const self, const float axis[], float angle)
{
  Quaternion other;
  float half_angle = angle / 2;
  float vec1[3], vec2[3], vec3[2];

  // Calcalate other, a quaternion giving the rotation of angle about axis.
  other.w = cos(half_angle);

  other.vec[0] = axis[0]; other.vec[1] = axis[1]; other.vec[2] = axis[2];
  vector_mult(other.vec, sin(half_angle) / vector_mag(axis));

  // m_vec = axis * (CoordType) (sin(half_angle) / axis.mag());

  // Multiply this quaternion by other to rotate it by the desired ammount
  
  self->w = self->w * other.w + vector_dot(self->vec, other.vec);

  vec1[0] = self->vec[0]; vec1[1] = self->vec[1]; vec1[2] = self->vec[2];
  vector_mult(vec1, other.w);
  vec2[0] = other.vec[0]; vec2[1] = other.vec[1]; vec2[2] = other.vec[2];
  vector_mult(vec2, self->w);
  vector_cross(self->vec, other.vec, vec3);

  self->vec[0] = vec1[0]; self->vec[1] = vec1[1]; self->vec[2] = vec1[2];
  vector_add(self->vec, vec2);
  vector_sub(self->vec, vec3);

  // return *this;

  // q = q * other;
}

void quaternion_rotmatrix(const Quaternion * q, const float matrix[])
{
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
