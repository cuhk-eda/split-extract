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
Go to the `bin` directory and run binary `extract` with a toy case `c432`:
~~~
$ cd bin
$ ./extract --input_def ../toys/c432/c432_original.def --cell_lef ../toys/c432/NangateOpenCellLibrary.lef --metal 3 --output_csv c432_M3.csv
~~~

## 3. Modules

* `src`: C++ source code
    * `args`: header-only C++ argument [parser library](https://github.com/Taywee/args)
    * `db`: database, including the cell and net information
    * `def58`: Design [Exchange Format](https://si2.org/oa-tools-utils-libs)
    * `io`: parser interfaces
    * `lef58`: Library [Exchange Format](https://si2.org/oa-tools-utils-libs)
    * `lodepng`: [PNG encoder](https://lodev.org/lodepng) and decoder in C and C++
    * `sta`: timing library
    * `ut`: some utility code
* `toys`: toy test cases

## 4. License

READ THIS LICENSE AGREEMENT CAREFULLY BEFORE USING THIS PRODUCT. BY USING THIS PRODUCT YOU INDICATE YOUR ACCEPTANCE OF THE TERMS OF THE FOLLOWING AGREEMENT. THESE TERMS APPLY TO YOU AND ANY SUBSEQUENT LICENSEE OF THIS PRODUCT.



License Agreement for SplitExtract



Copyright (c) 2020-2021, The Chinese University of Hong Kong



All rights reserved.



CU-SD LICENSE (adapted from the original BSD license) Redistribution of the any code, with or without modification, are permitted provided that the conditions below are met. 



1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.



2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.



3. Neither the name nor trademark of the copyright holder or the author may be used to endorse or promote products derived from this software without specific prior written permission.



4. Users are entirely responsible, to the exclusion of the author, for compliance with (a) regulations set by owners or administrators of employed equipment, (b) licensing terms of any other software, and (c) local, national, and international regulations regarding use, including those regarding import, export, and use of encryption software.



THIS FREE SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR ANY CONTRIBUTOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, EFFECTS OF UNAUTHORIZED OR MALICIOUS NETWORK ACCESS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
