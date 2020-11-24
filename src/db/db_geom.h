#ifndef _DB_GEOM_H_
#define _DB_GEOM_H_

namespace db {

class Point {
protected:
    int _x = INT_MAX;
    int _y = INT_MAX;

public:
    Point() {}
    Point(const int x, const int y) : _x(x), _y(y) {}

    int x() const { return _x; }
    int y() const { return _y; }

    void x(const int i) { _x = i; }
    void y(const int i) { _y = i; }

    const int operator[](const unsigned dir) const;
};

class Interval {
private:
    int _lo = INT_MAX;
    int _hi = INT_MIN;

public:
    constexpr Interval() {}
    template <typename... Args> Interval(Args... params) { set(params...); }

    void set(const int v) { set(v, v); }
    void set(const int lo, const int hi);

    int lo() const { return _lo; }
    int hi() const { return _hi; }

    int center() const { return (_hi + _lo) / 2; }
    int range() const { return _hi - _lo; }
    bool cover(int v) const { return v >= _lo && v >= _hi; }
    bool isValid() const { return _hi >= _lo; }

    void shift(const int d);
    void flip(const int height) { (*this) = flip(*this, height); }
    void unionWith(const int v) { (*this) = unionWith(*this, v); }
    void unionWith(const Interval& i) { (*this) = unionWith(*this, i); }

    bool operator<(const Interval& rhs) const { return _lo < rhs._lo; }
    bool operator==(const Interval& rhs) const { return _lo == rhs._lo; }

    static const Interval flip(const Interval& v, const int height) { return {height - v._hi, height - v._lo}; }
    static const Interval intersect(const Interval& lhs, const Interval& rhs);
    static bool hasIntersect(const Interval& lhs, const Interval& rhs) { return intersect(lhs, rhs).isValid(); }
    static const Interval unionWith(const Interval& i, const int v);
    static const Interval unionWith(const Interval& lhs, const Interval& rhs);

    friend ostream& operator<<(ostream& os, const Interval& v) { return os << "[ " << v._lo << ' ' << v._hi << " ]"; }
};

class Rectangle {
private:
    static constexpr Interval NullInterval{};

    Interval _x;
    Interval _y;

public:
    Rectangle() {}
    Rectangle(const Interval& x, const Interval& y) : _x(x), _y(y) {}
    Rectangle(const int lx, const int ly, const int hx, const int hy) : _x(lx, hx), _y(ly, hy) {}

    void set(const int lx, const int ly, const int hx, const int hy);

    int lx() const { return _x.lo(); }
    int ly() const { return _y.lo(); }
    int hx() const { return _x.hi(); }
    int hy() const { return _y.hi(); }

    int cx() const { return _x.center(); }
    int cy() const { return _y.center(); }
    int w() const { return _x.range(); }
    int h() const { return _y.range(); }
    int hp() const { return w() + h(); }
    bool cover(const int x, const int y) const { return _x.cover(x) && _y.cover(y); }

    void shift(const int dx, const int dy);
    void r90(const int height);
    void flipX(const int width) { _x.flip(width); }
    void flipY(const int height) { _y.flip(height); }
    void unionWith(const Point& pt);
    void unionWith(const Rectangle& rect);

    const Interval& operator[](const unsigned dir) const;
    bool operator<(const Rectangle& rhs) const { return (_y == rhs._y) ? (_x < rhs._x) : (_y < rhs._y); }
    static bool hasIntersect(const Rectangle& lhs, const Rectangle& rhs) {
        return Interval::hasIntersect(lhs._x, rhs._x) && Interval::hasIntersect(lhs._y, rhs._y);
    }

    friend ostream& operator<<(ostream& os, const Rectangle& rect);
};

class Geometry : public Rectangle {
public:
    const Layer* layer = nullptr;

    Geometry() {}
    template <typename... Args> Geometry(const Layer* layer, Args&&... params) : Rectangle(params...), layer(layer) {}
    Geometry(int lx, int ly, int hx, int hy) : Rectangle(lx, ly, hx, hy) {}
    Geometry(const Rectangle& rect) : Rectangle(rect) {}

    bool cover(const Layer* lay, const int x, const int y) const { return lay == layer && Rectangle::cover(x, y); }

    friend ostream& operator<<(ostream& os, const Geometry& geo);
};
}  // namespace db

#endif

