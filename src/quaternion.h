// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2001 Alistair Riddoch

#ifndef QUATERNION_H
#define QUATERNION_H

// #include "Vector3D.h"

typedef struct quaternion {
    float x, y, z, w;
} Quaternion;

#if 0
  public:
    static const int cX = 0;    // Used to indicate which axis
    static const int cY = 1;
    static const int cZ = 2;
    static const int cW = 3;

    Quaternion() : x(0), y(0), z(0), w(1), _set(false) { }
    Quaternion(double x, double y, double z, double w) : x(x), y(y), z(z), w(w),
                                                         _set(true) { }
    explicit Quaternion(const Atlas::Message::Object::ListType &l) : _set(true)
    {
        x = l[0].AsNum();
        y = l[1].AsNum();
        z = l[2].AsNum();
        w = l[3].AsNum();
    }
    Quaternion(const Vector3D & from, const Vector3D & to);

    double & X() { return x; }
    double & Y() { return y; }
    double & Z() { return z; }
    double & W() { return w; }

    double X() const { return x; }
    double Y() const { return y; }
    double Z() const { return z; }
    double W() const { return w; }

    double & operator[](int index) {
        switch(index) {
            case cX:
                return x;
            case cY:
                return y;
            case cZ:
                return z;
            case cW:
                return w;
            default:
                //Throw an exception here maybe
                return z;
        }
    }

    bool operator==(const Quaternion & other) const {
        //"Check if two vector are equal";
        return ((x==other.x) && (y==other.y) && (z==other.z) && (w==other.w));
    }

    bool operator!=(const Quaternion & other) const {
        //"Check if two vector are equal";
        return ((x!=other.x) || (y!=other.y) || (z!=other.z) || (w!=other.w));
    }
};

inline std::ostream & operator<<(std::ostream& s, const Quaternion& q) {
    return s << "[" << q.x << "," << q.y << "," << q.z << "," << q.w << "]";
}

#endif

#endif // QUATERNION_H
