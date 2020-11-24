#include <lodepng.h>

#include "db.h"

using namespace db;

bool Image::isSeparate(const Pin* p) const { return isSeparate(p->splitNet()); }

void Image::setData(const int y, const int x, const unsigned char s) {
    if (x >= _x && x < _x + _xStep * _xNum && y >= _y && y < _y + _yStep * _yNum) {
        _data[static_cast<int>((y - _y) / _yStep)][static_cast<int>((x - _x) / _xStep)] |= s;
    }
}

void Image::setRouting(const NetRouting* r, const unsigned base) {
    const int offset = 4 - static_cast<int>(DBModule::Metal);
    for (const NetRouteNode& nrn : r->nodes) {
        setDataAt(nrn.y(), nrn.x(), base + max(0, nrn.layer()->rIdx + offset));
    }
    for (const NetRouteSegment& nrs : r->segments) {
        const int fromRIdx = r->nodes[nrs.fromNode].layer()->rIdx;
        const int toRIdx = r->nodes[nrs.toNode].layer()->rIdx;
        const int fromx = static_cast<int>((r->nodes[nrs.fromNode].x() - _x) / _xStep);
        const int fromy = static_cast<int>((r->nodes[nrs.fromNode].y() - _y) / _yStep);
        const int tox = static_cast<int>((r->nodes[nrs.toNode].x() - _x) / _xStep);
        const int toy = static_cast<int>((r->nodes[nrs.toNode].y() - _y) / _yStep);
        if (fromRIdx < 0 || toRIdx < 0 || fromRIdx != toRIdx) continue;

        if (fromx == tox) {
            if (fromx < 0 || fromx >= static_cast<int>(_xNum)) continue;
            const int imax = min(static_cast<int>(_yNum) - 1, max(fromy, toy));
            for (int i = max(0, min(fromy, toy)); i <= imax; ++i)
                _data[i][fromx] |= (1 << (base + max(0, fromRIdx + offset)));
        } else if (fromy == toy) {
            if (fromy < 0 || fromy >= static_cast<int>(_yNum)) continue;
            const int imax = min(static_cast<int>(_xNum) - 1, max(fromx, tox));
            for (int i = max(0, min(fromx, tox)); i <= imax; ++i)
                _data[fromy][i] |= (1 << (base + max(0, fromRIdx + offset)));
        }
    }
}

void Image::setRouting(const Pin* p, const unsigned base) { return setRouting(p->splitNet(), base); }

void Image::encode(const string& filename, const unsigned dir) const {
    vector<unsigned char> image(_xNum * _yNum);
    for (unsigned i = 0; i != _yNum; ++i) {
        for (unsigned j = 0; j != _xNum; ++j) {
            switch (dir) {
                case 0:
                    image[(i * _xNum + j)] = _data[i][j];
                    break;
                case 1:
                    image[(j * _yNum + i)] = _data[i][j];
                    break;
                default:
                    printlog(LOG_ERROR, "unidentified dir %d", dir);
                    return;
            }
        }
    }

    lodepng::State state;
    state.info_raw.colortype = LCT_GREY;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_GREY;
    state.info_png.color.bitdepth = 8;
    state.encoder.auto_convert = 0;
    std::vector<unsigned char> buffer;
    unsigned error = 0;
    switch (dir) {
        case 0:
            error = lodepng::encode(buffer, &image[0], _xNum, _yNum, state);
            break;
        case 1:
            error = lodepng::encode(buffer, &image[0], _yNum, _xNum, state);
            break;
        default:
            printlog(LOG_ERROR, "unidentified dir %d", dir);
            return;
    }
    if (error) {
        printlog(LOG_ERROR, "encoder error %u: %s", error, lodepng_error_text(error));
        return;
    }
    error = lodepng::save_file(buffer, filename);
    switch (error) {
        case 0:
            return;
        case 79:
            printlog(LOG_ERROR, "saver error 79: failed to open file %s for writing", filename.c_str());
            return;
        default:
            printlog(LOG_ERROR, "saver error %u: %s", error, lodepng_error_text(error));
            return;
    }
}

Image& Image::operator+=(const Image& rhs) {
    for (unsigned i = 0; i != _yNum; ++i) {
        for (unsigned j = 0; j != _xNum; ++j) _data[i][j] |= rhs._data[i][j];
    }
    return *this;
}
