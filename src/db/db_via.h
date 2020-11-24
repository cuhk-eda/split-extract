#ifndef _DB_VIA_H_
#define _DB_VIA_H_

namespace db {
class Via {
public:
    int x;
    int y;
    ViaType* type;

    Via();
    Via(ViaType* type, int x, int y);
};

class ViaType {
private:
    bool isDef_ = false;

public:
    string name = "";
    vector<Geometry> rects;

    ViaType(const string& name, const bool isDef = false);

    template<typename... Args>
    void addRect(Args&&... params) { rects.emplace_back(params...); }
    void isDef(bool b) { isDef_ = b; }
    bool isDef() const { return isDef_; }
};
}  // namespace db

#endif
