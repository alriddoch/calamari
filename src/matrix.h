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

#ifndef MATRIX_H
#define MATRIX_H

#include <GL/gl.h>

void matrix_multiply(GLfloat *m, const GLfloat *n);
void matrix_rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void matrix_scale(GLfloat *m, GLfloat x, GLfloat y, GLfloat z);
void matrix_translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z);
void matrix_identity(GLfloat *m);
void matrix_transpose(GLfloat *m);
void matrix_invert(GLfloat *m);
void matrix_perspective(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
void matrix_ortho(GLfloat *m, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);

#endif // MATRIX_H
