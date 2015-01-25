/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *               2010 Kristian Høgsberg <krh@bitplanet.net>
 *               2010 Alexandros Frantzis <alexandros.frantzis@linaro.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "matrix.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

/** 
 * Multiplies two 4x4 matrices.
 * 
 * The result is stored in matrix m.
 * 
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
void matrix_multiply(GLfloat *m, GLfloat const * n)
{
  GLfloat tmp[16];
  const GLfloat *row, *column;
  std::div_t d;
  int i, j;

  for (i = 0; i < 16; i++)
  {
    tmp[i] = 0;
    d = div(i, 4);
    row = n + d.quot * 4;
    column = m + d.rem;
    for (j = 0; j < 4; j++)
    {
      tmp[i] += row[j] * column[j * 4];
    }
  }
  memcpy(m, &tmp, sizeof(tmp));
}

/** 
 * Rotates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to rotate
 * @param angle the angle to rotate
 * @param x the x component of the direction to rotate to
 * @param y the y component of the direction to rotate to
 * @param z the z component of the direction to rotate to
 */
void matrix_rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat s, c;

   sincosf((angle * M_PI) / 180.f, &s, &c);
   GLfloat r[16] = {
      x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
      x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0, 
      x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
      0, 0, 0, 1
   };

   matrix_multiply(m, r);
}

/** 
 * Scales a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to translate
 * @param x the x component of the scale factor
 * @param y the y component of the scale factor
 * @param z the z component of the scale factor
 */
void matrix_scale(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat t[16] = { x, 0, 0, 0,  0, y, 0, 0,  0, 0, z, 0,  0, 0, 0, 1 };

   matrix_multiply(m, t);
}

/** 
 * Translates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
void matrix_translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };

   matrix_multiply(m, t);
}

/** 
 * Creates an identity 4x4 matrix.
 * 
 * @param m the matrix make an identity matrix
 */
void matrix_identity(GLfloat *m)
{
   GLfloat t[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0,
   };

   memcpy(m, t, sizeof(t));
}

/** 
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
void matrix_transpose(GLfloat *m)
{
   GLfloat t[16] = {
      m[0], m[4], m[8],  m[12],
      m[1], m[5], m[9],  m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15]};

   memcpy(m, t, sizeof(t));
}

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
void matrix_invert(GLfloat *m)
{
   GLfloat t[16];
   matrix_identity(t);

   // Extract and invert the translation part 't'. The inverse of a
   // translation matrix can be calculated by negating the translation
   // coordinates.
   t[12] = -m[12];
   t[13] = -m[13];
   t[14] = -m[14];

   // Invert the rotation part 'r'. The inverse of a rotation matrix is
   // equal to its transpose.
   m[12] = m[13] = m[14] = 0;
   matrix_transpose(m);

   // inv(m) = inv(r) * inv(t)
   matrix_multiply(m, t);
}

/** 
 * Calculate a perspective projection transformation.
 * 
 * @param m the matrix to save the transformation in
 * @param fovy the field of view in the y direction
 * @param aspect the view aspect ratio
 * @param zNear the near clipping plane
 * @param zFar the far clipping plane
 */
void matrix_perspective(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
   GLfloat tmp[16];
   matrix_identity(tmp);

   GLfloat sine, cosine, cotangent, deltaZ;
   GLfloat radians = fovy / 2 * M_PI / 180;

   deltaZ = zFar - zNear;
   sincosf(radians, &sine, &cosine);

   if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
   {
     return;
   }

   cotangent = cosine / sine;

   tmp[0] = cotangent / aspect;
   tmp[5] = cotangent;
   tmp[10] = -(zFar + zNear) / deltaZ;
   tmp[11] = -1;
   tmp[14] = -2 * zNear * zFar / deltaZ;
   tmp[15] = 0;

   memcpy(m, tmp, sizeof(tmp));
}

void matrix_ortho(GLfloat * m,
                  GLfloat l, GLfloat r,
                  GLfloat b, GLfloat t, 
                  GLfloat n, GLfloat f)
{
   // FIXME I'm uncertain about some bits of it.
   GLfloat tmp[16] =
   {
     2.f / (r - l),      0.f,                0.f,         0.f,
     0.f,                2.f / (t - b),      0.f,         0.f,
     0.f,                0.f,                1 / (f - n), -n / (f - n),
     -(r + l) / (r - l), -(t + b) / (t - b), 0.f,         1.f,
   };
   memcpy(m, tmp, sizeof(tmp));
}
