# qpx-gamma

Data acquisition and analysis software for multi-detector gamma spectroscopy with Pixie-4.
Developed at NIST. No warranty and no endorsement of specific product. Please read license and disclaimer.

Features:
* support for multiple modules
* simultaneous building of multiple spectra
* coincidence matrices
* multi-threaded for minimal dead time
* optional list mode output
* loss-free counting spectra
* energy and FWHM calibration
* basic isotope database
* peak fitting with multiplet deconvolution
* SUM4 peak integration with uncertainty
* energy-gated coincidence matrix projections
* matrix symmetrization
* gain matching (analog and post-acquisition)
* risetime/flattop optimization
* analysis reports in *.txt

To thoroughly enjoy it you need:
* Pixie-4 in a PXI crate connected to your PC
* detectors
* radioactive stuff

To compile you need:
* boost libraries (5.7)
* Qt libraries (5.3)
* lua-dev

Linux:
* make sure PLX driver is working (driver and installation script included, see ./engine/plugins/pixie4/PLX/README)
* utter the following incantations:
  - to compile './install.sh' will also put sample data files in ~/qpxdata/
  - to run './qpx'
* to make global, follow it up with 'sudo make install'

For questions, contact:
<br>   Martin Shetty (martin.shetty@nist.gov)
<br>   Dagistan Sahin (dagistan.sahin@nist.gov)