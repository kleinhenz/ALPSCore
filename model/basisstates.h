/***************************************************************************
* ALPS++/model library
*
* model/basisstates.h    basis states for full lattice
*
* $Id$
*
* Copyright (C) 2003-2003 by Matthias Troyer <troyer@comp-phys.org>,
*                            Synge Todo <wistaria@comp-phys.org>,
*
* Permission is hereby granted, free of charge, to any person or organization 
* obtaining a copy of the software covered by this license (the "Software") 
* to use, reproduce, display, distribute, execute, and transmit the Software, 
* and to prepare derivative works of the Software, and to permit others
* to do so for non-commerical academic use, all subject to the following:
*
* The copyright notice in the Software and this entire statement, including 
* the above license grant, this restriction and the following disclaimer, 
* must be included in all copies of the Software, in whole or in part, and 
* all derivative works of the Software, unless such copies or derivative 
* works are solely in the form of machine-executable object code generated by 
* a source language processor.

* In any scientific publication based in part or wholly on the Software, the
* use of the Software has to be acknowledged and the publications quoted
* on the web page http://www.alps.org/license/ have to be referenced.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
**************************************************************************/

#ifndef ALPS_MODEL_BASISSTATES_H
#define ALPS_MODEL_BASISSTATES_H

#include <alps/model/basis.h>
#include <vector>

namespace alps {

template <class I, class S=StateDescriptor<I> >
class BasisStatesDescriptor : public std::vector<SiteBasisStates<I,S> >
{
public:
  typedef SiteBasisStates<I,S> site_state_type;
  typedef std::vector<site_state_type> base_type;
  typedef typename base_type::const_iterator const_iterator;
  template <class G> BasisStatesDescriptor(const BasisDescriptor<I>& b, const G& graph);
  const BasisDescriptor<I>& basis() const { return basis_;}
private:
  BasisDescriptor<I> basis_;
};

template <class I>
class IntegerState {
public:
  typedef I representation_type;
  
  class reference {
  public:
    reference(I& s, int i) : state_(s), mask_(1<<i) {}
    operator int() const { return (state_&mask_ ? 1 : 0);}
    template <class T>
    reference& operator=(T x)
    {
      if (x)
        state_|=mask_;
      else
        state_&=mask_;
      return *this;
    }
  private:
    I& state_;
    I mask_;
  };
  
  IntegerState(representation_type x=0) : state_(x) {}
  
  template <class J>
  IntegerState(const std::vector<J>& x) : state_(0)
  { 
    for (int i=0;i<x.size();++i)  
      if(x[i])
        state_ |=(1<<i);
  }
  int operator[](int i) const { return (state_>>i)&1;}
  reference operator[](int i) { return reference(state_,i);}
  operator representation_type() const { return state_;}
  representation_type state() const { return state_;}
private:
  representation_type state_;
};

template <class I>
bool operator == (IntegerState<I> x, IntegerState<I> y)
{ return x.state() == y.state(); }

template <class I>
bool operator < (IntegerState<I> x, IntegerState<I> y)
{ return x.state() < y.state(); }

template <class I, class S=std::vector<I>, class SS=StateDescriptor<I> >
class BasisStates : public std::vector<S>
{
public:
  typedef std::vector<S> base_type;
  typedef typename base_type::const_iterator const_iterator;
  typedef S value_type;
  typedef typename base_type::size_type size_type;
  typedef BasisStatesDescriptor<I,SS> basis_type;

  BasisStates(const BasisStatesDescriptor<I,SS>& b);
  inline size_type index(const value_type& x) const
  {
    const_iterator it=lower_bound(begin(),end(),x);
    return (*it==x ? it-begin() : size());
  }

  bool check_sort() const;
  const basis_type& basis() { return basis_;}
private:
  BasisStatesDescriptor<I,SS> basis_;
};


template <class I=unsigned int, class J=short, class S=IntegerState<I>, class SS=StateDescriptor<J> >
class LookupBasisStates : public BasisStates<J,S,SS>
{
public:
  typedef BasisStates<I,S,SS> base_type;
  typedef typename base_type::const_iterator const_iterator;
  typedef S value_type;
  typedef typename base_type::size_type size_type;
  typedef typename base_type::basis_type basis_type;

  LookupBasisStates(const BasisStatesDescriptor<J,SS>& b) 
    : BasisStates<J,S,SS>(b), use_lookup_(false) 
  {
    if (b.size()<=24) {
      use_lookup_=true;
      lookup_.resize(1<<b.size(),size());
      for (int i=0;i<size();++i)
        lookup_[operator[](i)]=i;
    }
  }
  
  inline size_type index(const value_type& x) const
  {
    if (use_lookup_)
      return lookup_[x];
    else
      return BasisStates<J,S,SS>::index(x);
  }

private:
  bool use_lookup_;
  std::vector<I> lookup_;
};


// -------------------------- implementation -----------------------------------


template <class I, class S> template <class G>
BasisStatesDescriptor<I,S>::BasisStatesDescriptor(const BasisDescriptor<I>& b, const G& g)
 : basis_(b)
{
  // construct SiteBasisStates for each site
  typename property_map<site_type_t,G,int>::const_type site_type(get_or_default(site_type_t(),g,0));
  for (typename boost::graph_traits<G>::vertex_iterator it=sites(g).first;it!=sites(g).second ; ++it) {
    push_back(site_state_type(basis_.site_basis(site_type[*it])));
  }
}


template <class I, class S, class SS>
bool BasisStates<I,S,SS>::check_sort() const
{
  for (int i=0;i<size()-1;++i)
    if (!((*this)[i]<(*this)[i+1]))
      return false;
  return true;
}

template <class I, class S, class SS>
BasisStates<I,S,SS>::BasisStates(const BasisStatesDescriptor<I,SS>& b)
 : basis_(b)
{
  if (b.empty())
    return;
  std::vector<I> idx(b.size(),0);
  unsigned int last=idx.size()-1;
  while (true) {
    unsigned int k=last;
    while (idx[k]>=b[k].size() && k) {
      idx[k]=0;
      if (k==0)
        break;
      --k;
      ++idx[k];
    }
    if (k==0 && idx[k]>=b[k].size())
      break;
    push_back(idx);
    ++idx[last];
  }
  if (!check_sort()) {
    std::sort(begin(),end());
    if (!check_sort())
      boost::throw_exception(std::logic_error("Basis not sorted correctly"));
  }
}

} // namespace alps

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
namespace alps {
#endif

template <class I, class S, class SS>
inline std::ostream& operator<<(std::ostream& out, const alps::BasisStates<I,S,SS>& q)
{
  out << "{\n";
  for (typename alps::BasisStates<I,S>::const_iterator it=q.begin();it!=q.end();++it) {
    out << "[ ";
    for (unsigned int i=0; i!= it->size();++i)
      out << q.basis()[i][(*it)[i]] << " ";
    out << " ]\n";
  }
  out << "}\n";
  return out;	
}

#ifndef BOOST_NO_OPERATORS_IN_NAMESPACE
} // namespace alps
#endif

#endif
