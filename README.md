# qpx-gamma

Data acquisition system for multi-detector gamma spectroscopy with Pixie-4.
Developed at NIST. No warranty and no endorsement of specific product. Please read license and disclaimer.

Main features:
* simultaneous building of multiple MCA spectra
* gg coincidence spectra (energy-energy-count)
* multi-threaded DAQ for minimal loss
* loss-free counting MCA spectra
* logging with timestamps
* calibration with isotope database

To thoroughly enjoy it you need:
* Pixie-4 in a PXI crate connected to your PC
* germanium detector (preferably >= two)
* radioactive stuff (play safe)

To compile you need:
* boost libraries (5.7)
* Qt libraries (5.3)
* gsl libraries

Linux:
* make sure PLX driver is working (read PLX_DRIVER_INSTALL)
* utter the following incantations:
  - to compile './install.sh' will also put sample data files in ~/qpxdata/
  - to run './qpx'
* to make global, follow it up with 'sudo make install'

For questions, contact:
   Martin Shetty (martin.shetty@nist.gov)
   Dagistan Sahin (dagistan.sahin@nist.gov)