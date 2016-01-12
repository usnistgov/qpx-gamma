# qpx-gamma

Data acquisition and analysis software for multi-detector gamma spectroscopy.
Developed at NIST. No warranty and no endorsement of specific product. Please read license and disclaimer.

![qpx-gamma](/screenshot.png)

Features:
* modular multithreaded architecture with multiple data sources and data sinks
* simultaneous building of multiple spectra (real-time sorting)
* coincidence matrices with symmetrization, projections, 2D peak search
* list mode output
* loss-free counting spectra
* energy, FWHM and efficiency calibration tools
* basic isotope database
* SUM4 peak integration with uncertainties
* gain matching (iterative online, and post-acquisition)
* automatic digitizer setting optimization to minimize FWHM

Supports:
* Linux and Mac
* XIA Pixie-4
* HV8
* IsegSHV via Wiener VMUSB

Reads:
* Canberra cnf, mca, tka, ava
* Radware spn, spe, mat, m4b
* N42 (ANSI N42.42-2006)
* Gammavision spe
* NSCLdaq evt

To install, you need:
* [boost](http://www.boost.org/)
* [Qt 5](http://www.qt.io/)
* If using Pixie-4, make sure PLX driver is working (driver and installation script included, see /hardware/pixie4/PLX/README)
* utter the following incantations:
  - to compile './install.sh' will also put sample data files in ~/qpx/
  - to run './qpx'
* to make global, follow it up with 'sudo make install'

[Upcoming/ToDo](https://trello.com/b/YKb96auO/qpx-todo-list)

For questions, contact:
<br>   Martin Shetty (martin.shetty@nist.gov)
<br>   Dagistan Sahin (dagistan.sahin@nist.gov)