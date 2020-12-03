# SplitExtract
SplitExtract is a heterogeneous feature extraction tool for split manufactured layouts.
Different from previous feature extraction tools that focus on vector-based features, SplitExtract also extracts normalized images based features representing routed layout.
In paticular, it's capable of extracting the following features:
* distances,
* numbers of sink pins and load capacitance,
* front-end-of-line layer wirelengths and vias,
* driver delay,
* layout images,
* ...

More details are in the following papers:
* Haocheng Li, Satwik Patnaik, Abhrajit Sengupta, Haoyu Yang, Johann Knechtel, Bei Yu, Evangeline F.Y. Young, Ozgur Sinanoglu, "[Attacking split manufacturing from a deep learning perspective](https://doi.org/10.1145/3316781.3317780)", ACM/IEEE Design Automation Conference (DAC), Las Vegas, NV, USA, June 2-6, 2019.
* Haocheng Li, Satwik Patnaik, Mohammed Ashraf, Haoyu Yang, Johann Knechtel, Bei Yu, Ozgur Sinanoglu, Evangeline F.Y. Young, "[Deep Learning Analysis for Split Manufactured Layouts with Routing Perturbation](https://doi.org/10.1109/TCAD.2020.3037297)", IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems (TCAD), 2020.

## 1. How to Build

**Step 1:** Download the source codes. For example,
~~~bash
$ git clone https://github.com/cuhk-eda/split-extract
~~~

**Step 2:** Go to the project root and build by
~~~bash
$ cd split-extract/src
$ make mode=release
~~~

Note that this will generate a folder `bin` under the root, which contains binaries and auxiliary files.
More details are in [`Makefile`](src/Makefile).

### Dependencies

* [GCC](https://gcc.gnu.org/)

## 2. How to Run

### Toy Test
Go to the `bin` directory and run binary.
