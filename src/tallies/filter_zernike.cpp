#include "openmc/tallies/filter.h"

#include <cmath>
#include <sstream>
#include <utility>  // For pair

#include <fmt/core.h>
#include <gsl/gsl>

#include "openmc/capi.h"
#include "openmc/error.h"
#include "openmc/math_functions.h"
#include "openmc/xml_interface.h"

namespace openmc {

//==============================================================================
// ZernikeFilter implementation
//==============================================================================

void
Filter::ZernikeFilter_from_xml(pugi::xml_node node)
{
  fatal_error("Zernike filters not yet supported on device (due to calc_zn() dynamic memory allocation).");
  set_order(std::stoi(get_node_value(node, "order")));
  x_ = std::stod(get_node_value(node, "x"));
  yy_ = std::stod(get_node_value(node, "y"));
  r_ = std::stod(get_node_value(node, "r"));
}

void
Filter::ZernikeFilter_get_all_bins(const Particle& p, TallyEstimator estimator,
                            FilterMatch& match) const
{
  // Determine the normalized (r,theta) coordinates.
  double x = p.r().x - x_;
  double y = p.r().y - yy_;
  double r = std::sqrt(x*x + y*y) / r_;
  double theta = std::atan2(y, x);

  if (r <= 1.0) {
    // Compute and return the Zernike weights.
    double zn[FILTERMATCH_BINS_WEIGHTS_SIZE];
    calc_zn(order_, r, theta, zn);
    for (int i = 0; i < n_bins_; i++) {
      //match.bins_.push_back(i);
      //match.weights_.push_back(zn[i]);
      match.push_back(i, zn[i]);
    }
  }
}

void
Filter::ZernikeFilter_to_statepoint(hid_t filter_group) const
{
  write_dataset(filter_group, "order", order_);
  write_dataset(filter_group, "x", x_);
  write_dataset(filter_group, "y", yy_);
  write_dataset(filter_group, "r", r_);
}

std::string
Filter::ZernikeFilter_text_label(int bin) const
{
  Expects(bin >= 0 && bin < n_bins_);
  for (int n = 0; n < order_+1; n++) {
    int last = (n + 1) * (n + 2) / 2;
    if (bin < last) {
      int first = last - (n + 1);
      int m = -n + (bin - first) * 2;
      return fmt::format("Zernike expansion, Z{},{}", n, m);
    }
  }
  UNREACHABLE();
}

void
Filter::ZernikeFilter_set_order(int order)
{
  if (order < 0) {
    throw std::invalid_argument{"Zernike order must be non-negative."};
  }
  order_ = order;
  n_bins_ = ((order+1) * (order+2)) / 2;
}

//==============================================================================
// ZernikeRadialFilter implementation
//==============================================================================

void
Filter::ZernikeRadialFilter_get_all_bins(const Particle& p, TallyEstimator estimator,
                                  FilterMatch& match) const
{
  // Determine the normalized radius coordinate.
  double x = p.r().x - x_;
  double y = p.r().y - yy_;
  double r = std::sqrt(x*x + y*y) / r_;

  if (r <= 1.0) {
    // Compute and return the Zernike weights.
    double zn[FILTERMATCH_BINS_WEIGHTS_SIZE];
    calc_zn_rad(order_, r, zn);
    for (int i = 0; i < n_bins_; i++) {
      //match.bins_.push_back(i);
      //match.weights_.push_back(zn[i]);
      match.push_back(i, zn[i]);
    }
  }
}

std::string
Filter::ZernikeRadialFilter_text_label(int bin) const
{
  return "Zernike expansion, Z" + std::to_string(2*bin) + ",0";
}

void
Filter::ZernikeRadialFilter_set_order(int order)
{
  Filter::ZernikeFilter_set_order(order);
  n_bins_ = order / 2 + 1;
}

//==============================================================================
// C-API functions
//==============================================================================

std::pair<int, Filter&> check_zernike_filter(int32_t index)
{
  // Make sure this is a valid index to an allocated filter.
  int err = verify_filter(index);
  if (err) {
    return {err, model::tally_filters[0]};
  }

  // Get a pointer to the filter and downcast.
  auto& filt = model::tally_filters[index];

  // Check the filter type.
  if (filt.get_type() != Filter::FilterType::ZernikeFilter) {
    set_errmsg("Not a Zernike filter.");
    err = OPENMC_E_INVALID_TYPE;
  }
  return {err, filt};
}

extern "C" int
openmc_zernike_filter_get_order(int32_t index, int* order)
{
  // Check the filter.
  auto check_result = check_zernike_filter(index);
  auto err = check_result.first;
  auto filt = check_result.second;
  if (err) return err;

  // Output the order.
  *order = filt.order();
  return 0;
}

extern "C" int
openmc_zernike_filter_get_params(int32_t index, double* x, double* y,
                                 double* r)
{
  // Check the filter.
  auto check_result = check_zernike_filter(index);
  auto err = check_result.first;
  auto filt = check_result.second;
  if (err) return err;

  // Output the params.
  *x = filt.x();
  *y = filt.yy();
  *r = filt.r();
  return 0;
}

extern "C" int
openmc_zernike_filter_set_order(int32_t index, int order)
{
  // Check the filter.
  auto check_result = check_zernike_filter(index);
  auto err = check_result.first;
  auto filt = check_result.second;
  if (err) return err;

  // Update the filter.
  filt.set_order(order);
  return 0;
}

extern "C" int
openmc_zernike_filter_set_params(int32_t index, const double* x,
                                 const double* y, const double* r)
{
  // Check the filter.
  auto check_result = check_zernike_filter(index);
  auto err = check_result.first;
  auto filt = check_result.second;
  if (err) return err;

  // Update the filter.
  if (x)
    filt.set_x(*x);
  if (y)
    filt.set_y(*y);
  if (r)
    filt.set_r(*r);
  return 0;
}

} // namespace openmc
