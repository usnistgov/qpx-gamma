# qpx-gamma

Data acquisition and analysis software for multi-detector gamma spectroscopy.
Developed at NIST. No warranty and no endorsement of specific product. Please read license and disclaimer.

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

Works with the following hardware:
* Pixie-4
* HV8
* IsegSHV via Wiener VMUSB

To compile, first install:
* boost libraries (5.7)
* Qt libraries (5.3)
* mesa-comon-dev
* libglu1-mesa-dev

Linux:
* If using Pixie-4, make sure PLX driver is working (driver and installation script included, see /hardware/pixie4/PLX/README)
* utter the following incantations:
  - to compile './install.sh' will also put sample data files in ~/qpx/
  - to run './qpx'
* to make global, follow it up with 'sudo make install'

For questions, contact:
<br>   Martin Shetty (martin.shetty@nist.gov)
<br>   Dagistan Sahin (dagistan.sahin@nist.gov)