# qpx-gamma

Data acquisition system for multi-detector gamma spectroscopy with Pixie-4.
Developed at NIST. We claim no responsibility and endorse no specific product...

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
* gsl libraries (for Windows get them precompiled from https://azylstra.net/blog/post/building-gsl-on-windows)

Linux:
* make sure PLX driver is working (read PLX_DRIVER_INSTALL)
* utter the following incantations:
  - to compile './install.sh' will also put sample data files in ~/qpxdata/
  - to run './qpx'
* to make global, follow it up with 'sudo make install'

Windows:
* go into qpx.pro and update library directories
* compiles successfully, but could not link
* get Qt5 to work with Visual Studio...