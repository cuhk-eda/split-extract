#ifndef _DB_LAYER_H_
#define _DB_LAYER_H_

namespace db {
class Track {
protected:
    vector<string> layers_;

public:
    char direction = 'x';
    int start = INT_MAX;
    unsigned num = 0;
    unsigned step = 0;

    Track(const char direction = 'x', const int start = INT_MAX, const unsigned num = 0, const unsigned step = 0)
        : direction(direction), start(start), num(num), step(step) {}
    Track(const Track& track) : Track(track.direction, track.start, track.num, track.step) {}

    void addLayer(const string& layer) { layers_.push_back(layer); }

    char macro() const;
    unsigned numLayers() const { return layers_.size(); }
    const string& layer(unsigned index) const { return layers_[index]; }
};

class Layer {
    friend class Database;

private:
    string _name = "";
    //'r' for route or 'c' for cut
    char _type = 'x';

    Layer* _below = nullptr;
    Layer* _above = nullptr;

public:
    char direction = 'x';
    //  index at route layers 'M1' = 0
    int rIdx = -1;
    //  index at cut layers 'V12' = 0
    int cIdx = -1;
    // for route layer
    int pitch = -1;
    int offset = -1;
    int width = -1;
    int spacing = -1;
    Track track;

    Layer(const string& name = "", const char type = 'x') : _name(name), _type(type) {}

    const string& name() const { return _name; }

    bool isRouteLayer() const { return _type == 'r'; }
    bool isCutLayer() const { return _type == 'c'; }
    Layer* getLayerBelow() { return _below; }
    Layer* getLayerAbove() { return _above; }
};
}  // namespace db

#endif

