/*
   Copyright (C) 2004-2010 Benjamin Redelings

This file is part of BAli-Phy.

BAli-Phy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

BAli-Phy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with BAli-Phy; see the file COPYING.  If not see
<http://www.gnu.org/licenses/>.  */

///
/// \file parameters.H
///
/// \brief This file defines the Parameters class which holds the model and state
///        for the MCMC.
///

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>
#include <valarray>
#include "alphabet.H"
#include "mytypes.H"
#include "sequencetree.H"
#include "probability-model.H"
#include "smodel.H"
#include "imodel.H"
#include "alignment.H"
#include "matcache.H"
#include "clone.H"
#include "substitution-cache.H"
#include "substitution-index.H"

#include "tools/partition.H"
#include "cow-ptr.H"
#include "cached_value.H"

//------------------------------- parameter-containing class --------------------------//

// Switching to branch-dependent smodels & imodels
// 
// Steps: for data_partition: 
// 1. Replace IModel_ with IModels_ .
// 2. Replace SModel_ with SModels_ .
// 3. Add an imodel_for_branch[] vector<>.
// 4. Initialize everything to 0.
// 5. Add an smodel_for_branch[] vector<>.
// 6. Initialize it to 0.
// 7. Update IModel() to IModel(int b)
// 8. Update SModel() to SModel(int b)
// 9. Update get_branch_HMM to use IModel(imodel_for_branch[b])->get_branch_HMM(t*branch_mean)
// 10. Update transition_P(int b) to use SModel(smodel_for_branch[b]).transition_p(l,m)
// 11. Update get_branch_HMM( ) and branch_hmm_type to actually use a
//     different IModel for disconnected b ranches.  BTW, what is our real
//     distribution on this? 
// • Perhaps this project is a "just publish, and make the count-the-papers people happy."
// 12. Update has_IModel to check if IModels_.size() == 0.
// 13. Add a procedure for manipulating/changing the imodel / smodel
// for a branch.
// 
// Steps: for Parameters:
// 1. Change smodel_for_partition to vector< vector<int> > smodels_for_partition.
// 2. Change imodel_for_partition to vector< vector<int> > imodels_for_partition.
// 3. ...
// 
// Logical issues
//
// 0. We need to store the state somewhere it can be manipulated.
//  • Does this mean we need the static flow framework?
//
// 1. All the branch smodels on the same tree must (currently) be related in some way.
// • They must have the same equilibrium frequencies.
// • They must have the same alphabet
// • They must have the same number of states.
// • They must have the same number of base models.
// 
// 2. We might want two smodels with e.g. different rates to have
//    correlated priors.  For example, we might want to put a
//    distribution on the geometric mean of the rates, and another
//    distribution on how different the two rates are. 
// 
// 3. All the branch imodels on the same tree must (currently) be related in some way
// • They must have the same equilibrium distribution for sequence lengths.
// 
// 4. We might also want imodels on the same tree to have
//    correlated priors.
//
// So: Should we make the current smodels and imodels into
// branch_smodels and branch_imodels, and make a higher level smodel
// and higher level imodel that can contain multiple branch models?
//
// So: we can store the imodel_for_branch[] and smodel_for_branch[] in
// the data_partitions.  Its just a way of storing things.  And then I
// don't have to write it down into the data_partitions( )
//
// So: unless we want different smodels to have different frequency parameters
// we should probably make a multi-branch model out of a single frequencymodel
// and multiple exchange models.



extern bool use_internal_index;

/// Each data_partition is a model with one parameter: mu (the branch mean)
class data_partition: public Probability_Model, public Mat_Cache
{
  friend class Parameters;

  /// The IndelModel
  polymorphic_cow_ptr<IndelModel> IModel_;

  /// The substitution::Model
  polymorphic_cow_ptr<substitution::MultiModel> SModel_;

  std::string partition_name;

  void recalc(const std::vector<int>&);

  /// Cached prior for just the alignment -- accessed through alignment_prior( )
  mutable cached_value<efloat_t> cached_alignment_prior;

  /// Cached prior for the pairwise alignment on each branch.
  mutable std::vector<cached_value<efloat_t> > cached_alignment_prior_for_branch;

  /// Cached transition counts for the pair-HMM on each branch.
  mutable std::vector<cached_value<ublas::matrix<int> > > cached_alignment_counts_for_branch;

  /// Cached sequence lengths -- accessed through seqlength( )
  mutable std::vector<cached_value<int> > cached_sequence_lengths;

  /// Cached HMMs for each branch -- accessed through get_branch_HMM( )
  mutable std::vector<cached_value<indel::PairHMM> > cached_branch_HMMs;

  /// Cached transition probability matrices -- accessed through transition_P( )
  mutable std::vector< cached_value< std::vector< Matrix> > > cached_transition_P;

  double branch_mean_;

  void branch_mean(double);
  void branch_mean_tricky(double);

  bool variable_alignment_;

public:

  bool smodel_full_tree;

  /// Is the alignment allowed to vary?
  bool variable_alignment() const {return variable_alignment_;}
  
  /// Set alignment variability
  void variable_alignment(bool b);

  data_partition* clone() const {return new data_partition(*this);}

  /// The leaf sequence data of this partition
  cow_ptr< std::vector< std::vector<int> > > sequences;

  /// The alignment data of this partition
  cow_ptr<alignment> A;

  /// Tree pushed down from above
  cow_ptr<SequenceTree> T;

  /// Which part of the substitution model does this branch use
  cow_ptr< std::vector<int> > branch_subst_category;

  int get_branch_subst_category(int b) const;

  /// Cached Transition Probabilities
  const std::vector<Matrix>& transition_P(int b) const;
  
  /// Cached Conditional Likelihoods
  mutable Likelihood_Cache LC;

  /// sub-alignment indices;
  mutable owned_ptr<subA_index_t> subA;

  /// cached branch HMMs
  const indel::PairHMM& get_branch_HMM(int b) const;
  std::vector<indel::PairHMM> get_branch_HMMs(const std::vector<int>&) const;

  std::vector<int> branch_HMM_type;

  /// Alignment constraint
  ublas::matrix<int> alignment_constraint;

  /// Temperatures -    0:likelihood     1:prior?
  std::vector<double> beta;

  void set_beta(double b);
  double get_beta() const;

  // get the alphabet for partition i
  const alphabet& get_alphabet() const {return A->get_alphabet();}

  /// Recalculate transition matrix for branch b
  void recalc_imodel_for_branch(int b);

  /// Recalculate all the cached transition matrices
  void recalc_imodel();
  void recalc_smodel();

  bool has_IModel() const {return IModel_;}
  /// The IndelModel
  const IndelModel& IModel() const;
  /// The IndelModel
  IndelModel& IModel();

  /// Get the substitution::Model
  const substitution::MultiModel& SModel() const {return *SModel_;}
  /// Get the substitution::Model
  substitution::MultiModel& SModel() {return *SModel_;}

  void setlength(int b, double l);
  void setlength_no_invalidate_LC(int b, double l);

  int seqlength(int n) const;

  // invalidate subA indices on b and b* and all DIRECTED branches after them
  void invalidate_subA_index_branch(int b);
  // invalidate subA indices on b and b*
  void invalidate_subA_index_one_branch(int b);
  // invalidate subA indices for all branches
  void invalidate_subA_index_all();

  void subA_index_allow_invalid_branches(bool);

  void note_alignment_changed_on_branch(int b);

  void note_alignment_changed();

  void note_sequence_length_changed(int n);

  /// parameter for exponential prior on branch length
  double branch_mean() const; 

  // The prior, likelihood, and probability
  efloat_t prior_no_alignment() const;
  efloat_t prior_alignment() const;

  efloat_t prior() const;
  efloat_t likelihood() const;

  efloat_t heated_likelihood() const;

  std::string name() const;

  data_partition(const std::string& n, const alignment&, const SequenceTree&,
		 const substitution::MultiModel&);
  data_partition(const std::string& n, const alignment&, const SequenceTree&,
		 const substitution::MultiModel&, const IndelModel&);
};

/// A class to contain all the MCMC state except the alignment
class Parameters: public SuperModel, public Probability_Model
{
  /// The substitution::Model
  std::vector<polymorphic_cow_ptr<substitution::MultiModel> > SModels;
  std::vector<int> smodel_for_partition;

  /// The IndelModel
  std::vector<polymorphic_cow_ptr<IndelModel> > IModels;
  std::vector<int> imodel_for_partition;

  // Scale for partition
  std::vector<int> scale_for_partition;
  int n_scales;

  void recalc(const std::vector<int>&);

public:

  /// Do we have an Exponential (0) or Gamma-0.5 (1) prior on branch lengths?
  int branch_prior_type;

  bool smodel_full_tree;

  bool variable_alignment() const;

  void variable_alignment(bool b);

  /// The alignments and cached computations
  std::vector<cow_ptr<data_partition> > data_partitions;

  int n_data_partitions() const {return data_partitions.size();}

  const data_partition& operator[](int i) const {return *data_partitions[i];}
        data_partition& operator[](int i)       {return *data_partitions[i];}

  int get_smodel_index_for_partition(int i) const {return smodel_for_partition[i];}

  int get_imodel_index_for_partition(int i) const {return imodel_for_partition[i];}

  void set_beta(double b);
  double get_beta() const;

  /// The Tree - mean branch lengths = 1
  cow_ptr<SequenceTree> T;

  /// Topology and Branch-Length Constraint
  cow_ptr<SequenceTree> TC;

  /// Alignment-constrained branches
  std::vector<int> AC;

  std::vector<int> branch_HMM_type;

  std::vector<double> beta_series;

  std::vector<double> all_betas;
  int beta_index;

  /// Did we most recently hit beta==1 (1) or beta=0 (0)
  int updown;

  /// Tree partitions to weight
  std::vector<Partition> partitions;
  std::vector<efloat_t> partition_weights;

  /// Is pinning enabled or not?
  std::vector<double> constants;
  /// Is pinning enabled or not?
  int features;

  double branch_length_max;

  // The prior, likelihood, and probability
  efloat_t prior_no_alignment() const;
  efloat_t prior_alignment() const;
  efloat_t prior() const;
  efloat_t likelihood() const;
  efloat_t probability() const { return prior() * likelihood(); }

  efloat_t heated_likelihood() const;

  /// How many substitution models?
  int n_smodels() const {return SModels.size();}
  /// Get the substitution::Model
  const substitution::MultiModel& SModel(int i) const {return *SModels[i];}
  /// Get the substitution::Model
        substitution::MultiModel& SModel(int i)       {return *SModels[i];}
  
  /// How many substitution models?
  int n_imodels() const {return IModels.size();}
  /// Get the substitution::Model
  const IndelModel& IModel(int i) const {return *IModels[i];}
  /// Get the substitution::Model
        IndelModel& IModel(int i)       {return *IModels[i];}

  /// Set branch 'b' to have length 'l', and compute the transition matrices
  void setlength(int b,double l); 
  void setlength_no_invalidate_LC(int b,double l); 
  
  /// Recalculate all the cached transition matrices
  void recalc_imodels();
  void recalc_imodel(int i);
  void recalc_smodels();
  void recalc_smodel(int i);
  void tree_propagate();

  void select_root(int b);
  void set_root(int b);

  // invalidate likelihoood caches on b and b* and all DIRECTED branches after them
  void LC_invalidate_branch(int b);

  // invalidate likelihoood caches on b and b* and all DIRECTED branches after them
  void LC_invalidate_one_branch(int b);

  // invalidate likelihoood caches on b and b* and all DIRECTED branches after them
  void LC_invalidate_all();

  // invalidate subA indices on b and b* and all DIRECTED branches after them
  void invalidate_subA_index_branch(int b);
  // invalidate subA indices on b and b*
  void invalidate_subA_index_one_branch(int b);
  // invalidate subA indices for all branches
  void invalidate_subA_index_all();

  void subA_index_allow_invalid_branches(bool);

  void note_alignment_changed_on_branch(int b);

  void note_alignment_changed();

  void note_sequence_length_changed(int n);

  int n_branch_means() const;

  double branch_mean() const;

  int branch_mean_index(int i) const;

  void branch_mean(int i,double); 

  void branch_mean_tricky(int i,double); 

  Parameters* clone() const {return new Parameters(*this);}

  std::string name() const {return "Full Model";}

  const Model& SubModels(int) const;
  Model& SubModels(int);

  Parameters(const std::vector<alignment>& A, const SequenceTree&, 
	     const std::vector<polymorphic_cow_ptr<substitution::MultiModel> >&,
	     const std::vector<int>&,
	     const std::vector<int>&);

  Parameters(const std::vector<alignment>& A, const SequenceTree&, 
	     const std::vector<polymorphic_cow_ptr<substitution::MultiModel> >&,
	     const std::vector<int>&,
	     const std::vector<polymorphic_cow_ptr<IndelModel> >&,
	     const std::vector<int>&,
	     const std::vector<int>&);
};

bool accept_MH(const Probability_Model& P1,const Probability_Model& P2,double rho);


#endif
