# qpx-gamma

Data acquisition and analysis software for multi-detector gamma spectroscopy.
Developed at NIST. No warranty and no endorsement of specific product. Please read license and disclaimer.

![qpx-gamma](/screenshot.png)

Features:
* simultaneous building of multiple spectra (real-time sorting)
* coincidence matrices with symmetrization, projections, 2D peak search
* list mode output
* loss-free counting spectra
* energy, FWHM and efficiency calibration tools
* basic isotope database
* SUM4 peak integration with uncertainties
* Multiplet deconvolution using Hypermet
* gain matching (iterative online, and post-acquisition)
* automatic digitizer setting optimization to minimize FWHM
* structured experiment automation

Supports:
* Linux (Ubuntu 16.10 recommended)and OSX
* XIA Pixie-4
* Radiation Technologies HV-8
* Wiener VmUsb
* Iseg VHS-12

Reads:
* Canberra cnf, mca, tka, ava
* Radware spn, spe, mat, m4b
* N42 (ANSI N42.42-2006)
* Gammavision spe
* NSCLdaq evt

Installing / running:
* './install.sh' will also download and install [boost](http://www.boost.org/),
 [Qt](http://www.qt.io/), and [ROOT](https://root.cern.ch/) dependencies, and 
copy default configuration and sample data files to ~/qpx/
* When prompted, the safe choices are boost 1.58 and Qt 5.5. Experimental with more recent libraries.
* If using Pixie-4, make sure PLX driver is working (driver and installation script included, see /sources/pixie4/PLX/README)
* to run: './qpx' or debug mode 'gdb ./qpxd'

[Upcoming/ToDo](https://trello.com/b/YKb96auO/qpx-todo-list)

For questions, contact:
<br>   Martin Shetty (martin.shetty@esss.se)
<br>   Dagistan Sahin (dagistan.sahin@nist.gov)
