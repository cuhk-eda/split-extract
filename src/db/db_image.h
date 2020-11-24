#ifndef _DB_IMAGE_
#define _DB_IMAGE_

namespace db {
class Image {
    friend class Database;

private:
    double _x = 0;
    double _y = 0;
    double _xStep = 1;
    double _yStep = 1;
    unsigned _xNum = 99;
    unsigned _yNum = 99;
    vector<vector<unsigned char>> _data;

public:
    Image(const double x = 0,
          const double y = 0,
          const double xStep = 1,
          const double yStep = 1,
          const unsigned xNum = 99,
          const unsigned yNum = 99)
        : _x(x),
          _y(y),
          _xStep(xStep),
          _yStep(yStep),
          _xNum(xNum),
          _yNum(yNum),
          _data(yNum, vector<unsigned char>(xNum, 0)) {}

    double xOrigin() const { return _x; }
    double yOrigin() const { return _y; }
    double xStep() const { return _xStep; }
    double yStep() const { return _yStep; }
    unsigned xNum() const { return _xNum; }
    unsigned yNum() const { return _yNum; }
    unsigned char data(const unsigned y, const unsigned x) const { return _data[y][x]; }
    bool isSeparate(const Rectangle* rect) const {
        return rect->lx() >= _x + _xStep * _xNum || rect->hx() <= _x || rect->ly() >= _y + _yStep * _yNum ||
               rect->hy() <= _y;
    }
    bool isSeparate(const Pin* p) const;
    void setData(const int y, const int x, const unsigned char s);
    void setDataAt(const int y, const int x, const int rIdx) {
        if (rIdx >= 0 && rIdx < 8) setData(y, x, 1 << rIdx);
    }
    void setRouting(const NetRouting* r, const unsigned base);
    void setRouting(const Pin* p, const unsigned base);
    void encode(const string& filename, const unsigned dir) const;

    Image& operator+=(const Image& rhs);
};
}  // namespace db

#endif
