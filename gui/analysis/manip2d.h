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

SinkPtr slice_rectangular(SinkPtr source, std::initializer_list<Pair> bounds, bool det1);

bool slice_diagonal_x(SinkPtr source, SinkPtr destination, size_t xc, size_t yc, size_t width, size_t minx, size_t maxx);
bool slice_diagonal_y(SinkPtr source, SinkPtr destination, size_t xc, size_t yc, size_t width, size_t minx, size_t maxx);


SinkPtr make_symmetrized(SinkPtr source);

PreciseFloat sum_with_neighbors(SinkPtr source, size_t x, size_t y);
PreciseFloat sum_diag(SinkPtr source, size_t x, size_t y, size_t width);

}
#endif
