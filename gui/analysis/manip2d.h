/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Spectrum::Manip2d set of functions for transforming spectra
 *
 ******************************************************************************/

#ifndef SPECTRUM_MANIP2D_H
#define SPECTRUM_MANIP2D_H

#include "daq_sink.h"
#include "custom_logger.h"

namespace Qpx {

bool slice_rectangular(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination, std::initializer_list<Pair> bounds);
bool slice_diagonal_x(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination, uint32_t xc, uint32_t yc, uint32_t width, uint32_t minx, uint32_t maxx);
bool slice_diagonal_y(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination, uint32_t xc, uint32_t yc, uint32_t width, uint32_t minx, uint32_t maxx);


bool symmetrize(std::shared_ptr<Sink> source, std::shared_ptr<Sink> destination);

PreciseFloat sum_with_neighbors(std::shared_ptr<Sink> source, uint16_t x, uint16_t y);
PreciseFloat sum_diag(std::shared_ptr<Sink> source, uint16_t x, uint16_t y, uint16_t width);

}
#endif
