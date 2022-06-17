#ifndef OPENMC_TALLIES_FILTER_H
#define OPENMC_TALLIES_FILTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gsl/gsl>

#include "openmc/constants.h"
#include "openmc/hdf5_interface.h"
#include "openmc/particle.h"
#include "openmc/tallies/filter_match.h"
#include "pugixml.hpp"


namespace openmc {

//==============================================================================
//! Modifies tally score events.
//==============================================================================

enum class LegendreAxis {
  x, y, z
};

class Filter
{
public:
  // Types of Filter
  enum class FilterType {
    AzimuthalFilter,
    CellFilter,
    CellInstanceFilter,
    CellbornFilter,
    CellFromFilter,
    DelayedGroupFilter,
    DistribcellFilter,
    EnergyFilter,
    EnergyFunctionFilter,
    LegendreFilter,
    MaterialFilter,
    MeshFilter,
    MeshSurfaceFilter,
    MuFilter,
    ParticleFilter,
    PolarFilter,
    SphericalHarmonicsFilter,
    SpatialLegendreFilter,
    SurfaceFilter,
    UniverseFilter,
    ZernikeFilter,
    ZernikeRadialFilter
  };

    enum class MeshDir {
    OUT_LEFT,  // x min
    IN_LEFT,  // x min
    OUT_RIGHT,  // x max
    IN_RIGHT,  // x max
    OUT_BACK,  // y min
    IN_BACK,  // y min
    OUT_FRONT,  // y max
    IN_FRONT,  // y max
    OUT_BOTTOM,  // z min
    IN_BOTTOM, // z min
    OUT_TOP, // z max
    IN_TOP // z max
  };

  //----------------------------------------------------------------------------
  // Constructors, destructors, factory functions
  
  ~Filter() = default;

  Filter();

  //! Create a new tally filter
  //
  //! \tparam T Type of the filter
  //! \param[in] id  Unique ID for the filter. If none is passed, an ID is
  //!    automatically assigned
  //! \return Pointer to the new filter object
  template<typename T>
  static T* create(int32_t id = -1);

  //! Create a new tally filter
  //
  //! \param[in] type  Type of the filter
  //! \param[in] id  Unique ID for the filter. If none is passed, an ID is
  //!    automatically assigned
  //! \return Pointer to the new filter object
  static Filter* create(const std::string& type, int32_t id = -1);

  //! Create a new tally filter from an XML node
  //
  //! \param[in] node XML node
  //! \return Pointer to the new filter object
  static Filter* create(pugi::xml_node node);

  //! Uses an XML input to fill the filter's data fields.
  virtual void from_xml(pugi::xml_node node) = 0;

  //----------------------------------------------------------------------------
  // Methods

  virtual std::string type() const = 0;

  //! Matches a tally event to a set of filter bins and weights.
  //!
  //! \param[in] p Particle being tracked
  //! \param[in] estimator Tally estimator being used
  //! \param[out] match will contain the matching bins and corresponding
  //!   weights; note that there may be zero matching bins
  virtual void
  get_all_bins(const Particle& p, TallyEstimator estimator, FilterMatch& match) const = 0;

  //! Writes data describing this filter to an HDF5 statepoint group.
  virtual void
  to_statepoint(hid_t filter_group) const
  {
    write_dataset(filter_group, "type", type());
    write_dataset(filter_group, "n_bins", n_bins_);
  }

  //! Return a string describing a filter bin for the tallies.out file.
  //
  //! For example, an `EnergyFilter` might return the string
  //! "Incoming Energy [0.625E-6, 20.0)".
  virtual std::string text_label(int bin) const = 0;

  //----------------------------------------------------------------------------
  // Accessors

  //! Get unique ID of filter
  //! \return Unique ID
  int32_t id() const { return id_; }

  //! Assign a unique ID to the filter
  //! \param[in]  Unique ID to assign. A value of -1 indicates that an ID should
  //!   be automatically assigned
  void set_id(int32_t id);

  //! Get number of bins
  //! \return Number of bins
  int n_bins() const { return n_bins_; }

  gsl::index index() const { return index_; }

  //----------------------------------------------------------------------------
  // Data members

private:
  int n_bins_;
  int32_t id_ {C_NONE};
  gsl::index index_;
  std::vector<double> bins_;
  std::vector<int32_t> cells_;
  std::unordered_map<int32_t, int> map_;
  std::vector<CellInstance> cell_instances_;
  std::unordered_map<CellInstance, gsl::index, CellInstanceHash> map_;
  std::vector<int> groups_;
  int32_t cell_;
  bool matches_transport_groups_ {false};
  std::vector<double> energy_;
  double x_;
  std::vector<double> y_; //TODO: There is a collision here. ZernikeFilter has it as a scalar double, EnergyFunctionFilter has it as a vector
  double r_;
  int order_;
  std::vector<int32_t> materials_;
  int32_t mesh_;
  std::vector<Particle::Type> particles_;
  SphericalHarmonicsCosine cosine_ {SphericalHarmonicsCosine::particle};
  LegendreAxis axis_;
  double min_;
  double max_;
  std::vector<int32_t> surfaces_;
  std::vector<int32_t> universes_;
};

//==============================================================================
// Global variables
//==============================================================================

namespace model {
  extern "C" int32_t n_filters;
  extern std::unordered_map<int, int> filter_map;
  extern std::vector<std::unique_ptr<Filter>> tally_filters;
}

//==============================================================================
// Non-member functions
//==============================================================================

//! Make sure index corresponds to a valid filter
int verify_filter(int32_t index);

} // namespace openmc
#endif // OPENMC_TALLIES_FILTER_H
