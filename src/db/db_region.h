#ifndef _DB_REGION_H_
#define _DB_REGION_H_

namespace db {
class Region : public Rectangle {
private:
    string _name = "";
    //  'f' for fence, 'g' for guide
    char _type = 'x';

public:
    static const unsigned char InvalidRegion = 0xff;

    unsigned char id = InvalidRegion;
    double density = 0;

    vector<string> members;
    vector<Rectangle> rects;

    Region(const string& name, const char type = 'x')
        : Rectangle(INT_MAX, INT_MAX, INT_MIN, INT_MIN), _name(name), _type(type) {}

    const string& name() const { return _name; }
    char type() const { return _type; }

    template <class... Args>
    void addRect(Args&&... args) {
        rects.emplace_back(args...);
        unionWith(rects.back());
    }
};
}  // namespace db

#endif
