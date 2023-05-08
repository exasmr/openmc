#include "openmc/tallies/filter.h"

#include "openmc/capi.h"
#include "openmc/constants.h"
#include "openmc/error.h"
#include "openmc/mesh.h"

namespace openmc {

void
Filter::MeshSurfaceFilter_get_all_bins(const Particle& p, TallyEstimator estimator,
                                FilterMatch& match) const
{
  int starting_length = match.bins_weights_length_;
  //model::meshes[mesh_]->surface_bins_crossed(p, match.bins_);
  model::meshes[mesh_].surface_bins_crossed(p, match);
  int ending_length = match.bins_weights_length_;
  //for (auto b : match.bins_) match.weights_.push_back(1.0);
    //match.weights_.push_back(1.0);
  for (int i = starting_length; i < ending_length; i++)
    match.weights_[i] = 1.0;
}

std::string
Filter::MeshSurfaceFilter_text_label(int bin) const
{
  auto& mesh = model::meshes[mesh_];
  int n_dim = mesh.n_dimension_;

  // Get flattend mesh index and surface index.
  int i_mesh = bin / (4 * n_dim);
  MeshDir surf_dir = static_cast<MeshDir>(bin % (4 * n_dim));

  // Get mesh index part of label.
  std::string out = MeshFilter_text_label(i_mesh);

  // Get surface part of label.
  switch (surf_dir) {
    case MeshDir::OUT_LEFT:
      out += " Outgoing, x-min";
      break;
    case MeshDir::IN_LEFT:
      out += " Incoming, x-min";
      break;
    case MeshDir::OUT_RIGHT:
      out += " Outgoing, x-max";
      break;
    case MeshDir::IN_RIGHT:
      out += " Incoming, x-max";
      break;
    case MeshDir::OUT_BACK:
      out += " Outgoing, y-min";
      break;
    case MeshDir::IN_BACK:
      out += " Incoming, y-min";
      break;
    case MeshDir::OUT_FRONT:
      out += " Outgoing, y-max";
      break;
    case MeshDir::IN_FRONT:
      out += " Incoming, y-max";
      break;
    case MeshDir::OUT_BOTTOM:
      out += " Outgoing, z-min";
      break;
    case MeshDir::IN_BOTTOM:
      out += " Incoming, z-min";
      break;
    case MeshDir::OUT_TOP:
      out += " Outgoing, z-max";
      break;
    case MeshDir::IN_TOP:
      out += " Incoming, z-max";
      break;
  }

  return out;
}

//==============================================================================
// C-API functions
//==============================================================================

extern "C" int openmc_meshsurface_filter_get_mesh(
  int32_t index, int32_t* index_mesh)
{return openmc_mesh_filter_get_mesh(index, index_mesh);}

extern "C" int openmc_meshsurface_filter_set_mesh(
  int32_t index, int32_t index_mesh)
{
  return openmc_mesh_filter_set_mesh(index, index_mesh);
}

} // namespace openmc
