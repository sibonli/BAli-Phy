/*
   Copyright (C) 2004-2006,2009 Benjamin Redelings

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

#include <set>
#include <map>

#include "util.H"
#include "myexception.H"
#include "model.H"
#include "expression.H"

using std::vector;
using std::string;

using boost::shared_ptr;

string parameter_name(const string& prefix, int i,int n) 
{
  if (i>=n)
    throw myexception()<<"substitution model: referred to parameter "<<i<<" but there are only "<<n<<" parameters.";
  return prefix + convertToString(i);
}

Parameter::Parameter(const string& n)
  :name(n), fixed(false), changed(true)
{
}

Parameter::Parameter(const string& n, shared_ptr<const Object> v)
  :name(n), value(v), fixed(false), changed(true)
{
}

Parameter::Parameter(const string& n, const Object& v)
  :name(n), value(v), fixed(false), changed(true)
{
}

Parameter::Parameter(const string& n, const Object& v, bool f)
  :name(n), value(v), fixed(f), changed(true)
{
}

Parameter::Parameter(const string& n, const Object& v, const Bounds<double>& b, bool f)
  :name(n), value(v), bounds(b), fixed(f), changed(true)
{
}

void Model::validate() const
{
  valid = true;
  for(int i=0;i<n_parameters();i++)
    parameters_[i].changed = false;
}

void Model::invalidate() const
{
  valid = false;
}

void Model::modify_parameter(int i) const
{
  parameters_[i].changed = true;

  invalidate();
}

void Model::modify_parameters(const vector<int>& indices)
{
  for(int i=0;i<indices.size();i++)
    modify_parameter(indices[i]);
}

void Model::modify_all_parameters() const
{
  for(int i=0;i<n_parameters();i++)
    modify_parameter(i);
}

vector<int> Model::modified_parameters() const
{
  vector<int> changed;

  for(int i=0;i<n_parameters();i++)
    if (parameters_[i].changed)
      changed.push_back(i);

  return changed;
}

void Model::recalc_all() 
{
  modify_all_parameters();
  update();
}

int Model::add_parameter(const Parameter& P)
{
  for(int i=0;i<n_parameters();i++)
    if (parameters_[i].name == P.name)
      throw myexception()<<"A parameter with name '"<<P.name<<"' already exists - cannot add another one.";

  parameters_.push_back(P);
  return parameters_.size()-1;
}

std::vector< shared_ptr<const Object> > Model::get_parameter_values() const
{
  std::vector< shared_ptr<const Object> > values(n_parameters());

  for(int i=0;i<values.size();i++)
    values[i] = get_parameter_value(i);
    
  return values;  
}

std::vector< shared_ptr<const Object> > Model::get_parameter_values(const std::vector<int>& indices) const
{
  std::vector< shared_ptr<const Object> > values(indices.size());
    
  for(int i=0;i<values.size();i++)
    values[i] = get_parameter_value(indices[i]);
  
  return values;  
}

void Model::write_value(int i,const shared_ptr<const Object>& value)
{
  parameters_[i].value = value;
  modify_parameter(i);
}

void Model::set_parameter_value(int i,Double value) 
{
  set_parameter_value(i, shared_ptr<const Object>( value.clone()) );
}

void Model::set_parameter_value(int i,const shared_ptr<const Object>& value) 
{
  set_parameter_values(vector<int>(1,i), vector< shared_ptr<const Object> >(1, value) );
}

void Model::set_parameter_values(const vector<int>& indices,const vector<Double>& p)
{
  vector< shared_ptr<const Object> > p2(p.size());
  for(int i=0;i<p.size();i++)
    p2[i] = shared_ptr<const Object>( p[i].clone() );

  set_parameter_values(indices,p2);
}

void Model::set_parameter_values(const vector<int>& indices,const vector<shared_ptr<const Object> >& p)
{
  assert(indices.size() == p.size());

  for(int i=0;i<indices.size();i++)
    write_value(indices[i], p[i]);

  update();
}

void Model::set_parameter_values(const vector<Double>& p) 
{
  assert(p.size() == n_parameters());
  set_parameter_values(iota<int>(n_parameters()), p);
}

void Model::set_parameter_values(const vector<shared_ptr<const Object> >& p) 
{
  assert(p.size() == n_parameters());
  set_parameter_values(iota<int>(n_parameters()), p);
}

Model::Model()
  :valid(false)
{ }

boost::shared_ptr<const Object> Model::evaluate()
{
  update();
  return boost::shared_ptr<const Object>(clone());
}

void Model::update()
{
  if (not is_valid())
  {
    recalc(modified_parameters());
    validate();
  }
}

int SuperModel::add_parameter(const Parameter& P)
{
  int index = Model::add_parameter(P);
  model_slots_for_index.push_back(vector<model_slot>());
  return index;
}

int SuperModel::n_submodels() const 
{
  return slot_expressions_for_submodel.size();
}

bool SuperModel::parameter_is_used_by_model(int index, int m) const
{
  for(int i=0;i<model_slots_for_index[index].size();i++)
    if (model_slots_for_index[index][i].model_index == m)
      return true;

  return false;
}

bool SuperModel::is_super_parameter(int index) const
{
  return parameter_is_used_by_model(index,-1);
}

int SuperModel::n_super_parameters() const 
{
  int n=0;
  for(int i=0;i<n_parameters();i++)
    if (is_super_parameter(i))
      n++;

  return n;
}

// apparent the super-parameters are the first ones
int SuperModel::add_super_parameter(const Parameter& P)
{
  int index = add_parameter(P);
  model_slots_for_index[index].push_back(model_slot());

  return index;
}

int SuperModel::register_last_submodel(const vector<arg_expression>& args)
{
  int m_index = slot_expressions_for_submodel.size()-1;

  // The number of slots should match the number of slots in the model expression
  assert(SubModels(m_index).n_parameters() == args.size());

  // An argument should not refer to a parent slot that does not exist.
  for(int i=0;i<args.size();i++)
    if (args[i].is_term_ref())
      assert(args[i].parent_index >=0 and args[i].parent_index < n_parameters());
    
  // Record for each arg that its used in this submodel
  for(int slot=0;slot<args.size();slot++)
    if (args[slot].is_term_ref())
    {
      int index = args[slot].parent_index;
      model_slots_for_index[index].push_back(model_slot(m_index,slot));
    }

  // Set the submodel parameters
  write();

  return m_index;
}

int SuperModel::register_submodel(const vector<arg_expression>& args)
{
  // bump the number of submodels
  slot_expressions_for_submodel.push_back( vector<arg_expression>() );

  return register_last_submodel(args);
}

int SuperModel::register_submodel(const string& prefix)
{
  // bump the number of submodels
  slot_expressions_for_submodel.push_back( vector<arg_expression>() );

  int m_index = slot_expressions_for_submodel.size()-1;

  const Model& M = SubModels(m_index);

  // Create the top-level parameters, and the list of references to them
  vector<arg_expression> args;

  for(int i=0;i<M.n_parameters();i++)
  {
    Parameter P = M.get_parameter(i);
    P.name = prefix + "::" + P.name;
    int index = add_parameter(P);
    args.push_back(index);
  }

  return register_last_submodel(args);
}

// can I write the supermodel so that it actually SHARES the values of the sub-models?
void SuperModel::write_value(int index, const shared_ptr<const Object>& p)
{
  assert(index < n_parameters());

  Model::write_value(index, p);

  const vector<model_slot>& model_slots = model_slots_for_index[index];

  // For each model that uses this top-level index...
  for(int i=0;i<model_slots.size();i++)
  {
    int m = model_slots[i].model_index;
    int s = model_slots[i].slot;

    //... write it down into a sub-model, if the usage is not from the top-level model.
    if (m != -1) 
      SubModels(m).write_value(s,p);
  }
}

void SuperModel::write() 
{
  for(int i=0;i<n_parameters();i++)
    write_value(i, parameters_[i].value);
}

void SuperModel::read_from_submodel(int m)
{
  for(int i=0;i<n_parameters();i++)
  {
    for(int j=0;j<model_slots_for_index[i].size();j++)
    {
      if (model_slots_for_index[i][j].model_index != m) continue;

      int s = model_slots_for_index[i][j].slot;

      parameters_[i].value = SubModels(m).get_parameter_value(s);
    }
  }
}

void SuperModel::read()
{
  for(int m = 0;m < n_submodels(); m++)
    read_from_submodel(m);

  check();
}

efloat_t SuperModel::prior() const {
  efloat_t  P = super_prior();
  for(int i=0;i<n_submodels();i++)
    P *= SubModels(i).prior();
  return P;
}

void SuperModel::check() const
{
  for(int m=0;m<n_submodels(); m++)
  {
    // Read the current argument lists for each sub-model
    const vector<arg_expression>& arg_expressions = slot_expressions_for_submodel[m];

    for(int i=0;i<arg_expressions.size();i++)
    {
      if (arg_expressions[i].is_term_ref())
      {
	int index = arg_expressions[i].parent_index;
	assert(SubModels(m).get_parameter_value(i)->equals( *get_parameter_value(index)) );
      }
      else
	assert(SubModels(m).get_parameter_value(i)->equals( *arg_expressions[i].constant_value ) );
    }
  }
}

void SuperModel::update()
{
  for(int i=0;i<n_submodels();i++)
    SubModels(i).update();

  Model::update();
}

SuperModel::SuperModel()
{ }

int find_parameter(const Model& M,const string& name) {
  for(int i=0;i<M.n_parameters();i++) 
    if (M.parameter_name(i) == name)
      return i;
  return -1;
}
 
void show_parameters(std::ostream& o,const Model& M) {
  for(int i=0;i<M.n_parameters();i++) {
    o<<"    ";
    if (M.is_fixed(i)) 
      o<<"*";
    o<<M.parameter_name(i)<<" = ";
    if (M.parameter_has_type<Double>(i))
      o<<M.get_parameter_value_as<Double>(i);
    else if (M.parameter_has_type<Int>(i))
      o<<M.get_parameter_value_as<Int>(i);
    else if (not M.get_parameter_value(i))
      o<<"[NULL]";
    else
      o<<"[unprintable]";
  }
  o<<"\n";
}

/// \brief Check if the model M has a parameter called name
///
/// \param M      The model
/// \param name   A parameter name
///
bool has_parameter(const Model& M, const string& name)
{
  for(int i=0;i<M.n_parameters();i++)
    if (M.parameter_name(i) == name)
      return true;
  return false;
}

/// \brief Check if the string s1 matches a pattern s2
///
/// \param s1   The string
/// \param s2   The pattern
///
bool match(const string& s1, const string& s2)
{
  if (s2.size() and s2[s2.size()-1] == '*') {
    int L = s2.size() - 1;
    if (L > s1.size()) return false;
    return (s1.substr(0,L) == s2.substr(0,L));
  }
  else
    return s1 == s2;
}

bool operator<(const vector<string>& p1, const vector<string>& p2)
{
  // less than
  if (p1.size() > p2.size()) return true;
  // greater than
  if (p1.size() < p2.size()) return false;
  
  for(int i=0;i<p1.size();i++)
  {
    int cmp = p1[i].compare(p2[i]);
    // less than
    if (cmp < 0) return true;
    // greater than
    if (cmp > 0) return false;
  }
  
  // equal
  return false;
}

typedef std::set< vector<string> > path_set_t;

/// Does this path have the given prefix?
bool path_has_prefix(const vector<string>& path, const vector<string>& path_prefix)
{
  if (path_prefix.size() > path.size()) return false;

  for(int i=0;i<path_prefix.size();i++)
    if (path[i] != path_prefix[i])
      return false;

  return true;
}

/// Are the paths all distinguishable from each other?
bool overlap(const path_set_t& set1, const path_set_t& set2)
{
  if (set1.empty() or set2.empty()) return false;

  path_set_t::const_iterator it1 = set1.begin(), it1End = set1.end();
  path_set_t::const_iterator it2 = set2.begin(), it2End = set2.end();

  if(*it1 > *set2.rbegin() || *it2 > *set1.rbegin()) return false;

  while(it1 != it1End && it2 != it2End)
  {
    if(*it1 < *it2)
      it1++; 
    else if (*it1 > *it2)
      it2++; 
    else
      return true;
  }

  return false;
}

/// Remove the nodes in paths that are direct children of the path_prefix
void remove_prefix(vector< vector<string> >& paths, const  vector<string>& path_prefix)
{
  for(int i=0;i<paths.size();i++)
  {
    if (not path_has_prefix(paths[i], path_prefix)) continue;

    paths[i].erase(paths[i].begin()+path_prefix.size()-1);
  }
}

/// Remove (internal) child paths if grandchild paths are not shared with any other child.
void check_remove_grandchildren(vector< vector<string> >& paths, const vector<string>& path_prefix)
{
  // construct the child paths and their locations
  typedef std::map<string, path_set_t> path_map_t;
  path_map_t grandchild_paths;

  int L = path_prefix.size();

  // find the grandchild paths for each child
  for(int i=0;i<paths.size();i++)
    if (path_has_prefix(paths[i], path_prefix))
    {
      // We don't consider leaf child paths
      if (paths[i].size() == path_prefix.size() + 1)
	continue;

      string child_name = paths[i][L];

      vector<string> grandchild_path = paths[i];
      grandchild_path.erase(grandchild_path.begin(),grandchild_path.begin()+L+1);

      grandchild_paths[child_name].insert(grandchild_path);
      assert(grandchild_path.size());
    }

  // check of the grandchild paths of any child overlap with the grandchild paths of any other child
  for(path_map_t::const_iterator i = grandchild_paths.begin();i != grandchild_paths.end();i++)
  {
    bool unique = true;
    for(path_map_t::const_iterator j = grandchild_paths.begin();j != grandchild_paths.end();j++)
    {
      if (i->first == j->first) continue;

      if (overlap(i->second,j->second)) unique = false;
    }
    if (unique) {
      vector<string> child_prefix = path_prefix;
      child_prefix.push_back(i->first);
      remove_prefix(paths, child_prefix);
    }
  }
}

// We can think of this collection of name lists as a tree.
// - Each name list is a path from the root to a tip.
// - Each node (except the root) has a string name associated with it.
// We consider all child nodes of internal node
//  If the set of grandchild lists under child node C does not overlap with the
//   grandchild lists under any other child node, then we can remove node C.
// We should always prefer to remove deeper nodes first.
//  Thus, leaf nodes should never be removed.
// We therefore consider all internal nodes of the tree, starting
//  with the ones furthest from the root, and remove their children
//  if it is allowable.

vector<string> short_parameter_names(vector<string> names)
{
  // for any sequence n[0] n[1] ... n[i-1] n[i] n[i+1] ..... N[L]
  // If we select all the sequences where where  n[0].... n[i-1] are the same
  //  Then we can get rid of n[i] if the sequences n[i+1]...N[L] are all different

  // construct the name paths
  vector< vector<string> > paths;
  for(int i=0;i<names.size();i++)
    paths.push_back(split(names[i],"::"));

  for(int i=0;i<paths.size();i++)
  {
    vector<string> prefix = paths[i];
    while(prefix.size())
    {
      prefix.pop_back();
      check_remove_grandchildren(paths, prefix);
    }
  }
  
  for(int i=0;i<names.size();i++)
    names[i] = join(paths[i],"::");

  return names;
}

vector<string> parameter_names(const Model& M)
{
  vector<string> names;
  for(int i=0;i<M.n_parameters();i++)
    names.push_back( M.parameter_name(i) );

  return names;
}

vector<string> short_parameter_names(const Model& M)
{
  return short_parameter_names( parameter_names( M ) );
}


bool path_match(const vector<string>& key, const vector<string>& pattern)
{
  int active_piece = 0;

  // require key[0] to match pattern[0] if key[0] starts w/ ^
  if (key[0].size() and key[0][0] == '^')
  { 
    int L = key[0].size()-1;
      
    if (not pattern.size())
      return false;
    
    if (not match(pattern[0], key[0].substr(1,L) ))
      return false;

    active_piece = 1;
  }

  // otherwise look for the pieces in sequential order
  for(int i=0;i<pattern.size() and active_piece < key.size();i++)
    if (match(pattern[i], key[active_piece]))
      active_piece++;

  return active_piece == key.size();
}

/// \brief Find the index of model parameters that match the pattern name
///
/// \param M      The model
/// \param name   The pattern
///
vector<int> parameters_with_extension(const Model& M, string name)
{
  vector<int> indices;

  const vector<string> key = split(name,"::");

  if (not key.size()) return indices;

  vector<string> skeleton;

  for(int i=0;i<M.n_parameters();i++)
  {
    vector<string> pattern = split(M.parameter_name(i),"::");

    if (not path_match(key, pattern)) continue;

    // check that all matching parameters have the same basename
    vector<string> this_skeleton = pattern;
    this_skeleton.pop_back();

    if (not indices.size())
      skeleton = this_skeleton;
    else if (skeleton != this_skeleton)
      throw myexception()<<"Key '"<<name<<"' matches both "<<join(skeleton,"::")<<" and "<<join(this_skeleton,"::")<<".";
    

    indices.push_back(i);
  }

  return indices;
}

efloat_t OpModel::prior() const
{
  efloat_t Pr = 1;
  for(int i=0;i<sub_models.size();i++)
    Pr *= sub_models[i]->prior();
  return Pr;
}

// can I write the supermodel so that it actually SHARES the values of the sub-models?
void OpModel::write_value(int index, const shared_ptr<const Object>& p)
{
  assert(index < n_parameters());

  Model::write_value(index, p);

  const vector<model_slot>& model_slots = model_slots_for_index[index];

  // For each model that uses this top-level index...
  for(int i=0;i<model_slots.size();i++)
  {
    int m = model_slots[i].model_index;
    int s = model_slots[i].slot;

    //... write it down into a sub-model, if the usage is not from the top-level model.
    if (m != -1) 
      sub_models[m]->write_value(s,p);
  }
}


string OpModel::name() const
{
  vector<string> arg_names;
  return Op->print_expression(arg_names);
}

void OpModel::check() const
{
  for(int i=0;i<n_parameters();i++)
    for(int j=0;j<model_slots_for_index[i].size();j++)
    {
      int m =  model_slots_for_index[i][j].model_index;
      int s =  model_slots_for_index[i][j].slot;
      if (m != -1)
	assert(sub_models[m]->get_parameter_value(s)->equals( *get_parameter_value(i)) );
    }
}

shared_ptr<const Object> OpModel::evaluate(int slot)
{
  const arg_expression& slot_arg = slot_expressions_for_op[slot];

  if (slot_arg.is_term_ref())
  {
    // find the parameter and return it
    int parameter_index = slot_arg.parent_index;
    return parameters_[parameter_index].value;
  }
  else if (slot_arg.is_constant())
  {
    // return the relevant constant
    return slot_arg.constant_value;
  }
  else
  {
    assert(slot_arg.is_submodel_ref());

    // update the relevant sub_model, and return a 
    int submodel_index = slot_arg.sub_model_index;

    return sub_models[submodel_index]->evaluate();
  }  
}

boost::shared_ptr<const Object> OpModel::evaluate()
{
  return (*Op)(*this);
}

int OpModel::add_submodel(shared_ptr<const Model> m)
{
  int m_index = sub_models.size();
  sub_models.push_back( m );

  vector<string> names = parameter_names( *this );

  vector<string> sub_names = parameter_names( *sub_models[m_index] );

  for(int slot=0;slot<sub_names.size();slot++)
  {
    int index = find_index(names, sub_names[slot]);

    // Record the use of this top-level index in submodel=m_index, slot=slot
    model_slots_for_index[index].push_back( model_slot(m_index,slot) );
    
    // default parameter values AND bounds from submodels
    if (not parameters_[index].value) {
      parameters_[index].value = sub_models[m_index]->get_parameter_value(slot);
      // should we continually narrow the bounds by and-ing them together?
      parameters_[index].bounds = sub_models[m_index]->get_bounds(slot);
    }
  }

  return m_index;
}

OpModel::OpModel(const expression_ref& r)
{
  shared_ptr<const operation_expression> e = boost::dynamic_pointer_cast<const operation_expression>(r);

  if (not e)
    throw myexception()<<"Trying to create an OpModel from a non-op expression:\n  "<<e->print();

  // find all named parameters, and add them to the OpModel
  vector<string> names = find_named_parameters(e);
  for(int i=0;i<names.size();i++)
  {
    add_parameter(Parameter(names[i]));
    model_slots_for_index.push_back(vector<model_slot>());
  }

  Op = e->op;
  for(int i=0;i<e->args.size();i++)
  {
    arg_expression a;

    // handle the args[i] being a named parameter
    if (shared_ptr<const named_parameter_expression> pe = boost::dynamic_pointer_cast<const named_parameter_expression>(e->args[i]))
      a.parent_index = find_index(names, pe->parameter_name);

    // handle the args[i] being a constant
    else if (shared_ptr<const constant_expression> ce = boost::dynamic_pointer_cast<const constant_expression>(e->args[i]) )
      a.constant_value = ce->value;

    // handle the args[i] being a model
    else if ( shared_ptr<const model_expression> me = boost::dynamic_pointer_cast<const model_expression>(e->args[i]) )
      a.sub_model_index = add_submodel( me->m );

    // handle the args[i] being an operation expression
    else if (shared_ptr<const operation_expression> oe = boost::dynamic_pointer_cast<const operation_expression>(e->args[i]) )
      a.sub_model_index = add_submodel(ptr<Model>( OpModel( expression_ref( oe ) ) ) );

    else
      throw myexception()<<"OpModel: can't handle sub-expression '"<<e->args[i]->print()<<"'";

    slot_expressions_for_op.push_back(a);
  }

}
