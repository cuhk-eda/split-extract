#ifndef _DB_PLACE_H_
#define _DB_PLACE_H_

namespace db{
    class CellPlacement{
        private:
            int _x = INT_MIN;
            int _y = INT_MIN;;
            bool _r90 = false;
            bool _flipX = false;
            bool _flipY = false;

        public:
            const static int Unplaced = INT_MIN;

            CellPlacement() {}
            CellPlacement(int x, int y, bool r90, bool fx, bool fy) : _x(x), _y(y), _r90(r90), _flipX(fx), _flipY(fy) {}

            int x() const { return _x; }
            int y() const { return _y; }
            bool r90() const { return _r90; }
            bool flipX() const { return _flipX; }
            bool flipY() const { return _flipY; }
    };

    class Placement{
        private:
            std::unordered_map<Cell*,CellPlacement> _placement;
        public:
            Placement(){
                _placement.clear();
            }
            void place(Cell *cell){
                _placement[cell] = CellPlacement();
            }
            void place(Cell *cell, int x, int y){
                bool r90 = _placement[cell].r90();
                bool fx = _placement[cell].flipX();
                bool fy = _placement[cell].flipY();
                _placement[cell] = CellPlacement(x, y, r90, fx, fy);
            }
            void place(Cell *cell, int x, int y, bool r90, bool fx, bool fy) { _placement[cell] = CellPlacement(x, y, r90, fx, fy); }
            void clear(){
                _placement.clear();
            }
            int x(Cell *cell) const {
                return _placement.at(cell).x();
                //return _placement[cell].x();
            }
            int y(Cell *cell) {
                return _placement[cell].y();
            }
            int r90(Cell *cell) { return _placement[cell].r90(); }
            int flipX(Cell *cell) { return _placement[cell].flipX(); }
            int flipY(Cell *cell) {
                return _placement[cell].flipY();
            }
            bool placed(Cell *cell) {
                return (x(cell) != INT_MIN) && (y(cell) != INT_MIN);
            }
    };
}

#endif

