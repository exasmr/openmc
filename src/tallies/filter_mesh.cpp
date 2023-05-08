#include "openmc/tallies/filter.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include "openmc/capi.h"
#include "openmc/constants.h"
#include "openmc/error.h"
#include "openmc/mesh.h"
#include "openmc/xml_interface.h"

namespace openmc {

void
Filter::MeshFilter_from_xml(pugi::xml_node node)
{
  auto bins_ = get_node_array<int32_t>(node, "bins");
  if (bins_.size() != 1) {
    fatal_error("Only one mesh can be specified per " + type()
                + " mesh filter.");
  }

  auto id = bins_[0];
  auto search = model::mesh_map.find(id);
  if (search != model::mesh_map.end()) {
    set_mesh(search->second);
  } else{
    fatal_error(fmt::format(
      "Could not find mesh {} specified on tally filter.", id));
  }
}

void
Filter::MeshFilter_get_all_bins(const Particle& p, TallyEstimator estimator, FilterMatch& match)
const
{
  if (estimator != TallyEstimator::TRACKLENGTH) {
    auto bin = model::meshes[mesh_].get_bin(p.r());
    if (bin >= 0) {
      //match.bins_.push_back(bin);
      //match.weights_.push_back(1.0);
      match.push_back(bin, 1.0);
    }
  } else {
    //model::meshes[mesh_]->bins_crossed(p, match.bins_, match.weights_);
    model::meshes[mesh_].bins_crossed(p, match);
  }
}

void
Filter::MeshFilter_to_statepoint(hid_t filter_group) const
{
  write_dataset(filter_group, "bins", model::meshes[mesh_].id_);
}

std::string
Filter::MeshFilter_text_label(int bin) const
{
  auto& mesh = model::meshes[mesh_];
  return mesh.bin_label(bin);
}

//==============================================================================
// C-API functions
//==============================================================================

extern "C" int
openmc_mesh_filter_get_mesh(int32_t index, int32_t* index_mesh)
{
  if (!index_mesh) {
    set_errmsg("Mesh index argument is a null pointer.");
    return OPENMC_E_INVALID_ARGUMENT;
  }

  // Make sure this is a valid index to an allocated filter.
  if (int err = verify_filter(index)) return err;

  // Get a pointer to the filter and downcast.
  const auto& filt = model::tally_filters[index];

  // Check the filter type.
  if (filt.get_type() != Filter::FilterType::MeshFilter) {
    set_errmsg("Tried to get mesh on a non-mesh filter.");
    return OPENMC_E_INVALID_TYPE;
  }

  // Output the mesh.
  return filt.mesh();
}

extern "C" int
openmc_mesh_filter_set_mesh(int32_t index, int32_t index_mesh)
{
  // Make sure this is a valid index to an allocated filter.
  if (int err = verify_filter(index)) return err;

  // Get a pointer to the filter and downcast.
  auto& filt = model::tally_filters[index];

  // Check the filter type.
  if (filt.get_type() != Filter::FilterType::MeshFilter) {
    set_errmsg("Tried to set mesh on a non-mesh filter.");
    return OPENMC_E_INVALID_TYPE;
  }

  // Check the mesh index.
  if (index_mesh < 0 || index_mesh >= model::meshes_size) {
    set_errmsg("Index in 'meshes' array is out of bounds.");
    return OPENMC_E_OUT_OF_BOUNDS;
  }

  // Update the filter.
  filt.set_mesh(index_mesh);
  return 0;
}

} // namespace openmc
