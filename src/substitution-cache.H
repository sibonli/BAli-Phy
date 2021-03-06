/*
   Copyright (C) 2004-2005,2007,2010 Benjamin Redelings

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

#ifndef SUBSTITUTION_CACHE_H
#define SUBSTITUTION_CACHE_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "mytypes.H"
#include "tree.H"
#include "smodel.H"

/// An object to store cached conditional likelihoods for a single branch
struct Likelihood_Cache_Branch: public std::vector< Matrix >
{
  efloat_t other_subst;
  Likelihood_Cache_Branch(int C,int M, int S)
    :std::vector<Matrix>(C,Matrix(M,S)),
     other_subst(1) 
  {}
};


/// \brief A class to manage storage and sharing of cached conditional likelihoods.
///
/// The Multi_Likelihood_Cache maintains a number of "locations", each
/// of which store cached conditional likelihoods for one branch.  Different
/// Likelihood_Cache objects each have a unique token, which maps each
/// of their branches to a "location" that indicates a Likelihood_Cache_Branch.
/// Different Likelihood_Cache objects may share the same Likelihood_Cache_Branch
/// if the cached likelihoods on that branch are the same for both objects.
///
class Multi_Likelihood_Cache: public std::vector< Likelihood_Cache_Branch >
{
protected:
  int C; // the (maximum) number of columns available per branch
  int M; // number of models
  int S; // number of states

  /// mapping[token][branch] -> location
  std::vector<std::vector<int> > mapping;

  /// number of users of each location
  std::vector<int> n_uses;

  /// a stack of unused_locations
  std::vector<int> unused_locations;

  /// is each token in use or not?
  std::vector<bool> active;

  /// length of each active cache (in columns)
  std::vector<int> length;
  int iterations_too_long;

  /// acquire and return an unused location
  int get_unused_location();

  /// release one use of location loc
  void release_location(int loc);

  /// Allocate space for s new 'branches'
  void allocate_branch_slots(int s);

  /// Is each location up to date?
  std::vector<int> up_to_date_;

  /// Can each token re-use the previously computed likelihood?
  std::vector<int> cv_up_to_date_;

public:

  /// Can token t re-use its previously computed likelihood?
  int  cv_up_to_date(int t) const {return cv_up_to_date_[t];}
  /// Can token t re-use its previously computed likelihood?
  int& cv_up_to_date(int t)       {return cv_up_to_date_[t];}

  /// Reserve backing store for t/b, and point t/b to it.
  void allocate_location(int t, int b);

  /// Where do we store caches for token t, branch b?
  int location(int t,int b) const {assert(mapping[t][b] != -1); return mapping[t][b];}

  /// Has branch b for token t been allocated a location yet?
  int location_allocated(int t, int b) const {return mapping[t][b] != -1;}

  /// Are cached conditional likelihoods for token t/branch b, up to date?
  bool up_to_date(int t,int b) const 
  {
    return location_allocated(t,b) and up_to_date_[ location(t,b) ];
  }

  /// Mark cached conditional likelihoods for token t/branch b up to date.
  void validate_branch(int token,int branch);

  /// Determine the number of CTMC models out model is a mixture of.
  int n_models() const {return M;}
  /// The size of the alphabet
  int n_states() const {return S;}

  /// Mark cached conditional likelihoods for token t/branch b invalid, and unshare.
  void invalidate_one_branch(int token,int branch);
  /// Mark cached conditional likelihoods for all branches of token t invalid.
  void invalidate_all(int token);

  /// Register that we CURRENTLY don't need more than l columns, and we need at least l
  void request_length(int l);
  /// Set the length of token t columns.
  void set_length(int l);
  /// Set the length of token t to l columns.
  void set_length(int token, int l);
  /// Get the length of token t in columns.
  int  get_length(int token) const {return length[token];}

  /// Return an unused token.
  int find_free_token() const;
  /// Create an unused token.
  int add_token(int B);
  /// Acquire a token for use with C columns and B/2 branches.
  int claim_token(int C,int B);
  /// Setup token2 to point to cached likelihoods for token1
  void copy_token(int token1,int token2);
  /// Initialize token
  void init_token(int token);
  /// Release token and mark unused.
  void release_token(int token);
  
  Multi_Likelihood_Cache(const substitution::MultiModelObject& M);
};

/// A single view into the shared Multi_Likelihood_Cache
class Likelihood_Cache {
  boost::shared_ptr<Multi_Likelihood_Cache> cache;

  /// The number of directed branches
  int B;

  /// The slot in 'cache' that we use.
  int token;

  /// Some matrices pre-allocated for scratch space
  std::vector<Matrix> scratch_matrices;

  std::vector<int> lengths;

public:
  /// Previously computed likelihood.
  efloat_t cached_value;

  /// Can we re-use our previously computed likelihood?
  int  cv_up_to_date() const {return cache->cv_up_to_date(token);}
  /// Can we re-use our previously computed likelihood?
  int& cv_up_to_date()       {return cache->cv_up_to_date(token);}

  /// Starting point for our likelihood calculations
  int root;

  /// Number of columns of our alignment
  int allocated_length() const {return cache->get_length(token);}
  /// The number of undirected branches;
  int n_branches() const {return B/2;}
  /// The number of CTMC models in our mixture.
  int n_models() const {return cache->n_models();}
  /// The number of states in our alphabet.
  int n_states() const {return cache->n_states();}

  /// Mark cached conditional likelihoods for all branches invalid.
  void invalidate_all();

  /// Mark cached conditional likelihoods for b and all branches after invalid.
  void invalidate_directed_branch(const Tree&,int b);

  /// Mark cached conditional likelihoods for b,b* and all branches after either invalid.
  void invalidate_branch(const Tree&,int b);

  /// Mark cached conditional likelihoods for b,b* and all branches after either invalid.
  void invalidate_one_branch(int b);

  /// Mark cached conditional likelihoods for all branches after b or b* invalid.
  void invalidate_branch_alignment(const Tree&,int b);

  /// Mark cached conditional likelihoods all branches after n invalid.
  void invalidate_node(const Tree&,int n);

  /// Set the length to l columns.
  void set_length(int l,int b);
  /// Get the length columns.
  int get_length(int b) const {return lengths[b];}
  /// Get the length columns.
  int get_length() const {return cache->get_length(token);}

  /// Are cached conditional likelihoods for branch b, up to date?
  bool up_to_date(int b) const {return cache->up_to_date(token,b);}
  /// Mark cached conditional likelihoods for branch b up to date.
  void validate_branch(int b) {cache->validate_branch(token,b);}
  /// Is there is backing store for us to work with?
  bool branch_available(int b) const {return cache->location_allocated(token,b);}
  /// Ensure there is backing store for us to work with
  void prepare_branch(int b) const {cache->allocate_location(token,b);}

  /// Cached conditional likelihoods for branch b
  const Likelihood_Cache_Branch& operator[](int b) const {
    int loc = cache->location(token,b);
    assert(loc != -1);
    return (*cache)[loc];
  }

  /// Cached conditional likelihoods for branch b
  Likelihood_Cache_Branch& operator[](int b) {
    int loc = cache->location(token,b);
    assert(loc != -1);
    return (*cache)[loc];
  }

  /// Cached conditional likelihoods for index i, branch b
  const Matrix& operator()(int i,int b) const {
    int loc = cache->location(token,b);
    assert(loc != -1);
    assert(0 <= i and i < get_length());
    return (*cache)[loc][i];
  }

  /// Cached conditional likelihoods for index i, branch b
  Matrix& operator()(int i,int b) {
    int loc = cache->location(token,b);
    assert(loc != -1);
    assert(0 <= i and i < get_length());
    return (*cache)[loc][i];
  }

  /// Scratch matrix i
  const Matrix& scratch(int i) const 
  {
    return scratch_matrices[i];
  }

  /// Scratch matrix i
  Matrix& scratch(int i) 
  {
    return scratch_matrices[i];
  }

  /// Construct a duplicate view to the same conditional likelihood caches
  Likelihood_Cache& operator=(const Likelihood_Cache&);

  Likelihood_Cache(const Likelihood_Cache& LC);
  Likelihood_Cache(const Tree& T, const substitution::MultiModelObject& M,int l=0);

  ~Likelihood_Cache();
};

/// Set the root to whichever node of b is closer to the current root.
void select_root(const Tree& T,int b,Likelihood_Cache& LC);

#endif
