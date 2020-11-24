#include "../global.h"

#include "db.h"
using namespace db;

/***** Interval *****/

const int Point::operator[](const unsigned dir) const {
    switch(dir) {
        case 0:
            return _x;
        case 1:
            return _y;
        default:
            printlog(LOG_ERROR, "unidentified dir %d", dir);
            return INT_MAX;
    }
}

/***** Interval *****/

void Interval::set(const int lo, const int hi) {
    _lo = lo;
    _hi = hi;
}

void Interval::shift(const int d) {
    _lo += d;
    _hi += d;
}

const Interval Interval::unionWith(const Interval& v, const int i) {
    if (!v.isValid()) return {i, i};
    return {min(v._lo, i), max(v._hi, i)};
}

const Interval Interval::unionWith(const Interval& lhs, const Interval& rhs) {
    if (!rhs.isValid()) return lhs;
    if (!lhs.isValid()) return rhs;
    return {min(lhs._lo, rhs._lo), max(lhs._hi, rhs._hi)};
}

const Interval Interval::intersect(const Interval& lhs, const Interval& rhs) {
    return {max(lhs._lo, rhs._lo), min(lhs._hi, rhs._hi)};
}

/***** Rectangle *****/

void Rectangle::set(const int lx, const int ly, const int hx, const int hy) {
    _x.set(lx, hx);
    _y.set(ly, hy);
}

void Rectangle::shift(const int dx, const int dy) {
    _x.shift(dx);
    _y.shift(dy);
}

void Rectangle::r90(const int height) {
    const Interval fy = Interval::flip(_y, height);
    _y = _x;
    _x = fy;
}

void Rectangle::unionWith(const Point& pt) {
    _x.unionWith(pt.x());
    _y.unionWith(pt.y());
}

void Rectangle::unionWith(const Rectangle& rect) {
    _x.unionWith(rect._x);
    _y.unionWith(rect._y);
}

const Interval& Rectangle::operator[](const unsigned dir) const {
    switch(dir) {
        case 0:
            return _x;
        case 1:
            return _y;
        default:
            printlog(LOG_ERROR, "unidentified dir %d", dir);
            return NullInterval;
    }
}

namespace db {
ostream& operator<<(ostream& os, const Rectangle& rect) { return os << "( " << rect._x << ' ' << rect._y << " )"; }
}  // namespace db

/***** Geometry *****/

namespace db {
ostream& operator<<(ostream& os, const Geometry& geo) { return os << geo.layer->name() << ": " << Rectangle(geo); }
}  // namespace db
